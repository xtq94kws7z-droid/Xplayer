#ifndef SINGLEAPPLICATIONMANAGER_H
#define SINGLEAPPLICATIONMANAGER_H

#include <QObject>
#include <memory>

class QLocalServer;
class QLockFile;
class QLocalSocket;

class SingleApplicationManager : public QObject {
  Q_OBJECT

public:
  enum class StartResult { PrimaryInstance, SecondaryInstance, FatalError };

  explicit SingleApplicationManager(QObject *parent = nullptr);
  ~SingleApplicationManager() override;
  StartResult start(const QStringList &arguments,
                    const QString &workingDirectory);

signals:
  void activationRequested(const QStringList &arguments,
                           const QString &workingDirectory);

private:
  bool acquirePrimaryGuard();
  void releasePrimaryGuard();
  bool notifyRunningInstance(const QByteArray &message) const;
  void processSocketData(QLocalSocket *socket);
  QByteArray createMessage(const QStringList &arguments,
                           const QString &workingDirectory) const;
  QString serverName() const;
  QString lockFilePath() const;

  QLocalServer *m_server = nullptr;
  std::unique_ptr<QLockFile> m_lockFile;
#ifdef Q_OS_WIN
  void *m_nativeMutexHandle = nullptr;
#endif
};

#endif 
