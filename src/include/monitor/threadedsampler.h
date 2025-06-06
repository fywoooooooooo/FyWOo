// threadedsampler.h
#pragma once

#include <QObject>
#include <QTimer>
#include <QThread>
#include <QMutex>
#include <QWaitCondition>
#include <QQueue>
#include <QDateTime>
#include "cpumonitor.h"
#include "memorymonitor.h"
#include "diskmonitor.h"
#include "networkmonitor.h"
#include "processmonitor.h"
#include "src/include/chart/chartwidget.h"
#include "src/include/storage/datastorage.h"

// 专用采集线程类
class SamplerThread : public QThread {
    Q_OBJECT

public:
    enum SamplerType {
        CPU,
        Memory,
        Disk,
        Network
    };

    explicit SamplerThread(SamplerType type, QObject *parent = nullptr);
    ~SamplerThread();

    void setSamplingInterval(int msecs);
    void stopSampling();

signals:
    void newDataPoint(double value, const QDateTime &timestamp);

protected:
    void run() override;

private:
    SamplerType m_type;
    bool m_running;
    int m_interval;
    QMutex m_mutex;
    
    // 各类监视器
    CpuMonitor m_cpu;
    MemoryMonitor m_memory;
    DiskMonitor m_disk;
    NetworkMonitor m_network;

    double sampleData();
};

// 多线程采样器主类
class ThreadedSampler : public QObject {
    Q_OBJECT

public:
    explicit ThreadedSampler(QObject *parent = nullptr);
    ~ThreadedSampler();

    void startSampling(int cpuInterval = 500, int memoryInterval = 1000, 
                      int diskInterval = 1000, int networkInterval = 500);
    void stopSampling();
    
    bool isGpuAvailable() const { return m_gpuAvailable; }
    QString gpuName() const { return m_gpuName; }
    QString driverVersion() const { return m_driverVersion; }
    void setStorage(DataStorage *storage);
    void setChartWidgets(ChartWidget *cpu, ChartWidget *mem, ChartWidget *gpu, ChartWidget *net);
    
    // 获取最新的性能数据
    double lastCpuUsage() const { return m_lastCpuUsage; }
    double lastMemoryUsage() const { return m_lastMemoryUsage; }
    double lastDiskIO() const { return m_lastDiskIO; }
    double lastNetworkUsage() const { return m_lastNetworkUsage; }

public slots:
    void processCpuData(double value, const QDateTime &timestamp);
    void processMemoryData(double value, const QDateTime &timestamp);
    void processDiskData(double value, const QDateTime &timestamp);
    void processNetworkData(double value, const QDateTime &timestamp);
    void checkGpuAvailability();

signals:
    void memoryStatsUpdated(quint64 total, quint64 used, quint64 free);
    void cpuUsageUpdated(double usage);
    void networkStatsUpdated(double uploadSpeed, double downloadSpeed);
    void diskStatsUpdated(qint64 readBytes, qint64 writeBytes);
    void gpuStatsUpdated(double usage, double temperature, quint64 memoryUsed, quint64 memoryTotal);
    void gpuAvailabilityChanged(bool available, const QString& gpuName, const QString& driverVersion);
    
    // 用于分析模块的信号
    void performanceDataUpdated(double cpuUsage, double memoryUsage, double diskIO, double networkUsage);

private:
    // 采样线程
    SamplerThread *m_cpuThread;
    SamplerThread *m_memoryThread;
    SamplerThread *m_diskThread;
    SamplerThread *m_networkThread;
    QTimer *m_gpuTimer;
    
    // 数据存储
    DataStorage *m_storage;
    
    // 图表控件
    ChartWidget *m_cpuChart;
    ChartWidget *m_memChart;
    ChartWidget *m_gpuChart;
    ChartWidget *m_netChart;
    
    // GPU相关
    bool m_gpuAvailable;
    QString m_gpuName;
    QString m_driverVersion;
    
    // 最新数据
    double m_lastCpuUsage;
    double m_lastMemoryUsage;
    double m_lastDiskIO;
    double m_lastNetworkUsage;
    
    // 互斥锁保护数据访问
    QMutex m_dataMutex;
    
    // 辅助函数
    bool detectGpu();
    void sampleGpuStats();
};