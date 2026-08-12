#include "librarypaginationutils.h"

#include <QtGlobal>

namespace LibraryPaginationUtils {
namespace {
constexpr int kServerSafePageSize = 50;
}

int initialRequestLimit()
{
    return kServerSafePageSize;
}

PageState initialPageState(int totalRecordCount, int returnedItemCount)
{
    PageState state;
    state.loadedCount = qMax(0, returnedItemCount);
    state.totalCount = qMax(qMax(0, totalRecordCount), state.loadedCount);
    state.nextStartIndex = state.loadedCount;
    state.hasMoreItems = state.loadedCount > 0 &&
                         state.loadedCount < state.totalCount;
    if (state.hasMoreItems) {
        state.nextRequestLimit =
            qMin(kServerSafePageSize, state.totalCount - state.loadedCount);
    }
    return state;
}

} // namespace LibraryPaginationUtils
