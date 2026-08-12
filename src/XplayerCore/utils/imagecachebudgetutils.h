#pragma once

class QImage;

namespace ImageCacheBudgetUtils
{
int costKb(const QImage& image, int maxSingleItemCostKb);
} // namespace ImageCacheBudgetUtils
