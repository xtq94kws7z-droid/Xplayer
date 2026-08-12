#ifndef FAVORITESREFRESHPOLICY_H
#define FAVORITESREFRESHPOLICY_H

#include <QtGlobal>

class FavoritesRefreshPolicy
{
public:
    struct RefreshDecision
    {
        bool shouldStart = false;
        int generation = 0;
    };

    static constexpr qint64 kAutoRefreshStaleMs = 30 * 1000;

    RefreshDecision requestRefresh(qint64 nowMs, bool force);
    bool completeRefresh(int generation, qint64 nowMs);
    bool isCurrentGeneration(int generation) const;
    bool isInFlight() const;
    int generation() const;

private:
    int m_generation = 0;
    bool m_inFlight = false;
    bool m_hasCompletedLoad = false;
    bool m_forceRefreshQueued = false;
    qint64 m_lastCompletedMs = 0;
};

#endif
