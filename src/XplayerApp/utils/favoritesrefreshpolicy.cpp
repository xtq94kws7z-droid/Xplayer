#include "favoritesrefreshpolicy.h"

FavoritesRefreshPolicy::RefreshDecision FavoritesRefreshPolicy::requestRefresh(
    qint64 nowMs, bool force)
{
    if (m_inFlight) {
        if (force) {
            m_forceRefreshQueued = true;
        }
        return {false, m_generation};
    }

    const bool freshEnough =
        m_hasCompletedLoad && nowMs - m_lastCompletedMs < kAutoRefreshStaleMs;
    if (!force && freshEnough) {
        return {false, m_generation};
    }

    m_inFlight = true;
    m_forceRefreshQueued = false;
    return {true, ++m_generation};
}

bool FavoritesRefreshPolicy::completeRefresh(int generation, qint64 nowMs)
{
    if (generation != m_generation) {
        return false;
    }

    m_inFlight = false;
    m_hasCompletedLoad = true;
    m_lastCompletedMs = nowMs;

    const bool queuedRefresh = m_forceRefreshQueued;
    m_forceRefreshQueued = false;
    return queuedRefresh;
}

bool FavoritesRefreshPolicy::isCurrentGeneration(int generation) const
{
    return generation == m_generation;
}

bool FavoritesRefreshPolicy::isInFlight() const
{
    return m_inFlight;
}

int FavoritesRefreshPolicy::generation() const
{
    return m_generation;
}
