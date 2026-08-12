#include "playbackengineprofile.h"

namespace PlaybackEngineProfile
{

namespace
{
void addOption(Profile &profile, const QString &name, const QString &value)
{
    profile.options.append({name, value});
}
} // namespace

QString Profile::optionValue(const QString &name) const
{
    for (const Option &option : options)
    {
        if (option.name == name)
        {
            return option.value;
        }
    }
    return {};
}

Profile buildDefaultProfile(const BuildContext &context)
{
    Profile profile;
    profile.name = context.runningInRemoteDesktop
                       ? QStringLiteral("xplayer-remote-safe")
                       : QStringLiteral("xplayer-balanced");

    addOption(profile, QStringLiteral("osc"), QStringLiteral("no"));
    addOption(profile, QStringLiteral("load-scripts"), QStringLiteral("no"));
    addOption(profile, QStringLiteral("config"),
              context.useCustomMpvConfig ? QStringLiteral("yes") : QStringLiteral("no"));
    addOption(profile, QStringLiteral("vo"), QStringLiteral("libmpv"));
    addOption(profile, QStringLiteral("keep-open"), QStringLiteral("yes"));

    addOption(profile, QStringLiteral("blend-subtitles"), QStringLiteral("video"));
    addOption(profile, QStringLiteral("sub-gray"), QStringLiteral("yes"));
    addOption(profile, QStringLiteral("sub-blur"), QStringLiteral("0"));
    addOption(profile, QStringLiteral("sub-gauss"), QStringLiteral("0"));
    addOption(profile, QStringLiteral("sub-hinting"), QStringLiteral("none"));
    addOption(profile, QStringLiteral("sub-ass-hinting"), QStringLiteral("none"));

    const QString videoSync =
        context.videoSync.trimmed().isEmpty()
            ? QStringLiteral("display-resample")
            : context.videoSync.trimmed();
    addOption(profile, QStringLiteral("video-sync"), videoSync);
    addOption(profile, QStringLiteral("interpolation"),
              videoSync == QStringLiteral("display-resample")
                  ? QStringLiteral("yes")
                  : QStringLiteral("no"));
    addOption(profile, QStringLiteral("display-fps"),
              QString::number(context.targetFrameRate > 0 ? context.targetFrameRate : 60));

    addOption(profile, QStringLiteral("gpu-api"), QStringLiteral("opengl"));
    addOption(profile, QStringLiteral("opengl-es"), QStringLiteral("no"));

    QString hwdec = context.hardwareDecoder.trimmed().isEmpty()
                        ? QStringLiteral("auto-copy")
                        : context.hardwareDecoder.trimmed();
    if (context.runningInRemoteDesktop)
    {
        hwdec = QStringLiteral("no");
        addOption(profile, QStringLiteral("profile"), QStringLiteral("sw-fast"));
        addOption(profile, QStringLiteral("gpu-dumb-mode"), QStringLiteral("yes"));
        addOption(profile, QStringLiteral("opengl-pbo"), QStringLiteral("no"));
        addOption(profile, QStringLiteral("dither-depth"), QStringLiteral("no"));
    }
    addOption(profile, QStringLiteral("hwdec"), hwdec);

    addOption(profile, QStringLiteral("cache"), QStringLiteral("yes"));
    addOption(profile, QStringLiteral("cache-secs"), QStringLiteral("45"));
    addOption(profile, QStringLiteral("demuxer-readahead-secs"), QStringLiteral("25"));
    addOption(profile, QStringLiteral("demuxer-max-bytes"), QStringLiteral("128MiB"));
    addOption(profile, QStringLiteral("demuxer-max-back-bytes"), QStringLiteral("64MiB"));

    if (context.volumeNormalization)
    {
        addOption(profile, QStringLiteral("af"),
                  QStringLiteral("dynaudnorm=g=5:f=250:r=0.9:p=0.5"));
    }

    if (context.useCustomMpvConfig && !context.customMpvConfigDir.trimmed().isEmpty())
    {
        addOption(profile, QStringLiteral("config-dir"), context.customMpvConfigDir.trimmed());
    }

    return profile;
}

LoadPolicy buildLoadPolicy(const LoadContext &context)
{
    LoadPolicy policy;
    if (!context.httpProxy.isEmpty())
    {
        policy.options.insert(QStringLiteral("http-proxy"), context.httpProxy);
    }
    if (context.forceSeekable)
    {
        policy.options.insert(QStringLiteral("force-seekable"), QStringLiteral("yes"));
    }

    if (context.isHttpStream)
    {
        policy.options.insert(QStringLiteral("cache"), QStringLiteral("yes"));
        policy.options.insert(QStringLiteral("cache-secs"),
                              context.usesRelay ? QStringLiteral("30") : QStringLiteral("45"));
        policy.options.insert(QStringLiteral("demuxer-readahead-secs"),
                              context.usesRelay ? QStringLiteral("15") : QStringLiteral("25"));
        policy.options.insert(QStringLiteral("demuxer-max-bytes"),
                              context.usesRelay ? QStringLiteral("96MiB") : QStringLiteral("128MiB"));
        policy.options.insert(QStringLiteral("demuxer-max-back-bytes"), QStringLiteral("64MiB"));
    }

    return policy;
}

QString playerFacingFailureMessage(const QString &reason)
{
    if (reason == QStringLiteral("eof"))
    {
        return {};
    }

    return QStringLiteral("播放失败，已停止缓冲。可以返回后重试，或切换片源/转码。");
}

} // namespace PlaybackEngineProfile
