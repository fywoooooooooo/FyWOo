#ifndef PROCESSPAGE_H
#define PROCESSPAGE_H

#include <QWidget>
#include <QVBoxLayout>
#include <QTableWidget>
#include <QTimer>
#include <QLineEdit>
#include <QPushButton>
#include <QLabel>

class ProcessPage : public QWidget
{
    Q_OBJECT

public:
    explicit ProcessPage(QWidget *parent = nullptr);
    ~ProcessPage();
    
    // 添加一个结构体用于存储进程数据，供MainWindow使用
    struct ProcessData {
        quint64 pid;
        QString name;
        double cpuUsage;
        quint64 memoryUsage;
    };
    
    // 获取当前进程数据列表
    QList<ProcessData> getCurrentProcessData() const;

signals:
    // 添加终止进程请求信号
    void processTerminationRequested(quint64 pid);

public slots:
    void updateProcessList();

private slots:
    void filterProcesses(const QString &filter);
    void terminateSelectedProcess();
    void refreshProcesses();

private:
    QTableWidget *m_processTable;
    QTimer *m_updateTimer;
    QLineEdit *m_searchEdit;
    QPushButton *m_terminateButton;
    QPushButton *m_refreshButton;
    QLabel *m_totalProcessesLabel;
    QLabel *m_cpuUsageLabel;
    QLabel *m_memoryUsageLabel;
    
    void setupUI();
    QString formatMemorySize(quint64 bytes);
    void updateSystemStats();
};

#endif // PROCESSPAGE_H