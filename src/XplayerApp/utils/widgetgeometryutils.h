#pragma once

#include <QPoint>
#include <QRect>
#include <QSize>
#include <QString>

class QLabel;
class QWidget;

namespace XplayerUi
{
bool setVisibleIfChanged(QWidget* widget, bool visible);
bool setGeometryIfChanged(QWidget* widget, const QRect& geometry);
bool moveIfChanged(QWidget* widget, const QPoint& position);
bool resizeIfChanged(QWidget* widget, const QSize& size);
bool setFixedSizeIfChanged(QWidget* widget, const QSize& size);
bool setTextIfChanged(QLabel* label, const QString& text);
} // namespace XplayerUi
