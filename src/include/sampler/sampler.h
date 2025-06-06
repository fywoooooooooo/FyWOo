#pragma once

#include <QObject>
#include <QTimer>
#include <QVector>
#include <QPair>
#include <QDateTime>
#include <QNetworkInterface>
#include <QProcess>
#include <QMessageBox>

#include "src/include/storage/datastorage.h"

class Sampler : public QObject {
    Q_OBJECT
public:
    explicit Sampler(QObject *parent = nullptr);
    ~Sampler();

    // 启动和停止采样器
    void start();
    void stop();
    
    // 设置采样间隔 (毫秒)
    void setInterval(int msec);
    
    // 获取当前采样间隔
    int interval() const;
    
    // 开启/关闭数据存储
    void enableDataStorage(bool enable);
    
    // 设置是否记录历史数据
    void setLoggingEnabled(bool enabled);
    
    // 获取当前状态是否正在运行
    bool isRunning() const;
    
    // 获取最近的资源使用率
    double getCurrentCpuUsage() const;
    quint64 getTotalMemory() const;
    quint64 getUsedMemory() const;
    quint64 getFreeMemory() const;
    double getCurrentNetworkUpload() const; // 单位: MB/s
    double getCurrentNetworkDownload() const; // 单位: MB/s

public slots:
    // 采样所有资源使用情况
    void sampleAll();
    
    // GPU检测对话框
    void showGpuNotFoundDialog();

signals:
    // 更新CPU使用率信号
    void cpuUsageUpdated(double usage);
    
    // 更新内存统计信号
    void memoryStatsUpdated(quint64 total, quint64 used, quint64 free);
    
    // 更新网络状态信号
    void networkStatsUpdated(double uploadSpeed, double downloadSpeed);
    
    // 更新磁盘状态信号
    void diskStatsUpdated(qint64 readBytes, qint64 writeBytes);
    
    // 更新GPU状态信号
    void gpuStatsUpdated(double usage, double temperature, quint64 memoryUsed, quint64 memoryTotal);
    
    // GPU可用性变化信号
    void gpuAvailabilityChanged(bool available, const QString& gpuName, const QString& driverVersion);
    
    // 整体性能数据更新信号
    void performanceDataUpdated(double cpuUsage, double memoryUsage, double diskIO, double networkUsage);

private:
    // 采样CPU使用率
    void sampleCpuUsage();
    
    // 采样内存使用情况
    void sampleMemoryUsage();
    
    // 采样网络使用情况
    void sampleNetworkUsage();
    
    // 采样磁盘使用情况
    void sampleDiskUsage();
    
    // 采样GPU使用情况
    void sampleGpuUsage();
    
    // 检查GPU可用性
    void checkGpuAvailability();
    
    // 初始化计时器
    void initTimer();
    
    // 存储数据
    void storeData();
    
    // 存储性能数据到数据库
    void storePerformanceData();
    
    // 获取系统信息
    void getSystemInfo();
    
    // 计时器
    QTimer *m_timer;
    
    // 是否正在运行
    bool m_running;
    
    // 采样间隔 (毫秒)
    int m_interval;
    
    // 是否启用数据存储
    bool m_storageEnabled;
    
    // 是否记录历史数据
    bool m_loggingEnabled;
    
    // CPU使用率
    double m_cpuUsage;
    quint64 m_lastTotalCpuTime;
    quint64 m_lastIdleCpuTime;
    
    // 内存使用情况
    quint64 m_totalMemory;
    quint64 m_usedMemory;
    quint64 m_freeMemory;
    
    // 网络使用情况
    quint64 m_lastReceivedBytes;
    quint64 m_lastSentBytes;
    QDateTime m_lastNetworkSampleTime;
    double m_uploadSpeed; // 单位: MB/s
    double m_downloadSpeed; // 单位: MB/s
    
    // 磁盘使用情况
    quint64 m_lastDiskReadBytes;
    quint64 m_lastDiskWriteBytes;
    QDateTime m_lastDiskSampleTime;
    double m_diskReadSpeed; // 单位: MB/s
    double m_diskWriteSpeed; // 单位: MB/s
    
    // GPU相关信息
    bool m_gpuAvailable;
    double m_gpuUsage;
    double m_gpuTemperature;
    quint64 m_gpuMemoryUsed;
    quint64 m_gpuMemoryTotal;
    QString m_gpuName;
    QString m_gpuDriverVersion;
    
    // 数据存储对象
    DataStorage *m_storage;
}; 