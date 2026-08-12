#include <QTextStream>

#include "../src/XplayerApp/utils/playbackprogresspolicy.h"

namespace
{
bool check(bool condition, const char *message)
{
    if (condition) {
        return true;
    }

    QTextStream(stderr) << "playbackprogresspolicy_test failed: "
                        << message << Qt::endl;
    return false;
}
} // namespace

int main()
{
    PlaybackProgressPolicy policy;

    const PlaybackProgressRequest first{100, false, false};
    if (!check(policy.enqueue(first).has_value(),
               "first progress request is sent")) {
        return 1;
    }

    const PlaybackProgressRequest duplicate{100, false, false};
    if (!check(!policy.enqueue(duplicate).has_value(),
               "duplicate progress is suppressed")) {
        return 1;
    }

    const PlaybackProgressRequest forced{100, true, true};
    if (!check(!policy.enqueue(forced).has_value(),
               "in-flight requests stay single-flight")) {
        return 1;
    }

    const auto pending = policy.complete();
    if (!check(pending.has_value() && pending->positionTicks == 100 &&
                   pending->isPaused,
               "latest pending state is dispatched after completion")) {
        return 1;
    }

    const auto done = policy.complete();
    if (!check(!done.has_value(), "completion without pending is idle")) {
        return 1;
    }

    policy.reset();
    if (!check(policy.enqueue(first).has_value(),
               "reset starts a new session")) {
        return 1;
    }

    return 0;
}
