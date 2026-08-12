#ifndef COLLAPSIBLESECTION_H
#define COLLAPSIBLESECTION_H

#include <QString>
#include <QWidget>

class QPushButton;
class QPropertyAnimation;
class QVBoxLayout;

class CollapsibleSection : public QWidget {
    Q_OBJECT
    friend class MotionComponentsTest;
public:
    explicit CollapsibleSection(const QString& title, QWidget* parent = nullptr);

    QVBoxLayout* contentLayout() const { return m_contentLayout; }
    void setExpanded(bool expanded);
    bool isExpanded() const { return m_expanded; }

private:
    void updateToggleText();

    QPushButton* m_toggle;
    QWidget* m_content;
    QVBoxLayout* m_contentLayout;
    QPropertyAnimation* m_heightAnimation = nullptr;
    bool m_expanded = false;
    QString m_title;
};

#endif 
