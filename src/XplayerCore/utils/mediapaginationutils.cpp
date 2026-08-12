#include "mediapaginationutils.h"

#include <QtGlobal>

namespace MediaPaginationUtils {
namespace {
constexpr int kServerSafePageSize = 50;
}

int safeInitialRequestLimit(int requestedLimit)
{
    if (requestedLimit <= 0) {
        return requestedLimit;
    }
    return qMin(requestedLimit, kServerSafePageSize);
}

int safePageLimit(int requestedLimit)
{
    if (requestedLimit <= 0) {
        return kServerSafePageSize;
    }
    return qMin(requestedLimit, kServerSafePageSize);
}

} // namespace MediaPaginationUtils
