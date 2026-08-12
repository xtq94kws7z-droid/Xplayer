#include "settingspagebase.h"
#include "../../utils/xplayerresponsiveutils.h"
#include <QLabel>
#include <QResizeEvent>

SettingsPageBase::SettingsPageBase(XplayerCore* core, const QString& pageTitle, QWidget* parent)
    : QWidget(parent), m_core(core)
{
    
    m_mainLayout = new QVBoxLayout(this);
    applyResponsiveLayout();

    
    if (!pageTitle.isEmpty()) {
        auto* titleLabel = new QLabel(pageTitle, this);
        titleLabel->setObjectName("SettingsPageTitle");
        m_mainLayout->addWidget(titleLabel);
        m_mainLayout->addSpacing(18); 
    }
}

void SettingsPageBase::resizeEvent(QResizeEvent *event)
{
    QWidget::resizeEvent(event);
    applyResponsiveLayout();
}

void SettingsPageBase::applyResponsiveLayout()
{
    if (!m_mainLayout) {
        return;
    }
    const qreal scale = XplayerResponsiveUtils::scaleForViewport(size());
    m_mainLayout->setContentsMargins(
        XplayerResponsiveUtils::scaled(48, scale, 32, 70),
        XplayerResponsiveUtils::scaled(42, scale, 30, 62),
        XplayerResponsiveUtils::scaled(64, scale, 40, 92),
        XplayerResponsiveUtils::scaled(48, scale, 34, 70));
    m_mainLayout->setSpacing(XplayerResponsiveUtils::scaled(12, scale, 8, 18));
}
