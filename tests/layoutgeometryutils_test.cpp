#include "utils/buttonlayoututils.h"
#include "utils/mediacardlayoututils.h"

#include <QFont>
#include <QFontMetrics>
#include <QtTest>

class LayoutGeometryUtilsTest final : public QObject
{
    Q_OBJECT

private slots:
    void textButtonWidthCoversRenderedGlyphBounds();
    void sectionMoreButtonKeepsComfortableTextInsets();
    void sectionMoreButtonTextKeepsGlyphsAwayFromClipEdges();
    void posterCardTextIsCenteredAndMetadataIsCompact();
    void detailEpisodeCardFitsTwoTitleLines();
    void responsiveDetailEpisodeCardKeepsTwoTitleLines();
};

void LayoutGeometryUtilsTest::textButtonWidthCoversRenderedGlyphBounds()
{
    QFont font(QStringLiteral("Times New Roman"));
    font.setPixelSize(20);
    font.setItalic(true);
    const QString text = QStringLiteral("fj");
    const QFontMetrics metrics(font);
    const QRect bounds = metrics.boundingRect(text);
    const int renderedLeft = qMin(0, bounds.left());
    const int renderedRight = qMax(metrics.horizontalAdvance(text),
                                   bounds.right() + 1);

    QCOMPARE(ButtonLayoutUtils::minimumTextButtonWidth(text, font, 8, 6),
             renderedRight - renderedLeft + 22);
}

void LayoutGeometryUtilsTest::sectionMoreButtonKeepsComfortableTextInsets()
{
    QFont font;
    font.setPixelSize(14);
    const QString text = QStringLiteral("查看全部 >");
    const int measuredWidth = ButtonLayoutUtils::minimumTextButtonWidth(
        text, font, 14, 8);

    QCOMPARE(ButtonLayoutUtils::minimumSectionMoreButtonWidth(text, font),
             measuredWidth);
}

void LayoutGeometryUtilsTest::sectionMoreButtonTextKeepsGlyphsAwayFromClipEdges()
{
    const QString text = QStringLiteral("查看全部 >");
    const QString displayText =
        ButtonLayoutUtils::sectionMoreButtonDisplayText(text);

    QCOMPARE(displayText.front().unicode(), ushort(0x2002));
    QCOMPARE(displayText.back().unicode(), ushort(0x2002));
    QCOMPARE(displayText.mid(1, text.size()), text);
}

void LayoutGeometryUtilsTest::posterCardTextIsCenteredAndMetadataIsCompact()
{
    const QRect cardRect(2, 2, 156, 292);
    const QRect imageRect(10, 10, 140, 210);
    const auto layout = MediaCardLayoutUtils::textLayout(
        cardRect, imageRect, 8, false, 1, 17);

    QVERIFY(layout.titleAlignment.testFlag(Qt::AlignHCenter));
    QVERIFY(layout.metaAlignment.testFlag(Qt::AlignHCenter));
    QCOMPARE(layout.titleRect.height(), 17);
    QCOMPARE(layout.metaRect.top() - layout.titleRect.bottom(), 3);
}

void LayoutGeometryUtilsTest::detailEpisodeCardFitsTwoTitleLines()
{
    constexpr int padding = 8;
    constexpr int imageHeight = 100;
    constexpr int titleLineSpacing = 17;
    constexpr int titleLineCount = 2;
    constexpr int metaHeight = 20;
    constexpr int bottomPadding = 8;

    const int cardHeight = MediaCardLayoutUtils::minimumTileHeight(
        padding, imageHeight, titleLineCount, titleLineSpacing, metaHeight,
        bottomPadding);
    const QRect cardRect(2, 2, 190, cardHeight - 4);
    const QRect imageRect(cardRect.left() + padding, cardRect.top() + padding,
                          174, imageHeight);
    const auto layout = MediaCardLayoutUtils::textLayout(
        cardRect, imageRect, padding, false, titleLineCount,
        titleLineSpacing);

    QCOMPARE(layout.titleRect.height(), titleLineCount * titleLineSpacing);
    QVERIFY(layout.metaRect.bottom() + bottomPadding <= cardRect.bottom());
}

void LayoutGeometryUtilsTest::responsiveDetailEpisodeCardKeepsTwoTitleLines()
{
    constexpr int imageHeight = 100;
    constexpr int horizontalPadding = 16;
    constexpr int contentPadding = 8;
    constexpr int titleLineSpacing = 17;

    const QSize tileSize = MediaCardLayoutUtils::detailEpisodeTileSize(
        imageHeight, horizontalPadding, contentPadding, titleLineSpacing);

    QCOMPARE(tileSize.width(), qRound(imageHeight * 16.0 / 9.0) +
                                   horizontalPadding);
    const QRect cardRect(2, 2, tileSize.width() - 4, tileSize.height() - 4);
    const QRect imageRect(cardRect.left() + contentPadding,
                          cardRect.top() + contentPadding,
                          cardRect.width() - contentPadding * 2,
                          imageHeight);
    const auto layout = MediaCardLayoutUtils::textLayout(
        cardRect, imageRect, contentPadding, false, 2, titleLineSpacing);

    QVERIFY(layout.metaRect.bottom() + contentPadding <= cardRect.bottom());
}

QTEST_MAIN(LayoutGeometryUtilsTest)
#include "layoutgeometryutils_test.moc"
