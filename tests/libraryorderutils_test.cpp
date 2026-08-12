#include "utils/libraryorderutils.h"

#include <QtTest>

class LibraryOrderUtilsTest final : public QObject
{
    Q_OBJECT

private slots:
    void appliesKnownOrderAndPreservesUnmatchedOrder();
};

namespace
{

VirtualFolder makeFolder(const QString &id)
{
    VirtualFolder folder;
    folder.id = id;
    folder.itemId = id;
    folder.name = id;
    return folder;
}

}

void LibraryOrderUtilsTest::appliesKnownOrderAndPreservesUnmatchedOrder()
{
    const QList<VirtualFolder> folders{
        makeFolder(QStringLiteral("b")),
        makeFolder(QStringLiteral("unknown-1")),
        makeFolder(QStringLiteral("a")),
        makeFolder(QStringLiteral("unknown-2"))};

    const QList<VirtualFolder> ordered = LibraryOrderUtils::applyOrder(
        folders, {QStringLiteral("a"), QStringLiteral("b")});

    QCOMPARE(ordered.size(), 4);
    QCOMPARE(ordered.at(0).itemId, QStringLiteral("a"));
    QCOMPARE(ordered.at(1).itemId, QStringLiteral("b"));
    QCOMPARE(ordered.at(2).itemId, QStringLiteral("unknown-1"));
    QCOMPARE(ordered.at(3).itemId, QStringLiteral("unknown-2"));
}

QTEST_MAIN(LibraryOrderUtilsTest)
#include "libraryorderutils_test.moc"
