#include "playbackprogresspolicy.h"

bool PlaybackProgressPolicy::representsSameState(
    const PlaybackProgressRequest& left,
    const PlaybackProgressRequest& right)
{
    return left.positionTicks == right.positionTicks &&
           left.isPaused == right.isPaused;
}

std::optional<PlaybackProgressRequest> PlaybackProgressPolicy::enqueue(
    const PlaybackProgressRequest& request)
{
    if (m_requestInFlight) {
        if (!m_pending.has_value() ||
            request.force ||
            !representsSameState(*m_pending, request)) {
            m_pending = request;
        }
        return std::nullopt;
    }

    if (!request.force && m_lastDispatched.has_value() &&
        representsSameState(*m_lastDispatched, request)) {
        return std::nullopt;
    }

    m_requestInFlight = true;
    m_lastDispatched = request;
    return request;
}

std::optional<PlaybackProgressRequest> PlaybackProgressPolicy::complete()
{
    m_requestInFlight = false;
    if (!m_pending.has_value()) {
        return std::nullopt;
    }

    const PlaybackProgressRequest next = *m_pending;
    m_pending.reset();
    return enqueue(next);
}

void PlaybackProgressPolicy::reset()
{
    m_lastDispatched.reset();
    m_pending.reset();
    m_requestInFlight = false;
}
