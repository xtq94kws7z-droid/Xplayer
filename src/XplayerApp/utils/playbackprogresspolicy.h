#pragma once

#include <optional>

struct PlaybackProgressRequest
{
    long long positionTicks = 0;
    bool isPaused = false;
    bool force = false;
};

class PlaybackProgressPolicy
{
public:
    std::optional<PlaybackProgressRequest> enqueue(
        const PlaybackProgressRequest& request);
    std::optional<PlaybackProgressRequest> complete();
    void reset();

private:
    static bool representsSameState(const PlaybackProgressRequest& left,
                                    const PlaybackProgressRequest& right);

    std::optional<PlaybackProgressRequest> m_lastDispatched;
    std::optional<PlaybackProgressRequest> m_pending;
    bool m_requestInFlight = false;
};
