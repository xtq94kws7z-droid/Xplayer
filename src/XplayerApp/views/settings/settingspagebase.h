#ifndef SETTINGSPAGEBASE_H
#define SETTINGSPAGEBASE_H

#include <QWidget>
#include <QVBoxLayout>
#include <QString>

class XplayerCore;
class QLabel;

class SettingsPageBase : public QWidget {
    Q_OBJECT
public:
    explicit SettingsPageBase(XplayerCore* core, const QString& pageTitle, QWidget* parent = nullptr);
    ~SettingsPageBase() override = default;

protected:
    void resizeEvent(QResizeEvent* event) override;

protected:
    void applyResponsiveLayout();

    XplayerCore* m_core;          
    QVBoxLayout* m_mainLayout;  
};

#endif 
