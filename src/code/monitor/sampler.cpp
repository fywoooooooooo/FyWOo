#include "src/include/monitor/sampler.h"
#include <QTimer>
#include <QProcess>
#include <QThread>
#include <QDebug>
#include <QFile>
#include <QTextStream>
#include <QRegularExpression>
#include <QStandardPaths>
#include <QDir>
#include <QCoreApplication>
#ifdef Q_OS_WIN
#include <windows.h>
#include <pdh.h>
#include <pdhmsg.h>
#include <psapi.h>
#include <iphlpapi.h>
#include <wbemidl.h>
#include <comdef.h>
#pragma comment(lib, "pdh.lib")
#pragma comment(lib, "psapi.lib")
#pragma comment(lib, "iphlpapi.lib")
#pragma comment(lib, "wbemuuid.lib")
#elif defined(Q_OS_LINUX)
#include <sys/sysinfo.h>
#include <unistd.h>
#include <fstream>
#include <sstream>
#endif

Sampler::Sampler(QObject *parent)
    : QObject(parent)
    , m_timer(new QTimer(this))
    , m_gpuAvailable(false)
    , m_gpuCheckPerformed(false)
    , m_gpuName(tr("δ��⵽"))
    , m_driverVersion("N/A")
{
    connect(m_timer, &QTimer::timeout, this, &Sampler::collect);
    
    // ��ʼ��GPU���
    checkGpuAvailability();
}

Sampler::~Sampler()
{
    stopSampling();
}

void Sampler::startSampling(int interval)
{
    m_timer->start(interval);
    
    // ����ִ��һ��GPU״̬��飬ȷ��UI��ó�ʼ״̬
    checkGpuAvailability();
    // ǿ�Ʒ��ͳ�ʼGPU״̬�ź�
    emit gpuAvailabilityChanged(m_gpuAvailable, m_gpuName, m_driverVersion);
}

void Sampler::stopSampling()
{
    m_timer->stop();
}

void Sampler::setStorage(DataStorage *storage)
{
    m_storage = storage;
}

void Sampler::setChartWidgets(ChartWidget *cpu, ChartWidget *mem, ChartWidget *gpu, ChartWidget *net)
{
    cpuChart = cpu;
    memChart = mem;
    gpuChart = gpu;
    netChart = net;
}

void Sampler::collect()
{
    // ���GPU�����Ա仯
    bool previousGpuAvailable = m_gpuAvailable;
    checkGpuAvailability();
    
    if (previousGpuAvailable != m_gpuAvailable) {
        emit gpuAvailabilityChanged(m_gpuAvailable, m_gpuName, m_driverVersion);
        
        if (m_gpuAvailable) {
            emit showGpuNotification(tr("GPU״̬"), tr("GPU������: %1").arg(m_gpuName));
        } else {
            emit showGpuNotification(tr("GPU״̬"), tr("GPU�ѶϿ�����"));
        }
    }
    
    // �ɼ�������������
    double cpuUsage = m_cpu.getCpuUsage();
    double memoryUsage = m_memory.getMemoryUsage();
    double diskIO = m_disk.getDiskIO();
    double networkUsage = lastNetworkUsage();
    
    // ���͸������ݸ����ź�
    emit cpuUsageUpdated(cpuUsage);
    
    quint64 totalMem = m_memory.getTotalMemory();
    quint64 usedMem = m_memory.getUsedMemory();
    quint64 freeMem = totalMem - usedMem;
    emit memoryStatsUpdated(totalMem, usedMem, freeMem);
    
    QPair<quint64, quint64> networkStats = m_network.getNetworkUsageDetailed();
    emit networkStatsUpdated(networkStats.second / (1024.0 * 1024.0), networkStats.first / (1024.0 * 1024.0));
    
    emit diskStatsUpdated(diskIO, diskIO);
    
    // ���GPU���ã��ɼ�GPU����
    if (m_gpuAvailable) {
        sampleGpuStats();
    }
    
    // �����ۺ����������ź�
    emit performanceDataUpdated(cpuUsage, memoryUsage, diskIO, networkUsage);
}

void Sampler::checkGpuAvailability()
{
    bool wasAvailable = m_gpuAvailable;
    m_gpuAvailable = detectGpu();
}

bool Sampler::detectGpu()
{
    
#ifdef Q_OS_WIN
    // Windowsƽ̨������ʹ��nvidia-smi��Ȼ��WMI�����dxdiag
    
    // 1. ����nvidia-smi��������NVIDIA GPU��
    QProcess nvidiaProcess;
    nvidiaProcess.start("nvidia-smi", QStringList() << "--query-gpu=name,driver_version" << "--format=csv,noheader");
    
    if (nvidiaProcess.waitForFinished(3000) && nvidiaProcess.exitCode() == 0) {
        QString output = nvidiaProcess.readAllStandardOutput().trimmed();
        if (!output.isEmpty()) {
            QStringList parts = output.split(",");
            if (parts.size() >= 2) {
                m_gpuName = parts[0].trimmed();
                m_driverVersion = parts[1].trimmed();
                return true;
            }
        }
    }
    
    // 2. ����WMI��ѯ������������GPU��
    QProcess wmiProcess;
    wmiProcess.start("wmic", QStringList() << "path" << "win32_VideoController" 
                     << "get" << "Name,DriverVersion,AdapterRAM" << "/format:csv");
    
    if (wmiProcess.waitForFinished(5000) && wmiProcess.exitCode() == 0) {
        QString output = wmiProcess.readAllStandardOutput();
        QStringList lines = output.split("\n");
        
        QString bestGpuName, bestDriverVersion;
        quint64 bestMemory = 0;
        
        for (int i = 1; i < lines.size(); i++) {
            QString line = lines[i].trimmed();
            if (line.isEmpty()) continue;
            
            QStringList parts = line.split(",");
            if (parts.size() < 4) continue;
            
            QString name = parts[2].trimmed();
            QString driver = parts[1].trimmed();
            
            // ���˵�������ʾ������
            if (name.contains("Microsoft Basic Display", Qt::CaseInsensitive) ||
                name.contains("Remote Display", Qt::CaseInsensitive) ||
                name.contains("VNC", Qt::CaseInsensitive) ||
                name.isEmpty()) {
                continue;
            }
            
            // �����ڴ��С
            quint64 memory = 0;
            if (parts.size() >= 4) {
                bool ok;
                memory = parts[0].trimmed().toULongLong(&ok);
                if (!ok) memory = 0;
            }
            
            // GPU���ȼ��������Կ� > �����Կ�
            bool isDiscrete = name.contains("NVIDIA", Qt::CaseInsensitive) || 
                             name.contains("AMD", Qt::CaseInsensitive) ||
                             name.contains("Radeon", Qt::CaseInsensitive);
            
            bool isIntegrated = name.contains("Intel", Qt::CaseInsensitive) ||
                               name.contains("UHD", Qt::CaseInsensitive) ||
                               name.contains("HD Graphics", Qt::CaseInsensitive) ||
                               name.contains("Iris", Qt::CaseInsensitive);
            
            // ����ѡ������Կ�
            if (isDiscrete) {
                if (bestGpuName.isEmpty() || memory > bestMemory || 
                    (!bestGpuName.contains("NVIDIA", Qt::CaseInsensitive) && 
                     !bestGpuName.contains("AMD", Qt::CaseInsensitive) &&
                     !bestGpuName.contains("Radeon", Qt::CaseInsensitive))) {
                    bestGpuName = name;
                    bestDriverVersion = driver;
                    bestMemory = memory;
                }
            }
            // ���û�ж����Կ���ѡ�񼯳��Կ�
            else if ((isIntegrated || bestGpuName.isEmpty()) && 
                    (!bestGpuName.contains("NVIDIA", Qt::CaseInsensitive) && 
                     !bestGpuName.contains("AMD", Qt::CaseInsensitive) &&
                     !bestGpuName.contains("Radeon", Qt::CaseInsensitive))) {
                if (bestGpuName.isEmpty() || memory > bestMemory) {
                    bestGpuName = name;
                    bestDriverVersion = driver;
                    bestMemory = memory;
                }
            }
        }
        
        if (!bestGpuName.isEmpty()) {
            m_gpuName = bestGpuName;
            m_driverVersion = bestDriverVersion;
            return true;
        }
    }
    
    // 3. �����dxdiag
    QProcess dxdiagProcess;
    dxdiagProcess.start("dxdiag", QStringList() << "/t" << "gpuinfo.txt");
    
    if (dxdiagProcess.waitForFinished(10000)) {
        QThread::msleep(500);
        
        QFile file("gpuinfo.txt");
        if (file.open(QIODevice::ReadOnly | QIODevice::Text)) {
            QTextStream in(&file);
            QString content = in.readAll();
            file.close();
            file.remove();
            
            QRegularExpression nameRx("Card name: (.+)");
            QRegularExpression driverRx("Driver Version: (.+)");
            
            QRegularExpressionMatch nameMatch = nameRx.match(content);
            if (nameMatch.hasMatch()) {
                m_gpuName = nameMatch.captured(1).trimmed();
                
                QRegularExpressionMatch driverMatch = driverRx.match(content);
                if (driverMatch.hasMatch()) {
                    m_driverVersion = driverMatch.captured(1).trimmed();
                } else {
                    m_driverVersion = "δ֪";
                }
                
                return true;
            }
        }
    }
    
#elif defined(Q_OS_LINUX)
    // Linuxƽ̨��ʹ��rocm-smi��lspci
    
    // 1. ����rocm-smi��AMD GPU��
    QProcess amdProcess;
    amdProcess.start("rocm-smi", QStringList() << "--showproductname" << "--showdriverversion");
    
    if (amdProcess.waitForFinished(3000) && amdProcess.exitCode() == 0) {
        QString output = amdProcess.readAllStandardOutput().trimmed();
        
        if (!output.isEmpty()) {
            QStringList lines = output.split("\n");
            QString gpuName, driverVersion;
            
            for (const QString& line : lines) {
                if (line.contains("GPU Product Name")) {
                    gpuName = line.section(':', 1).trimmed();
                } else if (line.contains("Driver Version")) {
                    driverVersion = line.section(':', 1).trimmed();
                }
            }
            
            if (!gpuName.isEmpty()) {
                m_gpuName = gpuName;
                m_driverVersion = driverVersion;
                return true;
            }
        }
    }
    
    // 2. ����lspci��ѯGPU
    QProcess lspciProcess;
    lspciProcess.start("lspci", QStringList() << "-v");
    
    if (lspciProcess.waitForFinished(3000) && lspciProcess.exitCode() == 0) {
        QString output = lspciProcess.readAllStandardOutput().trimmed();
        QStringList lines = output.split("\n");
        
        QString gpuInfo;
        for (int i = 0; i < lines.size(); i++) {
            if (lines[i].contains("VGA compatible controller") || 
                lines[i].contains("3D controller")) {
                gpuInfo = lines[i].section(':', 1).trimmed();
                
                // ����������Ϣ
                for (int j = i + 1; j < qMin(i + 10, lines.size()); j++) {
                    if (lines[j].contains("Kernel driver in use") || 
                        lines[j].contains("Module")) {
                        m_driverVersion = lines[j].section(':', 1).trimmed();
                        break;
                    }
                }
                break;
            }
        }
        
        if (!gpuInfo.isEmpty()) {
            m_gpuName = gpuInfo;
            if (m_driverVersion.isEmpty()) {
                m_driverVersion = "δ֪";
            }
            return true;
        }
    }
#endif
    
    m_gpuName = tr("δ��⵽");
    m_driverVersion = "N/A";
    return false;
}

void Sampler::sampleGpuStats()
{
    if (!m_gpuAvailable) {
        return;
    }
    double usage = 0.0;
    double temperature = 0.0;
    quint64 memoryUsed = 0;
    quint64 memoryTotal = 0;
    bool successful = false;

    if (m_gpuName.contains("NVIDIA", Qt::CaseInsensitive)) {
        QProcess process;
        process.start("nvidia-smi", QStringList() 
            << "--query-gpu=utilization.gpu,temperature.gpu,memory.used,memory.total" 
            << "--format=csv,noheader,nounits");
        
        if (process.waitForFinished(3000) && process.exitCode() == 0) {
            QString output = process.readAllStandardOutput().trimmed();
            QStringList values = output.split(",");
            
            if (values.size() >= 4) {
                bool ok;
                usage = values[0].trimmed().toDouble(&ok);
                if (!ok) usage = 0.0;
                
                temperature = values[1].trimmed().toDouble(&ok);
                if (!ok) temperature = 0.0;
                
                memoryUsed = values[2].trimmed().toULongLong(&ok) * 1024 * 1024; // MB to bytes
                if (!ok) memoryUsed = 0;
                
                memoryTotal = values[3].trimmed().toULongLong(&ok) * 1024 * 1024; // MB to bytes
                if (!ok) memoryTotal = 0;
                
                successful = true;
            }
        }
    }
    else if (m_gpuName.contains("AMD", Qt::CaseInsensitive) || m_gpuName.contains("Radeon", Qt::CaseInsensitive)) {
#ifdef Q_OS_LINUX
        QProcess process;
        process.start("rocm-smi", QStringList() << "--showuse" << "--showtemp" << "--showmemuse");
        
        if (process.waitForFinished(3000) && process.exitCode() == 0) {
            QString output = process.readAllStandardOutput();
            // ����rocm-smi���
            // ������Ҫ����ʵ�ʵ�rocm-smi�����ʽ������
            successful = true;
        }
#endif
    }
    
    // ����޷���ȡ�������ݣ��ṩĬ��ֵ
    if (!successful) {
        usage = 0.0;
        temperature = 0.0;
        memoryUsed = 0;
        memoryTotal = 0;
    }
    
    // ����GPUͳ���ź�
    emit gpuStatsUpdated(usage, temperature, memoryUsed, memoryTotal);
    
    // ȷ��GPU��Ȼ����
    if (successful) {
        if (!m_gpuAvailable) {
            m_gpuAvailable = true;
            emit gpuAvailabilityChanged(true, m_gpuName, m_driverVersion);
        }
    }
}

void Sampler::showGpuNotFoundDialog() {
    emit showGpuNotification("GPU Status", "No compatible GPU detected on this system.");
}
