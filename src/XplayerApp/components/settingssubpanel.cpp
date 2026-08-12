#include "settingssubpanel.h"
#include "../managers/thememanager.h"
#include <config/configstore.h>
#include <config/config_keys.h>
#include <QScrollArea>
#include <QScrollBar>
#include <QTimer>
#include "../utils/uianimationdefaults.h"

namespace {

QString toneForSettingsIcon(const QString &iconPath) {
    const QString path = iconPath.toLower();
    if (path.contains("appearance") || path.contains("theme") ||
        path.contains("search")) {
        return QStringLiteral("rose");
    }
    if (path.contains("library") || path.contains("folder") ||
        path.contains("home") || path.contains("completed")) {
        return QStringLiteral("green");
    }
    if (path.contains("player") || path.contains("video") ||
        path.contains("audio") || path.contains("subtitle") ||
        path.contains("danmaku")) {
        return QStringLiteral("purple");
    }
    if (path.contains("cache") || path.contains("download") ||
        path.contains("clear") || path.contains("log")) {
        return QStringLiteral("orange");
    }
    if (path.contains("delete") || path.contains("warning") ||
        path.contains("error")) {
        return QStringLiteral("red");
    }
    return QStringLiteral("blue");
}

} 

SettingsSubPanel::SettingsSubPanel(const QString &iconPath, QWidget *parent)
    : QFrame(parent), m_iconPath(iconPath) {
    setObjectName("SettingsSubPanel");

    
    setMaximumHeight(0);
    setMinimumHeight(0);
    setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Fixed);

    
    m_outerLayout = new QHBoxLayout(this);
    m_outerLayout->setContentsMargins(18, 10, 18, 10); 
    m_outerLayout->setSpacing(14);

    
    m_iconBox = new QWidget(this);
    m_iconBox->setObjectName("SettingsCardIconBox");
    m_iconBox->setProperty("tone", toneForSettingsIcon(m_iconPath));
    m_iconBox->setFixedSize(36, 36);
    auto *iconBoxLayout = new QHBoxLayout(m_iconBox);
    iconBoxLayout->setContentsMargins(0, 0, 0, 0);
    iconBoxLayout->setSpacing(0);

    m_iconLabel = new QLabel(m_iconBox);
    m_iconLabel->setFixedSize(20, 20);
    m_iconLabel->setAlignment(Qt::AlignCenter);
    m_iconLabel->setObjectName("SettingsCardIcon");
    updateIcon();
    iconBoxLayout->addWidget(m_iconLabel, 0, Qt::AlignCenter);

    m_outerLayout->addWidget(m_iconBox, 0, Qt::AlignVCenter);

    
    m_contentLayout = new QHBoxLayout();
    m_contentLayout->setContentsMargins(0, 0, 0, 0);
    m_contentLayout->setSpacing(12);
    m_contentLayout->setAlignment(Qt::AlignVCenter);
    m_outerLayout->addLayout(m_contentLayout, 1);

    
    m_animation = new QPropertyAnimation(this, "panelHeight", this);
    m_animation->setDuration(
        XplayerUi::durationMs(XplayerUi::MotionDuration::Panel));
    m_animation->setEasingCurve(
        XplayerUi::easingCurve(XplayerUi::MotionCurve::Resize));

    
    connect(m_animation, &QPropertyAnimation::finished, this, [this]() {
        if (!m_expanded) {
            hide();
        }
    });

    
    connect(ThemeManager::instance(), &ThemeManager::themeChanged,
            this, [this]() {
                updateStyleSheet();
                updateIcon();
            });
    updateStyleSheet();

    hide(); 
}

void SettingsSubPanel::updateIcon() {
    if (m_iconPath.isEmpty()) return;
    QIcon icon = ThemeManager::getAdaptiveIcon(m_iconPath);
    m_iconLabel->setPixmap(icon.pixmap(18, 18));
}

void SettingsSubPanel::updateStyleSheet() {
    bool isDark = ThemeManager::instance()->isDarkMode();

    
    QString accent = isDark ? "#3B82F6" : "#2F80ED";

    
    if (isDark) {
        setStyleSheet(QString(
            
            "#SettingsSubPanel {"
            "  background: rgba(255,255,255,0.025);"
            "  border: 1px solid rgba(255,255,255,0.06);"
            "  border-left: 3px solid %1;"
            "  border-radius: 12px;"
            "  margin-left: 20px;"
            "}"
            
            "#SettingsSubPanel ModernComboBox {"
            "  background-color: rgba(255,255,255,0.07);"
            "  border: 1px solid rgba(255,255,255,0.10);"
            "  border-radius: 8px;"
            "  color: #E2E8F0;"
            "  font-size: 13px; font-weight: 500;"
            "  font-family: 'Segoe UI','Microsoft YaHei',sans-serif;"
            "  padding: 5px 32px 5px 16px;"
            "  min-width: 120px;"
            "  selection-background-color: transparent;"
            "}"
            "#SettingsSubPanel ModernComboBox:hover {"
            "  background-color: rgba(255,255,255,0.10);"
            "  border-color: rgba(255,255,255,0.18);"
            "}"
            "#SettingsSubPanel ModernComboBox:focus {"
            "  border-color: #3B82F6;"
            "  background-color: rgba(59,130,246,0.08);"
            "  outline: none;"
            "}"
            "#SettingsSubPanel ModernComboBox::drop-down {"
            "  subcontrol-origin: padding; subcontrol-position: center right;"
            "  width: 32px; border: none; background: transparent;"
            "}"
            "#SettingsSubPanel ModernComboBox::down-arrow {"
            "  image: url(':/svg/dark/chevron-down.svg'); width: 14px; height: 14px;"
            "}"
            
            "#SettingsSubPanel ModernComboBox QAbstractItemView {"
            "  background-color: #1E293B;"
            "  border: 1px solid rgba(255,255,255,0.10);"
            "  border-radius: 8px; color: #E2E8F0; font-size: 13px;"
            "  outline: none; padding: 4px;"
            "  selection-background-color: rgba(59,130,246,0.20);"
            "  selection-color: #93C5FD;"
            "}"
            "#SettingsSubPanel ModernComboBox QAbstractItemView::item {"
            "  padding: 5px 12px; border-radius: 6px;"
            "}"
            "#SettingsSubPanel ModernComboBox QAbstractItemView::item:hover {"
            "  background-color: rgba(255,255,255,0.07); color: #F8FAFC;"
            "}"
            "#SettingsSubPanel ModernComboBox QAbstractItemView::item:selected {"
            "  background-color: rgba(59,130,246,0.20); color: #93C5FD;"
            "}"
            "#SettingsSubPanel ModernComboBox QAbstractItemView::item:selected:hover {"
            "  background-color: rgba(59,130,246,0.20); color: #93C5FD;"
            "}"
            
            "#SettingsSubPanel #SettingsCardDesc {"
            "  color: #64748B;"
            "  font-size: 12px;"
            "  font-family: 'Segoe UI','Microsoft YaHei',sans-serif;"
            "}"
            
            "#SettingsSubPanel QLineEdit {"
            "  background-color: rgba(255,255,255,0.07);"
            "  border: 1px solid rgba(255,255,255,0.10);"
            "  border-radius: 8px;"
            "  color: #E2E8F0;"
            "  font-size: 13px; font-weight: 500;"
            "  font-family: 'Segoe UI','Microsoft YaHei',sans-serif;"
            "  padding: 5px 12px;"
            "  max-width: 280px;"
            "}"
            "#SettingsSubPanel QLineEdit:hover {"
            "  border-color: rgba(255,255,255,0.18);"
            "}"
            "#SettingsSubPanel QLineEdit:focus {"
            "  border-color: #3B82F6;"
            "  background-color: rgba(59,130,246,0.08);"
            "}"
            "#SettingsSubPanel QLineEdit#DanmakuOptionSliderValue,"
            "#SettingsSubPanel QLineEdit#DanmakuOptionSliderValue:hover,"
            "#SettingsSubPanel QLineEdit#DanmakuOptionSliderValue:focus,"
            "#SettingsSubPanel QLineEdit#SubtitleOptionSliderValue,"
            "#SettingsSubPanel QLineEdit#SubtitleOptionSliderValue:hover,"
            "#SettingsSubPanel QLineEdit#SubtitleOptionSliderValue:focus {"
            "  background: transparent;"
            "  border: none;"
            "  border-radius: 0px;"
            "  color: #BFDBFE;"
            "  font-size: 13px;"
            "  font-weight: 700;"
            "  font-family: 'Segoe UI','Microsoft YaHei',sans-serif;"
            "  padding: 0px;"
            "  min-width: 0px;"
            "  max-width: 16777215px;"
            "  selection-background-color: rgba(96,165,250,0.22);"
            "  selection-color: #F8FAFC;"
            "}"
            "#SettingsSubPanel QFrame#TagInputContainer {"
            "  background-color: rgba(255,255,255,0.07);"
            "  border: 1px solid rgba(255,255,255,0.10);"
            "  border-radius: 8px;"
            "}"
            "#SettingsSubPanel QFrame#TagInputContainer:hover {"
            "  border-color: rgba(255,255,255,0.18);"
            "}"
            "#SettingsSubPanel QFrame#TagInputContainer:focus-within {"
            "  border-color: #3B82F6;"
            "  background-color: rgba(255,255,255,0.07);"
            "}"
            "#SettingsSubPanel QLineEdit#TagInputInlineEdit,"
            "#SettingsSubPanel QLineEdit#TagInputInlineEdit:hover,"
            "#SettingsSubPanel QLineEdit#TagInputInlineEdit:focus {"
            "  background: transparent;"
            "  border: none;"
            "  border-radius: 0px;"
            "  color: #E2E8F0;"
            "  font-size: 13px;"
            "  font-weight: 500;"
            "  font-family: 'Segoe UI','Microsoft YaHei',sans-serif;"
            "  padding: 0px 4px;"
            "  min-width: 0px;"
            "  max-width: 16777215px;"
            "  selection-background-color: rgba(59,130,246,0.25);"
            "  selection-color: #FFFFFF;"
            "}"
        ).arg(accent));
    } else {
        setStyleSheet(QString(
            
            "#SettingsSubPanel {"
            "  background: rgba(255,255,255,0.78);"
            "  border: 1px solid rgba(255,255,255,0.82);"
            "  border-left: 3px solid %1;"
            "  border-radius: 20px;"
            "  margin-left: 24px;"
            "  margin-right: 8px;"
            "}"
            
            "#SettingsSubPanel ModernComboBox {"
            "  background-color: #FFFFFF;"
            "  border: 1px solid #CBD5E1;"
            "  border-radius: 8px;"
            "  color: #1E293B;"
            "  font-size: 13px; font-weight: 500;"
            "  font-family: 'Segoe UI','Microsoft YaHei',sans-serif;"
            "  padding: 5px 32px 5px 16px;"
            "  min-width: 120px;"
            "  selection-background-color: transparent;"
            "}"
            "#SettingsSubPanel ModernComboBox:hover {"
            "  background-color: #F8FAFC; border-color: #94A3B8;"
            "}"
            "#SettingsSubPanel ModernComboBox:focus {"
            "  border-color: #3B82F6; background-color: #EFF6FF; outline: none;"
            "}"
            "#SettingsSubPanel ModernComboBox::drop-down {"
            "  subcontrol-origin: padding; subcontrol-position: center right;"
            "  width: 32px; border: none; background: transparent;"
            "}"
            "#SettingsSubPanel ModernComboBox::down-arrow {"
            "  image: url(':/svg/light/chevron-down.svg'); width: 14px; height: 14px;"
            "}"
            
            "#SettingsSubPanel ModernComboBox QAbstractItemView {"
            "  background-color: #FFFFFF;"
            "  border: 1px solid #E2E8F0;"
            "  border-radius: 8px; color: #1E293B; font-size: 13px;"
            "  outline: none; padding: 4px;"
            "  selection-background-color: rgba(59,130,246,0.10);"
            "  selection-color: #2563EB;"
            "}"
            "#SettingsSubPanel ModernComboBox QAbstractItemView::item {"
            "  padding: 5px 12px; border-radius: 6px;"
            "}"
            "#SettingsSubPanel ModernComboBox QAbstractItemView::item:hover {"
            "  background-color: #F1F5F9; color: #0F172A;"
            "}"
            "#SettingsSubPanel ModernComboBox QAbstractItemView::item:selected {"
            "  background-color: rgba(59,130,246,0.10); color: #2563EB;"
            "}"
            "#SettingsSubPanel ModernComboBox QAbstractItemView::item:selected:hover {"
            "  background-color: rgba(59,130,246,0.10); color: #2563EB;"
            "}"
            
            "#SettingsSubPanel #SettingsCardDesc {"
            "  color: #8A8A92;"
            "  font-size: 12px;"
            "  font-family: 'Segoe UI','Microsoft YaHei',sans-serif;"
            "}"
            
            "#SettingsSubPanel QLineEdit {"
            "  background-color: #FFFFFF;"
            "  border: 1px solid #CBD5E1;"
            "  border-radius: 8px;"
            "  color: #1E293B;"
            "  font-size: 13px; font-weight: 500;"
            "  font-family: 'Segoe UI','Microsoft YaHei',sans-serif;"
            "  padding: 5px 12px;"
            "  max-width: 280px;"
            "}"
            "#SettingsSubPanel QLineEdit:hover {"
            "  border-color: #94A3B8;"
            "}"
            "#SettingsSubPanel QLineEdit:focus {"
            "  border-color: #3B82F6; background-color: #EFF6FF;"
            "}"
            "#SettingsSubPanel QLineEdit#DanmakuOptionSliderValue,"
            "#SettingsSubPanel QLineEdit#DanmakuOptionSliderValue:hover,"
            "#SettingsSubPanel QLineEdit#DanmakuOptionSliderValue:focus,"
            "#SettingsSubPanel QLineEdit#SubtitleOptionSliderValue,"
            "#SettingsSubPanel QLineEdit#SubtitleOptionSliderValue:hover,"
            "#SettingsSubPanel QLineEdit#SubtitleOptionSliderValue:focus {"
            "  background: transparent;"
            "  border: none;"
            "  border-radius: 0px;"
            "  color: #2563EB;"
            "  font-size: 13px;"
            "  font-weight: 700;"
            "  font-family: 'Segoe UI','Microsoft YaHei',sans-serif;"
            "  padding: 0px;"
            "  min-width: 0px;"
            "  max-width: 16777215px;"
            "  selection-background-color: rgba(59,130,246,0.16);"
            "  selection-color: #0F172A;"
            "}"
            "#SettingsSubPanel QFrame#TagInputContainer {"
            "  background-color: #FFFFFF;"
            "  border: 1px solid #CBD5E1;"
            "  border-radius: 8px;"
            "}"
            "#SettingsSubPanel QFrame#TagInputContainer:hover {"
            "  border-color: #94A3B8;"
            "}"
            "#SettingsSubPanel QFrame#TagInputContainer:focus-within {"
            "  border-color: #3B82F6;"
            "  background-color: #FFFFFF;"
            "}"
            "#SettingsSubPanel QLineEdit#TagInputInlineEdit,"
            "#SettingsSubPanel QLineEdit#TagInputInlineEdit:hover,"
            "#SettingsSubPanel QLineEdit#TagInputInlineEdit:focus {"
            "  background: transparent;"
            "  border: none;"
            "  border-radius: 0px;"
            "  color: #1E293B;"
            "  font-size: 13px;"
            "  font-weight: 500;"
            "  font-family: 'Segoe UI','Microsoft YaHei',sans-serif;"
            "  padding: 0px 4px;"
            "  min-width: 0px;"
            "  max-width: 16777215px;"
            "  selection-background-color: rgba(59,130,246,0.25);"
            "  selection-color: #1E293B;"
            "}"
        ).arg(accent));
    }
}

void SettingsSubPanel::expand() {
    if (m_expanded) return;
    m_expanded = true;
    show();

    QObject::disconnect(m_scrollToSelfConnection);
    m_scrollToSelfConnection = {};

    auto scrollToSelf = [this]() {
        QWidget *p = parentWidget();
        while (p) {
            if (auto *scrollArea = qobject_cast<QScrollArea *>(p)) {
                scrollArea->ensureWidgetVisible(this, 0, 60);
                break;
            }
            p = p->parentWidget();
        }
    };

    int targetH = contentHeight();
    if (targetH == panelHeight()) {
        QTimer::singleShot(0, this, scrollToSelf);
        emit expandedChanged(true);
        return;
    }
    bool reduceAnimations = ConfigStore::instance()->get<bool>(ConfigKeys::UiAnimations, false);
    if (reduceAnimations) {
        setPanelHeight(targetH);
        
        QTimer::singleShot(0, this, scrollToSelf);
    } else {
        m_animation->stop();
        m_animation->setStartValue(panelHeight());
        m_animation->setEndValue(targetH);
        m_animation->start();
        
        m_scrollToSelfConnection =
            connect(m_animation, &QPropertyAnimation::finished,
                    this, scrollToSelf, Qt::SingleShotConnection);
    }
    emit expandedChanged(true);
}

void SettingsSubPanel::collapse() {
    if (!m_expanded) return;
    m_expanded = false;
    QObject::disconnect(m_scrollToSelfConnection);
    m_scrollToSelfConnection = {};

    bool reduceAnimations = ConfigStore::instance()->get<bool>(ConfigKeys::UiAnimations, false);
    if (panelHeight() == 0) {
        hide();
        emit expandedChanged(false);
        return;
    }
    if (reduceAnimations) {
        setPanelHeight(0);
        hide();
    } else {
        m_animation->stop();
        m_animation->setStartValue(panelHeight());
        m_animation->setEndValue(0);
        m_animation->start();
    }
    emit expandedChanged(false);
}

void SettingsSubPanel::setExpanded(bool expanded) {
    if (expanded) expand();
    else collapse();
}

void SettingsSubPanel::setExpandedImmediately(bool expanded) {
    m_animation->stop();
    QObject::disconnect(m_scrollToSelfConnection);
    m_scrollToSelfConnection = {};
    m_expanded = expanded;
    if (expanded) {
        setPanelHeight(contentHeight());
        show();
    } else {
        setPanelHeight(0);
        hide();
    }
    emit expandedChanged(expanded);
}

void SettingsSubPanel::initExpanded() {
    
    
    setExpandedImmediately(true);
}

int SettingsSubPanel::panelHeight() const {
    return maximumHeight();
}

void SettingsSubPanel::setPanelHeight(int h) {
    setMaximumHeight(h);
    setMinimumHeight(h);
}

int SettingsSubPanel::contentHeight() const {
    if (!m_outerLayout) {
        return 48;
    }

    m_outerLayout->invalidate();
    return qMax(58, m_outerLayout->sizeHint().height());
}
