#include "singleflightrefreshpolicy.h"

std::optional<SingleFlightRefreshPolicy::Request>
SingleFlightRefreshPolicy::request(bool force)
{
    if (m_inFlight) {
        m_forceQueued = m_forceQueued || force;
        return std::nullopt;
    }

    m_inFlight = true;
    return Request{++m_generation, force};
}

std::optional<SingleFlightRefreshPolicy::Request>
SingleFlightRefreshPolicy::complete(int generation)
{
    if (!m_inFlight || generation != m_generation) {
        return std::nullopt;
    }

    m_inFlight = false;
    if (!m_forceQueued) {
        return std::nullopt;
    }

    m_forceQueued = false;
    m_inFlight = true;
    return Request{++m_generation, true};
}

void SingleFlightRefreshPolicy::invalidate()
{
    ++m_generation;
    m_inFlight = false;
    m_forceQueued = false;
}

bool SingleFlightRefreshPolicy::isCurrent(int generation) const
{
    return m_inFlight && generation == m_generation;
}

bool SingleFlightRefreshPolicy::isInFlight() const
{
    return m_inFlight;
}
