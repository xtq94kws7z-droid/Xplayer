#ifndef SETTINGSVIEW_H
#define SETTINGSVIEW_H

#include "../baseview.h"
#include <QLabel>
#include <QList>
#include <QListWidget>
#include <QPointer>
#include <QScrollArea>
#include <QVBoxLayout>

class SlidingStackedWidget;
class QResizeEvent;
class SmoothScrollController;

class SettingsView : public BaseView {
    Q_OBJECT
public:
    explicit SettingsView(XplayerCore* core, QWidget* parent = nullptr);
    ~SettingsView() override = default;

protected:
    void resizeEvent(QResizeEvent* event) override;
    bool eventFilter(QObject* obj, QEvent* event) override;

private:
    void setupUi();
    void setupConnections();
    void applyResponsiveLayout();

    
    
    void ensurePageAt(int row);

    
    
    QScrollArea* wrapInScrollArea(QWidget* page, int row);

private slots:
    
    void onThemeChanged();

private:
    QWidget* m_leftPanel;
    QLabel* m_titleLabel;
    QListWidget* m_navMenu;
    QVBoxLayout* m_leftLayout = nullptr;
    SlidingStackedWidget* m_stack;

    
    
    QList<QScrollArea*>              m_scrollAreas;
    QList<SmoothScrollController*>   m_scrollControllers;

    
    QList<QPointer<QWidget>>   m_pages;
};

#endif 
