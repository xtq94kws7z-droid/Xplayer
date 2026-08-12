#include "buttonlayoututils.h"

#include <QFontMetrics>
#include <QString>

namespace ButtonLayoutUtils
{

int minimumTextButtonWidth(const QString& text, const QFont& font,
                           int horizontalPadding, int safetyMargin)
{
    const int padding = qMax(0, horizontalPadding);
    const int safety = qMax(0, safetyMargin);
    const QFontMetrics metrics(font);
    const QRect glyphBounds = metrics.boundingRect(text);
    const int renderedLeft = qMin(0, glyphBounds.left());
    const int renderedRight =
        qMax(metrics.horizontalAdvance(text), glyphBounds.right() + 1);
    return qMax(0, renderedRight - renderedLeft) + padding * 2 + safety;
}

int minimumSectionMoreButtonWidth(const QString& text, const QFont& font)
{
    return minimumTextButtonWidth(text, font, 14, 8);
}

QString sectionMoreButtonDisplayText(const QString& text)
{
    const QChar inset(0x2002);
    return inset + text + inset;
}

} // namespace ButtonLayoutUtils
