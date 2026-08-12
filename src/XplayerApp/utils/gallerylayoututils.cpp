#include "gallerylayoututils.h"

#include <QtGlobal>

namespace GalleryLayoutUtils
{

int viewportHeightForCard(int requestedHeight, int cardHeight)
{
    return qMax(qMax(0, requestedHeight), qMax(0, cardHeight));
}

} // namespace GalleryLayoutUtils
