#ifndef SHIMMERWIDGET_H
#define SHIMMERWIDGET_H

#include <QWidget>
#include <QSize>
#include <QPainterPath>

class QPropertyAnimation;


class ShimmerWidget : public QWidget
{
    Q_OBJECT
    Q_PROPERTY(qreal shimmerPhase READ shimmerPhase WRITE setShimmerPhase)

public:
    explicit ShimmerWidget(QWidget* parent = nullptr);
    ~ShimmerWidget() override;

    
    void setCardSize(const QSize& size);

    
    void setCardSpacing(int spacing);

    
    void setCardRadius(int radius);

    
    void setShowSubtitle(bool show);

    
    qreal shimmerPhase() const;
    void setShimmerPhase(qreal phase);

    
    void startAnimation();
    void stopAnimation();
    bool isAnimating() const;

protected:
    void paintEvent(QPaintEvent* event) override;

private:
    void rebuildShapePath();

    QSize m_cardSize{160, 270};
    int m_cardSpacing = 0;
    int m_cardRadius = 8;
    bool m_showSubtitle = true;
    qreal m_shimmerPhase = 0.0;
    QSize m_cachedCanvasSize;
    QPainterPath m_cachedShapePath;
    bool m_shapePathDirty = true;
    QPropertyAnimation* m_animation = nullptr;
};

#endif 
