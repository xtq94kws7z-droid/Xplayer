#include "components/posterstagewidget.h"
#include "components/settingssubpanel.h"
#include "components/slidingstackedwidget.h"
#include <utils/mediapaginationutils.h>
#include "utils/mediaimagecandidateutils.h"
#include "utils/librarypaginationutils.h"
#include "utils/dashboardrequestlimitutils.h"
#include "utils/dashboardresponsiveutils.h"
#include "utils/dialogresponsiveutils.h"
#include "utils/playbackengineprofile.h"
#include "utils/playbackloadingstateutils.h"
#include "utils/playerhudvisibilityutils.h"
#include "utils/playermediaswitchertextutils.h"
#include "utils/playerpopupstateutils.h"
#include "utils/playercontroltextutils.h"
#include "utils/playerpopupanimationutils.h"
#include "utils/playertrackoptiontextutils.h"
#include "utils/playbackwindowmodeutils.h"
#include "utils/posterwallutils.h"
#include "utils/posterstageresponsiveutils.h"
#include "utils/seasonresponsiveutils.h"
#include "utils/xplayerresponsiveutils.h"
#include "utils/gallerylayoututils.h"
#include "utils/buttonlayoututils.h"
#include "utils/uianimationdefaults.h"
#include "utils/windowsmaterial.h"
#include <config/config_keys.h>
#include <config/configstore.h>

#include <QColor>
#include <QImage>
#include <QLabel>
#include <QPainter>
#include <QPushButton>
#include <QShowEvent>
#include <QSignalSpy>
#include <QStandardItemModel>
#include <QtTest>

namespace ModelNotificationUtils {
QList<QPair<int, int>> coalescedRowRanges(QList<int> rows);
}

namespace {

MediaItem makeItem(const QString& id, const QString& name)
{
    MediaItem item;
    item.id = id;
    item.name = name;
    return item;
}

QImage makePosterImage(const QColor& base, const QColor& accent)
{
    QImage image(180, 270, QImage::Format_RGB32);
    image.fill(base);

    QPainter painter(&image);
    painter.setRenderHint(QPainter::Antialiasing, true);
    painter.fillRect(QRect(0, 0, image.width(), 74), accent.darker(120));
    painter.fillRect(QRect(18, 104, 144, 96), accent);
    painter.fillRect(QRect(42, 214, 96, 18), accent.lighter(130));
    painter.end();

    return image;
}

class ShowEventCounterWidget final : public QWidget
{
public:
    int showEventCount = 0;

protected:
    void showEvent(QShowEvent* event) override
    {
        ++showEventCount;
        QWidget::showEvent(event);
    }
};

} // namespace

class PosterWallUtilsTest final : public QObject
{
    Q_OBJECT

private slots:
    void deduplicatesInSourceOrder();
    void deduplicatesSharedArtworkAcrossDifferentItems();
    void deduplicatesSameTitleAcrossDifferentArtworkIds();
    void capsThePosterPool();
    void libraryCandidatesStayWithinTheLibrary();
    void acceptsLibraryItemsWithoutTypeMetadata();
    void filtersNonPosterMediaTypes();
    void capsMediaLibraryCandidates();
    void distributesPosterSampleBudgetAcrossLibraries();
    void capsPosterSampleBudgetPerLibrary();
    void randomlySelectsFromTheWholePosterPool();
    void randomSelectionPrefersItemsWithOverview();
    void itemDetailsEnrichSelectionWithoutReordering();
    void avoidsRepeatingThePreviousPosterSelection();
    void wrapsSelectionForwardAndBackward();
    void listsVisiblePosterIndicesInOrder();
    void returnsNeutralColorForEmptyImage();
    void treatsResizedCopiesAsTheSamePoster();
    void keepsDifferentPosterImagesDistinct();
    void fillsRemainingPosterSlotsWithFallbackItems();
    void detectsUnchangedPosterStageItems();
    void detectsChangedPosterStageDisplayData();
    void stageStartsEmpty();
    void stageAcceptsPosterItems();
    void stageStartsAtFirstPosterDeterministically();
    void stageStopsRotationWhileHidden();
    void stageDoesNotRestartRotationWhenItemsRefreshWhileHidden();
    void stageSnapsVisualStateWhenHidden();
    void stageIgnoresNonPosterModelChanges();
    void stageRefreshesForPosterModelChanges();
    void stageRendersFiveCenteredSlots();
    void immersiveNavigationUsesSnapshot();
    void detailNavigationUsesSnapshot();
    void complexNavigationAnimatesBothPagesAsSnapshots();
    void pinnedCurrentSnapshotDoesNotDriftWhenLeaving();
    void pinnedNextSnapshotDoesNotDriftWhenReturning();
    void pinnedDashboardNavigationSwitchesWithoutSnapshotMotion();
    void snapshotTransitionDoesNotReshowOutgoingPage();
    void rapidNavigationEmitsOneCompletion();
    void settingsSubPanelRetargetsFromCurrentHeight();
    void posterStageRetargetsFromCurrentVisualCenter();
    void detailFirstContentLoadingStartsImmediately();
    void deferredInitialLoadStartsAfterNavigationAnimation();
    void postNavigationCleanupWaitsPastAnimation();
    void coalescesSparseRowsIntoMinimalRanges();
    void buildsFallbackImageCandidatesWhenTagsAreMissing();
    void detailPosterCandidatesFallBackAfterBrokenPrimary();
    void libraryInitialLoadUsesServerSafePage();
    void dashboardCategoryUsesServerSafePage();
    void dashboardContentWidthIsCappedOnLargeViewports();
    void dashboardPosterMetricsScaleWithinBounds();
    void dashboardPosterTilesFitWholeRows();
    void dashboardHeroHeightStaysWithinBounds();
    void seasonMetricsScaleFromReferenceLayout();
    void seasonMetricsStayUsableOnSmallViewports();
    void dialogMinimumSizeFitsCompactScreens();
    void globalResponsiveScaleUsesViewportWidthAndHeight();
    void globalResponsiveScaleStaysWithinSafeBounds();
    void globalResponsiveScalingPreservesReferenceDimensions();
    void galleryHeightNeverClipsCardContent();
    void textButtonWidthIncludesPaddingAndDpiSafety();
    void posterStageUsesFiveDisplaySlots();
    void posterStagePanelWidthScalesWithinBounds();
    void posterStageWallStartsAfterPanelWithoutLargeGap();
    void posterStageInfoPanelKeepsStableGeometryAcrossItems();
    void posterStageInfoPanelKeepsStableAnchorAcrossHeroResizes();
    void posterStageControlsFitAtCompactWidth();
    void posterStageOverviewCompactsInsideFixedPanel();
    void posterStageShowsFallbackForMissingOverview();
    void posterStageNavigationButtonsStayCircular();
    void posterStageNavigationIconsAreCentered();
    void posterStagePlayButtonKeepsItsFinalSize();
    void playedHistoryUsesServerSafePage();
    void playedHistoryClampsLargeInitialRequests();
    void nativeMaterialRejectsNullWindow();
    void playerHudHidesOnlyDuringUnobstructedPlayback();
    void playerHudStaysVisibleForInteractiveSurfaces();
    void pointerPollingUsesFrameInterval();
    void startupAutoLoginDoesNotUseFixedDelay();
    void mpvWarmupStartsAfterFirstEventLoop();
    void playbackEngineProfileUsesSmoothDefaults();
    void playbackEngineProfileFallsBackForRemoteDesktop();
    void playbackLoadPolicyAddsNetworkCacheOptions();
    void playbackFailureMessageIsChineseAndActionable();
    void playbackLoadingOpeningShowsPreparingText();
    void playbackLoadingBufferingTakesPriorityOverOpening();
    void playbackLoadingSeekingTakesPriorityOverBuffering();
    void playbackLoadingIdleAndTeardownAreHidden();
    void mediaSwitcherUsesChineseMovieModeCopy();
    void mediaSwitcherUsesChineseSeriesModeCopy();
    void trackOptionUsesReadableChineseFallbacks();
    void trackOptionUsesChineseDisableAndToastCopy();
    void playbackControlMenuUsesChineseCopy();
    void playbackControlStatusToastsAreChinese();
    void popupOpenAnimationUsesLightweightBudget();
    void popupRequestOpensWithoutActivePopup();
    void popupRequestClosesWhenSameButtonClickedAgain();
    void popupRequestReplacesWhenDifferentButtonClicked();
    void playerWindowModeDefaultsToIndependent();
    void playerWindowModeRespectsExplicitDisabledSetting();
    void playerWindowLaunchTransitionIsShort();
    void playbackWindowMaximizeButtonUsesDistinctIconAndChineseTooltip();
    void topbarAndBottomMaximizeButtonsShareStateContract();
    void fullscreenNotifiesWindowsShellButMaximizedDoesNot();
};

void PosterWallUtilsTest::deduplicatesInSourceOrder()
{
    const QList<MediaItem> items =
        PosterWallUtils::mergeUniqueItems(
            {makeItem("a", "Resume A"), makeItem("b", "Resume B")},
            {makeItem("b", "Latest B"), makeItem("c", "Latest C")},
            {makeItem("d", "Recommended D")},
            {makeItem("e", "Completed E")}, 10);

    QCOMPARE(items.size(), 5);
    QCOMPARE(items.at(0).id, QStringLiteral("a"));
    QCOMPARE(items.at(1).id, QStringLiteral("b"));
    QCOMPARE(items.at(2).id, QStringLiteral("c"));
    QCOMPARE(items.at(3).id, QStringLiteral("d"));
    QCOMPARE(items.at(4).id, QStringLiteral("e"));
}

void PosterWallUtilsTest::deduplicatesSharedArtworkAcrossDifferentItems()
{
    MediaItem episodeOne = makeItem("episode-1", "Episode One");
    episodeOne.images.primaryImageItemId = QStringLiteral("series-walking-dead");
    episodeOne.images.primaryTag = QStringLiteral("walking-dead-poster");

    MediaItem episodeTwo = makeItem("episode-2", "Episode Two");
    episodeTwo.images.primaryImageItemId = QStringLiteral("series-walking-dead");
    episodeTwo.images.primaryTag = QStringLiteral("walking-dead-poster");

    MediaItem differentShow = makeItem("show-2", "Different Show");
    differentShow.images.primaryImageItemId = QStringLiteral("show-2");
    differentShow.images.primaryTag = QStringLiteral("different-show-poster");

    const QList<MediaItem> items = PosterWallUtils::mergeUniqueItems(
        {episodeOne, episodeTwo, differentShow}, {}, {}, {}, 10);

    QCOMPARE(items.size(), 2);
    QCOMPARE(items.at(0).id, QStringLiteral("episode-1"));
    QCOMPARE(items.at(1).id, QStringLiteral("show-2"));
}

void PosterWallUtilsTest::deduplicatesSameTitleAcrossDifferentArtworkIds()
{
    MediaItem first = makeItem("zhongkui-1", "钟馗");
    first.productionYear = 2025;
    first.images.primaryImageItemId = QStringLiteral("zhongkui-image-a");
    first.images.primaryTag = QStringLiteral("poster-a");

    MediaItem second = makeItem("zhongkui-2", "钟馗");
    second.productionYear = 2025;
    second.images.primaryImageItemId = QStringLiteral("zhongkui-image-b");
    second.images.primaryTag = QStringLiteral("poster-b");

    MediaItem third = makeItem("law-order", "法律与秩序：特殊受害者");
    third.productionYear = 1999;

    const QList<MediaItem> items =
        PosterWallUtils::mergeUniqueItems({first, second, third}, {}, {}, {}, 10);

    QCOMPARE(items.size(), 2);
    QCOMPARE(items.at(0).id, QStringLiteral("zhongkui-1"));
    QCOMPARE(items.at(1).id, QStringLiteral("law-order"));
}

void PosterWallUtilsTest::capsThePosterPool()
{
    const QList<MediaItem> items =
        PosterWallUtils::mergeUniqueItems(
            {makeItem("a", "A"), makeItem("b", "B")},
            {makeItem("c", "C"), makeItem("d", "D")},
            {makeItem("e", "E")},
            {}, 3);

    QCOMPARE(items.size(), 3);
    QCOMPARE(items.at(2).id, QStringLiteral("c"));
}

void PosterWallUtilsTest::libraryCandidatesStayWithinTheLibrary()
{
    QList<MediaItem> libraryItems;
    for (int index = 0; index < 20; ++index) {
        MediaItem item = makeItem(QStringLiteral("library-%1").arg(index),
                                  QStringLiteral("Library %1").arg(index));
        item.type = QStringLiteral("Movie");
        libraryItems.append(item);
    }
    const QList<MediaItem> candidates =
        PosterWallUtils::buildLibraryCandidates(libraryItems, 120);

    QCOMPARE(candidates.size(), 20);
    for (const MediaItem& item : candidates) {
        QVERIFY2(item.id.startsWith(QStringLiteral("library-")),
                 "History items must not displace a sufficiently large library.");
    }
}

void PosterWallUtilsTest::acceptsLibraryItemsWithoutTypeMetadata()
{
    const MediaItem item = makeItem(QStringLiteral("library-unknown-type"),
                                    QStringLiteral("Library item"));

    const QList<MediaItem> candidates =
        PosterWallUtils::buildLibraryCandidates({item}, 18);

    QCOMPARE(candidates.size(), 1);
    QCOMPARE(candidates.first().id, QStringLiteral("library-unknown-type"));
}

void PosterWallUtilsTest::filtersNonPosterMediaTypes()
{
    QList<MediaItem> libraryItems;
    MediaItem movie = makeItem(QStringLiteral("movie"), QStringLiteral("Movie"));
    movie.type = QStringLiteral("Movie");
    MediaItem episode = makeItem(QStringLiteral("episode"),
                                 QStringLiteral("Episode"));
    episode.type = QStringLiteral("Episode");
    MediaItem folder = makeItem(QStringLiteral("folder"), QStringLiteral("Folder"));
    folder.type = QStringLiteral("Folder");
    libraryItems << movie << episode << folder;

    const QList<MediaItem> candidates =
        PosterWallUtils::buildLibraryCandidates(libraryItems, 18);

    QCOMPARE(candidates.size(), 1);
    QCOMPARE(candidates.first().id, QStringLiteral("movie"));
}

void PosterWallUtilsTest::capsMediaLibraryCandidates()
{
    QList<MediaItem> libraryItems;
    for (int index = 0; index < 150; ++index) {
        MediaItem item = makeItem(QStringLiteral("library-%1").arg(index),
                                  QStringLiteral("Library %1").arg(index));
        item.type = QStringLiteral("Movie");
        libraryItems.append(item);
    }

    const QList<MediaItem> candidates =
        PosterWallUtils::buildLibraryCandidates(libraryItems, 120);

    QCOMPARE(candidates.size(), 120);
}

void PosterWallUtilsTest::distributesPosterSampleBudgetAcrossLibraries()
{
    QCOMPARE(PosterWallUtils::equalLibrarySampleQuotas(5, 120, 40),
             QList<int>({24, 24, 24, 24, 24}));
}

void PosterWallUtilsTest::capsPosterSampleBudgetPerLibrary()
{
    QCOMPARE(PosterWallUtils::equalLibrarySampleQuotas(2, 120, 40),
             QList<int>({40, 40}));
    QCOMPARE(PosterWallUtils::equalLibrarySampleQuotas(7, 10, 40),
             QList<int>({2, 2, 2, 1, 1, 1, 1}));
    QVERIFY(PosterWallUtils::equalLibrarySampleQuotas(0, 120, 40).isEmpty());
}

void PosterWallUtilsTest::randomlySelectsFromTheWholePosterPool()
{
    QList<MediaItem> candidates;
    for (int index = 0; index < 10; ++index) {
        candidates.append(makeItem(QStringLiteral("poster-%1").arg(index),
                                   QStringLiteral("Poster %1").arg(index)));
    }

    const QList<MediaItem> selected = PosterWallUtils::selectRandomItems(
        candidates, 3, {}, 17);

    QCOMPARE(selected.size(), 3);
    QStringList selectedIds;
    QStringList firstCandidateIds;
    QStringList repeatedIds;
    for (const MediaItem& item : selected) {
        selectedIds.append(item.id);
    }
    for (const MediaItem& item : candidates.mid(0, 3)) {
        firstCandidateIds.append(item.id);
    }
    for (const MediaItem& item :
         PosterWallUtils::selectRandomItems(candidates, 3, {}, 17)) {
        repeatedIds.append(item.id);
    }

    QVERIFY(selectedIds != firstCandidateIds);
    QCOMPARE(repeatedIds, selectedIds);
}

void PosterWallUtilsTest::randomSelectionPrefersItemsWithOverview()
{
    QList<MediaItem> candidates;
    for (int index = 0; index < 6; ++index) {
        MediaItem item = makeItem(QStringLiteral("poster-%1").arg(index),
                                  QStringLiteral("Poster %1").arg(index));
        if (index >= 3) {
            item.overview = QStringLiteral("Overview %1").arg(index);
        }
        candidates.append(item);
    }

    const QList<MediaItem> selected = PosterWallUtils::selectRandomItems(
        candidates, 3, {}, 17);

    QCOMPARE(selected.size(), 3);
    for (const MediaItem& item : selected) {
        QVERIFY(!item.overview.trimmed().isEmpty());
    }
}

void PosterWallUtilsTest::itemDetailsEnrichSelectionWithoutReordering()
{
    MediaItem first = makeItem(QStringLiteral("first"),
                               QStringLiteral("First"));
    MediaItem second = makeItem(QStringLiteral("second"),
                                QStringLiteral("Second"));
    MediaItem secondDetail = second;
    secondDetail.overview = QStringLiteral("Second overview");
    MediaItem firstDetail = first;
    firstDetail.overview = QStringLiteral("First overview");

    const QList<MediaItem> enriched = PosterWallUtils::enrichItemDetails(
        {first, second}, {secondDetail, firstDetail});

    QCOMPARE(enriched.size(), 2);
    QCOMPARE(enriched.at(0).id, QStringLiteral("first"));
    QCOMPARE(enriched.at(0).overview, QStringLiteral("First overview"));
    QCOMPARE(enriched.at(1).id, QStringLiteral("second"));
    QCOMPARE(enriched.at(1).overview, QStringLiteral("Second overview"));
}

void PosterWallUtilsTest::avoidsRepeatingThePreviousPosterSelection()
{
    QList<MediaItem> candidates;
    for (int index = 0; index < 10; ++index) {
        candidates.append(makeItem(QStringLiteral("poster-%1").arg(index),
                                   QStringLiteral("Poster %1").arg(index)));
    }

    const QList<MediaItem> previous = PosterWallUtils::selectRandomItems(
        candidates, 3, {}, 17);
    const QList<MediaItem> next = PosterWallUtils::selectRandomItems(
        candidates, 3, previous, 17);

    QCOMPARE(next.size(), 3);
    QStringList previousIds;
    QStringList nextIds;
    for (const MediaItem& item : previous) {
        previousIds.append(item.id);
    }
    for (const MediaItem& item : next) {
        nextIds.append(item.id);
    }
    QVERIFY(nextIds != previousIds);
}

void PosterWallUtilsTest::wrapsSelectionForwardAndBackward()
{
    QCOMPARE(PosterWallUtils::nextIndex(2, 3, 1), 0);
    QCOMPARE(PosterWallUtils::nextIndex(0, 3, -1), 2);
    QCOMPARE(PosterWallUtils::nextIndex(1, 0, 1), -1);
}

void PosterWallUtilsTest::listsVisiblePosterIndicesInOrder()
{
    QCOMPARE(PosterWallUtils::visibleIndices(0, 3, 3),
             QList<int>({0, 1, 2}));
    QCOMPARE(PosterWallUtils::visibleIndices(2, 4, 3),
             QList<int>({2, 3, 0}));
}

void PosterWallUtilsTest::returnsNeutralColorForEmptyImage()
{
    QCOMPARE(PosterWallUtils::dominantColor(QImage()),
             QColor(128, 136, 148));
}

void PosterWallUtilsTest::treatsResizedCopiesAsTheSamePoster()
{
    const QImage poster =
        makePosterImage(QColor(22, 43, 72), QColor(190, 82, 96));
    QImage resized =
        poster.scaled(96, 144, Qt::IgnoreAspectRatio,
                      Qt::SmoothTransformation);

    QPainter wash(&resized);
    wash.fillRect(resized.rect(), QColor(255, 255, 255, 8));
    wash.end();

    QVERIFY(PosterWallUtils::areImagesVisuallySimilar(poster, resized));
}

void PosterWallUtilsTest::keepsDifferentPosterImagesDistinct()
{
    const QImage first =
        makePosterImage(QColor(22, 43, 72), QColor(190, 82, 96));
    const QImage second =
        makePosterImage(QColor(102, 28, 48), QColor(44, 152, 176));

    QVERIFY(!PosterWallUtils::areImagesVisuallySimilar(first, second));
}

void PosterWallUtilsTest::fillsRemainingPosterSlotsWithFallbackItems()
{
    const MediaItem first = makeItem("alpha", "Alpha");
    const MediaItem second = makeItem("beta", "Beta");
    const MediaItem third = makeItem("gamma", "Gamma");

    const QImage sharedPoster =
        makePosterImage(QColor(18, 34, 62), QColor(200, 90, 96));
    const QImage distinctPoster =
        makePosterImage(QColor(92, 34, 78), QColor(38, 154, 176));

    const QList<int> indices = PosterWallUtils::posterStageIndices(
        {first, second, third}, 0, 3,
        [&sharedPoster, &distinctPoster](const MediaItem& item) {
            if (item.id == QStringLiteral("gamma")) {
                return sharedPoster;
            }
            if (item.id == QStringLiteral("beta")) {
                return distinctPoster;
            }
            return sharedPoster;
        });

    QCOMPARE(indices.size(), 3);
    QCOMPARE(indices.at(0), 2);
    QCOMPARE(indices.at(1), 1);
    QCOMPARE(indices.at(2), 0);
}

void PosterWallUtilsTest::detectsUnchangedPosterStageItems()
{
    MediaItem first = makeItem("alpha", "Alpha");
    first.overview = QStringLiteral("First overview");
    first.images.primaryTag = QStringLiteral("poster-a");
    MediaItem second = makeItem("beta", "Beta");

    QVERIFY(PosterWallUtils::sameStageItems({first, second}, {first, second}));
}

void PosterWallUtilsTest::detectsChangedPosterStageDisplayData()
{
    MediaItem before = makeItem("alpha", "Alpha");
    before.overview = QStringLiteral("Old overview");
    before.images.primaryTag = QStringLiteral("poster-a");

    MediaItem renamed = before;
    renamed.name = QStringLiteral("Alpha Prime");
    QVERIFY(!PosterWallUtils::sameStageItems({before}, {renamed}));

    MediaItem newPoster = before;
    newPoster.images.primaryTag = QStringLiteral("poster-b");
    QVERIFY(!PosterWallUtils::sameStageItems({before}, {newPoster}));
}

void PosterWallUtilsTest::stageStartsEmpty()
{
    PosterStageWidget stage;
    QVERIFY(!stage.hasContent());
}

void PosterWallUtilsTest::stageAcceptsPosterItems()
{
    PosterStageWidget stage;
    stage.setItems({makeItem("poster-1", "Poster One")});
    QVERIFY(stage.hasContent());
    QCOMPARE(stage.items().size(), 1);
}

void PosterWallUtilsTest::stageStartsAtFirstPosterDeterministically()
{
    PosterStageWidget stage;
    stage.setItems({makeItem("poster-1", "Poster One"),
                    makeItem("poster-2", "Poster Two"),
                    makeItem("poster-3", "Poster Three")});

    QCOMPARE(stage.m_currentIndex, 0);
    QCOMPARE(stage.m_visualCenter, 0.0);
}

void PosterWallUtilsTest::stageStopsRotationWhileHidden()
{
    PosterStageWidget stage;
    stage.setItems({makeItem("poster-1", "Poster One"),
                    makeItem("poster-2", "Poster Two")});
    stage.show();
    QTest::qWait(1);

    QVERIFY(stage.m_rotationTimer->isActive());

    stage.hide();

    QVERIFY(!stage.m_rotationTimer->isActive());
}

void PosterWallUtilsTest::stageDoesNotRestartRotationWhenItemsRefreshWhileHidden()
{
    PosterStageWidget stage;
    stage.setItems({makeItem("poster-1", "Poster One"),
                    makeItem("poster-2", "Poster Two")});
    stage.show();
    QTest::qWait(1);
    stage.hide();

    QVERIFY(!stage.m_rotationTimer->isActive());

    MediaItem refreshed = makeItem("poster-1", "Poster One");
    refreshed.productionYear = 2026;
    stage.setItems({refreshed, makeItem("poster-2", "Poster Two")});

    QVERIFY(!stage.m_rotationTimer->isActive());
    QCOMPARE(stage.m_currentIndex, 0);
}

void PosterWallUtilsTest::stageSnapsVisualStateWhenHidden()
{
    PosterStageWidget stage;
    stage.resize(1280, 360);
    stage.setItems({makeItem("poster-1", "Poster One"),
                    makeItem("poster-2", "Poster Two"),
                    makeItem("poster-3", "Poster Three")});
    stage.show();
    QTest::qWait(1);

    stage.setCurrentIndex(2, true);
    QVERIFY(stage.m_slideAnimation != nullptr);
    stage.m_slideAnimation->setCurrentTime(120);
    QVERIFY(!qFuzzyCompare(stage.m_visualCenter,
                           static_cast<qreal>(stage.m_currentIndex)));

    stage.hide();

    QVERIFY(!stage.m_slideAnimation->state());
    QCOMPARE(stage.m_visualCenter, static_cast<qreal>(stage.m_currentIndex));
    QCOMPARE(stage.m_slideDirection, 0);
}

void PosterWallUtilsTest::stageIgnoresNonPosterModelChanges()
{
    const MediaItem item = makeItem("poster-1", "Poster One");
    QStandardItemModel model(1, 1);
    const QModelIndex index = model.index(0, 0);
    model.setData(index, QVariant::fromValue(item), Qt::UserRole + 1);

    QPixmap poster(180, 270);
    poster.fill(QColor(58, 88, 126));
    model.setData(index, poster, Qt::UserRole + 2);

    PosterStageWidget stage;
    stage.setModel(&model);
    stage.setItems({item});

    QSignalSpy atmosphereSpy(&stage, &PosterStageWidget::atmosphereChanged);
    Q_EMIT model.dataChanged(index, index, {Qt::DisplayRole});

    QCOMPARE(atmosphereSpy.count(), 0);
}

void PosterWallUtilsTest::stageRefreshesForPosterModelChanges()
{
    const MediaItem item = makeItem("poster-1", "Poster One");
    QStandardItemModel model(1, 1);
    const QModelIndex index = model.index(0, 0);
    model.setData(index, QVariant::fromValue(item), Qt::UserRole + 1);

    QPixmap poster(180, 270);
    poster.fill(QColor(110, 62, 132));
    model.setData(index, poster, Qt::UserRole + 2);

    PosterStageWidget stage;
    stage.setModel(&model);
    stage.setItems({item});

    QSignalSpy atmosphereSpy(&stage, &PosterStageWidget::atmosphereChanged);
    Q_EMIT model.dataChanged(index, index, {Qt::UserRole + 2});

    QCOMPARE(atmosphereSpy.count(), 1);
}

void PosterWallUtilsTest::stageRendersFiveCenteredSlots()
{
    PosterStageWidget stage;
    stage.resize(1280, 420);

    QList<MediaItem> items;
    for (int index = 0; index < 18; ++index) {
        items.append(makeItem(QStringLiteral("poster-%1").arg(index),
                              QStringLiteral("Poster %1").arg(index)));
    }

    stage.setItems(items);
    stage.show();
    QTest::qWait(1);
    QImage canvas(stage.size(), QImage::Format_ARGB32_Premultiplied);
    canvas.fill(Qt::transparent);
    stage.render(&canvas);

    QCOMPARE(stage.m_posterHitRects.size(),
             PosterStageResponsiveUtils::posterSlotCount());
}

void PosterWallUtilsTest::immersiveNavigationUsesSnapshot()
{
    const bool previousSetting = ConfigStore::instance()->get<bool>(
        ConfigKeys::SnapshotNavigation, false);
    ConfigStore::instance()->set(ConfigKeys::SnapshotNavigation, false);

    QWidget host;
    host.resize(640, 360);
    SlidingStackedWidget switcher(&host);
    switcher.resize(host.size());

    QWidget playerView;
    playerView.setProperty("isImmersive", true);
    QWidget homeView;
    switcher.addWidget(&playerView);
    switcher.addWidget(&homeView);
    switcher.setCurrentWidget(&playerView);

    switcher.slideInWgt(&homeView, SlidingStackedWidget::LeftToRight);

    QCOMPARE(switcher.findChildren<QLabel*>().size(), 2);
    ConfigStore::instance()->set(ConfigKeys::SnapshotNavigation, previousSetting);
}

void PosterWallUtilsTest::detailNavigationUsesSnapshot()
{
    const bool previousSetting = ConfigStore::instance()->get<bool>(
        ConfigKeys::SnapshotNavigation, false);
    ConfigStore::instance()->set(ConfigKeys::SnapshotNavigation, false);

    QWidget host;
    host.resize(960, 540);
    SlidingStackedWidget switcher(&host);
    switcher.resize(host.size());

    QWidget dashboardView;
    QWidget detailView;
    detailView.setProperty("preferSnapshotTransition", true);
    switcher.addWidget(&dashboardView);
    switcher.addWidget(&detailView);
    switcher.setCurrentWidget(&dashboardView);

    switcher.slideInWgt(&detailView, SlidingStackedWidget::RightToLeft);

    QCOMPARE(switcher.findChildren<QLabel*>().size(), 2);
    QCOMPARE(dashboardView.isVisible(), false);
    QCOMPARE(detailView.isVisible(), false);
    ConfigStore::instance()->set(ConfigKeys::SnapshotNavigation, previousSetting);
}

void PosterWallUtilsTest::complexNavigationAnimatesBothPagesAsSnapshots()
{
    const bool previousSetting = ConfigStore::instance()->get<bool>(
        ConfigKeys::SnapshotNavigation, false);
    ConfigStore::instance()->set(ConfigKeys::SnapshotNavigation, false);

    QWidget host;
    host.resize(960, 540);
    SlidingStackedWidget switcher(&host);
    switcher.resize(host.size());

    QWidget detailView;
    QWidget dashboardView;
    detailView.setProperty("preferSnapshotTransition", true);
    dashboardView.setProperty("preferSnapshotTransition", true);
    switcher.addWidget(&detailView);
    switcher.addWidget(&dashboardView);
    switcher.setCurrentWidget(&detailView);

    switcher.slideInWgt(&dashboardView, SlidingStackedWidget::LeftToRight);

    const auto snapshots = switcher.findChildren<QLabel*>();
    QCOMPARE(snapshots.size(), 2);
    QCOMPARE(detailView.isVisible(), false);
    QCOMPARE(dashboardView.isVisible(), false);

    ConfigStore::instance()->set(ConfigKeys::SnapshotNavigation, previousSetting);
}

void PosterWallUtilsTest::pinnedCurrentSnapshotDoesNotDriftWhenLeaving()
{
    const bool previousSetting = ConfigStore::instance()->get<bool>(
        ConfigKeys::SnapshotNavigation, false);
    ConfigStore::instance()->set(ConfigKeys::SnapshotNavigation, false);

    QWidget host;
    host.resize(960, 540);
    SlidingStackedWidget switcher(&host);
    switcher.resize(host.size());
    switcher.setSpeed(1000);

    QWidget dashboardView;
    QWidget detailView;
    dashboardView.setProperty("preferSnapshotTransition", true);
    dashboardView.setProperty("pinSnapshotTransition", true);
    detailView.setProperty("preferSnapshotTransition", true);
    switcher.addWidget(&dashboardView);
    switcher.addWidget(&detailView);
    switcher.setCurrentWidget(&dashboardView);

    switcher.slideInWgt(&detailView, SlidingStackedWidget::RightToLeft);
    QCOMPARE(switcher.currentWidget(), &detailView);
    QCOMPARE(switcher.findChildren<QLabel*>().size(), 0);

    ConfigStore::instance()->set(ConfigKeys::SnapshotNavigation, previousSetting);
}

void PosterWallUtilsTest::pinnedNextSnapshotDoesNotDriftWhenReturning()
{
    const bool previousSetting = ConfigStore::instance()->get<bool>(
        ConfigKeys::SnapshotNavigation, false);
    ConfigStore::instance()->set(ConfigKeys::SnapshotNavigation, false);

    QWidget host;
    host.resize(960, 540);
    SlidingStackedWidget switcher(&host);
    switcher.resize(host.size());
    switcher.setSpeed(1000);

    QWidget detailView;
    QWidget dashboardView;
    detailView.setProperty("preferSnapshotTransition", true);
    dashboardView.setProperty("preferSnapshotTransition", true);
    dashboardView.setProperty("pinSnapshotTransition", true);
    switcher.addWidget(&detailView);
    switcher.addWidget(&dashboardView);
    switcher.setCurrentWidget(&detailView);

    switcher.slideInWgt(&dashboardView, SlidingStackedWidget::LeftToRight);
    QCOMPARE(switcher.currentWidget(), &dashboardView);
    QCOMPARE(switcher.findChildren<QLabel*>().size(), 0);

    ConfigStore::instance()->set(ConfigKeys::SnapshotNavigation, previousSetting);
}

void PosterWallUtilsTest::pinnedDashboardNavigationSwitchesWithoutSnapshotMotion()
{
    const bool previousSetting = ConfigStore::instance()->get<bool>(
        ConfigKeys::SnapshotNavigation, false);
    ConfigStore::instance()->set(ConfigKeys::SnapshotNavigation, false);

    QWidget host;
    host.resize(960, 540);
    SlidingStackedWidget switcher(&host);
    switcher.resize(host.size());

    QWidget dashboardView;
    QWidget detailView;
    dashboardView.setProperty("preferSnapshotTransition", true);
    dashboardView.setProperty("pinSnapshotTransition", true);
    detailView.setProperty("preferSnapshotTransition", true);
    switcher.addWidget(&dashboardView);
    switcher.addWidget(&detailView);
    switcher.setCurrentWidget(&dashboardView);

    switcher.slideInWgt(&detailView, SlidingStackedWidget::RightToLeft);

    QCOMPARE(switcher.currentWidget(), &detailView);
    QCOMPARE(switcher.findChildren<QLabel*>().size(), 0);

    ConfigStore::instance()->set(ConfigKeys::SnapshotNavigation, previousSetting);
}

void PosterWallUtilsTest::snapshotTransitionDoesNotReshowOutgoingPage()
{
    const bool previousSetting = ConfigStore::instance()->get<bool>(
        ConfigKeys::SnapshotNavigation, false);
    ConfigStore::instance()->set(ConfigKeys::SnapshotNavigation, false);

    QWidget host;
    host.resize(960, 540);
    SlidingStackedWidget switcher(&host);
    switcher.resize(host.size());
    switcher.setSpeed(20);

    ShowEventCounterWidget dashboardView;
    ShowEventCounterWidget detailView;
    dashboardView.setProperty("preferSnapshotTransition", true);
    dashboardView.setProperty("pinSnapshotTransition", true);
    detailView.setProperty("preferSnapshotTransition", true);
    switcher.addWidget(&dashboardView);
    switcher.addWidget(&detailView);
    switcher.setCurrentWidget(&dashboardView);
    host.show();
    QTest::qWait(10);

    const int dashboardShowsBeforeNavigation = dashboardView.showEventCount;
    QSignalSpy finishedSpy(&switcher, &SlidingStackedWidget::animationFinished);
    switcher.slideInWgt(&detailView, SlidingStackedWidget::RightToLeft);
    QTRY_COMPARE_WITH_TIMEOUT(finishedSpy.count(), 1, 500);

    QCOMPARE(switcher.currentWidget(), &detailView);
    QCOMPARE(dashboardView.showEventCount, dashboardShowsBeforeNavigation);

    ConfigStore::instance()->set(ConfigKeys::SnapshotNavigation, previousSetting);
}

void PosterWallUtilsTest::rapidNavigationEmitsOneCompletion()
{
    const bool previousSetting = ConfigStore::instance()->get<bool>(
        ConfigKeys::SnapshotNavigation, false);
    ConfigStore::instance()->set(ConfigKeys::SnapshotNavigation, false);

    QWidget host;
    host.resize(960, 540);
    SlidingStackedWidget switcher(&host);
    switcher.resize(host.size());
    switcher.setSpeed(20);

    QWidget first;
    QWidget second;
    QWidget third;
    switcher.addWidget(&first);
    switcher.addWidget(&second);
    switcher.addWidget(&third);
    switcher.setCurrentWidget(&first);

    QSignalSpy finishedSpy(&switcher, &SlidingStackedWidget::animationFinished);

    switcher.slideInWgt(&second, SlidingStackedWidget::RightToLeft);
    switcher.slideInWgt(&third, SlidingStackedWidget::RightToLeft);

    QCOMPARE(finishedSpy.count(), 0);
    QTRY_COMPARE_WITH_TIMEOUT(finishedSpy.count(), 1, 500);
    QCOMPARE(switcher.currentWidget(), &third);

    ConfigStore::instance()->set(ConfigKeys::SnapshotNavigation, previousSetting);
}

void PosterWallUtilsTest::settingsSubPanelRetargetsFromCurrentHeight()
{
    SettingsSubPanel panel(QStringLiteral(":/svg/light/settings.svg"));
    panel.resize(520, 320);
    panel.setExpandedImmediately(true);

    panel.collapse();
    QTest::qWait(30);
    const int currentHeight = panel.property("panelHeight").toInt();
    QVERIFY(currentHeight > 0);

    panel.expand();

    const auto animations = panel.findChildren<QPropertyAnimation *>();
    QVERIFY(!animations.isEmpty());
    QCOMPARE(animations.constFirst()->startValue().toInt(), currentHeight);
}

void PosterWallUtilsTest::posterStageRetargetsFromCurrentVisualCenter()
{
    PosterStageWidget stage;
    stage.resize(1280, 420);
    stage.setItems({makeItem("poster-1", "Poster One"),
                    makeItem("poster-2", "Poster Two"),
                    makeItem("poster-3", "Poster Three"),
                    makeItem("poster-4", "Poster Four")});
    stage.m_reducedMotion = false;
    stage.m_currentIndex = 0;
    stage.m_visualCenter = 0.0;

    stage.setCurrentIndex(1, true);
    QVERIFY(stage.m_slideAnimation != nullptr);
    stage.m_slideAnimation->setCurrentTime(140);
    const qreal inFlightCenter = stage.m_visualCenter;
    QVERIFY(inFlightCenter > 0.0);
    QVERIFY(inFlightCenter < 1.0);

    stage.setCurrentIndex(2, true);

    QCOMPARE(stage.m_slideAnimation->startValue().toReal(), inFlightCenter);
    QVERIFY(stage.m_slideAnimation->endValue().toReal() > inFlightCenter);
    QCOMPARE(stage.m_currentIndex, 2);
}

void PosterWallUtilsTest::detailFirstContentLoadingStartsImmediately()
{
    QCOMPARE(XplayerUi::kDetailFirstLoadDelayMs, 0);
    QCOMPARE(XplayerUi::kDetailDataPresentationDelayMs, 0);
}

void PosterWallUtilsTest::deferredInitialLoadStartsAfterNavigationAnimation()
{
    QVERIFY(XplayerUi::kDeferredInitialLoadDelayMs > XplayerUi::kPanelAnimationMs);
    QCOMPARE(XplayerUi::kDeferredInitialLoadDelayMs,
             XplayerUi::kPanelAnimationMs + XplayerUi::kFrameIntervalMs);
}

void PosterWallUtilsTest::postNavigationCleanupWaitsPastAnimation()
{
    QVERIFY(XplayerUi::kPostNavigationCleanupDelayMs > XplayerUi::kPanelAnimationMs);
    QCOMPARE(XplayerUi::kPostNavigationCleanupDelayMs,
             XplayerUi::kPanelAnimationMs + XplayerUi::kFrameIntervalMs);
}

void PosterWallUtilsTest::coalescesSparseRowsIntoMinimalRanges()
{
    const QList<QPair<int, int>> ranges =
        ModelNotificationUtils::coalescedRowRanges({8, 1, 2, 4, 5, 5, 6, -1, 10});

    QCOMPARE(ranges.size(), 4);
    QCOMPARE(ranges.at(0), qMakePair(1, 2));
    QCOMPARE(ranges.at(1), qMakePair(4, 6));
    QCOMPARE(ranges.at(2), qMakePair(8, 8));
    QCOMPARE(ranges.at(3), qMakePair(10, 10));
}

void PosterWallUtilsTest::buildsFallbackImageCandidatesWhenTagsAreMissing()
{
    MediaItem item;
    item.id = QStringLiteral("library-id");
    item.type = QStringLiteral("Folder");

    const QList<MediaImageCandidateUtils::ImageCandidateDescriptor> candidates =
        MediaImageCandidateUtils::buildCandidates(item, false, true);

    QCOMPARE(candidates.size(), 3);
    QCOMPARE(candidates.at(0).targetImageId, QStringLiteral("library-id"));
    QCOMPARE(candidates.at(0).imageType, QStringLiteral("Primary"));
    QVERIFY(candidates.at(0).imageTag.isEmpty());
    QCOMPARE(candidates.at(1).imageType, QStringLiteral("Thumb"));
    QCOMPARE(candidates.at(2).imageType, QStringLiteral("Backdrop"));
}

void PosterWallUtilsTest::detailPosterCandidatesFallBackAfterBrokenPrimary()
{
    MediaItem item;
    item.id = QStringLiteral("302141");
    item.type = QStringLiteral("Series");
    item.images.primaryTag = QStringLiteral("broken-primary-tag");
    item.images.thumbTag = QStringLiteral("working-thumb-tag");
    item.images.backdropTag = QStringLiteral("working-backdrop-tag");

    const QList<MediaImageCandidateUtils::ImageCandidateDescriptor> candidates =
        MediaImageCandidateUtils::buildDetailPosterCandidates(item, true);

    QVERIFY(candidates.size() >= 6);
    QCOMPARE(candidates.at(0).targetImageId, QStringLiteral("302141"));
    QCOMPARE(candidates.at(0).imageType, QStringLiteral("Primary"));
    QCOMPARE(candidates.at(0).imageTag, QStringLiteral("broken-primary-tag"));
    QCOMPARE(candidates.at(1).imageType, QStringLiteral("Thumb"));
    QCOMPARE(candidates.at(1).imageTag, QStringLiteral("working-thumb-tag"));
    QCOMPARE(candidates.at(2).imageType, QStringLiteral("Backdrop"));
    QCOMPARE(candidates.at(2).imageTag, QStringLiteral("working-backdrop-tag"));
    QCOMPARE(candidates.at(3).imageType, QStringLiteral("Primary"));
    QVERIFY(candidates.at(3).imageTag.isEmpty());
}

void PosterWallUtilsTest::libraryInitialLoadUsesServerSafePage()
{
    const LibraryPaginationUtils::PageState state =
        LibraryPaginationUtils::initialPageState(1257, 50);

    QCOMPARE(LibraryPaginationUtils::initialRequestLimit(), 50);
    QCOMPARE(state.loadedCount, 50);
    QCOMPARE(state.totalCount, 1257);
    QCOMPARE(state.nextStartIndex, 50);
    QCOMPARE(state.nextRequestLimit, 50);
    QVERIFY(state.hasMoreItems);
}

void PosterWallUtilsTest::dashboardCategoryUsesServerSafePage()
{
    QCOMPARE(DashboardRequestLimitUtils::progressivePageRequestLimit(100, 0), 50);
    QCOMPARE(DashboardRequestLimitUtils::progressivePageRequestLimit(300, 0), 50);
    QCOMPARE(DashboardRequestLimitUtils::progressivePageRequestLimit(300, 24), 24);
    QCOMPARE(DashboardRequestLimitUtils::progressivePageRequestLimit(18, 100), 18);
    QCOMPARE(DashboardRequestLimitUtils::progressivePageRequestLimit(50, 50), 50);
}

void PosterWallUtilsTest::dashboardContentWidthIsCappedOnLargeViewports()
{
    const auto compact =
        DashboardResponsiveUtils::metricsForViewport(QSize(1280, 720));
    const auto desktop =
        DashboardResponsiveUtils::metricsForViewport(QSize(1920, 1080));
    const auto wide =
        DashboardResponsiveUtils::metricsForViewport(QSize(2560, 1440));

    QVERIFY(compact.contentWidth < desktop.contentWidth);
    QVERIFY(desktop.contentWidth < wide.contentWidth);
    QVERIFY(wide.contentWidth <= DashboardResponsiveUtils::maxContentWidth());
    QVERIFY(wide.scale > desktop.scale);
    QCOMPARE(wide.posterTile.width() > desktop.posterTile.width(), true);
}

void PosterWallUtilsTest::dashboardPosterMetricsScaleWithinBounds()
{
    const auto compact =
        DashboardResponsiveUtils::metricsForViewport(QSize(1280, 720));
    const auto desktop =
        DashboardResponsiveUtils::metricsForViewport(QSize(1920, 1080));
    const auto wide =
        DashboardResponsiveUtils::metricsForViewport(QSize(2560, 1440));

    QVERIFY(compact.posterTile.width() < desktop.posterTile.width());
    QVERIFY(desktop.posterTile.width() < wide.posterTile.width());
    QCOMPARE(desktop.posterTile.height(),
             qRound(desktop.posterTile.width() * 1.85));
    QVERIFY(compact.posterTitlePixels < desktop.posterTitlePixels);
    QVERIFY(desktop.posterTitlePixels <= wide.posterTitlePixels);
}

void PosterWallUtilsTest::dashboardPosterTilesFitWholeRows()
{
    const auto desktop =
        DashboardResponsiveUtils::metricsForViewport(QSize(1920, 1080));
    const auto wide =
        DashboardResponsiveUtils::metricsForViewport(QSize(2560, 1440));

    const int desktopColumns =
        desktop.contentWidth / qMax(1, desktop.posterTile.width());
    const int wideColumns =
        wide.contentWidth / qMax(1, wide.posterTile.width());

    QVERIFY(desktopColumns >= 8);
    QVERIFY(wideColumns >= desktopColumns);
    QVERIFY(desktop.contentWidth -
                desktopColumns * desktop.posterTile.width() <=
            10);
    QVERIFY(wide.contentWidth - wideColumns * wide.posterTile.width() <= 10);
}

void PosterWallUtilsTest::dashboardHeroHeightStaysWithinBounds()
{
    const auto compact =
        DashboardResponsiveUtils::metricsForViewport(QSize(1280, 720));
    const auto desktop =
        DashboardResponsiveUtils::metricsForViewport(QSize(1920, 1080));
    const auto wide =
        DashboardResponsiveUtils::metricsForViewport(QSize(2560, 1440));

    QVERIFY(compact.heroHeight < desktop.heroHeight);
    QVERIFY(desktop.heroHeight < wide.heroHeight);
    QVERIFY(desktop.heroHeight >= 360);
    QVERIFY(wide.heroHeight <= 520);
    QCOMPARE(wide.galleryHeight,
             wide.posterTile.height() + qMax(4, qRound(4 * wide.scale)));
}

void PosterWallUtilsTest::seasonMetricsScaleFromReferenceLayout()
{
    const auto metrics =
        SeasonResponsiveUtils::metricsForViewport(QSize(1920, 1080));

    QCOMPARE(metrics.posterSize, QSize(250, 375));
    QCOMPARE(metrics.actionButtonSize, 36);
    QCOMPARE(metrics.actionIconSize, 20);
    QCOMPARE(metrics.headerHorizontalMargin, 40);
    QCOMPARE(metrics.headerTopMargin, 40);
    QCOMPARE(metrics.mediaGridBasePadding, 40);
}

void PosterWallUtilsTest::seasonMetricsStayUsableOnSmallViewports()
{
    const auto small =
        SeasonResponsiveUtils::metricsForViewport(QSize(1280, 720));
    const auto reference =
        SeasonResponsiveUtils::metricsForViewport(QSize(1920, 1080));
    const auto large =
        SeasonResponsiveUtils::metricsForViewport(QSize(3840, 2160));

    QVERIFY(small.posterSize.width() < reference.posterSize.width());
    QCOMPARE(small.posterSize.height(),
             qRound(small.posterSize.width() * 1.5));
    QVERIFY(small.posterSize.width() >= 190);
    QVERIFY(small.headerHorizontalMargin >= 28);
    QVERIFY(small.mediaGridBasePadding >= 28);

    QVERIFY(large.posterSize.width() > reference.posterSize.width());
    QVERIFY(large.posterSize.width() <= 360);
    QCOMPARE(large.posterSize.height(),
             qRound(large.posterSize.width() * 1.5));
}

void PosterWallUtilsTest::dialogMinimumSizeFitsCompactScreens()
{
    QCOMPARE(DialogResponsiveUtils::boundedMinimumSize(
                 QSize(920, 620), QSize(1920, 1080)),
             QSize(920, 620));

    const QSize compact = DialogResponsiveUtils::boundedMinimumSize(
        QSize(920, 620), QSize(1280, 720));
    QVERIFY(compact.width() <= 1100);
    QVERIFY(compact.height() <= 620);
    QVERIFY(compact.width() >= 760);
    QVERIFY(compact.height() >= 520);
}

void PosterWallUtilsTest::globalResponsiveScaleUsesViewportWidthAndHeight()
{
    QCOMPARE(XplayerResponsiveUtils::scaleForViewport(QSize(1920, 1080)),
             1.0);
    QVERIFY(XplayerResponsiveUtils::scaleForViewport(QSize(2560, 1440)) >
            1.0);
    QVERIFY(XplayerResponsiveUtils::scaleForViewport(QSize(1920, 720)) < 1.0);
}

void PosterWallUtilsTest::globalResponsiveScaleStaysWithinSafeBounds()
{
    const qreal compact =
        XplayerResponsiveUtils::scaleForViewport(QSize(640, 480));
    const qreal large =
        XplayerResponsiveUtils::scaleForViewport(QSize(7680, 4320));

    QVERIFY(compact >= XplayerResponsiveUtils::minimumScale());
    QVERIFY(large <= XplayerResponsiveUtils::maximumScale());
}

void PosterWallUtilsTest::globalResponsiveScalingPreservesReferenceDimensions()
{
    const qreal scale = 1.25;
    QCOMPARE(XplayerResponsiveUtils::scaled(32, scale), 40);
    QCOMPARE(XplayerResponsiveUtils::scaled(250, scale), 313);
}

void PosterWallUtilsTest::galleryHeightNeverClipsCardContent()
{
    QCOMPARE(GalleryLayoutUtils::viewportHeightForCard(280, 296), 296);
    QCOMPARE(GalleryLayoutUtils::viewportHeightForCard(300, 296), 300);
    QCOMPARE(GalleryLayoutUtils::viewportHeightForCard(228, 296), 296);
}

void PosterWallUtilsTest::textButtonWidthIncludesPaddingAndDpiSafety()
{
    QFont font;
    font.setPixelSize(18);
    const QString text = QStringLiteral("查看全部 >");
    const int textWidth = QFontMetrics(font).horizontalAdvance(text);

    QCOMPARE(ButtonLayoutUtils::minimumTextButtonWidth(text, font, 8, 6),
             textWidth + 22);
}

void PosterWallUtilsTest::posterStageUsesFiveDisplaySlots()
{
    QCOMPARE(PosterStageResponsiveUtils::posterSlotCount(), 5);
}

void PosterWallUtilsTest::posterStagePanelWidthScalesWithinBounds()
{
    const int compact = PosterStageResponsiveUtils::infoPanelWidth(800);
    const int desktop = PosterStageResponsiveUtils::infoPanelWidth(1920);
    const int wide = PosterStageResponsiveUtils::infoPanelWidth(2560);

    QCOMPARE(compact, 292);
    QVERIFY(desktop > compact);
    QVERIFY(wide > desktop);
    QVERIFY(wide <= 424);
}

void PosterWallUtilsTest::posterStageWallStartsAfterPanelWithoutLargeGap()
{
    const int panelWidth = PosterStageResponsiveUtils::infoPanelWidth(1920);
    const int wallLeft =
        PosterStageResponsiveUtils::posterWallLeft(1920, panelWidth);

    QVERIFY(wallLeft > panelWidth + 36);
    QVERIFY(wallLeft < panelWidth + 60);
    QVERIFY(wallLeft < 1920 / 2);
}

void PosterWallUtilsTest::posterStageInfoPanelKeepsStableGeometryAcrossItems()
{
    PosterStageWidget stage;
    stage.resize(1280, 420);
    stage.setReducedMotion(true);

    MediaItem shortItem = makeItem(QStringLiteral("short"), QStringLiteral("短简介"));
    shortItem.overview = QStringLiteral("短简介。");
    MediaItem longItem = makeItem(QStringLiteral("long"), QStringLiteral("很长的简介"));
    longItem.productionYear = 2026;
    longItem.seriesName = QStringLiteral("测试剧集");
    longItem.overview =
        QStringLiteral("这是一段明显更长的影片简介，用来覆盖多行换行、标题和元数据变化时的布局高度变化。"
                       "左侧信息框应该像固定的玻璃卡片一样稳定停在原位，而不是因为文字长短上下跳动。");

    stage.setItems({shortItem, longItem});
    stage.show();
    QTest::qWait(10);

    const QRect firstGeometry = stage.m_infoPanel->geometry();
    stage.setCurrentIndex(1, false);
    QTest::qWait(10);
    const QRect secondGeometry = stage.m_infoPanel->geometry();

    QCOMPARE(secondGeometry.top(), firstGeometry.top());
    QCOMPARE(secondGeometry.height(), firstGeometry.height());
}

void PosterWallUtilsTest::posterStageInfoPanelKeepsStableAnchorAcrossHeroResizes()
{
    PosterStageWidget stage;
    stage.resize(1920, 420);
    stage.setReducedMotion(true);
    stage.setItems({makeItem(QStringLiteral("poster-1"),
                             QStringLiteral("Poster One")),
                    makeItem(QStringLiteral("poster-2"),
                             QStringLiteral("Poster Two"))});
    stage.show();
    QTest::qWait(10);

    const QRect firstGeometry = stage.m_infoPanel->geometry();
    stage.resize(1280, 360);
    QTest::qWait(10);
    const QRect resizedGeometry = stage.m_infoPanel->geometry();

    QCOMPARE(resizedGeometry.top(), firstGeometry.top());
    QCOMPARE(resizedGeometry.height(), firstGeometry.height());
}

void PosterWallUtilsTest::posterStageControlsFitAtCompactWidth()
{
    PosterStageWidget stage;
    stage.resize(1280, 303);
    stage.setReducedMotion(true);

    QList<MediaItem> items;
    for (int index = 0; index < 18; ++index) {
        items.append(makeItem(QStringLiteral("poster-%1").arg(index),
                              QStringLiteral("Poster %1").arg(index)));
    }
    stage.setItems(items);
    stage.m_playButton->setStyleSheet(
        QStringLiteral("padding: 7px 18px; font-size: 13px;"));
    stage.show();
    QTest::qWait(10);

    const auto dots = stage.m_dotsWidget->findChildren<QLabel*>(
        QString(), Qt::FindDirectChildrenOnly);
    QVERIFY2(dots.size() <= 7,
             "The page indicator must not squeeze primary controls at compact widths.");
    QVERIFY2(stage.m_playButton->width() >= stage.m_playButton->sizeHint().width(),
             "The play button must fit its full translated label and style padding.");
    QVERIFY(stage.m_infoPanel->rect().contains(
        stage.m_playButton->geometry().bottomRight()));
}

void PosterWallUtilsTest::posterStageOverviewCompactsInsideFixedPanel()
{
    MediaItem item = makeItem(QStringLiteral("poster-1"),
                              QStringLiteral("Poster One"));
    item.overview = QStringLiteral(
        "这是一个非常漫长的作品简介。主角在失去一切之后踏上旅程，途中遇到了许多伙伴和敌人。"
        "他们必须面对危险的考验，并在最后做出改变命运的选择。这个故事还会继续展开更多内容。");

    PosterStageWidget stage;
    stage.resize(1280, 360);
    stage.setItems({item});
    stage.show();
    QTest::qWait(20);

    QVERIFY(stage.m_overviewLabel->text().size() < item.overview.size());
    QVERIFY(stage.m_overviewLabel->text().endsWith(QStringLiteral("…")));
    QCOMPARE(stage.m_overviewLabel->toolTip(), item.overview);
}

void PosterWallUtilsTest::posterStageShowsFallbackForMissingOverview()
{
    MediaItem item = makeItem(QStringLiteral("poster-empty-overview"),
                              QStringLiteral("Poster Without Overview"));
    item.overview.clear();

    PosterStageWidget stage;
    stage.resize(1280, 360);
    stage.setItems({item});
    stage.show();
    QTest::qWait(20);

    QVERIFY(!stage.m_overviewLabel->text().trimmed().isEmpty());
}

void PosterWallUtilsTest::posterStageNavigationButtonsStayCircular()
{
    PosterStageWidget stage;
    stage.resize(1280, 360);
    stage.setReducedMotion(true);
    stage.setItems({makeItem(QStringLiteral("poster-1"),
                             QStringLiteral("Poster One")),
                    makeItem(QStringLiteral("poster-2"),
                             QStringLiteral("Poster Two"))});
    stage.show();
    QTest::qWait(10);

    QCOMPARE(stage.m_previousButton->size(), stage.m_nextButton->size());
    const QString expectedRadius = QStringLiteral("border-radius: %1px")
                                       .arg(stage.m_previousButton->width() / 2);
    QVERIFY(stage.m_previousButton->styleSheet().contains(expectedRadius));
    QVERIFY(stage.m_nextButton->styleSheet().contains(expectedRadius));
}

void PosterWallUtilsTest::posterStageNavigationIconsAreCentered()
{
    PosterStageWidget stage;
    stage.resize(1280, 360);
    stage.setItems({makeItem(QStringLiteral("poster-1"),
                             QStringLiteral("Poster One")),
                    makeItem(QStringLiteral("poster-2"),
                             QStringLiteral("Poster Two"))});
    stage.show();
    QTest::qWait(10);

    QVERIFY(stage.m_previousButton->text().isEmpty());
    QVERIFY(stage.m_nextButton->text().isEmpty());
    QVERIFY(!stage.m_previousButton->icon().isNull());
    QVERIFY(!stage.m_nextButton->icon().isNull());
    QCOMPARE(stage.m_previousButton->iconSize(),
             stage.m_nextButton->iconSize());
}

void PosterWallUtilsTest::posterStagePlayButtonKeepsItsFinalSize()
{
    PosterStageWidget stage;
    stage.resize(980, 300);
    stage.setReducedMotion(true);
    stage.setItems({makeItem(QStringLiteral("poster-1"),
                             QStringLiteral("Poster One")),
                    makeItem(QStringLiteral("poster-2"),
                             QStringLiteral("Poster Two"))});
    stage.show();
    QTest::qWait(10);

    QCOMPARE(stage.m_playButton->minimumWidth(),
             stage.m_playButton->maximumWidth());
    QCOMPARE(stage.m_playButton->minimumHeight(),
             stage.m_playButton->maximumHeight());
    QVERIFY(stage.m_playButton->width() >=
            stage.m_playButton->sizeHint().width());
    QVERIFY(stage.m_playButton->height() >=
            stage.m_playButton->sizeHint().height());
}

void PosterWallUtilsTest::playedHistoryUsesServerSafePage()
{
    QCOMPARE(MediaPaginationUtils::safePageLimit(300), 50);
    QCOMPARE(MediaPaginationUtils::safePageLimit(100), 50);
    QCOMPARE(MediaPaginationUtils::safePageLimit(50), 50);
    QCOMPARE(MediaPaginationUtils::safePageLimit(18), 18);
    QCOMPARE(MediaPaginationUtils::safePageLimit(0), 50);
}

void PosterWallUtilsTest::playedHistoryClampsLargeInitialRequests()
{
    QCOMPARE(MediaPaginationUtils::safeInitialRequestLimit(201), 50);
    QCOMPARE(MediaPaginationUtils::safeInitialRequestLimit(100), 50);
    QCOMPARE(MediaPaginationUtils::safeInitialRequestLimit(50), 50);
    QCOMPARE(MediaPaginationUtils::safeInitialRequestLimit(18), 18);
    QCOMPARE(MediaPaginationUtils::safeInitialRequestLimit(0), 0);
}

void PosterWallUtilsTest::nativeMaterialRejectsNullWindow()
{
    QVERIFY(!WindowsMaterial::applyBackdrop(
        0, WindowsMaterial::Kind::MainWindow));
    QVERIFY(!WindowsMaterial::setDarkMode(0, true));
}

void PosterWallUtilsTest::playerHudHidesOnlyDuringUnobstructedPlayback()
{
    PlayerHudVisibilityUtils::AutoHideContext context;
    context.isPlaying = true;
    context.isAppActive = true;
    context.pointerInsidePlayer = true;

    QCOMPARE(PlayerHudVisibilityUtils::autoHideDecision(context),
             PlayerHudVisibilityUtils::AutoHideDecision::HideNow);

    context.isPlaying = false;
    QCOMPARE(PlayerHudVisibilityUtils::autoHideDecision(context),
             PlayerHudVisibilityUtils::AutoHideDecision::KeepVisibleNoRetry);

    context.isPlaying = true;
    context.isLoading = true;
    QCOMPARE(PlayerHudVisibilityUtils::autoHideDecision(context),
             PlayerHudVisibilityUtils::AutoHideDecision::KeepVisibleAndRetry);
}

void PosterWallUtilsTest::playerHudStaysVisibleForInteractiveSurfaces()
{
    PlayerHudVisibilityUtils::AutoHideContext context;
    context.isPlaying = true;
    context.isAppActive = true;
    context.pointerInsidePlayer = true;
    context.pointerOnChrome = true;
    QCOMPARE(PlayerHudVisibilityUtils::autoHideDecision(context),
             PlayerHudVisibilityUtils::AutoHideDecision::KeepVisibleAndRetry);

    context.pointerOnChrome = false;
    context.pointerOnPopup = true;
    QCOMPARE(PlayerHudVisibilityUtils::autoHideDecision(context),
             PlayerHudVisibilityUtils::AutoHideDecision::KeepVisibleAndRetry);

    context.pointerOnPopup = false;
    context.rightSidebarVisible = true;
    context.pointerOnRightSidebar = true;
    QCOMPARE(PlayerHudVisibilityUtils::autoHideDecision(context),
             PlayerHudVisibilityUtils::AutoHideDecision::KeepVisibleAndRetry);

    context.pointerOnRightSidebar = false;
    context.hasActiveDialog = true;
    QCOMPARE(PlayerHudVisibilityUtils::autoHideDecision(context),
             PlayerHudVisibilityUtils::AutoHideDecision::KeepVisibleAndRetry);
}

void PosterWallUtilsTest::pointerPollingUsesFrameInterval()
{
    QCOMPARE(XplayerUi::kPointerPollMs, XplayerUi::kFrameIntervalMs);
}

void PosterWallUtilsTest::startupAutoLoginDoesNotUseFixedDelay()
{
    QCOMPARE(XplayerUi::kAutoLoginDelayMs, 0);
}

void PosterWallUtilsTest::mpvWarmupStartsAfterFirstEventLoop()
{
    QCOMPARE(XplayerUi::kMpvWarmupDelayMs, 0);
}

void PosterWallUtilsTest::playbackEngineProfileUsesSmoothDefaults()
{
    PlaybackEngineProfile::BuildContext context;
    context.videoSync = QStringLiteral("display-resample");
    context.hardwareDecoder = QStringLiteral("auto-copy");
    context.targetFrameRate = 60;

    const PlaybackEngineProfile::Profile profile =
        PlaybackEngineProfile::buildDefaultProfile(context);

    QCOMPARE(profile.name, QStringLiteral("xplayer-balanced"));
    QCOMPARE(profile.optionValue(QStringLiteral("video-sync")),
             QStringLiteral("display-resample"));
    QCOMPARE(profile.optionValue(QStringLiteral("interpolation")),
             QStringLiteral("yes"));
    QCOMPARE(profile.optionValue(QStringLiteral("display-fps")),
             QStringLiteral("60"));
    QCOMPARE(profile.optionValue(QStringLiteral("hwdec")),
             QStringLiteral("auto-copy"));
    QCOMPARE(profile.optionValue(QStringLiteral("demuxer-max-bytes")),
             QStringLiteral("128MiB"));
    QCOMPARE(profile.optionValue(QStringLiteral("cache-secs")),
             QStringLiteral("45"));
}

void PosterWallUtilsTest::playbackEngineProfileFallsBackForRemoteDesktop()
{
    PlaybackEngineProfile::BuildContext context;
    context.hardwareDecoder = QStringLiteral("d3d11va-copy");
    context.runningInRemoteDesktop = true;

    const PlaybackEngineProfile::Profile profile =
        PlaybackEngineProfile::buildDefaultProfile(context);

    QCOMPARE(profile.name, QStringLiteral("xplayer-remote-safe"));
    QCOMPARE(profile.optionValue(QStringLiteral("hwdec")), QStringLiteral("no"));
    QCOMPARE(profile.optionValue(QStringLiteral("profile")), QStringLiteral("sw-fast"));
    QCOMPARE(profile.optionValue(QStringLiteral("gpu-dumb-mode")), QStringLiteral("yes"));
}

void PosterWallUtilsTest::playbackLoadPolicyAddsNetworkCacheOptions()
{
    PlaybackEngineProfile::LoadContext context;
    context.isHttpStream = true;
    context.usesRelay = false;
    context.httpProxy = QStringLiteral("http://127.0.0.1:7890");
    context.forceSeekable = true;

    const PlaybackEngineProfile::LoadPolicy policy =
        PlaybackEngineProfile::buildLoadPolicy(context);

    QCOMPARE(policy.options.value(QStringLiteral("http-proxy")).toString(),
             QStringLiteral("http://127.0.0.1:7890"));
    QCOMPARE(policy.options.value(QStringLiteral("force-seekable")).toString(),
             QStringLiteral("yes"));
    QCOMPARE(policy.options.value(QStringLiteral("cache")).toString(),
             QStringLiteral("yes"));
    QCOMPARE(policy.options.value(QStringLiteral("demuxer-readahead-secs")).toString(),
             QStringLiteral("25"));
    QVERIFY(policy.options.contains(QStringLiteral("demuxer-max-bytes")));
}

void PosterWallUtilsTest::playbackFailureMessageIsChineseAndActionable()
{
    QCOMPARE(PlaybackEngineProfile::playerFacingFailureMessage(QStringLiteral("error")),
             QStringLiteral("播放失败，已停止缓冲。可以返回后重试，或切换片源/转码。"));
    QCOMPARE(PlaybackEngineProfile::playerFacingFailureMessage(QStringLiteral("eof")),
             QString());
}

void PosterWallUtilsTest::playbackLoadingOpeningShowsPreparingText()
{
    PlaybackLoadingStateUtils::Context context;
    context.hasMedia = true;
    context.isOpening = true;

    const PlaybackLoadingStateUtils::State state =
        PlaybackLoadingStateUtils::resolveState(context);

    QCOMPARE(state, PlaybackLoadingStateUtils::State::Opening);
    QCOMPARE(PlaybackLoadingStateUtils::displayText(state),
             QStringLiteral("正在准备播放..."));
}

void PosterWallUtilsTest::playbackLoadingBufferingTakesPriorityOverOpening()
{
    PlaybackLoadingStateUtils::Context context;
    context.hasMedia = true;
    context.isOpening = true;
    context.isBuffering = true;

    const PlaybackLoadingStateUtils::State state =
        PlaybackLoadingStateUtils::resolveState(context);

    QCOMPARE(state, PlaybackLoadingStateUtils::State::Buffering);
    QCOMPARE(PlaybackLoadingStateUtils::displayText(state),
             QStringLiteral("正在缓冲..."));
}

void PosterWallUtilsTest::playbackLoadingSeekingTakesPriorityOverBuffering()
{
    PlaybackLoadingStateUtils::Context context;
    context.hasMedia = true;
    context.isBuffering = true;
    context.isSeeking = true;

    const PlaybackLoadingStateUtils::State state =
        PlaybackLoadingStateUtils::resolveState(context);

    QCOMPARE(state, PlaybackLoadingStateUtils::State::Seeking);
    QCOMPARE(PlaybackLoadingStateUtils::displayText(state),
             QStringLiteral("正在定位..."));
}

void PosterWallUtilsTest::playbackLoadingIdleAndTeardownAreHidden()
{
    PlaybackLoadingStateUtils::Context idleContext;
    idleContext.hasMedia = true;
    QCOMPARE(PlaybackLoadingStateUtils::resolveState(idleContext),
             PlaybackLoadingStateUtils::State::Hidden);
    QCOMPARE(PlaybackLoadingStateUtils::displayText(PlaybackLoadingStateUtils::State::Hidden),
             QString());

    PlaybackLoadingStateUtils::Context teardownContext;
    teardownContext.hasMedia = true;
    teardownContext.isBuffering = true;
    teardownContext.isTearingDown = true;
    QCOMPARE(PlaybackLoadingStateUtils::resolveState(teardownContext),
             PlaybackLoadingStateUtils::State::Hidden);
}

void PosterWallUtilsTest::mediaSwitcherUsesChineseMovieModeCopy()
{
    QCOMPARE(PlayerMediaSwitcherTextUtils::movieModeTitle(),
             QStringLiteral("继续观看"));
    QCOMPARE(PlayerMediaSwitcherTextUtils::loadingMessage(),
             QStringLiteral("正在加载..."));
    QCOMPARE(PlayerMediaSwitcherTextUtils::emptyMovieMessage(),
             QStringLiteral("暂无可切换的影片"));
}

void PosterWallUtilsTest::mediaSwitcherUsesChineseSeriesModeCopy()
{
    QCOMPARE(PlayerMediaSwitcherTextUtils::seriesFallbackTitle(),
             QStringLiteral("剧集切换"));
    QCOMPARE(PlayerMediaSwitcherTextUtils::seasonsLabel(),
             QStringLiteral("季"));
    QCOMPARE(PlayerMediaSwitcherTextUtils::episodesLabel(),
             QStringLiteral("剧集"));
    QCOMPARE(PlayerMediaSwitcherTextUtils::emptyEpisodeMessage(),
             QStringLiteral("当前季暂无剧集"));
}

void PosterWallUtilsTest::trackOptionUsesReadableChineseFallbacks()
{
    QCOMPARE(PlayerTrackOptionTextUtils::trackLabel(
                 PlayerTrackOptionTextUtils::TrackKind::Audio, 1,
                 QStringLiteral("DTS-HD 5.1"), QStringLiteral("eng")),
             QStringLiteral("DTS-HD 5.1"));
    QCOMPARE(PlayerTrackOptionTextUtils::trackLabel(
                 PlayerTrackOptionTextUtils::TrackKind::Audio, 2,
                 QString(), QStringLiteral("eng")),
             QStringLiteral("英语"));
    QCOMPARE(PlayerTrackOptionTextUtils::trackLabel(
                 PlayerTrackOptionTextUtils::TrackKind::Subtitle, 3,
                 QString(), QStringLiteral("chi")),
             QStringLiteral("中文"));
    QCOMPARE(PlayerTrackOptionTextUtils::trackLabel(
                 PlayerTrackOptionTextUtils::TrackKind::Subtitle, 4,
                 QString(), QString()),
             QStringLiteral("字幕 4"));
}

void PosterWallUtilsTest::trackOptionUsesChineseDisableAndToastCopy()
{
    QCOMPARE(PlayerTrackOptionTextUtils::disableLabel(
                 PlayerTrackOptionTextUtils::TrackKind::Audio),
             QStringLiteral("关闭音轨"));
    QCOMPARE(PlayerTrackOptionTextUtils::disableLabel(
                 PlayerTrackOptionTextUtils::TrackKind::Subtitle),
             QStringLiteral("关闭字幕"));
    QCOMPARE(PlayerTrackOptionTextUtils::disabledToast(
                 PlayerTrackOptionTextUtils::TrackKind::Audio),
             QStringLiteral("已关闭音轨"));
    QCOMPARE(PlayerTrackOptionTextUtils::selectedToast(
                 PlayerTrackOptionTextUtils::TrackKind::Subtitle,
                 QStringLiteral("中文")),
             QStringLiteral("字幕：中文"));
}

void PosterWallUtilsTest::playbackControlMenuUsesChineseCopy()
{
    QCOMPARE(PlayerControlTextUtils::networkSpeedLabel(),
             QStringLiteral("网络速度"));
    QCOMPARE(PlayerControlTextUtils::statisticsLabel(),
             QStringLiteral("播放信息"));
    QCOMPARE(PlayerControlTextUtils::subtitleSettingsLabel(),
             QStringLiteral("字幕设置"));
    QCOMPARE(PlayerControlTextUtils::strmDirectPlayLabel(),
             QStringLiteral("STRM 直连播放"));
}

void PosterWallUtilsTest::playbackControlStatusToastsAreChinese()
{
    QCOMPARE(PlayerControlTextUtils::networkSpeedToast(true),
             QStringLiteral("已显示网络速度"));
    QCOMPARE(PlayerControlTextUtils::networkSpeedToast(false),
             QStringLiteral("已隐藏网络速度"));
    QCOMPARE(PlayerControlTextUtils::speedToast(QStringLiteral("1.25")),
             QStringLiteral("倍速：1.25x"));
    QCOMPARE(PlayerControlTextUtils::strmDirectToast(true),
             QStringLiteral("STRM 直连播放已开启，正在重新加载..."));
}

void PosterWallUtilsTest::popupOpenAnimationUsesLightweightBudget()
{
    QVERIFY(PlayerPopupAnimationUtils::openDurationMs() <= XplayerUi::kMicroAnimationMs);
    QCOMPARE(PlayerPopupAnimationUtils::openOffsetY(), 8);
}

void PosterWallUtilsTest::popupRequestOpensWithoutActivePopup()
{
    QCOMPARE(PlayerPopupStateUtils::actionForPopupRequest(false, false),
             PlayerPopupStateUtils::PopupRequestAction::Open);
}

void PosterWallUtilsTest::popupRequestClosesWhenSameButtonClickedAgain()
{
    QCOMPARE(PlayerPopupStateUtils::actionForPopupRequest(true, true),
             PlayerPopupStateUtils::PopupRequestAction::CloseCurrent);
}

void PosterWallUtilsTest::popupRequestReplacesWhenDifferentButtonClicked()
{
    QCOMPARE(PlayerPopupStateUtils::actionForPopupRequest(true, false),
             PlayerPopupStateUtils::PopupRequestAction::Replace);
}

void PosterWallUtilsTest::playerWindowModeDefaultsToIndependent()
{
    QVERIFY(PlaybackWindowModeUtils::defaultIndependentWindowEnabled());
    QVERIFY(PlaybackWindowModeUtils::shouldUseIndependentWindow(true));
}

void PosterWallUtilsTest::playerWindowModeRespectsExplicitDisabledSetting()
{
    QVERIFY(!PlaybackWindowModeUtils::shouldUseIndependentWindow(false));
}

void PosterWallUtilsTest::playerWindowLaunchTransitionIsShort()
{
    QVERIFY(PlaybackWindowModeUtils::launchTransitionDurationMs() >= 120);
    QVERIFY(PlaybackWindowModeUtils::launchTransitionDurationMs() <= 220);
}

void PosterWallUtilsTest::playbackWindowMaximizeButtonUsesDistinctIconAndChineseTooltip()
{
    QCOMPARE(PlaybackWindowModeUtils::maximizeIconPath(false),
             QStringLiteral(":/svg/player/max.svg"));
    QCOMPARE(PlaybackWindowModeUtils::maximizeIconPath(true),
             QStringLiteral(":/svg/player/restore.svg"));
    QCOMPARE(PlaybackWindowModeUtils::maximizeTooltipText(false),
             QStringLiteral("全屏播放"));
    QCOMPARE(PlaybackWindowModeUtils::maximizeTooltipText(true),
             QStringLiteral("退出全屏"));
}

void PosterWallUtilsTest::topbarAndBottomMaximizeButtonsShareStateContract()
{
    QCOMPARE(PlaybackWindowModeUtils::topbarMaximizeIconPath(false),
             PlaybackWindowModeUtils::maximizeIconPath(false));
    QCOMPARE(PlaybackWindowModeUtils::topbarMaximizeIconPath(true),
             PlaybackWindowModeUtils::maximizeIconPath(true));
    QCOMPARE(PlaybackWindowModeUtils::topbarMaximizeTooltipText(false),
             PlaybackWindowModeUtils::maximizeTooltipText(false));
    QCOMPARE(PlaybackWindowModeUtils::topbarMaximizeTooltipText(true),
             PlaybackWindowModeUtils::maximizeTooltipText(true));
}

void PosterWallUtilsTest::fullscreenNotifiesWindowsShellButMaximizedDoesNot()
{
    QVERIFY(PlaybackWindowModeUtils::shouldNotifyWindowsShellFullscreen(
        Qt::WindowFullScreen));
    QVERIFY(!PlaybackWindowModeUtils::shouldNotifyWindowsShellFullscreen(
        Qt::WindowMaximized));
    QVERIFY(!PlaybackWindowModeUtils::shouldNotifyWindowsShellFullscreen(
        Qt::WindowNoState));
}

QTEST_MAIN(PosterWallUtilsTest)
#include "posterwallutils_test.moc"
