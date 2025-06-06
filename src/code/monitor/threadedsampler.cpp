// threadedsampler.cpp
#include "src/include/monitor/threadedsampler.h"
#include <QDebug>
#include <QProcess>

// SamplerThread ʵ��
SamplerThread::SamplerThread(SamplerType type, QObject *parent)
    : QThread(parent)
    , m_type(type)
    , m_running(false)
    , m_interval(1000) // Ĭ��1��������
{
}

SamplerThread::~SamplerThread()
{
    stopSampling();
    wait(); // �ȴ��߳̽���
}

void SamplerThread::setSamplingInterval(int msecs)
{
    if (msecs > 0) {
        QMutexLocker locker(&m_mutex);
        m_interval = msecs;
    }
}

void SamplerThread::stopSampling()
{
    QMutexLocker locker(&m_mutex);
    m_running = false;
}

void SamplerThread::run()
{
    m_running = true;
    
    while (m_running) {
        // ��ȡ��ǰʱ���
        QDateTime timestamp = QDateTime::currentDateTime();
        
        // ��������
        double value = sampleData();
        
        // �������ݵ�
        emit newDataPoint(value, timestamp);
        
        // ��ȡ��ǰ�������
        int currentInterval;
        {
            QMutexLocker locker(&m_mutex);
            currentInterval = m_interval;
            if (!m_running) break;
        }
        
        // ����ָ��ʱ��
        msleep(currentInterval);
    }
}

double SamplerThread::sampleData()
{
    // ���ݼ��������Ͳɼ���ͬ������
    switch (m_type) {
        case CPU:
            return m_cpu.getCpuUsage();
        case Memory:
            return m_memory.getMemoryUsage();
        case Disk:
            return m_disk.getDiskIO();
        case Network: {
            QPair<quint64, quint64> usage = m_network.getNetworkUsageDetailed();
            // �ϴ��������ٶȵ��ܺͣ�ת��ΪMB/s
            return static_cast<double>(usage.first + usage.second) / (1024.0 * 1024.0);
        }
        default:
            return 0.0;
    }
}

// ThreadedSampler ʵ��
ThreadedSampler::ThreadedSampler(QObject *parent)
    : QObject(parent)
    , m_cpuThread(nullptr)
    , m_memoryThread(nullptr)
    , m_diskThread(nullptr)
    , m_networkThread(nullptr)
    , m_gpuTimer(new QTimer(this))
    , m_storage(nullptr)
    , m_cpuChart(nullptr)
    , m_memChart(nullptr)
    , m_gpuChart(nullptr)
    , m_netChart(nullptr)
    , m_gpuAvailable(false)
    , m_lastCpuUsage(0.0)
    , m_lastMemoryUsage(0.0)
    , m_lastDiskIO(0.0)
    , m_lastNetworkUsage(0.0)
{
    // ���������߳�
    m_cpuThread = new SamplerThread(SamplerThread::CPU, this);
    m_memoryThread = new SamplerThread(SamplerThread::Memory, this);
    m_diskThread = new SamplerThread(SamplerThread::Disk, this);
    m_networkThread = new SamplerThread(SamplerThread::Network, this);
    
    // �����źŲ�
    connect(m_cpuThread, &SamplerThread::newDataPoint, this, &ThreadedSampler::processCpuData);
    connect(m_memoryThread, &SamplerThread::newDataPoint, this, &ThreadedSampler::processMemoryData);
    connect(m_diskThread, &SamplerThread::newDataPoint, this, &ThreadedSampler::processDiskData);
    connect(m_networkThread, &SamplerThread::newDataPoint, this, &ThreadedSampler::processNetworkData);
    
    // GPU��ʱ��
    connect(m_gpuTimer, &QTimer::timeout, this, &ThreadedSampler::checkGpuAvailability);
    
    // ���GPU������
    checkGpuAvailability();
}

ThreadedSampler::~ThreadedSampler()
{
    stopSampling();
}

void ThreadedSampler::startSampling(int cpuInterval, int memoryInterval, int diskInterval, int networkInterval)
{
    // ���ø��̲߳������
    m_cpuThread->setSamplingInterval(cpuInterval);
    m_memoryThread->setSamplingInterval(memoryInterval);
    m_diskThread->setSamplingInterval(diskInterval);
    m_networkThread->setSamplingInterval(networkInterval);
    
    // �����߳�
    if (!m_cpuThread->isRunning()) m_cpuThread->start();
    if (!m_memoryThread->isRunning()) m_memoryThread->start();
    if (!m_diskThread->isRunning()) m_diskThread->start();
    if (!m_networkThread->isRunning()) m_networkThread->start();
    
    // ����GPU��ʱ��
    m_gpuTimer->start(5000); // ÿ5����һ��GPU
    

}

void ThreadedSampler::stopSampling()
{
    // ֹͣ�����߳�
    m_cpuThread->stopSampling();
    m_memoryThread->stopSampling();
    m_diskThread->stopSampling();
    m_networkThread->stopSampling();
    
    // ֹͣGPU��ʱ��
    m_gpuTimer->stop();
    

}

void ThreadedSampler::setStorage(DataStorage *storage)
{
    m_storage = storage;
}

void ThreadedSampler::setChartWidgets(ChartWidget *cpu, ChartWidget *mem, ChartWidget *gpu, ChartWidget *net)
{
    m_cpuChart = cpu;
    m_memChart = mem;
    m_gpuChart = gpu;
    m_netChart = net;
}

void ThreadedSampler::processCpuData(double value, const QDateTime &timestamp)
{
    qDebug() << "[ThreadedSampler] processCpuData called, value:" << value << ", timestamp:" << timestamp;
    QMutexLocker locker(&m_dataMutex);
    m_lastCpuUsage = value;
    
    // ����ͼ��
    if (m_cpuChart) {
        m_cpuChart->updateValue(value);
    }
    
    // �洢����
    if (m_storage) {
        qDebug() << "[ThreadedSampler] storeSample: CPU";
        m_storage->storeSample("CPU", value, timestamp);
    }
    
    // �����ź�
    emit cpuUsageUpdated(value);
    
    // �����������ݸ����ź�
    emit performanceDataUpdated(m_lastCpuUsage, m_lastMemoryUsage, m_lastDiskIO, m_lastNetworkUsage);
}

void ThreadedSampler::processMemoryData(double value, const QDateTime &timestamp)
{
    qDebug() << "[ThreadedSampler] processMemoryData called, value:" << value << ", timestamp:" << timestamp;
    QMutexLocker locker(&m_dataMutex);
    m_lastMemoryUsage = value;
    
    // ��ȡ��ϸ�ڴ���Ϣ
    MemoryMonitor memMonitor;
    quint64 totalMem = memMonitor.getTotalMemory();
    quint64 usedMem = memMonitor.getUsedMemory();
    quint64 freeMem = totalMem - usedMem;
    
    // ����ͼ��
    if (m_memChart) {
        m_memChart->updateValue(value);
    }
    
    // �洢����
    if (m_storage) {
        qDebug() << "[ThreadedSampler] storeSample: Memory";
        m_storage->storeSample("Memory", value, timestamp);
    }
    
    // �����ź�
    emit memoryStatsUpdated(totalMem, usedMem, freeMem);
    
    // �����������ݸ����ź�
    emit performanceDataUpdated(m_lastCpuUsage, m_lastMemoryUsage, m_lastDiskIO, m_lastNetworkUsage);
}

void ThreadedSampler::processDiskData(double value, const QDateTime &timestamp)
{
    qDebug() << "[ThreadedSampler] processDiskData called, value:" << value << ", timestamp:" << timestamp;
    QMutexLocker locker(&m_dataMutex);
    m_lastDiskIO = value;
    
    // �������IO�Ƕ�д�ٶȵ��ܺͣ����ǽ���ƽ���������д
    qint64 readBytes = static_cast<qint64>(value * 1024 * 1024 / 2); // ת��Ϊ�ֽ�
    qint64 writeBytes = readBytes;
    
    // �洢����
    if (m_storage) {
        qDebug() << "[ThreadedSampler] storeSample: Disk";
        m_storage->storeSample("Disk", value, timestamp);
    }
    
    // �����ź�
    emit diskStatsUpdated(readBytes, writeBytes);
    
    // �����������ݸ����ź�
    emit performanceDataUpdated(m_lastCpuUsage, m_lastMemoryUsage, m_lastDiskIO, m_lastNetworkUsage);
}

void ThreadedSampler::processNetworkData(double value, const QDateTime &timestamp)
{
    qDebug() << "[ThreadedSampler] processNetworkData called, value:" << value << ", timestamp:" << timestamp;
    QMutexLocker locker(&m_dataMutex);
    m_lastNetworkUsage = value;
    
    // ��ȡ��ϸ������Ϣ
    NetworkMonitor netMonitor;
    QPair<quint64, quint64> networkUsage = netMonitor.getNetworkUsageDetailed();
    double uploadSpeed = static_cast<double>(networkUsage.first) / (1024.0 * 1024.0); // ת��ΪMB
    double downloadSpeed = static_cast<double>(networkUsage.second) / (1024.0 * 1024.0); // ת��ΪMB
    
    // ����ͼ��
    if (m_netChart) {
        m_netChart->updateValue(value);
    }
    
    // �洢����
    if (m_storage) {
        qDebug() << "[ThreadedSampler] storeSample: Network";
        m_storage->storeSample("Network", value, timestamp);
    }
    
    // �����ź�
    emit networkStatsUpdated(uploadSpeed, downloadSpeed);
    
    // �����������ݸ����ź�
    emit performanceDataUpdated(m_lastCpuUsage, m_lastMemoryUsage, m_lastDiskIO, m_lastNetworkUsage);
}

void ThreadedSampler::checkGpuAvailability()
{
    bool wasAvailable = m_gpuAvailable;
    QString oldGpuName = m_gpuName;
    QString oldDriverVersion = m_driverVersion;

    m_gpuAvailable = detectGpu();

    if (m_gpuAvailable != wasAvailable || 
        m_gpuName != oldGpuName || 
        m_driverVersion != oldDriverVersion) {
        emit gpuAvailabilityChanged(m_gpuAvailable, m_gpuName, m_driverVersion);
    }
    
    if (m_gpuAvailable) {
        sampleGpuStats();
    }
}

bool ThreadedSampler::detectGpu()
{
    // ʹ�� nvidia-smi ��� NVIDIA GPU
    QProcess process;
    process.start("nvidia-smi", QStringList() << "--query-gpu=name,driver_version" << "--format=csv,noheader");
    
    if (!process.waitForFinished(3000)) {
        return false;
    }

    if (process.exitCode() != 0) {
        return false;
    }

    QString output = process.readAllStandardOutput().trimmed();
    if (output.isEmpty()) {
        return false;
    }

    QStringList parts = output.split(",");
    if (parts.size() >= 2) {
        m_gpuName = parts[0].trimmed();
        m_driverVersion = parts[1].trimmed();
        return true;
    }

    return false;
}

void ThreadedSampler::sampleGpuStats()
{
    qDebug() << "[ThreadedSampler] sampleGpuStats called.";
    QProcess process;
    process.start("nvidia-smi", QStringList() 
        << "--query-gpu=utilization.gpu,temperature.gpu,memory.used,memory.total" 
        << "--format=csv,noheader,nounits");
    
    if (!process.waitForFinished(3000)) {
        qDebug() << "[ThreadedSampler] nvidia-smi did not finish.";
        return;
    }

    if (process.exitCode() != 0) {
        qDebug() << "[ThreadedSampler] nvidia-smi exit code != 0.";
        return;
    }

    QString output = process.readAllStandardOutput().trimmed();
    QStringList parts = output.split(",");
    
    if (parts.size() >= 4) {
        double usage = parts[0].trimmed().toDouble();
        double temperature = parts[1].trimmed().toDouble();
        quint64 memoryUsed = parts[2].trimmed().toULongLong() * 1024 * 1024; // MiB to bytes
        quint64 memoryTotal = parts[3].trimmed().toULongLong() * 1024 * 1024; // MiB to bytes

        emit gpuStatsUpdated(usage, temperature, memoryUsed, memoryTotal);

        // ���� GPU ͼ��
        if (m_gpuChart) {
            m_gpuChart->updateValue(usage);
        }

        // �洢 GPU ����
        if (m_storage) {
            qDebug() << "[ThreadedSampler] storeSample: GPU";
            m_storage->storeSample("GPU", usage);
        }
    }
}