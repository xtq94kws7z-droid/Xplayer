#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QCloseEvent>   
#include <QElapsedTimer> 
#include <QMainWindow>

class XplayerCore;
class LoginView;
class HomeView;
class QStackedWidget;
class QLineEdit;
class QCompleter;
class QStringListModel;
class TrayManager; 
class SearchHistoryPopup;
class QResizeEvent;
namespace QWK {
class WidgetWindowAgent;
}

class MainWindow : public QMainWindow
{
    Q_OBJECT

    public:
        MainWindow(QWidget *parent = nullptr);
        ~MainWindow();

    public Q_SLOTS:
        void navigateToHome();
        void navigateToLogin();
        void handleExternalActivation(const QStringList &arguments,
                                      const QString &workingDirectory);

    protected:
        bool event(QEvent *event) override;
        bool eventFilter(QObject * watched, QEvent * event) override;
        void resizeEvent(QResizeEvent *event) override;
        void closeEvent(QCloseEvent * event) override; 

    private:
        void applyResponsiveLayout();
        void setupGlobalSearchHistory();
        void hideGlobalSearchTransientUi();
        void updateGlobalSearchCompleter(const QString &text = QString());
        void showGlobalSearchHistoryPopup(const QString &filterText = QString());
        void submitGlobalSearch(const QString &query);
        QString currentSearchServerId() const;
        HomeView* ensureHomeView();

        XplayerCore *m_core;
        QStackedWidget *m_viewStack;
        LoginView *m_loginView;
        HomeView *m_homeView = nullptr;
        QWK::WidgetWindowAgent *m_windowAgent = nullptr;
        QLineEdit *m_globalSearchBox;
        QCompleter *m_globalSearchCompleter = nullptr;
        QStringListModel *m_globalSearchModel = nullptr;
        SearchHistoryPopup *m_globalSearchHistoryPopup = nullptr;
        TrayManager *m_trayManager = nullptr; 

        
        QElapsedTimer m_backClickTimer;

        quint32 m_defaultWidth{450};
        quint32 m_defaultHeight{320};
        bool m_realQuit{false};        
        bool m_themeAnimating{false};  
        bool m_wasPausedByTray{false}; 
        bool m_hadPlayerWhenHiddenToTray{false};
};

#endif 
