#ifndef LIBRARYPAGINATIONUTILS_H
#define LIBRARYPAGINATIONUTILS_H

namespace LibraryPaginationUtils {

struct PageState {
    int loadedCount = 0;
    int totalCount = 0;
    int nextStartIndex = 0;
    int nextRequestLimit = 0;
    bool hasMoreItems = false;
};

int initialRequestLimit();
PageState initialPageState(int totalRecordCount, int returnedItemCount);

} // namespace LibraryPaginationUtils

#endif
