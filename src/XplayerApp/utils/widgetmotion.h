#pragma once

#include "uianimationdefaults.h"

#include <QByteArray>
#include <QByteArrayView>
#include <QPoint>
#include <QVariant>

#include <functional>

class QObject;
class QAbstractButton;
class QPropertyAnimation;
class QVariantAnimation;
class QWidget;

namespace XplayerUi
{
struct MotionSpec
{
    MotionDuration duration = MotionDuration::Standard;
    MotionCurve curve = MotionCurve::Move;
    int durationOverrideMs = -1;
};

int stopAnimationsFor(QObject* target, QByteArrayView propertyName = {});

QPropertyAnimation* animateMove(QWidget* widget,
                                const QPoint& targetPosition,
                                MotionSpec spec = {});

QPropertyAnimation* animateProperty(QObject* target,
                                    const QByteArray& propertyName,
                                    const QVariant& to,
                                    MotionSpec spec = {});

QPropertyAnimation* animateOpacity(QWidget* widget,
                                   qreal from,
                                   qreal to,
                                   MotionSpec spec = {MotionDuration::Quick,
                                                      MotionCurve::Enter});

QVariantAnimation* animateVariant(QObject* target,
                                  const QByteArray& propertyName,
                                  const QVariant& from,
                                  const QVariant& to,
                                  MotionSpec spec = {});

QVariantAnimation* animateValue(QObject* owner,
                                const QByteArray& animationKey,
                                const QVariant& from,
                                const QVariant& to,
                                MotionSpec spec,
                                QObject* context,
                                std::function<void(const QVariant&)> onValue);

QVariantAnimation* animateButtonIconSpin(
    QAbstractButton* button,
    MotionSpec spec = {MotionDuration::SpinnerCycle, MotionCurve::Spinner});
} // namespace XplayerUi
