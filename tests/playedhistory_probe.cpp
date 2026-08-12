#include <QCoreApplication>
#include <QElapsedTimer>
#include <QTextStream>
#include <qcorotask.h>
#include <services/manager/servermanager.h>
#include <services/media/mediaservice.h>
#include <xplayercore.h>

int main(int argc, char *argv[])
{
    QCoreApplication app(argc, argv);
    app.setApplicationName(QStringLiteral("Xplayer"));
    app.setOrganizationName(QStringLiteral("Godking"));
    app.setOrganizationDomain(QStringLiteral("local.xplayer"));

    XplayerCore core;
    core.serverManager()->loadSettings();
    if (!core.serverManager()->activeProfile().isValid()) {
        QTextStream(stderr) << "NO_ACTIVE_PROFILE\n";
        return 2;
    }

    QElapsedTimer timer;
    timer.start();
    try {
        MediaQueryPage page = QCoro::waitFor(
            core.mediaService()->getPlayedItemsPage(
                QStringLiteral("DatePlayed"), QStringLiteral("Descending"),
                0, 100));
        QTextStream(stdout) << "OK returned=" << page.items.size()
                            << " total=" << page.totalRecordCount
                            << " hasTotal=" << page.hasTotalRecordCount
                            << " elapsedMs=" << timer.elapsed() << '\n';
        return page.items.isEmpty() ? 3 : 0;
    } catch (const std::exception &e) {
        QTextStream(stderr) << "FAILED elapsedMs=" << timer.elapsed()
                            << " error=" << e.what() << '\n';
        return 1;
    }
}
