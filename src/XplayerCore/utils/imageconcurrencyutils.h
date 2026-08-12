#ifndef IMAGECONCURRENCYUTILS_H
#define IMAGECONCURRENCYUTILS_H

#include <QtGlobal>

namespace ImageConcurrencyUtils
{
constexpr int kStartupImageConcurrency = 4;
constexpr int kNormalImageConcurrency = 8;
constexpr qint64 kStartupImageBurstDurationMs = 5000;

int maxConcurrentRequests(qint64 startupElapsedMs);
}

#endif
