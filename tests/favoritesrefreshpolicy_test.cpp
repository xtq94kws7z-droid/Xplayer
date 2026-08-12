#include <QtTest/QtTest>

#include "utils/favoritesrefreshpolicy.h"

class FavoritesRefreshPolicyTest : public QObject
{
    Q_OBJECT

private slots:
    void startsFirstVisibleRefresh();
    void coalescesVisibleRefreshWhileInFlight();
    void queuesForcedRefreshWhileInFlight();
    void suppressesFreshAutomaticRefreshes();
};

void FavoritesRefreshPolicyTest::startsFirstVisibleRefresh()
{
    FavoritesRefreshPolicy policy;

    const auto decision = policy.requestRefresh(1000, false);

    QCOMPARE(decision.shouldStart, true);
    QCOMPARE(decision.generation, 1);
    QCOMPARE(policy.isInFlight(), true);
}

void FavoritesRefreshPolicyTest::coalescesVisibleRefreshWhileInFlight()
{
    FavoritesRefreshPolicy policy;
    policy.requestRefresh(1000, false);

    const auto decision = policy.requestRefresh(1100, false);

    QCOMPARE(decision.shouldStart, false);
    QCOMPARE(decision.generation, 1);
    QCOMPARE(policy.completeRefresh(1, 1200), false);
    QCOMPARE(policy.isInFlight(), false);
}

void FavoritesRefreshPolicyTest::queuesForcedRefreshWhileInFlight()
{
    FavoritesRefreshPolicy policy;
    policy.requestRefresh(1000, false);

    const auto decision = policy.requestRefresh(1100, true);

    QCOMPARE(decision.shouldStart, false);
    QCOMPARE(policy.completeRefresh(1, 1200), true);

    const auto queued = policy.requestRefresh(1200, true);
    QCOMPARE(queued.shouldStart, true);
    QCOMPARE(queued.generation, 2);
}

void FavoritesRefreshPolicyTest::suppressesFreshAutomaticRefreshes()
{
    FavoritesRefreshPolicy policy;
    policy.requestRefresh(1000, false);
    policy.completeRefresh(1, 1500);

    const auto fresh = policy.requestRefresh(2000, false);
    QCOMPARE(fresh.shouldStart, false);

    const auto stale = policy.requestRefresh(1500 + FavoritesRefreshPolicy::kAutoRefreshStaleMs + 1, false);
    QCOMPARE(stale.shouldStart, true);
    QCOMPARE(stale.generation, 2);
}

QTEST_MAIN(FavoritesRefreshPolicyTest)
#include "favoritesrefreshpolicy_test.moc"
