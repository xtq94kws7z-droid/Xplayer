#pragma once

class QImage;
class QSize;
class QString;

namespace ImageCacheBudgetUtils
{
int costKb(const QImage& image, int maxSingleItemCostKb);
int effectiveMaxWidth(int requestedMaxWidth, const QString& quality,
                      bool enforceRequestedLimit);
QSize decodeSizeForMaxWidth(const QSize& sourceSize, int maxWidth);
} // namespace ImageCacheBudgetUtils
