#include "settingsview.h"
#include "../../components/slidingstackedwidget.h"
#include "../../managers/thememanager.h" 
#include "pageabout.h"
#include "pageappearance.h"
#include "pagegeneral.h"
#include "pageplayer.h"
#include "pagelibrary.h"
#include <QApplication>
#include <QEvent>
#include <QHBoxLayout>
#include <QScrollArea>
#include <QScrollBar>
#include <QVBoxLayout>
#include <QWheelEvent>
#include "../../utils/smoothscrollcontroller.h"
#include "../../utils/uianimationdefaults.h"
#include "../../utils/xplayerresponsiveutils.h"


SettingsView::SettingsView(XplayerCore *core, QWidget *parent)
    : BaseView(core, parent) {
  
  setAttribute(Qt::WA_StyledBackground, true);
  setObjectName("SettingsRootView");

  setupUi();
  setupConnections();

  
  
  m_navMenu->setCurrentRow(0);
}

void SettingsView::setupUi() {
  auto *mainLayout = new QHBoxLayout(this);
  
  mainLayout->setContentsMargins(0, 0, 0, 0);
  mainLayout->setSpacing(0);

  
  m_leftPanel = new QWidget(this);
  m_leftPanel->setObjectName("SettingsLeftPanel");
  m_leftLayout = new QVBoxLayout(m_leftPanel);
  

  
  m_titleLabel = new QLabel(tr("Settings"), m_leftPanel);
  m_titleLabel->setObjectName("SettingsMainTitle");

  
  m_navMenu = new QListWidget(m_leftPanel);
  m_navMenu->setObjectName("SettingsNavMenu");
  m_navMenu->setFocusPolicy(Qt::NoFocus);
  
  m_navMenu->setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
  m_navMenu->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);

  
  auto *itemGeneral = new QListWidgetItem(
      ThemeManager::getAdaptiveIcon(":/svg/dark/general.svg"), tr(" General"));
  itemGeneral->setData(Qt::UserRole, ":/svg/dark/general.svg");

  auto *itemAppearance = new QListWidgetItem(
      ThemeManager::getAdaptiveIcon(":/svg/dark/appearance.svg"),
      tr(" Appearance"));
  itemAppearance->setData(Qt::UserRole, ":/svg/dark/appearance.svg");

  auto *itemPlayer = new QListWidgetItem(
      ThemeManager::getAdaptiveIcon(":/svg/dark/player.svg"), tr(" Player"));
  itemPlayer->setData(Qt::UserRole, ":/svg/dark/player.svg");

  auto *itemLibrary = new QListWidgetItem(
      ThemeManager::getAdaptiveIcon(":/svg/dark/library.svg"), tr(" Library"));
  itemLibrary->setData(Qt::UserRole, ":/svg/dark/library.svg");

  auto *itemAbout = new QListWidgetItem(
      ThemeManager::getAdaptiveIcon(":/svg/dark/about.svg"), tr(" About"));
  itemAbout->setData(Qt::UserRole, ":/svg/dark/about.svg");

  
  itemGeneral->setSizeHint(QSize(232, 52));
  itemAppearance->setSizeHint(QSize(232, 52));
  itemPlayer->setSizeHint(QSize(232, 52));
  itemLibrary->setSizeHint(QSize(232, 52));
  itemAbout->setSizeHint(QSize(232, 52));

  m_navMenu->addItem(itemGeneral);
  m_navMenu->addItem(itemAppearance);
  m_navMenu->addItem(itemLibrary);
  m_navMenu->addItem(itemPlayer);
  m_navMenu->addItem(itemAbout);

  m_leftLayout->addWidget(m_titleLabel);
  m_leftLayout->addWidget(m_navMenu);

  
  m_stack = new SlidingStackedWidget(this);
  m_stack->setObjectName("SettingsStack");

  
  
  
  
  const int kPageCount = 5;
  m_scrollAreas.reserve(kPageCount);
  m_scrollControllers.reserve(kPageCount);
  m_pages.reserve(kPageCount);
  for (int i = 0; i < kPageCount; ++i) {
    auto *placeholder = new QWidget(m_stack);
    placeholder->setAttribute(Qt::WA_StyledBackground, true);
    placeholder->setObjectName("SettingsPagePlaceholder");
    m_stack->addWidget(placeholder);

    m_scrollAreas.append(nullptr);
    m_scrollControllers.append(nullptr);
    m_pages.append(QPointer<QWidget>());
  }

  mainLayout->addWidget(m_leftPanel);
  mainLayout->addWidget(m_stack, 1);

  
  qApp->postEvent(this, new QEvent(QEvent::StyleChange));
  applyResponsiveLayout();
}

void SettingsView::resizeEvent(QResizeEvent *event)
{
  BaseView::resizeEvent(event);
  applyResponsiveLayout();
}

void SettingsView::applyResponsiveLayout()
{
  const qreal scale = XplayerResponsiveUtils::scaleForViewport(size());
  if (m_leftPanel) {
    m_leftPanel->setFixedWidth(
        XplayerResponsiveUtils::scaled(292, scale, 236, 424));
  }
  if (m_leftLayout) {
    m_leftLayout->setContentsMargins(
        XplayerResponsiveUtils::scaled(32, scale, 24, 46),
        XplayerResponsiveUtils::scaled(38, scale, 28, 56),
        XplayerResponsiveUtils::scaled(28, scale, 22, 42),
        XplayerResponsiveUtils::scaled(34, scale, 26, 50));
    m_leftLayout->setSpacing(XplayerResponsiveUtils::scaled(28, scale, 20, 42));
  }
  if (m_navMenu) {
    const QSize itemSize(
        XplayerResponsiveUtils::scaled(232, scale, 184, 336),
        XplayerResponsiveUtils::scaled(52, scale, 44, 76));
    m_navMenu->setIconSize(QSize(
        XplayerResponsiveUtils::scaled(20, scale, 16, 28),
        XplayerResponsiveUtils::scaled(20, scale, 16, 28)));
    for (int i = 0; i < m_navMenu->count(); ++i) {
      if (auto *item = m_navMenu->item(i)) {
        item->setSizeHint(itemSize);
      }
    }
  }
}

QScrollArea *SettingsView::wrapInScrollArea(QWidget *page, int row) {
  
  page->setAttribute(Qt::WA_StyledBackground, true);

  auto *scroll = new QScrollArea(m_stack);
  scroll->setObjectName("SettingsScrollArea");
  scroll->setWidget(page);
  scroll->setWidgetResizable(true);
  scroll->setFrameShape(QFrame::NoFrame);

  
  scroll->viewport()->setAutoFillBackground(false);

  
  scroll->viewport()->installEventFilter(this);

  
  if (row >= 0 && row < m_scrollAreas.size()) {
    m_scrollAreas[row] = scroll;
    auto *controller =
        new SmoothScrollController(scroll->verticalScrollBar(), this);
    controller->setDuration(XplayerUi::kScrollAnimationMs);
    m_scrollControllers[row] = controller;
  }

  return scroll;
}

void SettingsView::ensurePageAt(int row) {
  if (row < 0 || row >= m_pages.size()) {
    return;
  }
  if (m_pages[row]) {
    
    return;
  }

  qDebug() << "[SettingsView] Lazy instantiate page row=" << row;

  QWidget *page = nullptr;
  switch (row) {
  case 0:
    page = new PageGeneral(m_core, m_stack);
    break;
  case 1:
    page = new PageAppearance(m_core, m_stack);
    break;
  case 2:
    page = new PageLibrary(m_core, m_stack);
    break;
  case 3:
    page = new PagePlayer(m_core, m_stack);
    break;
  case 4:
    page = new PageAbout(m_core, m_stack);
    break;
  default:
    return;
  }

  QScrollArea *scroll = wrapInScrollArea(page, row);

  
  
  QWidget *placeholder = m_stack->widget(row);
  const bool wasBlocked = m_stack->blockSignals(true);
  m_stack->insertWidget(row, scroll); 
  m_stack->removeWidget(placeholder);
  m_stack->blockSignals(wasBlocked);
  placeholder->deleteLater();

  m_pages[row] = scroll;
}

void SettingsView::setupConnections() {
  
  connect(m_navMenu, &QListWidget::currentRowChanged, this, [this](int row) {
    if (row >= 0) {
      ensurePageAt(row);
      m_stack->setCurrentIndex(row);
    }
  });

  
  connect(ThemeManager::instance(), &ThemeManager::themeChanged, this,
          &SettingsView::onThemeChanged);
}


void SettingsView::onThemeChanged() {
  for (int i = 0; i < m_navMenu->count(); ++i) {
    auto *item = m_navMenu->item(i);
    
    QString svgPath = item->data(Qt::UserRole).toString();
    if (!svgPath.isEmpty()) {
      
      item->setIcon(ThemeManager::getAdaptiveIcon(svgPath));
    }
  }
}

bool SettingsView::eventFilter(QObject *obj, QEvent *event) {
  if (event->type() == QEvent::Wheel) {
    
    for (int i = 0; i < m_scrollAreas.size(); ++i) {
      
      if (!m_scrollAreas[i]) {
        continue;
      }
      if (obj == m_scrollAreas[i]->viewport()) {
        auto *we   = static_cast<QWheelEvent *>(event);
        auto *controller =
            i < m_scrollControllers.size() ? m_scrollControllers[i] : nullptr;
        if (controller && controller->scrollByWheelEvent(we, Qt::Vertical)) {
          return true;
        }
        we->ignore();
        return false;
      }
    }
  }

  return QWidget::eventFilter(obj, event);
}
