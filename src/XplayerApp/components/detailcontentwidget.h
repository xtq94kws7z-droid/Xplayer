#ifndef DETAILCONTENTWIDGET_H
#define DETAILCONTENTWIDGET_H

#include <QWidget>
#include <QPixmap>
#include <QImage>
#include <QColor>
#include <QSize>

template <typename T>
class QFutureWatcher;
class QResizeEvent;

class DetailContentWidget : public QWidget {
    Q_OBJECT
    Q_PROPERTY(QColor baseColor READ baseColor WRITE setBaseColor)
    Q_PROPERTY(QColor gradientColor READ gradientColor WRITE setGradientColor)

public:
    explicit DetailContentWidget(QWidget* parent = nullptr);

    
    void setBackdrop(const QPixmap& pix);

    
    QColor baseColor() const;
    void setBaseColor(const QColor& color);

    QColor gradientColor() const;
    void setGradientColor(const QColor& color);

protected:
    void paintEvent(QPaintEvent* event) override;
    void resizeEvent(QResizeEvent* event) override;

private:
    void scheduleBackdropScale();
    QSize targetBackdropSize() const;

    QPixmap m_backdrop;
    QPixmap m_scaledBackdrop;
    QImage m_backdropImage;
    int m_cachedWidth;
    int m_scaleGeneration = 0;
    QSize m_pendingScaleSize;

    
    QColor m_baseColor;
    QColor m_gradientColor;
};

#endif 
