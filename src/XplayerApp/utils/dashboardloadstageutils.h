#pragma once

namespace DashboardLoadStageUtils
{
enum class Section
{
    Resume,
    Recommended,
    Latest,
    Completed,
    Libraries
};

bool isFirstStage(Section section);
int deferredDelayMs();
} // namespace DashboardLoadStageUtils
