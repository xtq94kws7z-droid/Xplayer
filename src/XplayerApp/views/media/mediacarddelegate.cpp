#include "mediacarddelegate.h"
#include "medialistmodel.h"
#include "../../managers/thememanager.h"
#include "../../utils/mediacardlayoututils.h"
#include "../../utils/textwraputils.h"
#include <models/media/mediaitem.h>
#include <QAbstractItemView>
#include <QApplication>
#include <QEvent>
#include <QFontMetrics>
#include <QIcon>
#include <QLinearGradient>
#include <QMouseEvent>
#include <QPainter>
#include <QPainterPath>
#include <QPixmapCache>
#include <QRadialGradient>
#include <QStyle>
#include <QStyleOptionViewItem>
#include <algorithm>

namespace
{

enum class HoverOverlayMode
{
    None,
    FullControls,
    MenuOnly
};

constexpr int kPosterRadius = 16;
constexpr int kTileRadius = 18;
constexpr int kEpisodeRadius = 14;
constexpr int kButtonSize = 28;
constexpr int kPlayButtonSize = 48;
constexpr int kButtonGap = 6;
constexpr int kTextGap = 6;
constexpr int kHoverOutlineWidth = 2;
constexpr int kTitleCacheLimit = 1024;

bool isPlayableItem(const MediaItem &item)
{
    return item.mediaType == QStringLiteral("Video") ||
           item.type == QStringLiteral("Movie") ||
           item.type == QStringLiteral("Episode") ||
           item.type == QStringLiteral("Video") ||
           item.type == QStringLiteral("MusicVideo");
}

bool isPlaylistItem(const MediaItem &item)
{
    return item.type == QStringLiteral("Playlist");
}

bool canShowMenuOnlyButtonForTile(const MediaItem &item)
{
    if (item.id.trimmed().isEmpty()) {
        return false;
    }

    const QString collectionType = item.collectionType.trimmed().toLower();
    const QString type = item.type.trimmed().toLower();
    return collectionType != QStringLiteral("playlists") &&
           collectionType != QStringLiteral("boxsets") &&
           type != QStringLiteral("playlist") &&
           type != QStringLiteral("boxset");
}

HoverOverlayMode hoverOverlayModeForItem(
    const MediaItem &item, MediaCardDelegate::CardStyle style,
    bool showMoreButtonForNonPlayableTiles)
{
    if (style == MediaCardDelegate::EpisodeList) {
        return isPlayableItem(item) ? HoverOverlayMode::FullControls
                                    : HoverOverlayMode::None;
    }

    if (isPlayableItem(item) || item.type == QStringLiteral("Series") ||
        item.type == QStringLiteral("Season")) {
        return HoverOverlayMode::FullControls;
    }

    if (isPlaylistItem(item)) {
        return HoverOverlayMode::MenuOnly;
    }

    if (style == MediaCardDelegate::LibraryTile &&
        showMoreButtonForNonPlayableTiles && canShowMenuOnlyButtonForTile(item)) {
        return HoverOverlayMode::MenuOnly;
    }

    return HoverOverlayMode::None;
}

bool shouldShowPlayButton(HoverOverlayMode overlayMode,
                          MediaCardDelegate::HoverControls controls)
{
    return overlayMode == HoverOverlayMode::FullControls &&
           controls.testFlag(MediaCardDelegate::HoverControlPlay);
}

bool shouldShowFavoriteButton(HoverOverlayMode overlayMode,
                              MediaCardDelegate::HoverControls controls)
{
    return overlayMode == HoverOverlayMode::FullControls &&
           controls.testFlag(MediaCardDelegate::HoverControlFavorite);
}

bool shouldShowMoreButton(HoverOverlayMode overlayMode,
                          MediaCardDelegate::HoverControls controls)
{
    return overlayMode != HoverOverlayMode::None &&
           controls.testFlag(MediaCardDelegate::HoverControlMore);
}

int imageRadiusForStyle(MediaCardDelegate::CardStyle style)
{
    if (style == MediaCardDelegate::LibraryTile) {
        return kTileRadius;
    }
    if (style == MediaCardDelegate::EpisodeList) {
        return kEpisodeRadius;
    }
    return kPosterRadius;
}

QRect imageRectForStyle(MediaCardDelegate::CardStyle style, const QRect &rect,
                        int padding)
{
    const int contentWidth = qMax(1, rect.width() - padding * 2);

    if (style == MediaCardDelegate::EpisodeList) {
        const int imageHeight = qMax(1, rect.height() - padding * 2);
        const int imageWidth = qRound(imageHeight * 16.0 / 9.0);
        return QRect(rect.x() + padding, rect.y() + padding, imageWidth,
                     imageHeight);
    }

    if (style == MediaCardDelegate::LibraryTile) {
        const int imageHeight = qRound(contentWidth * 9.0 / 16.0);
        return QRect(rect.x() + padding, rect.y() + padding, contentWidth,
                     imageHeight);
    }

    const int imageHeight = qRound(contentWidth * 1.5);
    return QRect(rect.x() + padding, rect.y() + padding, contentWidth,
                 imageHeight);
}

QRect playButtonRect(const QRect &imageRect)
{
    return QRect(imageRect.center().x() - kPlayButtonSize / 2,
                 imageRect.center().y() - kPlayButtonSize / 2, kPlayButtonSize,
                 kPlayButtonSize);
}

QRect moreButtonRect(const QRect &imageRect)
{
    return QRect(imageRect.right() - kButtonSize - 8,
                 imageRect.bottom() - kButtonSize - 8, kButtonSize,
                 kButtonSize);
}

QRect favoriteButtonRect(const QRect &moreRect)
{
    return QRect(moreRect.left() - kButtonSize - kButtonGap, moreRect.top(),
                 kButtonSize, kButtonSize);
}

QPainterPath roundedRectPath(const QRect &rect, int radius)
{
    QPainterPath path;
    path.addRoundedRect(rect, radius, radius);
    return path;
}

void drawRoundedPixmap(QPainter *painter, const QPixmap &pixmap,
                       const QRect &target, int radius)
{
    if (pixmap.isNull()) {
        return;
    }

    const QPainterPath clipPath = roundedRectPath(target, radius);
    painter->save();
    painter->setClipPath(clipPath);
    painter->drawPixmap(target, pixmap);
    painter->restore();
}

void drawImageBorder(QPainter *painter, const QRect &rect, int radius,
                     bool isDarkMode)
{
    painter->save();
    painter->setBrush(Qt::NoBrush);
    painter->setPen(QPen(isDarkMode ? QColor(255, 255, 255, 34)
                                    : QColor(255, 255, 255, 185),
                         1.0));
    painter->drawRoundedRect(rect.adjusted(0, 0, -1, -1), radius, radius);
    painter->restore();
}

void drawFloatingHoverSurface(QPainter *painter, const QRect &cardRect,
                              const QRect &imageRect, int radius,
                              bool isDarkMode)
{
    painter->save();
    painter->setRenderHint(QPainter::Antialiasing, true);
    painter->setPen(Qt::NoPen);

    const QRectF shadowRect = imageRect.adjusted(-10, 8, 10, 20);
    QRadialGradient shadow(shadowRect.center(),
                           qMax(shadowRect.width(), shadowRect.height()) *
                               0.58);
    shadow.setColorAt(0.0, isDarkMode ? QColor(0, 0, 0, 110)
                                      : QColor(15, 23, 42, 58));
    shadow.setColorAt(1.0, QColor(0, 0, 0, 0));
    painter->setBrush(shadow);
    painter->drawEllipse(shadowRect);

    QPainterPath surfacePath;
    surfacePath.addRoundedRect(cardRect.adjusted(1, 1, -1, -1), radius + 5,
                               radius + 5);
    QLinearGradient surface(cardRect.topLeft(), cardRect.bottomRight());
    if (isDarkMode) {
        surface.setColorAt(0.0, QColor(255, 255, 255, 20));
        surface.setColorAt(1.0, QColor(255, 255, 255, 7));
    } else {
        surface.setColorAt(0.0, QColor(15, 23, 42, 20));
        surface.setColorAt(1.0, QColor(255, 255, 255, 36));
    }
    painter->fillPath(surfacePath, surface);

    painter->setBrush(Qt::NoBrush);
    painter->setPen(QPen(isDarkMode ? QColor(255, 255, 255, 44)
                                    : QColor(255, 255, 255, 172),
                         1.0));
    painter->drawRoundedRect(cardRect.adjusted(1, 1, -2, -2), radius + 5,
                             radius + 5);
    painter->restore();
}

void drawHoverImageChrome(QPainter *painter, const QRect &imageRect,
                          int radius, bool isDarkMode)
{
    painter->save();
    painter->setRenderHint(QPainter::Antialiasing, true);
    painter->setBrush(Qt::NoBrush);
    painter->setPen(QPen(isDarkMode ? QColor(255, 255, 255, 92)
                                    : QColor(255, 255, 255, 235),
                         1.2));
    painter->drawRoundedRect(imageRect.adjusted(-1, -1, 1, 1), radius + 1,
                             radius + 1);
    painter->restore();
}

void drawPlaceholder(QPainter *painter, const QRect &rect, int radius,
                     const QColor &bg, const QColor &textColor,
                     const QString &label)
{
    painter->save();
    painter->setPen(Qt::NoPen);
    painter->setBrush(bg);
    painter->drawRoundedRect(rect, radius, radius);

    QFont font = painter->font();
    font.setBold(true);
    painter->setFont(font);
    painter->setPen(textColor);
    painter->drawText(rect.adjusted(8, 8, -8, -8), Qt::AlignCenter,
                      label.trimmed().left(1).toUpper());
    painter->restore();
}

void drawIconButton(QPainter *painter, const QRect &rect, const QIcon &icon,
                    const QColor &bg, const QColor &iconColor)
{
    painter->save();
    painter->setRenderHint(QPainter::Antialiasing, true);
    painter->setPen(Qt::NoPen);
    painter->setBrush(bg);
    painter->drawEllipse(rect);

    QIcon effectiveIcon = icon;
    if (!icon.isNull()) {
        effectiveIcon.paint(painter, rect.adjusted(7, 7, -7, -7),
                            Qt::AlignCenter, QIcon::Normal);
    } else {
        painter->setPen(QPen(iconColor, 2.0, Qt::SolidLine, Qt::RoundCap,
                             Qt::RoundJoin));
        const QPoint center = rect.center();
        const QPoint points[3] = {
            QPoint(center.x() - 4, center.y() - 7),
            QPoint(center.x() - 4, center.y() + 7),
            QPoint(center.x() + 8, center.y())
        };
        painter->drawPolyline(points, 3);
    }
    painter->restore();
}

QString metaTextForItem(const MediaItem &item,
                        MediaCardDelegate::CardStyle style)
{
    if (style == MediaCardDelegate::EpisodeList) {
        QStringList parts;
        if (item.parentIndexNumber >= 0 && item.indexNumber >= 0) {
            parts << QStringLiteral("S%1E%2")
                         .arg(item.parentIndexNumber, 2, 10, QChar('0'))
                         .arg(item.indexNumber, 2, 10, QChar('0'));
        }
        if (item.runTimeTicks > 0) {
            const long long minutes = item.runTimeTicks / 10000000 / 60;
            if (minutes > 0) {
                parts << QStringLiteral("%1 min").arg(minutes);
            }
        }
        if (!item.seriesName.isEmpty()) {
            parts << item.seriesName;
        }
        return parts.join(QStringLiteral("  •  "));
    }

    QStringList parts;
    if (item.productionYear > 0) {
        parts << QString::number(item.productionYear);
    }
    if (item.type == QStringLiteral("Season") && item.recursiveItemCount > 0) {
        parts << QStringLiteral("%1 Episodes").arg(item.recursiveItemCount);
    }
    return parts.join(QStringLiteral("  •  "));
}

QString iconThemeDir(bool isDarkMode)
{
    return isDarkMode ? QStringLiteral("dark") : QStringLiteral("light");
}

QIcon adaptiveIcon(const QString &path, bool isDarkMode,
                   const QColor &color = QColor())
{
    const QString themedPath =
        path.contains(QStringLiteral("/light/"))
            ? path.mid(0, path.indexOf(QStringLiteral("/light/")) + 1) +
                  iconThemeDir(isDarkMode) +
                  path.mid(path.indexOf(QStringLiteral("/light/")) + 6)
            : path;
    return ThemeManager::getAdaptiveIcon(themedPath, color);
}

} // namespace

MediaCardThemeHelper::MediaCardThemeHelper(QWidget *parent)
    : QWidget(parent)
{
}

MediaCardDelegate::MediaCardDelegate(CardStyle style, QObject *parent)
    : QStyledItemDelegate(parent), m_style(style),
      m_tileSize(style == LibraryTile ? QSize(282, 226) : QSize(160, 296)),
      m_themeHelper(nullptr)
{
    if (auto *theme = ThemeManager::instance()) {
        connect(theme, &ThemeManager::themeChanged, this,
                [this](ThemeManager::Theme) {
                    m_themeCacheDirty = true;
                    m_hoverIconCacheDirty = true;
                    m_titleWrapCache.clear();
                });
    }
}

MediaCardDelegate::~MediaCardDelegate()
{
    delete m_themeHelper;
}

void MediaCardDelegate::ensureThemeCache() const
{
    if (!m_themeCacheDirty) {
        return;
    }

    ThemeManager *theme = ThemeManager::instance();
    m_cachedDarkMode = theme && theme->isDarkMode();
    m_cachedFontScale = ThemeManager::fontScaleFactor();
    m_cachedHoverBg = m_cachedDarkMode ? QColor(255, 255, 255, 18)
                                       : QColor(15, 23, 42, 18);
    m_cachedSelectedBg = QColor(47, 128, 237, 42);
    m_cachedTitleColor = m_cachedDarkMode ? QColor(244, 247, 251)
                                          : QColor(22, 26, 32);
    m_cachedSubTitleColor = m_cachedDarkMode ? QColor(166, 176, 190)
                                             : QColor(92, 99, 112);
    m_cachedPlaceholderBg = m_cachedDarkMode ? QColor(40, 45, 54)
                                             : QColor(232, 236, 242);
    m_cachedPlaceholderTextColor = m_cachedDarkMode ? QColor(138, 148, 162)
                                                    : QColor(126, 135, 148);

    if (m_themeHelper) {
        m_themeHelper->setHoverBg(m_cachedHoverBg);
        m_themeHelper->setSelectedBg(m_cachedSelectedBg);
        m_themeHelper->setTitleColor(m_cachedTitleColor);
        m_themeHelper->setSubTitleColor(m_cachedSubTitleColor);
        m_themeHelper->setPlaceholderBg(m_cachedPlaceholderBg);
        m_themeHelper->setPlaceholderTextColor(m_cachedPlaceholderTextColor);
    }

    m_themeCacheDirty = false;
}

void MediaCardDelegate::ensureHoverIconCache() const
{
    if (!m_hoverIconCacheDirty) {
        return;
    }

    const QColor iconColor(Qt::white);
    m_cachedMoreIcon = ThemeManager::getAdaptiveIcon(
        QStringLiteral(":/svg/light/more-line.svg"), iconColor);
    m_cachedPlayIcon = ThemeManager::getAdaptiveIcon(
        QStringLiteral(":/svg/player/play.svg"), iconColor);
    m_cachedHeartOutlineIcon = ThemeManager::getAdaptiveIcon(
        QStringLiteral(":/svg/light/heart-outline.svg"), iconColor);
    m_cachedHeartFillIcon = ThemeManager::getAdaptiveIcon(
        QStringLiteral(":/svg/light/heart-fill.svg"), iconColor);
    m_hoverIconCacheDirty = false;
}

QString MediaCardDelegate::cachedWrappedTitle(const MediaItem &item,
                                              const QFont &font, int width,
                                              int maxLines) const
{
    const QString title = item.name.trimmed();
    if (title.isEmpty() || width <= 0) {
        return title;
    }

    const QString cacheKey =
        QStringLiteral("%1|%2|%3|%4|%5")
            .arg(item.id, QString::number(width),
                 QString::number(font.pixelSize()), QString::number(maxLines),
                 title);
    auto it = m_titleWrapCache.constFind(cacheKey);
    if (it != m_titleWrapCache.constEnd()) {
        return it.value();
    }

    QString wrapped = TextWrapUtils::wrapPlainText(title, font, width);
    const QStringList lines = wrapped.split(QLatin1Char('\n'));
    if (maxLines > 0 && lines.size() > maxLines) {
        QStringList limited = lines.mid(0, maxLines);
        QString last = limited.last();
        QFontMetrics fm(font);
        last = fm.elidedText(last, Qt::ElideRight, width);
        limited.last() = last;
        wrapped = limited.join(QLatin1Char('\n'));
    }

    if (m_titleWrapCache.size() >= kTitleCacheLimit) {
        m_titleWrapCache.clear();
    }
    m_titleWrapCache.insert(cacheKey, wrapped);
    return wrapped;
}

QSize MediaCardDelegate::sizeHint(const QStyleOptionViewItem &option,
                                  const QModelIndex &index) const
{
    Q_UNUSED(option)
    Q_UNUSED(index)
    return m_tileSize;
}

void MediaCardDelegate::paint(QPainter *painter,
                              const QStyleOptionViewItem &option,
                              const QModelIndex &index) const
{
    if (!index.isValid()) {
        return;
    }

    ensureThemeCache();

    MediaItem fallbackItem;
    const MediaListModel *mediaModel =
        qobject_cast<const MediaListModel *>(index.model());
    const MediaItem *itemPtr =
        mediaModel ? mediaModel->itemAt(index.row()) : nullptr;
    if (!itemPtr) {
        fallbackItem = index.data(MediaListModel::ItemDataRole).value<MediaItem>();
        itemPtr = &fallbackItem;
    }
    const MediaItem &item = *itemPtr;
    const bool hovered = option.state.testFlag(QStyle::State_MouseOver);
    const bool selected = option.state.testFlag(QStyle::State_Selected) ||
                          (!m_highlightedItemId.isEmpty() &&
                           item.id == m_highlightedItemId);
    const int padding = m_contentPadding;
    const QRect cardRect = option.rect.adjusted(2, 2, -2, -2);
    const QRect imageRect = imageRectForStyle(m_style, cardRect, padding);
    const int radius = imageRadiusForStyle(m_style);

    painter->save();
    painter->setRenderHint(QPainter::Antialiasing, true);

    if (selected) {
        painter->setPen(QPen(QColor(47, 128, 237, 190),
                             kHoverOutlineWidth));
        painter->setBrush(m_cachedSelectedBg);
        painter->drawRoundedRect(cardRect, radius, radius);
    } else if (hovered) {
        drawFloatingHoverSurface(painter, cardRect, imageRect, radius,
                                 m_cachedDarkMode);
    }

    const QVariant pixVariant = index.data(MediaListModel::PosterPixmapRole);
    const QPixmap pixmap = pixVariant.value<QPixmap>();
    if (!pixmap.isNull()) {
        const QString cacheKey =
            QStringLiteral("mediacard:%1:%2:%3:%4")
                .arg(item.id)
                .arg(imageRect.size().width())
                .arg(imageRect.size().height())
                .arg(pixmap.cacheKey());
        QPixmap scaled;
        if (!QPixmapCache::find(cacheKey, &scaled)) {
            const qreal dpr = painter->device()->devicePixelRatioF();
            const QSize physicalTarget = imageRect.size() * dpr;
            scaled = pixmap.scaled(physicalTarget,
                                   Qt::KeepAspectRatioByExpanding,
                                   Qt::SmoothTransformation);
            if (scaled.size() != physicalTarget) {
                const int x = qMax(0, (scaled.width() - physicalTarget.width()) / 2);
                const int y = qMax(0, (scaled.height() - physicalTarget.height()) / 2);
                scaled = scaled.copy(QRect(QPoint(x, y), physicalTarget));
            }
            scaled.setDevicePixelRatio(dpr);
            QPixmapCache::insert(cacheKey, scaled);
        }
        drawRoundedPixmap(painter, scaled, imageRect, radius);
    } else {
        drawPlaceholder(painter, imageRect, radius, m_cachedPlaceholderBg,
                        m_cachedPlaceholderTextColor, item.name);
    }
    drawImageBorder(painter, imageRect, radius, m_cachedDarkMode);
    if (hovered && !selected) {
        drawHoverImageChrome(painter, imageRect, radius, m_cachedDarkMode);
    }

    const HoverOverlayMode overlayMode =
        hoverOverlayModeForItem(item, m_style, m_showMoreButtonForNonPlayableTiles);
    if (hovered && overlayMode != HoverOverlayMode::None) {
        ensureHoverIconCache();
        painter->save();
        QPainterPath overlayPath = roundedRectPath(imageRect, radius);
        QLinearGradient overlay(imageRect.topLeft(), imageRect.bottomLeft());
        overlay.setColorAt(0.0, QColor(0, 0, 0, 22));
        overlay.setColorAt(0.48, QColor(0, 0, 0, 48));
        overlay.setColorAt(1.0, QColor(0, 0, 0, 142));
        painter->setPen(Qt::NoPen);
        painter->fillPath(overlayPath, overlay);
        painter->restore();

        const QColor buttonBg(11, 18, 32, 218);
        const QColor iconColor(Qt::white);
        const QRect moreRect = moreButtonRect(imageRect);
        if (shouldShowMoreButton(overlayMode, m_hoverControls)) {
            drawIconButton(painter, moreRect, m_cachedMoreIcon, buttonBg,
                           iconColor);
        }
        if (shouldShowFavoriteButton(overlayMode, m_hoverControls)) {
            drawIconButton(painter, favoriteButtonRect(moreRect),
                           item.isFavorite() ? m_cachedHeartFillIcon
                                             : m_cachedHeartOutlineIcon,
                           buttonBg, iconColor);
        }
        if (shouldShowPlayButton(overlayMode, m_hoverControls)) {
            drawIconButton(painter, playButtonRect(imageRect), m_cachedPlayIcon,
                           QColor(20, 24, 32, 220), iconColor);
        }
    }

    QFont titleFont = option.font;
    titleFont.setPixelSize(
        qMax(8, qRound(m_titleFontPixelSize * m_cachedFontScale)));
    titleFont.setBold(true);
    const int titleLines = m_style == EpisodeList ? 1 : 2;
    const int titleWidth = m_style == EpisodeList
                               ? qMax(1, cardRect.right() - imageRect.right() -
                                             padding * 2 - 8)
                               : qMax(1, cardRect.width() - padding * 2);
    const QString wrappedTitle =
        cachedWrappedTitle(item, titleFont, titleWidth, titleLines);
    const int actualTitleLines =
        qMax(1, wrappedTitle.count(QLatin1Char('\n')) + 1);
    const auto textLayout = MediaCardLayoutUtils::textLayout(
        cardRect, imageRect, padding, m_style == EpisodeList,
        actualTitleLines, QFontMetrics(titleFont).lineSpacing());

    painter->setFont(titleFont);
    painter->setPen(m_cachedTitleColor);
    painter->drawText(textLayout.titleRect, textLayout.titleAlignment,
                      wrappedTitle);

    const QString meta = metaTextForItem(item, m_style);
    if (!meta.isEmpty()) {
        QFont subTitleFont = option.font;
        subTitleFont.setPixelSize(
            qMax(8, qRound(m_subTitleFontPixelSize * m_cachedFontScale)));
        painter->setFont(subTitleFont);
        painter->setPen(m_cachedSubTitleColor);
        painter->drawText(textLayout.metaRect, textLayout.metaAlignment,
                          QFontMetrics(subTitleFont).elidedText(
                              meta, Qt::ElideRight,
                              textLayout.metaRect.width()));
    }

    painter->restore();
}

bool MediaCardDelegate::eventFilter(QObject *object, QEvent *event)
{
    if (event->type() != QEvent::MouseButtonRelease) {
        return QStyledItemDelegate::eventFilter(object, event);
    }

    QWidget *viewport = qobject_cast<QWidget *>(object);
    if (!viewport) {
        return QStyledItemDelegate::eventFilter(object, event);
    }

    QAbstractItemView *view =
        qobject_cast<QAbstractItemView *>(viewport->parentWidget());
    if (!view) {
        return QStyledItemDelegate::eventFilter(object, event);
    }

    QMouseEvent *mouseEvent = static_cast<QMouseEvent *>(event);
    const QModelIndex index = view->indexAt(mouseEvent->pos());
    if (!index.isValid()) {
        return QStyledItemDelegate::eventFilter(object, event);
    }

    MediaItem fallbackItem;
    const MediaListModel *mediaModel =
        qobject_cast<const MediaListModel *>(index.model());
    const MediaItem *itemPtr =
        mediaModel ? mediaModel->itemAt(index.row()) : nullptr;
    if (!itemPtr) {
        fallbackItem = index.data(MediaListModel::ItemDataRole).value<MediaItem>();
        itemPtr = &fallbackItem;
    }
    const MediaItem &item = *itemPtr;
    const HoverOverlayMode overlayMode =
        hoverOverlayModeForItem(item, m_style, m_showMoreButtonForNonPlayableTiles);
    if (overlayMode == HoverOverlayMode::None) {
        return QStyledItemDelegate::eventFilter(object, event);
    }

    const QRect visualRect = view->visualRect(index);
    const QRect imageRect = imageRectForStyle(m_style, visualRect.adjusted(2, 2, -2, -2),
                                              m_contentPadding);
    const QRect moreRect = moreButtonRect(imageRect);

    if (shouldShowMoreButton(overlayMode, m_hoverControls) &&
        moreRect.contains(mouseEvent->pos())) {
        Q_EMIT moreMenuRequested(item, viewport->mapToGlobal(moreRect.center()));
        return true;
    }

    if (shouldShowFavoriteButton(overlayMode, m_hoverControls) &&
        favoriteButtonRect(moreRect).contains(mouseEvent->pos())) {
        Q_EMIT favoriteRequested(item);
        return true;
    }

    if (shouldShowPlayButton(overlayMode, m_hoverControls) &&
        playButtonRect(imageRect).contains(mouseEvent->pos())) {
        Q_EMIT playRequested(item);
        return true;
    }

    return QStyledItemDelegate::eventFilter(object, event);
}
