// sampler.h
#pragma once

#include <QObject>
#include <QTimer>
#include "cpumonitor.h"
#include "memorymonitor.h"
#include "diskmonitor.h"
#include "networkmonitor.h"
#include "processmonitor.h"
#include "src/include/chart/chartwidget.h"
#include "src/include/storage/datastorage.h"

class Sampler : public QObject {
    Q_OBJECT

public:
    explicit Sampler(QObject *parent = nullptr);
    ~Sampler();
    void startSampling(int interval = 1000);
    void stopSampling();
    bool isGpuAvailable() const { return m_gpuAvailable; }
    QString gpuName() const { return m_gpuName; }
    QString driverVersion() const { return m_driverVersion; }
    void setStorage(DataStorage *storage);
    void setChartWidgets(ChartWidget *cpu, ChartWidget *mem, ChartWidget *gpu, ChartWidget *net);
    
    // ????????????????
    double lastCpuUsage() const { return m_cpu.getCpuUsage(); }
    double lastMemoryUsage() const { return m_memory.getMemoryUsage(); }
    double lastDiskIO() const { return m_disk.getDiskIO(); }
    double lastNetworkUsage() const { 
        QPair<quint64, quint64> usage = m_network.getNetworkUsageDetailed();
        return static_cast<double>(usage.first + usage.second) / (1024.0 * 1024.0); // ????MB
    }

public slots:
    void collect();

signals:
    void memoryStatsUpdated(quint64 total, quint64 used, quint64 free);
    void cpuUsageUpdated(double usage);
    void networkStatsUpdated(double uploadSpeed, double downloadSpeed);
    void diskStatsUpdated(qint64 readBytes, qint64 writeBytes);
    void gpuStatsUpdated(double usage, double temperature, quint64 memoryUsed, quint64 memoryTotal);
    void gpuAvailabilityChanged(bool available, const QString& gpuName, const QString& driverVersion);
    
    // ??????????????
    void performanceDataUpdated(double cpuUsage, double memoryUsage, double diskIO, double networkUsage);
    void showGpuNotification(const QString& title, const QString& message);

private:
    QTimer *m_timer;
    CpuMonitor m_cpu;
    MemoryMonitor m_memory;
    DiskMonitor m_disk;
    NetworkMonitor m_network;
    DataStorage *m_storage;

    ChartWidget *cpuChart;
    ChartWidget *memChart;
    ChartWidget *gpuChart;
    ChartWidget *netChart;

    bool m_gpuAvailable;
    bool m_gpuCheckPerformed;
    bool m_gpuNotificationShown = false;
    QString m_gpuName;
    QString m_driverVersion;

    void checkGpuAvailability();
    bool detectGpu();
    void sampleGpuStats();
    void showGpuNotFoundDialog();
};
