#ifndef PLAYERWINDOW_H
#define PLAYERWINDOW_H

#include <QWidget>
#include <QPointer>
#include <QMetaObject>

class XplayerCore;
class PlayerView;
class QCloseEvent;
class QParallelAnimationGroup;
namespace QWK {
class WidgetWindowAgent;
}




class PlayerWindow : public QWidget
{
    Q_OBJECT
public:
    explicit PlayerWindow(XplayerCore* core, QWidget *parent = nullptr);

    void playMedia(const QString &mediaId, const QString &title,
                   const QString &streamUrl, long long startPositionTicks = 0,
                   const QVariant& sourceInfoVar = QVariant());
    void playLaunchTransition();

private:
    void bindScreenSignals();
    void setMacSystemButtonsVisible(bool visible);
    void closeEvent(QCloseEvent *event) override;

    PlayerView* m_playerView = nullptr;
    QWK::WidgetWindowAgent* m_windowAgent = nullptr;
    QParallelAnimationGroup *m_launchAnimation = nullptr;
    QMetaObject::Connection m_screenChangedConnection;
    QMetaObject::Connection m_dpiChangedConnection;
};

#endif 
