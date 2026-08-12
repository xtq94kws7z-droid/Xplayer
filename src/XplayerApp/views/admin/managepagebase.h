#ifndef MANAGEPAGEBASE_H
#define MANAGEPAGEBASE_H

#include <QWidget>
#include <QVBoxLayout>
#include <QString>

class XplayerCore;
class QLabel;

class ManagePageBase : public QWidget {
    Q_OBJECT
public:
    explicit ManagePageBase(XplayerCore* core, const QString& pageTitle, QWidget* parent = nullptr);
    ~ManagePageBase() override = default;

protected:
    XplayerCore* m_core;
    QVBoxLayout* m_mainLayout;
};

#endif 
