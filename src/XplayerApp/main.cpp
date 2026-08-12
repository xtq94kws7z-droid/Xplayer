#include "mainwindow.h"

#include <QApplication>
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QIcon>
#include <QLocale>
#include <QStandardPaths>
#include <QSurfaceFormat>
#include <QThread>
#include <QTimer>
#include <QTranslator>
#include "api/proxymanager.h"
#include "components/mpvcontroller.h"
#include "managers/languagemanager.h"
#include "managers/logmanager.h"
#include "managers/singleapplicationmanager.h"
#include "config/config_keys.h"
#include "config/configstore.h"
#include "utils/uianimationdefaults.h"

#ifdef Q_OS_WIN
#include <windows.h>
#endif

namespace {
constexpr auto kOrganizationName = "Godking";
constexpr auto kLegacyOrganizationName = "AlanHJ";

void moveMissingStorageTree(const QString &sourcePath,
                            const QString &destinationPath) {
  const QDir source(sourcePath);
  if (!source.exists() || sourcePath == destinationPath) {
    return;
  }

  const QString destinationParent = QFileInfo(destinationPath).absolutePath();
  QDir().mkpath(destinationParent);
  if (!QFileInfo::exists(destinationPath) &&
      QDir().rename(sourcePath, destinationPath)) {
    return;
  }

  QDir().mkpath(destinationPath);
  const QFileInfoList entries = source.entryInfoList(
      QDir::Dirs | QDir::Files | QDir::NoDotAndDotDot | QDir::Hidden |
          QDir::System,
      QDir::Name);
  for (const QFileInfo &entry : entries) {
    const QString destination =
        QDir(destinationPath).filePath(entry.fileName());
    if (entry.isDir()) {
      moveMissingStorageTree(entry.absoluteFilePath(), destination);
    } else if (!QFileInfo::exists(destination)) {
      if (!QFile::rename(entry.absoluteFilePath(), destination) &&
          QFile::copy(entry.absoluteFilePath(), destination)) {
        QFile::remove(entry.absoluteFilePath());
      }
    }
  }
}

QStringList applicationStoragePaths() {
  QStringList paths = {
      QStandardPaths::writableLocation(QStandardPaths::AppLocalDataLocation),
      QStandardPaths::writableLocation(QStandardPaths::AppDataLocation),
      QStandardPaths::writableLocation(QStandardPaths::AppConfigLocation),
  };
  paths.removeDuplicates();
  return paths;
}

void migrateLegacyOrganizationStorage(QApplication &application) {
  application.setOrganizationName(kLegacyOrganizationName);
  const QStringList legacyPaths = applicationStoragePaths();

  application.setOrganizationName(kOrganizationName);
  const QStringList currentPaths = applicationStoragePaths();
  const qsizetype pathCount = qMin(legacyPaths.size(), currentPaths.size());
  for (qsizetype index = 0; index < pathCount; ++index) {
    moveMissingStorageTree(legacyPaths.at(index), currentPaths.at(index));
  }
}
} // namespace

int main(int argc, char *argv[]) {
#ifdef Q_OS_WIN
  bool isRDP = GetSystemMetrics(SM_REMOTESESSION) != 0;
  if (isRDP) {
    
    
    qputenv("QT_OPENGL", "software");
  }
#endif

  
  QSurfaceFormat format;
  format.setVersion(3, 3); 
  format.setProfile(QSurfaceFormat::CoreProfile);
  format.setDepthBufferSize(24);  
  format.setStencilBufferSize(8); 
  format.setSwapInterval(1);      
  format.setSwapBehavior(QSurfaceFormat::DoubleBuffer); 
  QSurfaceFormat::setDefaultFormat(format);

  QGuiApplication::setAttribute(Qt::AA_DontCreateNativeWidgetSiblings);
  QApplication a(argc, argv);
  a.setApplicationName(APP_NAME);
  a.setApplicationVersion(APP_VERSION);
  a.setOrganizationDomain("local.xplayer");
  migrateLegacyOrganizationStorage(a);

  SingleApplicationManager singleApplication;
  const auto singleApplicationResult = singleApplication.start(
      QCoreApplication::arguments(), QDir::currentPath());
  if (singleApplicationResult ==
      SingleApplicationManager::StartResult::SecondaryInstance) {
    return 0;
  }
  if (singleApplicationResult ==
      SingleApplicationManager::StartResult::FatalError) {
    return 1;
  }

  LogManager::instance()->init();
#if !defined(Q_OS_MACOS) && !defined(Q_OS_MAC)
  QGuiApplication::setDesktopFileName(QStringLiteral("xplayer"));
  const QIcon appIcon(QStringLiteral(":/images/xplayer_icon.png"));
  a.setWindowIcon(appIcon);
#endif

  LanguageManager::instance()->init();

  
  
  ProxyManager::installApplicationFactory();

  MainWindow w;
  QObject::connect(&singleApplication,
                   &SingleApplicationManager::activationRequested, &w,
                   &MainWindow::handleExternalActivation);
#if !defined(Q_OS_MACOS) && !defined(Q_OS_MAC)
  w.setWindowIcon(appIcon);
#endif
  w.show();

  QTimer::singleShot(XplayerUi::kMpvWarmupDelayMs, &a, []() {
    MpvController::warmupOnce();
  });

  int ret = a.exec();
  
  return ret;
}
