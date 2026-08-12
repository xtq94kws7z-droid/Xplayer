#include "widgetgeometryutils.h"

#include <QLabel>
#include <QWidget>

namespace XplayerUi
{
bool setVisibleIfChanged(QWidget* widget, bool visible)
{
    if (!widget || widget->isVisible() == visible)
        return false;

    widget->setVisible(visible);
    return true;
}

bool setGeometryIfChanged(QWidget* widget, const QRect& geometry)
{
    if (!widget || widget->geometry() == geometry)
        return false;

    widget->setGeometry(geometry);
    return true;
}

bool moveIfChanged(QWidget* widget, const QPoint& position)
{
    if (!widget || widget->pos() == position)
        return false;

    widget->move(position);
    return true;
}

bool resizeIfChanged(QWidget* widget, const QSize& size)
{
    if (!widget || widget->size() == size)
        return false;

    widget->resize(size);
    return true;
}

bool setFixedSizeIfChanged(QWidget* widget, const QSize& size)
{
    if (!widget)
        return false;

    const bool alreadyFixedToSize =
        widget->minimumSize() == size && widget->maximumSize() == size;
    if (alreadyFixedToSize)
        return false;

    widget->setFixedSize(size);
    return true;
}

bool setTextIfChanged(QLabel* label, const QString& text)
{
    if (!label || label->text() == text)
        return false;

    label->setText(text);
    return true;
}
} // namespace XplayerUi
