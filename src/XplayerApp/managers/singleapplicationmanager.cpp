#include "singleapplicationmanager.h"

#include <QCoreApplication>
#include <QCryptographicHash>
#include <QDebug>
#include <QDir>
#include <QElapsedTimer>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QLocalServer>
#include <QLocalSocket>
#include <QLockFile>
#include <QPointer>
#include <QStandardPaths>
#include <QThread>

#ifdef Q_OS_WIN
#include <windows.h>
#endif

namespace {
constexpr int kConnectAttemptTimeoutMs = 120;
constexpr int kReplyTimeoutMs = 300;
constexpr int kStartupRaceTimeoutMs = 2500;
constexpr int kRetryDelayMs = 40;
constexpr qsizetype kMaximumMessageBytes = 256 * 1024;
constexpr int kProtocolVersion = 1;
const QByteArray kAcknowledgement = QByteArrayLiteral("ok\n");
}

SingleApplicationManager::SingleApplicationManager(QObject *parent)
    : QObject(parent) {}

SingleApplicationManager::~SingleApplicationManager() {
  if (m_server) {
    m_server->close();
  }
  releasePrimaryGuard();
}

SingleApplicationManager::StartResult
SingleApplicationManager::start(const QStringList &arguments,
                                const QString &workingDirectory) {
  const QString name = serverName();
  const QByteArray message = createMessage(arguments, workingDirectory);
  if (!acquirePrimaryGuard()) {
    const bool notified = notifyRunningInstance(message);
    qInfo() << "SingleApplication: another instance owns the lock"
            << "| activation notified:" << notified;
    return StartResult::SecondaryInstance;
  }

  QLocalServer::removeServer(name);
  m_server = new QLocalServer(this);
  m_server->setSocketOptions(QLocalServer::UserAccessOption);
  if (!m_server->listen(name)) {
    qWarning() << "SingleApplication: failed to listen" << name
               << "| error:" << m_server->errorString();
    releasePrimaryGuard();
    return StartResult::FatalError;
  }

  connect(m_server, &QLocalServer::newConnection, this, [this]() {
    while (QLocalSocket *socket = m_server->nextPendingConnection()) {
      socket->setParent(m_server);
      connect(socket, &QLocalSocket::readyRead, this,
              [this, socket]() { processSocketData(socket); });
      connect(socket, &QLocalSocket::disconnected, socket,
              &QObject::deleteLater);
      processSocketData(socket);
    }
  });

  qInfo() << "SingleApplication: primary instance is listening";
  return StartResult::PrimaryInstance;
}

bool SingleApplicationManager::acquirePrimaryGuard() {
#ifdef Q_OS_WIN
  const QString mutexName = QStringLiteral("Local\\%1-mutex").arg(serverName());
  HANDLE handle = CreateMutexW(nullptr, TRUE,
                               reinterpret_cast<LPCWSTR>(mutexName.utf16()));
  if (!handle) {
    qWarning() << "SingleApplication: CreateMutexW failed"
               << "| error:" << GetLastError();
    return false;
  }

  if (GetLastError() != ERROR_ALREADY_EXISTS) {
    m_nativeMutexHandle = handle;
    return true;
  }

  const DWORD waitResult = WaitForSingleObject(handle, 0);
  if (waitResult == WAIT_OBJECT_0 || waitResult == WAIT_ABANDONED) {
    m_nativeMutexHandle = handle;
    qWarning() << "SingleApplication: recovered an abandoned instance mutex";
    return true;
  }

  CloseHandle(handle);
  return false;
#else
  QDir().mkpath(QFileInfo(lockFilePath()).absolutePath());
  m_lockFile = std::make_unique<QLockFile>(lockFilePath());
  m_lockFile->setStaleLockTime(0);
  return m_lockFile->tryLock();
#endif
}

void SingleApplicationManager::releasePrimaryGuard() {
#ifdef Q_OS_WIN
  if (m_nativeMutexHandle) {
    HANDLE handle = static_cast<HANDLE>(m_nativeMutexHandle);
    ReleaseMutex(handle);
    CloseHandle(handle);
    m_nativeMutexHandle = nullptr;
  }
#else
  if (m_lockFile) {
    m_lockFile->unlock();
    m_lockFile.reset();
  }
#endif
}

bool SingleApplicationManager::notifyRunningInstance(
    const QByteArray &message) const {
  QElapsedTimer retryTimer;
  retryTimer.start();

  do {
    QLocalSocket socket;
    socket.connectToServer(serverName(), QIODevice::ReadWrite);
    if (socket.waitForConnected(kConnectAttemptTimeoutMs)) {
      if (socket.write(message) == message.size() &&
          socket.waitForBytesWritten(kReplyTimeoutMs) &&
          socket.waitForReadyRead(kReplyTimeoutMs) &&
          socket.readAll().contains(kAcknowledgement.trimmed())) {
        socket.disconnectFromServer();
        return true;
      }
      socket.abort();
    }

    QThread::msleep(kRetryDelayMs);
  } while (retryTimer.elapsed() < kStartupRaceTimeoutMs);

  return false;
}

void SingleApplicationManager::processSocketData(QLocalSocket *socket) {
  if (!socket) {
    return;
  }

  QByteArray buffer = socket->property("singleApplicationBuffer").toByteArray();
  buffer.append(socket->readAll());
  if (buffer.size() > kMaximumMessageBytes) {
    qWarning() << "SingleApplication: rejected oversized IPC message";
    socket->abort();
    return;
  }

  const qsizetype terminator = buffer.indexOf('\n');
  if (terminator < 0) {
    socket->setProperty("singleApplicationBuffer", buffer);
    return;
  }

  const QByteArray payload = buffer.left(terminator);
  QJsonParseError parseError;
  const QJsonDocument document = QJsonDocument::fromJson(payload, &parseError);
  if (parseError.error != QJsonParseError::NoError || !document.isObject()) {
    qWarning() << "SingleApplication: rejected malformed IPC message"
               << "| error:" << parseError.errorString();
    socket->abort();
    return;
  }

  const QJsonObject object = document.object();
  if (object.value(QStringLiteral("version")).toInt() != kProtocolVersion) {
    qWarning() << "SingleApplication: rejected unsupported IPC version";
    socket->abort();
    return;
  }

  QStringList arguments;
  const QJsonArray argumentValues =
      object.value(QStringLiteral("arguments")).toArray();
  arguments.reserve(argumentValues.size());
  for (const QJsonValue &value : argumentValues) {
    if (value.isString()) {
      arguments.append(value.toString());
    }
  }
  const QString workingDirectory =
      object.value(QStringLiteral("workingDirectory")).toString();

  socket->write(kAcknowledgement);
  socket->flush();
  socket->disconnectFromServer();

  qInfo() << "SingleApplication: activation requested by another launch"
          << "| argumentCount:" << arguments.size();
  emit activationRequested(arguments, workingDirectory);
}

QByteArray SingleApplicationManager::createMessage(
    const QStringList &arguments, const QString &workingDirectory) const {
  QJsonArray argumentValues;
  for (const QString &argument : arguments) {
    argumentValues.append(argument);
  }

  QJsonObject object;
  object.insert(QStringLiteral("version"), kProtocolVersion);
  object.insert(QStringLiteral("arguments"), argumentValues);
  object.insert(QStringLiteral("workingDirectory"), workingDirectory);
  QByteArray message = QJsonDocument(object).toJson(QJsonDocument::Compact);
  message.append('\n');
  return message;
}

QString SingleApplicationManager::lockFilePath() const {
  const QString storageDirectory =
      QStandardPaths::writableLocation(QStandardPaths::AppLocalDataLocation);
  return QDir(storageDirectory).filePath(QStringLiteral("instance.lock"));
}

QString SingleApplicationManager::serverName() const {
  const QByteArray identity =
      QCoreApplication::organizationName().toUtf8() + '/' +
      QCoreApplication::applicationName().toUtf8() + '/' +
      QStandardPaths::writableLocation(QStandardPaths::AppLocalDataLocation)
          .toUtf8();
  const QByteArray suffix =
      QCryptographicHash::hash(identity, QCryptographicHash::Sha256).toHex().left(16);
  return QStringLiteral("xplayer-%1").arg(QString::fromLatin1(suffix));
}
