#include "utils/posterwallpreparationpolicy.h"

#include <QTextStream>

namespace
{
bool check(bool condition, const char* message)
{
    if (condition) {
        return true;
    }
    QTextStream(stderr) << "posterwallpreparationpolicy_test failed: "
                        << message << Qt::endl;
    return false;
}
} // namespace

int main()
{
    using namespace PosterWallPreparationPolicy;

    if (!check(actionForFailure(0, 2) == FailureAction::Retry,
               "first failure retries")) {
        return 1;
    }
    if (!check(actionForFailure(1, 2) == FailureAction::Retry,
               "second failure retries")) {
        return 1;
    }
    if (!check(actionForFailure(2, 2) == FailureAction::ResolveUnavailable,
               "third failure resolves unavailable")) {
        return 1;
    }
    if (!check(canApplyResult(QStringLiteral("server|user"),
                              QStringLiteral("server|user")),
               "matching context accepts async result")) {
        return 1;
    }
    if (!check(!canApplyResult(QStringLiteral("server|old-user"),
                               QStringLiteral("server|new-user")),
               "account switch rejects stale async result")) {
        return 1;
    }
    if (!check(!canApplyResult(QString(), QString()),
               "empty context never accepts async result")) {
        return 1;
    }
    return 0;
}
