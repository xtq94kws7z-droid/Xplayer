#include "widgetmotion.h"

#include "widgetgeometryutils.h"

#include <QAbstractAnimation>
#include <QAbstractButton>
#include <QGraphicsEffect>
#include <QGraphicsOpacityEffect>
#include <QIcon>
#include <QPointer>
#include <QPropertyAnimation>
#include <QPixmap>
#include <QStyle>
#include <QTransform>
#include <QVariantAnimation>
#include <QWidget>

#include <utility>

namespace
{
constexpr const char* kMotionPropertyName = "_xplayerMotionProperty";
constexpr const char* kButtonSpinRunningProperty = "xplayerIconSpinRunning";
constexpr const char* kButtonSpinAngleProperty = "xplayerIconSpinAngle";
constexpr const char* kButtonIconSpinKey = "buttonIconSpin";

QByteArray propertyTag(QByteArrayView propertyName)
{
    return QByteArray(propertyName.data(), propertyName.size());
}

void tagAnimation(QAbstractAnimation* animation, QByteArrayView propertyName)
{
    if (animation)
        animation->setProperty(kMotionPropertyName, propertyTag(propertyName));
}

bool animationMatches(QAbstractAnimation* animation, QByteArrayView propertyName)
{
    if (!animation)
        return false;

    if (propertyName.isEmpty())
        return true;

    if (auto* propertyAnimation = qobject_cast<QPropertyAnimation*>(animation))
    {
        if (QByteArrayView(propertyAnimation->propertyName()) == propertyName)
            return true;
    }

    const QByteArray tag = animation->property(kMotionPropertyName).toByteArray();
    return QByteArrayView(tag) == propertyName;
}

int resolvedDurationMs(const XplayerUi::MotionSpec& spec)
{
    return spec.durationOverrideMs >= 0
               ? spec.durationOverrideMs
               : XplayerUi::durationMs(spec.duration);
}
} // namespace

namespace XplayerUi
{
int stopAnimationsFor(QObject* target, QByteArrayView propertyName)
{
    if (!target)
        return 0;

    int stopped = 0;
    const auto animations = target->findChildren<QAbstractAnimation*>(
        QString(), Qt::FindDirectChildrenOnly);
    for (auto* animation : animations)
    {
        if (!animationMatches(animation, propertyName))
            continue;

        if (animation->state() == QAbstractAnimation::Running)
        {
            animation->stop();
            ++stopped;
        }
    }

    return stopped;
}

QPropertyAnimation* animateMove(QWidget* widget,
                                const QPoint& targetPosition,
                                MotionSpec spec)
{
    if (!widget || widget->pos() == targetPosition)
        return nullptr;

    stopAnimationsFor(widget, "pos");

    const int duration = resolvedDurationMs(spec);
    if (duration <= 0)
    {
        moveIfChanged(widget, targetPosition);
        return nullptr;
    }

    auto* animation = new QPropertyAnimation(widget, "pos", widget);
    animation->setStartValue(widget->pos());
    animation->setEndValue(targetPosition);
    animation->setDuration(duration);
    animation->setEasingCurve(easingCurve(spec.curve));
    tagAnimation(animation, "pos");
    animation->start(QAbstractAnimation::DeleteWhenStopped);
    return animation;
}

QPropertyAnimation* animateProperty(QObject* target,
                                    const QByteArray& propertyName,
                                    const QVariant& to,
                                    MotionSpec spec)
{
    if (!target || propertyName.isEmpty())
        return nullptr;

    const QVariant from = target->property(propertyName.constData());
    if (!from.isValid())
        return nullptr;

    if (from == to)
    {
        return nullptr;
    }

    stopAnimationsFor(target, QByteArrayView(propertyName));

    const int duration = resolvedDurationMs(spec);
    if (duration <= 0)
    {
        target->setProperty(propertyName.constData(), to);
        return nullptr;
    }

    auto* animation = new QPropertyAnimation(target, propertyName, target);
    animation->setStartValue(from);
    animation->setEndValue(to);
    animation->setDuration(duration);
    animation->setEasingCurve(easingCurve(spec.curve));
    tagAnimation(animation, QByteArrayView(propertyName));
    animation->start(QAbstractAnimation::DeleteWhenStopped);
    return animation;
}

QPropertyAnimation* animateOpacity(QWidget* widget,
                                   qreal from,
                                   qreal to,
                                   MotionSpec spec)
{
    if (!widget)
        return nullptr;

    auto* opacityEffect = qobject_cast<QGraphicsOpacityEffect*>(widget->graphicsEffect());
    if (!opacityEffect && widget->graphicsEffect())
        return nullptr;

    if (!opacityEffect)
    {
        opacityEffect = new QGraphicsOpacityEffect(widget);
        widget->setGraphicsEffect(opacityEffect);
    }

    if (qFuzzyCompare(opacityEffect->opacity(), to) && qFuzzyCompare(from, to))
        return nullptr;

    stopAnimationsFor(opacityEffect, "opacity");

    const int duration = resolvedDurationMs(spec);
    if (duration <= 0)
    {
        opacityEffect->setOpacity(to);
        return nullptr;
    }

    auto* animation = new QPropertyAnimation(opacityEffect, "opacity", opacityEffect);
    animation->setStartValue(from);
    animation->setEndValue(to);
    animation->setDuration(duration);
    animation->setEasingCurve(easingCurve(spec.curve));
    tagAnimation(animation, "opacity");
    animation->start(QAbstractAnimation::DeleteWhenStopped);
    return animation;
}

QVariantAnimation* animateVariant(QObject* target,
                                  const QByteArray& propertyName,
                                  const QVariant& from,
                                  const QVariant& to,
                                  MotionSpec spec)
{
    if (!target || propertyName.isEmpty())
        return nullptr;

    if (from == to && target->property(propertyName.constData()) == to)
        return nullptr;

    stopAnimationsFor(target, QByteArrayView(propertyName));

    const int duration = resolvedDurationMs(spec);
    if (duration <= 0)
    {
        target->setProperty(propertyName.constData(), to);
        return nullptr;
    }

    auto* animation = new QVariantAnimation(target);
    animation->setStartValue(from);
    animation->setEndValue(to);
    animation->setDuration(duration);
    animation->setEasingCurve(easingCurve(spec.curve));
    tagAnimation(animation, QByteArrayView(propertyName));

    QPointer<QObject> safeTarget(target);
    QObject::connect(animation,
                     &QVariantAnimation::valueChanged,
                     target,
                     [safeTarget, propertyName](const QVariant& value) {
                         if (safeTarget)
                             safeTarget->setProperty(propertyName.constData(), value);
                     });

    animation->start(QAbstractAnimation::DeleteWhenStopped);
    return animation;
}

QVariantAnimation* animateValue(QObject* owner,
                                const QByteArray& animationKey,
                                const QVariant& from,
                                const QVariant& to,
                                MotionSpec spec,
                                QObject* context,
                                std::function<void(const QVariant&)> onValue)
{
    if (!owner || !context || animationKey.isEmpty() || !onValue)
        return nullptr;

    if (from == to)
    {
        onValue(to);
        return nullptr;
    }

    stopAnimationsFor(owner, QByteArrayView(animationKey));

    const int duration = resolvedDurationMs(spec);
    if (duration <= 0)
    {
        onValue(to);
        return nullptr;
    }

    auto* animation = new QVariantAnimation(owner);
    animation->setStartValue(from);
    animation->setEndValue(to);
    animation->setDuration(duration);
    animation->setEasingCurve(easingCurve(spec.curve));
    tagAnimation(animation, QByteArrayView(animationKey));

    onValue(from);
    QObject::connect(animation,
                     &QVariantAnimation::valueChanged,
                     context,
                     [callback = std::move(onValue)](const QVariant& value) {
                         callback(value);
                     });

    animation->start(QAbstractAnimation::DeleteWhenStopped);
    return animation;
}

QVariantAnimation* animateButtonIconSpin(QAbstractButton* button, MotionSpec spec)
{
    if (!button || button->property(kButtonSpinRunningProperty).toBool())
        return nullptr;

    const QIcon originalIcon = button->icon();
    const QSize iconSize = button->iconSize();
    if (iconSize.isEmpty())
        return nullptr;

    const QPixmap basePixmap = originalIcon.pixmap(iconSize);
    if (basePixmap.isNull())
        return nullptr;

    button->setProperty(kButtonSpinRunningProperty, true);
    button->setProperty(kButtonSpinAngleProperty, -1.0);
    if (button->style())
    {
        button->style()->unpolish(button);
        button->style()->polish(button);
    }

    QPointer<QAbstractButton> safeButton(button);
    auto* animation = animateValue(
        button,
        kButtonIconSpinKey,
        0.0,
        360.0,
        spec,
        button,
        [safeButton, basePixmap, iconSize](const QVariant& value) {
            if (!safeButton)
                return;

            const qreal angle = value.toReal();
            const QVariant previousAngle =
                safeButton->property(kButtonSpinAngleProperty);
            if (previousAngle.isValid() &&
                qAbs(previousAngle.toReal() - angle) < 0.5)
                return;

            QTransform transform;
            transform.translate(iconSize.width() / 2.0, iconSize.height() / 2.0);
            transform.rotate(angle);
            transform.translate(-iconSize.width() / 2.0, -iconSize.height() / 2.0);

            const QPixmap rotated =
                basePixmap.transformed(transform, Qt::SmoothTransformation);
            const int xOffset = (rotated.width() - iconSize.width()) / 2;
            const int yOffset = (rotated.height() - iconSize.height()) / 2;
            safeButton->setIcon(QIcon(rotated.copy(xOffset,
                                                   yOffset,
                                                   iconSize.width(),
                                                   iconSize.height())));
            safeButton->setProperty(kButtonSpinAngleProperty, angle);
        });

    if (!animation)
    {
        button->setIcon(originalIcon);
        button->setProperty(kButtonSpinRunningProperty, false);
        button->setProperty(kButtonSpinAngleProperty, QVariant());
        return nullptr;
    }

    QObject::connect(animation,
                     &QVariantAnimation::finished,
                     button,
                     [safeButton, originalIcon]() {
                         if (!safeButton)
                             return;

                         safeButton->setIcon(originalIcon);
                         safeButton->setProperty(kButtonSpinRunningProperty, false);
                         safeButton->setProperty(kButtonSpinAngleProperty, QVariant());
                         if (safeButton->style())
                         {
                             safeButton->style()->unpolish(safeButton);
                             safeButton->style()->polish(safeButton);
                         }
                     });

    return animation;
}
} // namespace XplayerUi
