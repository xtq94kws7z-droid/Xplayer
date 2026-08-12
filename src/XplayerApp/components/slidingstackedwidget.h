#ifndef SLIDINGSTACKEDWIDGET_H
#define SLIDINGSTACKEDWIDGET_H

#include <QStackedWidget>
#include <QPropertyAnimation>
#include <QParallelAnimationGroup>
#include <QEasingCurve>
#include <QPointer>
#include <QList>

class QLabel;


class SlidingStackedWidget : public QStackedWidget
{
    Q_OBJECT
public:
    enum SlideDirection {
        LeftToRight,
        RightToLeft,
        TopToBottom,
        BottomToTop,
        Automatic 
    };

    explicit SlidingStackedWidget(QWidget *parent = nullptr);

    void setSpeed(int speed);
    void setEasingCurve(QEasingCurve::Type curveType);

    
    void slideInIdx(int idx, SlideDirection direction = Automatic);
    void slideInWgt(QWidget *widget, SlideDirection direction = Automatic);
    void disposeWidgetWhenSafe(QWidget *widget);

Q_SIGNALS:
    
    
    void animationFinished();

private Q_SLOTS:
    void animationDoneSlot();

private:
    void completeAnimation(bool emitFinishedSignal);
    void flushPendingWidgetDisposals();
    void clearSnapshotLabels();
    QLabel *createSnapshotLabel(QWidget *source, const QRect &geometry);

    int m_speed;
    QEasingCurve::Type m_animationType;
    QParallelAnimationGroup *m_animGroup;
    bool m_isAnimating;
    int m_nextIndex;
    QList<QPointer<QWidget>> m_pendingWidgetDisposals;
    QList<QPointer<QWidget>> m_snapshotHiddenWidgets;
    QList<QPointer<QLabel>> m_snapshotLabels;
    bool m_deferPendingDisposalsUntilAfterAnimation = false;
};

#endif 
