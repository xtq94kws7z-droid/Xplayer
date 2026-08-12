#include "playermediaswitchertextutils.h"

namespace PlayerMediaSwitcherTextUtils
{

QString movieModeTitle()
{
    return QStringLiteral("继续观看");
}

QString seriesFallbackTitle()
{
    return QStringLiteral("剧集切换");
}

QString loadingMessage()
{
    return QStringLiteral("正在加载...");
}

QString emptyMovieMessage()
{
    return QStringLiteral("暂无可切换的影片");
}

QString emptyEpisodeMessage()
{
    return QStringLiteral("当前季暂无剧集");
}

QString seasonsLabel()
{
    return QStringLiteral("季");
}

QString episodesLabel()
{
    return QStringLiteral("剧集");
}

} // namespace PlayerMediaSwitcherTextUtils
