#ifndef PAGETASKS_H
#define PAGETASKS_H

#include "managepagebase.h"
#include <models/admin/adminmodels.h>
#include <qcorotask.h>
#include <QHash>
#include <optional>

class QLabel;
class QPushButton;
class QScrollArea;
class QHideEvent;
class QProgressBar;
class QShowEvent;
class QTimer;
class QVBoxLayout;
class QWidget;

class PageTasks : public ManagePageBase {
    Q_OBJECT
public:
    explicit PageTasks(XplayerCore* core, QWidget* parent = nullptr);
    QScrollArea* scrollArea() const { return m_scrollArea; }

protected:
    void showEvent(QShowEvent* event) override;
    void hideEvent(QHideEvent* event) override;

private:
    QCoro::Task<void> loadData(bool isManual = false);
    QCoro::Task<void> onRunClicked(QString taskId, QString taskName);
    QCoro::Task<void> onStopClicked(QString taskId, QString taskName);
    void onCardClicked(const QString& taskId, const QString& taskName);
    QWidget* createSummaryCard(const QString& title,
                               QLabel*& valueLabel,
                               QWidget* parent);
    void clearTaskSections();
    void updateSummaryCards(const QList<ScheduledTaskInfo>& tasks);
    void updateTaskCards(const QList<ScheduledTaskInfo>& tasks);
    void rebuildTaskSections(const QList<ScheduledTaskInfo>& tasks);
    void addTaskSection(const QString& title,
                        const QString& subtitle,
                        const QList<ScheduledTaskInfo>& tasks);
    void addTaskCard(const ScheduledTaskInfo& task, QVBoxLayout* parentLayout);

    QPushButton* m_btnRefresh = nullptr;
    QLabel* m_statsLabel = nullptr;
    QScrollArea* m_scrollArea = nullptr;
    QWidget* m_scrollContentWidget = nullptr;
    QVBoxLayout* m_scrollContentLayout = nullptr;
    QVBoxLayout* m_taskSectionsLayout = nullptr;

    QWidget* m_emptyStateContainer = nullptr;
    QLabel* m_emptyLabel = nullptr;
    QWidget* m_bottomSpacer = nullptr;
    QLabel* m_totalValueLabel = nullptr;
    QLabel* m_runningValueLabel = nullptr;
    QLabel* m_scheduledValueLabel = nullptr;
    QLabel* m_manualValueLabel = nullptr;

    QTimer* m_refreshTimer = nullptr;
    bool m_loaded = false;
    bool m_hasLoadedData = false;
    bool m_isLoading = false;
    QString m_taskStructureKey;
    QString m_taskDynamicKey;
    QHash<QString, QLabel*> m_taskStatusLabels;
    QHash<QString, QLabel*> m_taskProgressLabels;
    QHash<QString, QProgressBar*> m_taskProgressBars;

    std::optional<QCoro::Task<void>> m_pendingTask;
};

#endif 
