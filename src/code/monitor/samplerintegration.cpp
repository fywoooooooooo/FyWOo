// samplerintegration.cpp
#include "src/include/monitor/samplerintegration.h"
#include <QDebug>

SamplerIntegration::SamplerIntegration(QObject *parent)
    : QObject(parent)
    , m_threadedSampler(new ThreadedSampler(this))
    , m_anomalyDetector(new AnomalyDetector(this))
{
    // ���Ӷ��̲߳��������źŵ��쳣�����
    connect(m_threadedSampler, &ThreadedSampler::performanceDataUpdated,
            this, &SamplerIntegration::processPerformanceData);
    
    // �����쳣��������ź�
    connect(m_anomalyDetector, &AnomalyDetector::anomalyDetected,
            this, &SamplerIntegration::onAnomalyDetected);
}

SamplerIntegration::~SamplerIntegration()
{
    stopMonitoring();
}

void SamplerIntegration::startHighPrecisionMonitoring()
{
    // ʹ�ø��ߵĲ���Ƶ���������̲߳�����
    // CPU: 200ms, �ڴ�: 500ms, ����: 500ms, ����: 200ms
    m_threadedSampler->startSampling(200, 500, 500, 200);
}

void SamplerIntegration::stopMonitoring()
{
    m_threadedSampler->stopSampling();
}

void SamplerIntegration::setAnomalyThreshold(double threshold)
{
    m_anomalyDetector->setThresholdFactor(threshold);
}

void SamplerIntegration::setStorage(DataStorage *storage)
{
    m_threadedSampler->setStorage(storage);
}

void SamplerIntegration::setChartWidgets(ChartWidget *cpu, ChartWidget *mem, ChartWidget *gpu, ChartWidget *net)
{
    m_threadedSampler->setChartWidgets(cpu, mem, gpu, net);
}

void SamplerIntegration::processPerformanceData(double cpuUsage, double memoryUsage, double diskIO, double networkUsage)
{
    // ������ݵ㵽�쳣�����
    m_anomalyDetector->addCpuDataPoint(cpuUsage);
    m_anomalyDetector->addMemoryDataPoint(memoryUsage);
    m_anomalyDetector->addDiskDataPoint(diskIO);
    m_anomalyDetector->addNetworkDataPoint(networkUsage);
    
    // ִ���쳣���
    m_anomalyDetector->detectCpuAnomaly();
    m_anomalyDetector->detectMemoryAnomaly();
    m_anomalyDetector->detectDiskAnomaly();
    m_anomalyDetector->detectNetworkAnomaly();
}

void SamplerIntegration::onAnomalyDetected(const QString& source, double value, double threshold, const QDateTime& timestamp)
{
    // �쳣�����
    
    // ����������֪ͨ�û��Ĵ��룬����ϵͳ����֪ͨ�򵯴�
}