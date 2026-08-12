#include "playbackloadingstateutils.h"

namespace PlaybackLoadingStateUtils
{

State resolveState(const Context &context)
{
    if (context.isTearingDown || !context.hasMedia)
    {
        return State::Hidden;
    }
    if (context.isSeeking)
    {
        return State::Seeking;
    }
    if (context.isBuffering)
    {
        return State::Buffering;
    }
    if (context.isOpening)
    {
        return State::Opening;
    }
    return State::Hidden;
}

QString displayText(State state)
{
    switch (state)
    {
    case State::Opening:
        return QStringLiteral("正在准备播放...");
    case State::Buffering:
        return QStringLiteral("正在缓冲...");
    case State::Seeking:
        return QStringLiteral("正在定位...");
    case State::Hidden:
        break;
    }
    return {};
}

} // namespace PlaybackLoadingStateUtils
