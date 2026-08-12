#ifndef SINGLEFLIGHTREFRESHPOLICY_H
#define SINGLEFLIGHTREFRESHPOLICY_H

#include <optional>

class SingleFlightRefreshPolicy
{
public:
    struct Request
    {
        int generation = 0;
        bool force = false;
    };

    std::optional<Request> request(bool force);
    std::optional<Request> complete(int generation);
    void invalidate();
    bool isCurrent(int generation) const;
    bool isInFlight() const;

private:
    int m_generation = 0;
    bool m_inFlight = false;
    bool m_forceQueued = false;
};

#endif
