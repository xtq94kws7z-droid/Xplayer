#pragma once

#include <QList>
#include <QPair>
#include <QString>
#include <QVariantMap>

namespace PlaybackEngineProfile
{

struct Option
{
    QString name;
    QString value;
};

struct Profile
{
    QString name;
    QList<Option> options;

    QString optionValue(const QString &name) const;
};

struct BuildContext
{
    QString videoSync = QStringLiteral("display-resample");
    QString hardwareDecoder = QStringLiteral("auto-copy");
    int targetFrameRate = 60;
    bool runningInRemoteDesktop = false;
    bool volumeNormalization = false;
    bool useCustomMpvConfig = false;
    QString customMpvConfigDir;
};

struct LoadContext
{
    bool isHttpStream = false;
    bool usesRelay = false;
    QString httpProxy;
    bool forceSeekable = false;
};

struct LoadPolicy
{
    QVariantMap options;
};

Profile buildDefaultProfile(const BuildContext &context);
LoadPolicy buildLoadPolicy(const LoadContext &context);
QString playerFacingFailureMessage(const QString &reason);

} // namespace PlaybackEngineProfile
