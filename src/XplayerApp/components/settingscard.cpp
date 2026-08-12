#include "settingscard.h"
#include "elidedlabel.h"
#include "modernswitch.h"
#include "moderntaginput.h"
#include "../managers/thememanager.h"
#include <QComboBox>
#include <QHBoxLayout>
#include <QLineEdit>
#include <QSlider>
#include <QPushButton>
#include <QSpinBox>
#include <QString>

#include "../../XplayerCore/config/configstore.h"

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

SettingsCard::SettingsCard(const QString &iconSvgPath, const QString &title,
                           const QString &description, QWidget *controlWidget,
                           const QString &configKey, QWidget *parent,
                           const QVariant &defaultValue)
    : QFrame(parent), m_controlWidget(controlWidget), m_configKey(configKey),
      m_iconSvgPath(iconSvgPath), m_defaultValue(defaultValue) {
  
  setObjectName("SettingsCard");

  
  if (m_controlWidget) {
    m_controlWidget->setParent(this);
  }

  setupUi(title, description);

  
  if (!m_configKey.isEmpty() && m_controlWidget) {
    setupDataBinding();
  }

  
  connect(ThemeManager::instance(), &ThemeManager::themeChanged, this,
          &SettingsCard::onThemeChanged);
}

SettingsCard::~SettingsCard() = default;

void SettingsCard::setupUi(const QString &title, const QString &description) {
  
  m_mainLayout = new QHBoxLayout(this);
  m_mainLayout->setContentsMargins(18, 14, 18, 14);
  m_mainLayout->setSpacing(16);
  setMinimumHeight(68);

  
  m_iconBox = new QWidget(this);
  m_iconBox->setObjectName("SettingsCardIconBox");
  m_iconBox->setProperty("tone", toneForSettingsIcon(m_iconSvgPath));
  m_iconBox->setFixedSize(40, 40);
  auto *iconBoxLayout = new QHBoxLayout(m_iconBox);
  iconBoxLayout->setContentsMargins(0, 0, 0, 0);
  iconBoxLayout->setSpacing(0);

  m_iconLabel = new QLabel(this);
  m_iconLabel->setFixedSize(22, 22);
  m_iconLabel->setAlignment(Qt::AlignCenter);
  m_iconLabel->setObjectName("SettingsCardIcon");
  updateIcon(); 
  iconBoxLayout->addWidget(m_iconLabel, 0, Qt::AlignCenter);

  
  auto *textLayout = new QVBoxLayout();
  textLayout->setContentsMargins(0, 0, 0, 0);
  textLayout->setSpacing(2);
  textLayout->setAlignment(Qt::AlignVCenter);

  m_titleLabel = new QLabel(title, this);
  m_titleLabel->setObjectName("SettingsCardTitle");

  m_descLabel = new ElidedLabel(this);
  m_descLabel->setObjectName("SettingsCardDesc");
  m_descLabel->setFullText(description);

  textLayout->addWidget(m_titleLabel);
  if (!description.isEmpty()) {
    textLayout->addWidget(m_descLabel);
  } else {
    m_descLabel->hide();
  }

  
  m_mainLayout->addWidget(m_iconBox, 0, Qt::AlignVCenter);
  m_mainLayout->addLayout(textLayout, 1);

  
  if (m_controlWidget) {
    if (auto *combo = qobject_cast<QComboBox *>(m_controlWidget)) {
      combo->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed);
    } else if (auto *btn = qobject_cast<QPushButton *>(m_controlWidget)) {
      btn->setObjectName("SettingsCardButton");
      btn->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed);
    } else if (auto *lineEdit = qobject_cast<QLineEdit *>(m_controlWidget)) {
      lineEdit->setMinimumWidth(200);
      lineEdit->setFixedHeight(32);
      lineEdit->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
    } else if (auto *spinBox = qobject_cast<QSpinBox *>(m_controlWidget)) {
      spinBox->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed);
    } else if (qobject_cast<ModernTagInput *>(m_controlWidget)) {
      
      m_controlWidget->setMinimumWidth(220);
      m_controlWidget->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
    }
    m_mainLayout->addWidget(m_controlWidget, 0, Qt::AlignVCenter);
  }
}


void SettingsCard::updateIcon() {
  if (m_iconSvgPath.isEmpty())
    return;
  QIcon icon = ThemeManager::getAdaptiveIcon(m_iconSvgPath);
  m_iconLabel->setPixmap(icon.pixmap(20, 20));
}

void SettingsCard::onThemeChanged() { updateIcon(); }

void SettingsCard::setupDataBinding() {
  auto *store = ConfigStore::instance();

  
  
  
  if (auto *switchControl = qobject_cast<ModernSwitch *>(m_controlWidget)) {
    
    switchControl->setChecked(store->get<bool>(m_configKey, m_defaultValue.toBool()));

    
    connect(switchControl, &ModernSwitch::toggled, this,
            [this, store](bool checked) { store->set(m_configKey, checked); });
  }
  
  
  
  else if (auto *comboControl = qobject_cast<QComboBox *>(m_controlWidget)) {
    
    QString currentVal = store->get<QString>(m_configKey);
    if (currentVal.isEmpty() && m_defaultValue.isValid()) {
      currentVal = m_defaultValue.toString();
    }
    int idx = comboControl->findData(currentVal);
    if (idx >= 0) {
      comboControl->setCurrentIndex(idx);
    }

    
    connect(comboControl, QOverload<int>::of(&QComboBox::currentIndexChanged),
            this, [this, store, comboControl](int index) {
              store->set(m_configKey, comboControl->itemData(index));
            });
  }
  
  
  
  else if (auto *lineEdit = qobject_cast<QLineEdit *>(m_controlWidget)) {
    
    lineEdit->setText(store->get<QString>(m_configKey, m_defaultValue.toString()));

    
    connect(lineEdit, &QLineEdit::editingFinished, this,
            [this, store, lineEdit]() { store->set(m_configKey, lineEdit->text()); });
  }
  
  
  
  else if (auto *spinBox = qobject_cast<QSpinBox *>(m_controlWidget)) {
    spinBox->setValue(store->get<int>(m_configKey, m_defaultValue.toInt()));

    connect(spinBox, QOverload<int>::of(&QSpinBox::valueChanged), this,
            [this, store](int value) { store->set(m_configKey, value); });
  }
  
  
  
  else if (auto *tagInput = qobject_cast<ModernTagInput *>(m_controlWidget)) {
    
    tagInput->setValue(store->get<QString>(m_configKey, m_defaultValue.toString()));

    
    connect(tagInput, &ModernTagInput::valueChanged, this,
            [this, store](const QString &val) { store->set(m_configKey, val); });
  }

  
  
  
  connect(store, &ConfigStore::valueChanged, this,
          &SettingsCard::onConfigValueChanged);
}

void SettingsCard::onConfigValueChanged(const QString &key,
                                        const QVariant &newValue) {
  if (key != m_configKey || !m_controlWidget)
    return;

  
  m_controlWidget->blockSignals(true);

  if (auto *switchControl = qobject_cast<ModernSwitch *>(m_controlWidget)) {
    switchControl->setChecked(newValue.toBool());
  } else if (auto *comboControl = qobject_cast<QComboBox *>(m_controlWidget)) {
    int idx = comboControl->findData(newValue);
    if (idx >= 0) {
      comboControl->setCurrentIndex(idx);
    }
  } else if (auto *lineEdit = qobject_cast<QLineEdit *>(m_controlWidget)) {
    lineEdit->setText(newValue.toString());
  } else if (auto *spinBox = qobject_cast<QSpinBox *>(m_controlWidget)) {
    spinBox->setValue(newValue.toInt());
  } else if (auto *tagInput = qobject_cast<ModernTagInput *>(m_controlWidget)) {
    tagInput->setValue(newValue.toString());
  }

  m_controlWidget->blockSignals(false);
}
