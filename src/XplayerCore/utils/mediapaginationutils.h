#ifndef MEDIAPAGINATIONUTILS_H
#define MEDIAPAGINATIONUTILS_H

namespace MediaPaginationUtils {

int safeInitialRequestLimit(int requestedLimit);
int safePageLimit(int requestedLimit);

} // namespace MediaPaginationUtils

#endif
