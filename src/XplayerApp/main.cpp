#include "mainwindow.h"

#include <QApplication>
#include <QDir>
#include <QIcon>
#include <QLocale>
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
  a.setOrganizationName(kOrganizationName);

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
