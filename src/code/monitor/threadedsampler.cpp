// threadedsampler.cpp
#include "src/include/monitor/threadedsampler.h"
#include <QDebug>
#include <QProcess>

// SamplerThread 实现
SamplerThread::SamplerThread(SamplerType type, QObject *parent)
    : QThread(parent)
    , m_type(type)
    , m_running(false)
    , m_interval(1000) // 默认1秒采样间隔
{
}

SamplerThread::~SamplerThread()
{
    stopSampling();
    wait(); // 等待线程结束
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
        // 获取当前时间戳
        QDateTime timestamp = QDateTime::currentDateTime();
        
        // 采样数据
        double value = sampleData();
        
        // 发送数据点
        emit newDataPoint(value, timestamp);
        
        // 获取当前间隔设置
        int currentInterval;
        {
            QMutexLocker locker(&m_mutex);
            currentInterval = m_interval;
            if (!m_running) break;
        }
        
        // 休眠指定时间
        msleep(currentInterval);
    }
}

double SamplerThread::sampleData()
{
    // 根据监视器类型采集不同的数据
    switch (m_type) {
        case CPU:
            return m_cpu.getCpuUsage();
        case Memory:
            return m_memory.getMemoryUsage();
        case Disk:
            return m_disk.getDiskIO();
        case Network: {
            QPair<quint64, quint64> usage = m_network.getNetworkUsageDetailed();
            // 上传和下载速度的总和，转换为MB/s
            return static_cast<double>(usage.first + usage.second) / (1024.0 * 1024.0);
        }
        default:
            return 0.0;
    }
}

// ThreadedSampler 实现
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
    // 创建采样线程
    m_cpuThread = new SamplerThread(SamplerThread::CPU, this);
    m_memoryThread = new SamplerThread(SamplerThread::Memory, this);
    m_diskThread = new SamplerThread(SamplerThread::Disk, this);
    m_networkThread = new SamplerThread(SamplerThread::Network, this);
    
    // 连接信号槽
    connect(m_cpuThread, &SamplerThread::newDataPoint, this, &ThreadedSampler::processCpuData);
    connect(m_memoryThread, &SamplerThread::newDataPoint, this, &ThreadedSampler::processMemoryData);
    connect(m_diskThread, &SamplerThread::newDataPoint, this, &ThreadedSampler::processDiskData);
    connect(m_networkThread, &SamplerThread::newDataPoint, this, &ThreadedSampler::processNetworkData);
    
    // GPU定时器
    connect(m_gpuTimer, &QTimer::timeout, this, &ThreadedSampler::checkGpuAvailability);
    
    // 检查GPU可用性
    checkGpuAvailability();
}

ThreadedSampler::~ThreadedSampler()
{
    stopSampling();
}

void ThreadedSampler::startSampling(int cpuInterval, int memoryInterval, int diskInterval, int networkInterval)
{
    // 设置各线程采样间隔
    m_cpuThread->setSamplingInterval(cpuInterval);
    m_memoryThread->setSamplingInterval(memoryInterval);
    m_diskThread->setSamplingInterval(diskInterval);
    m_networkThread->setSamplingInterval(networkInterval);
    
    // 启动线程
    if (!m_cpuThread->isRunning()) m_cpuThread->start();
    if (!m_memoryThread->isRunning()) m_memoryThread->start();
    if (!m_diskThread->isRunning()) m_diskThread->start();
    if (!m_networkThread->isRunning()) m_networkThread->start();
    
    // 启动GPU定时器
    m_gpuTimer->start(5000); // 每5秒检查一次GPU
    

}

void ThreadedSampler::stopSampling()
{
    // 停止所有线程
    m_cpuThread->stopSampling();
    m_memoryThread->stopSampling();
    m_diskThread->stopSampling();
    m_networkThread->stopSampling();
    
    // 停止GPU定时器
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
    
    // 更新图表
    if (m_cpuChart) {
        m_cpuChart->updateValue(value);
    }
    
    // 存储数据
    if (m_storage) {
        qDebug() << "[ThreadedSampler] storeSample: CPU";
        m_storage->storeSample("CPU", value, timestamp);
    }
    
    // 发送信号
    emit cpuUsageUpdated(value);
    
    // 发送性能数据更新信号
    emit performanceDataUpdated(m_lastCpuUsage, m_lastMemoryUsage, m_lastDiskIO, m_lastNetworkUsage);
}

void ThreadedSampler::processMemoryData(double value, const QDateTime &timestamp)
{
    qDebug() << "[ThreadedSampler] processMemoryData called, value:" << value << ", timestamp:" << timestamp;
    QMutexLocker locker(&m_dataMutex);
    m_lastMemoryUsage = value;
    
    // 获取详细内存信息
    MemoryMonitor memMonitor;
    quint64 totalMem = memMonitor.getTotalMemory();
    quint64 usedMem = memMonitor.getUsedMemory();
    quint64 freeMem = totalMem - usedMem;
    
    // 更新图表
    if (m_memChart) {
        m_memChart->updateValue(value);
    }
    
    // 存储数据
    if (m_storage) {
        qDebug() << "[ThreadedSampler] storeSample: Memory";
        m_storage->storeSample("Memory", value, timestamp);
    }
    
    // 发送信号
    emit memoryStatsUpdated(totalMem, usedMem, freeMem);
    
    // 发送性能数据更新信号
    emit performanceDataUpdated(m_lastCpuUsage, m_lastMemoryUsage, m_lastDiskIO, m_lastNetworkUsage);
}

void ThreadedSampler::processDiskData(double value, const QDateTime &timestamp)
{
    qDebug() << "[ThreadedSampler] processDiskData called, value:" << value << ", timestamp:" << timestamp;
    QMutexLocker locker(&m_dataMutex);
    m_lastDiskIO = value;
    
    // 假设磁盘IO是读写速度的总和，我们将其平均分配给读写
    qint64 readBytes = static_cast<qint64>(value * 1024 * 1024 / 2); // 转换为字节
    qint64 writeBytes = readBytes;
    
    // 存储数据
    if (m_storage) {
        qDebug() << "[ThreadedSampler] storeSample: Disk";
        m_storage->storeSample("Disk", value, timestamp);
    }
    
    // 发送信号
    emit diskStatsUpdated(readBytes, writeBytes);
    
    // 发送性能数据更新信号
    emit performanceDataUpdated(m_lastCpuUsage, m_lastMemoryUsage, m_lastDiskIO, m_lastNetworkUsage);
}

void ThreadedSampler::processNetworkData(double value, const QDateTime &timestamp)
{
    qDebug() << "[ThreadedSampler] processNetworkData called, value:" << value << ", timestamp:" << timestamp;
    QMutexLocker locker(&m_dataMutex);
    m_lastNetworkUsage = value;
    
    // 获取详细网络信息
    NetworkMonitor netMonitor;
    QPair<quint64, quint64> networkUsage = netMonitor.getNetworkUsageDetailed();
    double uploadSpeed = static_cast<double>(networkUsage.first) / (1024.0 * 1024.0); // 转换为MB
    double downloadSpeed = static_cast<double>(networkUsage.second) / (1024.0 * 1024.0); // 转换为MB
    
    // 更新图表
    if (m_netChart) {
        m_netChart->updateValue(value);
    }
    
    // 存储数据
    if (m_storage) {
        qDebug() << "[ThreadedSampler] storeSample: Network";
        m_storage->storeSample("Network", value, timestamp);
    }
    
    // 发送信号
    emit networkStatsUpdated(uploadSpeed, downloadSpeed);
    
    // 发送性能数据更新信号
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
    // 使用 nvidia-smi 检测 NVIDIA GPU
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

        // 更新 GPU 图表
        if (m_gpuChart) {
            m_gpuChart->updateValue(usage);
        }

        // 存储 GPU 数据
        if (m_storage) {
            qDebug() << "[ThreadedSampler] storeSample: GPU";
            m_storage->storeSample("GPU", usage);
        }
    }
}