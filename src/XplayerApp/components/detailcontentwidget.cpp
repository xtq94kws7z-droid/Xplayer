#include "detailcontentwidget.h"
#include <QFutureWatcher>
#include <QPainter>
#include <QPointer>
#include <QResizeEvent>
#include <QStyleOption>
#include <QLinearGradient>
#include <QtConcurrent/QtConcurrentRun>

DetailContentWidget::DetailContentWidget(QWidget* parent)
    : QWidget(parent),
    m_cachedWidth(-1),
    m_baseColor(QColor("#FFFFFF")),
    m_gradientColor(QColor(249, 250, 251))
{
    
    setAttribute(Qt::WA_StyledBackground, true);
}

void DetailContentWidget::setBackdrop(const QPixmap& pix) {
    if (m_backdrop.cacheKey() == pix.cacheKey() &&
        m_backdrop.size() == pix.size()) {
        return;
    }

    m_backdrop = pix;
    m_backdropImage = pix.toImage();
    m_scaledBackdrop = QPixmap();
    m_cachedWidth = -1;
    m_pendingScaleSize = QSize();
    ++m_scaleGeneration;
    scheduleBackdropScale();
    update();
}

QColor DetailContentWidget::baseColor() const {
    return m_baseColor;
}

void DetailContentWidget::setBaseColor(const QColor& color) {
    if (m_baseColor != color) {
        m_baseColor = color;
        update(); 
    }
}

QColor DetailContentWidget::gradientColor() const {
    return m_gradientColor;
}

void DetailContentWidget::setGradientColor(const QColor& color) {
    if (m_gradientColor != color) {
        m_gradientColor = color;
        update();
    }
}

void DetailContentWidget::paintEvent(QPaintEvent* event) {
    Q_UNUSED(event);

    
    QStyleOption opt;
    opt.initFrom(this);
    QPainter painter(this);
    style()->drawPrimitive(QStyle::PE_Widget, &opt, &painter, this);

    painter.fillRect(rect(), m_baseColor);

    if (m_backdrop.isNull()) return;

    const QSize targetSize = targetBackdropSize();
    if (!targetSize.isValid()) return;

    if (m_cachedWidth != width() || m_scaledBackdrop.isNull()) {
        scheduleBackdropScale();
    }

    
    if (!m_scaledBackdrop.isNull() && m_cachedWidth == width()) {
        painter.drawPixmap(0, 0, m_scaledBackdrop);
    } else {
        painter.drawPixmap(0, 0, m_backdrop);
    }

    
    QLinearGradient gradient(0, 0, 0, targetSize.height());
    int r = m_gradientColor.red();
    int g = m_gradientColor.green();
    int b = m_gradientColor.blue();

    
    gradient.setColorAt(0.0,  QColor(r, g, b, 0));
    gradient.setColorAt(0.15, QColor(r, g, b, 150));
    gradient.setColorAt(0.4,  QColor(r, g, b, 180));
    gradient.setColorAt(1.0,  QColor(r, g, b, 255));

    painter.fillRect(0, 0, width(), targetSize.height(), gradient);
}

void DetailContentWidget::resizeEvent(QResizeEvent* event)
{
    QWidget::resizeEvent(event);
    if (event->oldSize().width() != event->size().width()) {
        scheduleBackdropScale();
    }
}

QSize DetailContentWidget::targetBackdropSize() const
{
    if (m_backdropImage.isNull() || width() <= 0 ||
        m_backdropImage.width() <= 0) {
        return QSize();
    }

    return QSize(width(),
                 width() * m_backdropImage.height() / m_backdropImage.width());
}

void DetailContentWidget::scheduleBackdropScale()
{
    const QSize targetSize = targetBackdropSize();
    if (!targetSize.isValid()) {
        return;
    }
    if (!m_scaledBackdrop.isNull() && m_cachedWidth == width()) {
        return;
    }
    if (m_pendingScaleSize == targetSize) {
        return;
    }

    m_pendingScaleSize = targetSize;
    const int generation = ++m_scaleGeneration;
    const QImage source = m_backdropImage;
    QPointer<DetailContentWidget> safeThis(this);

    auto* watcher = new QFutureWatcher<QImage>(this);
    connect(watcher, &QFutureWatcher<QImage>::finished, this,
            [safeThis, watcher, generation, targetSize]()
            {
                if (safeThis && safeThis->m_scaleGeneration == generation) {
                    const QImage scaledImage = watcher->result();
                    if (!scaledImage.isNull()) {
                        safeThis->m_scaledBackdrop =
                            QPixmap::fromImage(scaledImage);
                        safeThis->m_cachedWidth = targetSize.width();
                        safeThis->m_pendingScaleSize = QSize();
                        safeThis->update();
                    }
                }
                watcher->deleteLater();
            });
    watcher->setFuture(QtConcurrent::run(
        [source, targetSize]() mutable
        {
            return source.scaled(targetSize, Qt::KeepAspectRatio,
                                 Qt::SmoothTransformation);
        }));
}
