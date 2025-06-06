#ifndef PROCESSSELECTIONDIALOG_H
#define PROCESSSELECTIONDIALOG_H

#include <QDialog>
#include <QListWidget>
#include <QPushButton>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QLabel>
#include <QCheckBox>
#include <QScrollArea>
#include <QMap>
#include "../common/processinfo.h"

class ProcessSelectionDialog : public QDialog
{
    Q_OBJECT

public:
    explicit ProcessSelectionDialog(const QList<ProcessInfo> &processes, const QString &title, QWidget *parent = nullptr);
    ~ProcessSelectionDialog();

    QList<quint64> getSelectedPids() const;

private slots:
    void onOkClicked();
    void onCancelClicked();

private:
    void setupUi(const QString &title);

    QListWidget *m_processListWidget;
    QPushButton *m_okButton;
    QPushButton *m_cancelButton;

    QList<ProcessInfo> m_processes;
    QList<quint64> m_selectedPids;
    QMap<QListWidgetItem*, quint64> m_itemToPidMap;
};

#endif // PROCESSSELECTIONDIALOG_H