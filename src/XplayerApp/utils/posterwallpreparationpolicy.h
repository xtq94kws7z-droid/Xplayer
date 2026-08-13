#pragma once

#include <QString>

namespace PosterWallPreparationPolicy
{
enum class FailureAction
{
    Retry,
    ResolveUnavailable
};

FailureAction actionForFailure(int completedRetries, int retryLimit);
bool canApplyResult(const QString& requestContextKey,
                    const QString& currentContextKey);
} // namespace PosterWallPreparationPolicy
