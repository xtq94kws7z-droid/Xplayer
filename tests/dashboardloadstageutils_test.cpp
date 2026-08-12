#include "../src/XplayerApp/utils/dashboardloadstageutils.h"

#include <QTextStream>

namespace
{
bool check(bool condition, const char *message)
{
    if (condition) {
        return true;
    }

    QTextStream(stderr) << "dashboardloadstageutils_test failed: "
                        << message << Qt::endl;
    return false;
}

bool verifyResumeAndRecommendedAreFirstStage()
{
    return check(DashboardLoadStageUtils::isFirstStage(
                     DashboardLoadStageUtils::Section::Resume),
                 "resume belongs to first stage") &&
           check(DashboardLoadStageUtils::isFirstStage(
                     DashboardLoadStageUtils::Section::Recommended),
                 "recommended belongs to first stage");
}

bool verifySecondarySectionsAreDeferred()
{
    return check(!DashboardLoadStageUtils::isFirstStage(
                     DashboardLoadStageUtils::Section::Latest),
                 "latest is deferred") &&
           check(!DashboardLoadStageUtils::isFirstStage(
                     DashboardLoadStageUtils::Section::Completed),
                 "completed is deferred") &&
           check(!DashboardLoadStageUtils::isFirstStage(
                     DashboardLoadStageUtils::Section::Libraries),
                 "libraries are deferred");
}

bool verifyDeferredDelayUsesBoundedUiBudget()
{
    const int delayMs = DashboardLoadStageUtils::deferredDelayMs();
    return check(delayMs > 0, "deferred delay is positive") &&
           check(delayMs <= 300, "deferred delay is bounded");
}
} // namespace

int main()
{
    return verifyResumeAndRecommendedAreFirstStage() &&
                   verifySecondarySectionsAreDeferred() &&
                   verifyDeferredDelayUsesBoundedUiBudget()
               ? 0
               : 1;
}
