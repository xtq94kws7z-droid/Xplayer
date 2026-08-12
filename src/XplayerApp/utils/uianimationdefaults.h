#pragma once

#include <QEasingCurve>

namespace XplayerUi
{
constexpr int kTargetFrameRate = 60;
constexpr int kFrameIntervalMs = 1000 / kTargetFrameRate;
constexpr int kQuickAnimationMs = 180;
constexpr int kStandardAnimationMs = 240;
constexpr int kPanelAnimationMs = 280;
constexpr int kScrollAnimationMs = 170;
constexpr int kSpinnerCycleAnimationMs = 800;
constexpr int kDetailFirstLoadDelayMs = 0;
constexpr int kDetailDataPresentationDelayMs = 0;
constexpr int kDetailHeavyLoadDelayMs = kPanelAnimationMs + kFrameIntervalMs;
constexpr int kDeferredInitialLoadDelayMs = kPanelAnimationMs + kFrameIntervalMs;
constexpr int kCompactAnimationMs = 160;
constexpr int kHeroAnimationMs = 420;
constexpr int kWindowExpandAnimationMs = 500;
constexpr int kWindowRestoreAnimationMs = 350;
constexpr int kPostNavigationCleanupDelayMs = kPanelAnimationMs + kFrameIntervalMs;
constexpr int kMicroAnimationMs = 120;
constexpr int kSidebarAutoHideMs = 4200;
constexpr int kPointerPollMs = kFrameIntervalMs;
constexpr int kOsdHoldMs = 1100;
constexpr int kToastHoldMs = 2200;
constexpr int kAutoLoginDelayMs = 0;
constexpr int kMpvWarmupDelayMs = 0;

enum class MotionDuration
{
    Instant,
    Micro,
    Quick,
    Compact,
    Standard,
    Panel,
    Large,
    Hero,
    SpinnerCycle
};

enum class MotionCurve
{
    Enter,
    Exit,
    Move,
    Resize,
    Pop,
    Spinner
};

inline int durationMs(MotionDuration duration)
{
    switch (duration)
    {
    case MotionDuration::Instant:
        return 0;
    case MotionDuration::Micro:
        return kMicroAnimationMs;
    case MotionDuration::Quick:
        return kQuickAnimationMs;
    case MotionDuration::Compact:
        return kCompactAnimationMs;
    case MotionDuration::Standard:
        return kStandardAnimationMs;
    case MotionDuration::Panel:
        return kPanelAnimationMs;
    case MotionDuration::Large:
        return 360;
    case MotionDuration::Hero:
        return kHeroAnimationMs;
    case MotionDuration::SpinnerCycle:
        return kSpinnerCycleAnimationMs;
    }
    return kStandardAnimationMs;
}

inline QEasingCurve easingCurve(MotionCurve curve)
{
    switch (curve)
    {
    case MotionCurve::Enter:
        return QEasingCurve(QEasingCurve::OutCubic);
    case MotionCurve::Exit:
        return QEasingCurve(QEasingCurve::InCubic);
    case MotionCurve::Move:
        return QEasingCurve(QEasingCurve::OutCubic);
    case MotionCurve::Resize:
        return QEasingCurve(QEasingCurve::InOutCubic);
    case MotionCurve::Pop:
        return QEasingCurve(QEasingCurve::InExpo);
    case MotionCurve::Spinner:
        return QEasingCurve(QEasingCurve::Linear);
    }
    return QEasingCurve(QEasingCurve::OutCubic);
}
} // namespace XplayerUi
