#include "dashboardloadstageutils.h"
#include "uianimationdefaults.h"

namespace DashboardLoadStageUtils
{
bool isFirstStage(Section section)
{
    return section == Section::Resume || section == Section::Recommended;
}

int deferredDelayMs()
{
    return XplayerUi::kDeferredInitialLoadDelayMs;
}
} // namespace DashboardLoadStageUtils
