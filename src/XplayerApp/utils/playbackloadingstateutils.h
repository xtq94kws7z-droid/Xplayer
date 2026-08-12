#pragma once

#include <QString>

namespace PlaybackLoadingStateUtils
{

enum class State
{
    Hidden,
    Opening,
    Buffering,
    Seeking
};

struct Context
{
    bool hasMedia = false;
    bool isOpening = false;
    bool isBuffering = false;
    bool isSeeking = false;
    bool isTearingDown = false;
};

State resolveState(const Context &context);
QString displayText(State state);

} // namespace PlaybackLoadingStateUtils
