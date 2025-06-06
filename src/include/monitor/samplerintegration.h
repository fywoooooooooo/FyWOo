// samplerintegration.h
#pragma once

#include <QObject>
#include "src/include/monitor/threadedsampler.h"
#include "src/include/analysis/anomalydetector.h"
#include "src/include/chart/chartwidget.h"
#include "src/include/storage/datastorage.h"

// 这个类演示如何集成ThreadedSampler到现有应用中
class SamplerIntegration : public QObject {
    Q_OBJECT

public:
    explicit SamplerIntegration(QObject *parent = nullptr);
    ~SamplerIntegration();
    
    // 启动高精度监控
    void startHighPrecisionMonitoring();
    
    // 停止监控
    void stopMonitoring();
    
    // 设置异常检测阈值
    void setAnomalyThreshold(double threshold);
    
    // 设置数据存储
    void setStorage(DataStorage *storage);
    
    // 设置图表控件
    void setChartWidgets(ChartWidget *cpu, ChartWidget *mem, ChartWidget *gpu, ChartWidget *net);

private slots:
    // 处理性能数据并进行异常检测
    void processPerformanceData(double cpuUsage, double memoryUsage, double diskIO, double networkUsage);
    
    // 处理检测到的异常
    void onAnomalyDetected(const QString& source, double value, double threshold, const QDateTime& timestamp);

private:
    ThreadedSampler *m_threadedSampler;
    AnomalyDetector *m_anomalyDetector;
};