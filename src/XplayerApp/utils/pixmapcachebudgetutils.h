#pragma once

#include <QtGlobal>

class QPixmap;

namespace PixmapCacheBudgetUtils
{
qsizetype costBytes(const QPixmap& pixmap);
bool exceedsBudget(qsizetype currentCostBytes, qsizetype budgetBytes);
} // namespace PixmapCacheBudgetUtils
