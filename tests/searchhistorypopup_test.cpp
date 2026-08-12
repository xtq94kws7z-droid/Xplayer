#include "components/searchhistorychip.h"
#include "components/searchhistorypopup.h"
#include "../../src/XplayerCore/config/config_keys.h"
#include "../../src/XplayerCore/config/configstore.h"

#include <QToolButton>
#include <QVariantAnimation>
#include <QtTest>

#include <algorithm>

class SearchHistoryPopupTest final : public QObject
{
    Q_OBJECT

private slots:
    void sortTransitionDoesNotRebuildAfterExpansionRetarget();
};

namespace
{

QList<SearchHistoryManager::SearchHistoryEntry> makeEntries()
{
    QList<SearchHistoryManager::SearchHistoryEntry> entries;
    entries.reserve(24);
    for (int index = 0; index < 24; ++index)
    {
        SearchHistoryManager::SearchHistoryEntry entry;
        entry.term = QStringLiteral("term-%1").arg(index);
        entry.normalizedTerm = entry.term;
        entry.searchCount = 24 - index;
        entry.lastSearchedAtMs = 1000 + index;
        entries.append(entry);
    }
    return entries;
}

SearchHistoryChip *findChip(const SearchHistoryPopup &popup, const QString &term)
{
    const auto chips = popup.findChildren<SearchHistoryChip *>();
    const auto it = std::find_if(
        chips.cbegin(), chips.cend(),
        [&term](const SearchHistoryChip *chip) {
            return chip && chip->term() == term;
        });
    return it == chips.cend() ? nullptr : *it;
}

QVariantAnimation *findSortAnimation(const SearchHistoryPopup &popup)
{
    const auto animations = popup.findChildren<QVariantAnimation *>();
    const auto it = std::find_if(
        animations.cbegin(), animations.cend(),
        [](const QVariantAnimation *animation) {
            return animation &&
                   animation->duration() == 190;
        });
    return it == animations.cend() ? nullptr : *it;
}

}

void SearchHistoryPopupTest::sortTransitionDoesNotRebuildAfterExpansionRetarget()
{
    ConfigStore::instance()->set(ConfigKeys::UiAnimations, false);

    QWidget host;
    host.resize(900, 700);
    host.show();

    QWidget anchor(&host);
    anchor.setGeometry(300, 40, 240, 32);
    anchor.show();

    SearchHistoryPopup popup(&host);
    popup.setEntries(makeEntries());
    popup.showBelow(&anchor);
    QCoreApplication::processEvents();

    auto *hotButton = popup.findChildren<QToolButton *>(
        QStringLiteral("SearchHistoryModeButton"))
                          .value(1);
    auto *expandButton =
        popup.findChild<QToolButton *>(QStringLiteral("SearchHistoryExpandButton"));
    auto *sortAnimation = findSortAnimation(popup);
    QVERIFY(hotButton != nullptr);
    QVERIFY(expandButton != nullptr);
    QVERIFY(sortAnimation != nullptr);

    hotButton->click();
    sortAnimation->setCurrentTime(sortAnimation->duration() / 4);

    expandButton->click();
    auto *expandedChip = findChip(popup, QStringLiteral("term-0"));
    QVERIFY(expandedChip != nullptr);
    QCOMPARE(popup.findChildren<SearchHistoryChip *>().size(), 24);

    sortAnimation->setCurrentTime(sortAnimation->duration());

    QCOMPARE(findChip(popup, QStringLiteral("term-0")), expandedChip);
}

QTEST_MAIN(SearchHistoryPopupTest)
#include "searchhistorypopup_test.moc"
