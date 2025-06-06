#ifndef GPUPAGE_H
#define GPUPAGE_H

#include <QWidget>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QLabel>
#include <QFrame>
#include <QProgressBar>
#include "src/include/chart/chartwidget.h"
#include <QDebug>

class GpuPage : public QWidget
{
    Q_OBJECT

public:
    explicit GpuPage(QWidget *parent = nullptr);
    ~GpuPage();

public slots:
    // 更新GPU统计信息
    void updateGpuStats(double usage, double temperature, quint64 memoryUsed, quint64 memoryTotal);
    
    // 处理GPU可用性变更
    void handleGpuAvailabilityChange(bool available, const QString &gpuName, const QString &driverVersion);
    
    // 更新GPU数据 - 仅适配接口，内部调用updateGpuStats
    void updateGpuData(double usage, double temperature, quint64 memoryUsed, quint64 memoryTotal) {
        updateGpuStats(usage, temperature, memoryUsed, memoryTotal);
    }

private:
    void setupUi();
    
    // UI组件
    QFrame *gpuStatusFrame;
    QLabel *gpuInfoLabel;
    QLabel *gpuDriverLabel;
    
    QFrame *gpuUsageFrame;
    QLabel *gpuUsageTextLabel;
    QProgressBar *gpuUsageBar;
    QLabel *gpuUsageLabel;
    
    QFrame *gpuTempFrame;
    QLabel *gpuTempTextLabel;
    QProgressBar *gpuTempBar;
    QLabel *gpuTempLabel;
    
    QFrame *gpuMemoryFrame;
    QLabel *gpuMemoryTextLabel;
    QProgressBar *gpuMemoryBar;
    QLabel *gpuMemoryLabel;
    
    QFrame *gpuChartFrame;
    QVBoxLayout *gpuChartLayout;
    
    ChartWidget *m_chart;
    bool m_gpuAvailable;
    
    // 格式化内存大小显示
    QString formatMemorySize(quint64 bytes);
};

#endif // GPUPAGE_H
