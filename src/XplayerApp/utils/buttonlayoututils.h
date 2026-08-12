#pragma once

class QFont;
class QString;

namespace ButtonLayoutUtils
{

int minimumTextButtonWidth(const QString& text, const QFont& font,
                           int horizontalPadding, int safetyMargin);
int minimumSectionMoreButtonWidth(const QString& text, const QFont& font);
QString sectionMoreButtonDisplayText(const QString& text);

} // namespace ButtonLayoutUtils
