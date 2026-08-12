#include <QtTest/QtTest>

#include "utils/singleflightrefreshpolicy.h"

class SingleFlightRefreshPolicyTest : public QObject
{
    Q_OBJECT

private slots:
    void startsOneRefreshAtATime();
    void dropsPeriodicRefreshWhileBusy();
    void queuesOneForcedRefreshWhileBusy();
    void invalidatesInFlightRefreshWhenPageLeaves();
};

void SingleFlightRefreshPolicyTest::startsOneRefreshAtATime()
{
    SingleFlightRefreshPolicy policy;

    const auto first = policy.request(false);

    QVERIFY(first.has_value());
    QCOMPARE(first->generation, 1);
    QCOMPARE(first->force, false);
    QCOMPARE(policy.isInFlight(), true);
}

void SingleFlightRefreshPolicyTest::dropsPeriodicRefreshWhileBusy()
{
    SingleFlightRefreshPolicy policy;
    const auto first = policy.request(false);

    QVERIFY(!policy.request(false).has_value());
    QVERIFY(!policy.complete(first->generation).has_value());
    QCOMPARE(policy.isInFlight(), false);
}

void SingleFlightRefreshPolicyTest::queuesOneForcedRefreshWhileBusy()
{
    SingleFlightRefreshPolicy policy;
    const auto first = policy.request(false);

    QVERIFY(!policy.request(true).has_value());
    QVERIFY(!policy.request(true).has_value());

    const auto queued = policy.complete(first->generation);
    QVERIFY(queued.has_value());
    QCOMPARE(queued->generation, 2);
    QCOMPARE(queued->force, true);
    QCOMPARE(policy.isInFlight(), true);
    QVERIFY(!policy.complete(first->generation).has_value());
    QVERIFY(!policy.complete(queued->generation).has_value());
    QCOMPARE(policy.isInFlight(), false);
}

void SingleFlightRefreshPolicyTest::invalidatesInFlightRefreshWhenPageLeaves()
{
    SingleFlightRefreshPolicy policy;
    const auto leaving = policy.request(false);

    policy.invalidate();

    QCOMPARE(policy.isCurrent(leaving->generation), false);
    QCOMPARE(policy.isInFlight(), false);
    QVERIFY(!policy.complete(leaving->generation).has_value());

    const auto returning = policy.request(false);
    QVERIFY(returning.has_value());
    QVERIFY(returning->generation > leaving->generation);
}

QTEST_MAIN(SingleFlightRefreshPolicyTest)
#include "singleflightrefreshpolicy_test.moc"
