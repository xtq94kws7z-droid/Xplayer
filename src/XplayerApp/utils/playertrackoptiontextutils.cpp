#include "playertrackoptiontextutils.h"

namespace PlayerTrackOptionTextUtils
{
namespace
{

QString languageLabel(QString language)
{
    language = language.trimmed().toLower();
    if (language.isEmpty())
    {
        return {};
    }

    if (language == QLatin1String("zh") || language == QLatin1String("zho") ||
        language == QLatin1String("chi") || language == QLatin1String("chs") ||
        language == QLatin1String("cht") || language == QLatin1String("cmn"))
    {
        return QStringLiteral("中文");
    }
    if (language == QLatin1String("en") || language == QLatin1String("eng"))
    {
        return QStringLiteral("英语");
    }
    if (language == QLatin1String("ja") || language == QLatin1String("jpn"))
    {
        return QStringLiteral("日语");
    }
    if (language == QLatin1String("ko") || language == QLatin1String("kor"))
    {
        return QStringLiteral("韩语");
    }
    if (language == QLatin1String("fr") || language == QLatin1String("fre") ||
        language == QLatin1String("fra"))
    {
        return QStringLiteral("法语");
    }
    if (language == QLatin1String("de") || language == QLatin1String("ger") ||
        language == QLatin1String("deu"))
    {
        return QStringLiteral("德语");
    }
    if (language == QLatin1String("es") || language == QLatin1String("spa"))
    {
        return QStringLiteral("西班牙语");
    }

    return language.toUpper();
}

QString fallbackLabel(TrackKind kind, int id)
{
    return kind == TrackKind::Audio
               ? QStringLiteral("音轨 %1").arg(id)
               : QStringLiteral("字幕 %1").arg(id);
}

} // namespace

QString trackLabel(TrackKind kind, int id, QString title, QString language)
{
    title = title.trimmed();
    if (!title.isEmpty())
    {
        return title;
    }

    const QString readableLanguage = languageLabel(language);
    if (!readableLanguage.isEmpty())
    {
        return readableLanguage;
    }

    return fallbackLabel(kind, id);
}

QString disableLabel(TrackKind kind)
{
    return kind == TrackKind::Audio ? QStringLiteral("关闭音轨")
                                    : QStringLiteral("关闭字幕");
}

QString disabledToast(TrackKind kind)
{
    return kind == TrackKind::Audio ? QStringLiteral("已关闭音轨")
                                    : QStringLiteral("已关闭字幕");
}

QString selectedToast(TrackKind kind, const QString &label)
{
    return kind == TrackKind::Audio ? QStringLiteral("音轨：%1").arg(label)
                                    : QStringLiteral("字幕：%1").arg(label);
}

} // namespace PlayerTrackOptionTextUtils
