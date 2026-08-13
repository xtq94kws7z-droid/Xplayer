#include "posterwallpreparationpolicy.h"

namespace PosterWallPreparationPolicy
{
FailureAction actionForFailure(int completedRetries, int retryLimit)
{
    return completedRetries < retryLimit ? FailureAction::Retry
                                         : FailureAction::ResolveUnavailable;
}

bool canApplyResult(const QString& requestContextKey,
                    const QString& currentContextKey)
{
    return !requestContextKey.isEmpty() &&
           requestContextKey == currentContextKey;
}
} // namespace PosterWallPreparationPolicy
