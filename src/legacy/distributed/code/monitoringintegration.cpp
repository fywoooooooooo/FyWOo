#include "src/include/distributed/monitoringintegration.h"
#include <QDebug>
#include <QSettings>
#include <QFile>
#include <QDir>

MonitoringIntegration::MonitoringIntegration(QObject *parent)
    : QObject(parent)
    , m_mode(StandaloneMode)
    , m_isRunning(false)
    , m_distributedMonitor(nullptr)
    , m_monitorNode(nullptr)
    , m_adaptiveSampler(nullptr)
    , m_performanceAnalyzer(nullptr)
    , m_dataStorage(nullptr)
    , m_serverPort(8765)
{
    // 初始化随机数生成器
    qsrand(QDateTime::currentDateTime().toSecsSinceEpoch());
}

MonitoringIntegration::~MonitoringIntegration()
{
    // 停止监控
    stop();
    
    // 释放资源
    if (m_distributedMonitor) {
        delete m_distributedMonitor;
        m_distributedMonitor = nullptr;
    }
    
    if (m_monitorNode) {
        delete m_monitorNode;
        m_monitorNode = nullptr;
    }
    
    if (m_adaptiveSampler) {
        delete m_adaptiveSampler;
        m_adaptiveSampler = nullptr;
    }
}

bool MonitoringIntegration::initialize(RunMode mode, const QString &configFile)
{
    // 设置运行模式
    m_mode = mode;
    
    // 如果已经初始化，先停止并清理
    if (m_isRunning) {
        stop();
    }
    
    // 如果提供了配置文件，加载配置
    if (!configFile.isEmpty()) {
        if (!loadConfig(configFile)) {
            qDebug() << "加载配置文件失败:" << configFile;
            return false;
        }
    }
    
    // 创建性能分析器和数据存储（所有模式都需要）
    if (!m_performanceAnalyzer) {
        m_performanceAnalyzer = new PerformanceAnalyzer(this);
    }
    
    if (!m_dataStorage) {
        m_dataStorage = new DataStorage(this);
        m_dataStorage->initialize("performance_data.db");
    }
    
    // 创建自适应采样器（所有模式都需要）
    if (!m_adaptiveSampler) {
        m_adaptiveSampler = new AdaptiveSampler(this);
        
        // 连接信号槽
        connect(m_adaptiveSampler, &AdaptiveSampler::samplingIntervalChanged,
                this, &MonitoringIntegration::handleSamplingIntervalChanged);
    }
    
    // 根据不同模式创建相应组件
    switch (m_mode) {
        case ServerMode: {
            // 服务器模式：创建分布式监控管理器
            if (!m_distributedMonitor) {
                m_distributedMonitor = new DistributedMonitor(this);
                
                // 设置性能分析器和数据存储
                m_distributedMonitor->setPerformanceAnalyzer(m_performanceAnalyzer);
                m_distributedMonitor->setDataStorage(m_dataStorage);
                
                // 连接信号槽
                connect(m_distributedMonitor, &DistributedMonitor::anomalyDetected,
                        this, &MonitoringIntegration::handleDistributedAnomaly);
                connect(m_distributedMonitor, &DistributedMonitor::nodeStatusChanged,
                        this, &MonitoringIntegration::handleNodeStatusChanged);
            }
            
            // 初始化服务器
            if (!m_distributedMonitor->initServer(m_serverPort)) {
                qDebug() << "初始化分布式监控服务器失败";
                return false;
            }
            
            emit statusChanged("服务器模式初始化完成，监听端口: " + QString::number(m_serverPort));
            break;
        }
        case ClientMode: {
            // 客户端模式：创建监控节点
            if (!m_monitorNode) {
                m_monitorNode = new MonitorNode(this);
                
                // 设置节点信息
                QString nodeId = QSysInfo::machineHostName() + "-" + QString::number(qrand() % 10000);
                QString nodeName = QSysInfo::prettyProductName();
                m_monitorNode->setNodeInfo(nodeId, nodeName);
            }
            
            emit statusChanged("客户端模式初始化完成");
            break;
        }
        case StandaloneMode: {
            // 独立模式：只使用性能分析器和自适应采样器
            emit statusChanged("独立模式初始化完成");
            break;
        }
    }
    
    return true;
}

bool MonitoringIntegration::start()
{
    if (m_isRunning) {
        qDebug() << "监控系统已经在运行中";
        return true;
    }
    
    bool success = false;
    
    // 根据不同模式启动相应组件
    switch (m_mode) {
        case ServerMode: {
            // 服务器模式：启动分布式监控管理器
            if (m_distributedMonitor) {
                m_distributedMonitor->startMonitoring();
                success = true;
                emit statusChanged("分布式监控服务器已启动");
            }
            break;
        }
        case ClientMode: {
            // 客户端模式：连接到服务器并开始采样
            if (m_monitorNode) {
                bool connected = m_monitorNode->connectToServer(m_serverAddress, m_serverPort);
                if (connected) {
                    m_monitorNode->startSampling();
                    success = true;
                    emit statusChanged("已连接到监控服务器并开始数据采集");
                } else {
                    emit statusChanged("连接到监控服务器失败");
                }
            }
            break;
        }
        case StandaloneMode: {
            // 独立模式：启动性能分析器
            if (m_performanceAnalyzer) {
                // 这里假设性能分析器有start方法
                // 实际使用时需要根据性能分析器的API调整
                success = true;
                emit statusChanged("独立监控已启动");
            }
            break;
        }
    }
    
    if (success) {
        m_isRunning = true;
    }
    
    return success;
}

void MonitoringIntegration::stop()
{
    if (!m_isRunning) {
        return;
    }
    
    // 根据不同模式停止相应组件
    switch (m_mode) {
        case ServerMode: {
            // 服务器模式：停止分布式监控管理器
            if (m_distributedMonitor) {
                m_distributedMonitor->stopMonitoring();
                emit statusChanged("分布式监控服务器已停止");
            }
            break;
        }
        case ClientMode: {
            // 客户端模式：停止采样并断开连接
            if (m_monitorNode) {
                m_monitorNode->stopSampling();
                m_monitorNode->disconnectFromServer();
                emit statusChanged("已停止数据采集并断开与监控服务器的连接");
            }
            break;
        }
        case StandaloneMode: {
            // 独立模式：停止性能分析器
            if (m_performanceAnalyzer) {
                // 这里假设性能分析器有stop方法
                // 实际使用时需要根据性能分析器的API调整
                emit statusChanged("独立监控已停止");
            }
            break;
        }
    }
    
    m_isRunning = false;
}

bool MonitoringIntegration::isRunning() const
{
    return m_isRunning;
}

void MonitoringIntegration::setServerAddress(const QHostAddress &address, quint16 port)
{
    m_serverAddress = address;
    m_serverPort = port;
    
    qDebug() << "设置服务器地址:" << address.toString() << ":" << port;
}

void MonitoringIntegration::setServerPort(quint16 port)
{
    m_serverPort = port;
    
    // 如果服务器已经在运行，需要重新启动
    if (m_mode == ServerMode && m_isRunning && m_distributedMonitor) {
        m_distributedMonitor->stopMonitoring();
        m_distributedMonitor->initServer(port);
        m_distributedMonitor->startMonitoring();
    }
    
    qDebug() << "设置服务器端口:" << port;
}

bool MonitoringIntegration::addRemoteNode(const QString &nodeId, const QString &nodeName, const QHostAddress &address, quint16 port)
{
    // 只有服务器模式才能添加远程节点
    if (m_mode != ServerMode || !m_distributedMonitor) {
        qDebug() << "只有服务器模式才能添加远程节点";
        return false;
    }
    
    // 添加节点
    bool success = m_distributedMonitor->addNode(nodeId, nodeName, address, port);
    if (success) {
        // 如果已经在运行，尝试连接到节点
        if (m_isRunning) {
            m_distributedMonitor->connectToNode(nodeId);
        }
        
        qDebug() << "添加远程节点:" << nodeName << "(" << nodeId << ")" << address.toString() << ":" << port;
    }
    
    return success;
}

void MonitoringIntegration::setSamplingStrategy(AdaptiveSampler::SamplingStrategy strategy)
{
    if (m_adaptiveSampler) {
        m_adaptiveSampler->setSamplingStrategy(strategy);
        qDebug() << "设置采样策略:" << strategy;
    }
}

void MonitoringIntegration::setCompressionAlgorithm(AdaptiveSampler::CompressionAlgorithm algorithm)
{
    if (m_adaptiveSampler) {
        m_adaptiveSampler->setCompressionAlgorithm(algorithm);
        qDebug() << "设置压缩算法:" << algorithm;
    }
}

void MonitoringIntegration::setSamplingInterval(int msec)
{
    // 设置自适应采样器的基础采样间隔
    if (m_adaptiveSampler) {
        m_adaptiveSampler->setBaseSamplingInterval(msec);
    }
    
    // 根据不同模式设置相应组件的采样间隔
    switch (m_mode) {
        case ServerMode: {
            if (m_distributedMonitor) {
                m_distributedMonitor->setSamplingInterval(msec);
            }
            break;
        }
        case ClientMode: {
            if (m_monitorNode) {
                m_monitorNode->setSamplingInterval(msec);
            }
            break;
        }
        case StandaloneMode: {
            // 独立模式下可能需要设置性能分析器的采样间隔
            // 这里假设性能分析器有setSamplingInterval方法
            // 实际使用时需要根据性能分析器的API调整
            break;
        }
    }
    
    qDebug() << "设置采样间隔:" << msec << "毫秒";
}

bool MonitoringIntegration::exportReport(const QString &filePath) const
{
    // 根据不同模式导出报告
    switch (m_mode) {
        case ServerMode: {
            // 服务器模式：导出分布式监控报告
            if (m_distributedMonitor) {
                return m_distributedMonitor->exportMonitoringReport(filePath);
            }
            break;
        }
        case ClientMode:
        case StandaloneMode: {
            // 客户端模式和独立模式：导出性能分析报告
            if (m_dataStorage) {
                // 这里假设数据存储有exportSystemData方法
                // 实际使用时需要根据数据存储的API调整
                return m_dataStorage->exportSystemData(filePath);
            }
            break;
        }
    }
    
    return false;
}

void MonitoringIntegration::handleDistributedAnomaly(const QString &nodeId, const QString &metricName, double value)
{
    // 转发异常检测信号
    emit anomalyDetected(nodeId, metricName, value);
    qDebug() << "检测到分布式异常:" << nodeId << metricName << value;
}

void MonitoringIntegration::handleNodeStatusChanged(const QString &nodeId, DistributedMonitor::NodeStatus status)
{
    // 处理节点状态变化
    QString statusStr;
    switch (status) {
        case DistributedMonitor::Connected:
            statusStr = "已连接";
            break;
        case DistributedMonitor::Disconnected:
            statusStr = "已断开";
            break;
        case DistributedMonitor::Connecting:
            statusStr = "连接中";
            break;
        case DistributedMonitor::Failed:
            statusStr = "连接失败";
            break;
    }
    
    emit statusChanged(QString("节点 %1 状态变化: %2").arg(nodeId).arg(statusStr));
    qDebug() << "节点状态变化:" << nodeId << statusStr;
}

void MonitoringIntegration::handleSamplingIntervalChanged(const QString &metricName, int newInterval)
{
    // 处理采样间隔变化
    emit statusChanged(QString("指标 %1 采样间隔变化: %2 毫秒").arg(metricName).arg(newInterval));
    qDebug() << "采样间隔变化:" << metricName << newInterval << "毫秒";
}

bool MonitoringIntegration::loadConfig(const QString &configFile)
{
    QSettings settings(configFile, QSettings::IniFormat);
    
    // 读取基本设置
    m_mode = static_cast<RunMode>(settings.value("General/Mode", StandaloneMode).toInt());
    m_serverAddress = QHostAddress(settings.value("Server/Address", "127.0.0.1").toString());
    m_serverPort = settings.value("Server/Port", 8765).toUInt();
    
    // 读取采样设置
    int samplingInterval = settings.value("Sampling/Interval", 5000).toInt();
    int samplingStrategy = settings.value("Sampling/Strategy", AdaptiveSampler::FixedRate).toInt();
    int compressionAlgorithm = settings.value("Sampling/Compression", AdaptiveSampler::None).toInt();
    
    // 应用设置
    if (m_adaptiveSampler) {
        m_adaptiveSampler->setBaseSamplingInterval(samplingInterval);
        m_adaptiveSampler->setSamplingStrategy(static_cast<AdaptiveSampler::SamplingStrategy>(samplingStrategy));
        m_adaptiveSampler->setCompressionAlgorithm(static_cast<AdaptiveSampler::CompressionAlgorithm>(compressionAlgorithm));
    }
    
    // 读取远程节点设置（服务器模式）
    if (m_mode == ServerMode) {
        int nodeCount = settings.value("Nodes/Count", 0).toInt();
        for (int i = 0; i < nodeCount; ++i) {
            QString prefix = QString("Nodes/Node%1/").arg(i);
            QString nodeId = settings.value(prefix + "Id").toString();
            QString nodeName = settings.value(prefix + "Name").toString();
            QHostAddress nodeAddress(settings.value(prefix + "Address").toString());
            quint16 nodePort = settings.value(prefix + "Port").toUInt();
            
            if (!nodeId.isEmpty() && !nodeName.isEmpty() && !nodeAddress.isNull() && nodePort > 0) {
                // 添加远程节点（不立即连接）
                if (m_distributedMonitor) {
                    m_distributedMonitor->addNode(nodeId, nodeName, nodeAddress, nodePort);
                }
            }
        }
    }
    
    qDebug() << "从配置文件加载设置:" << configFile;
    return true;
}

bool MonitoringIntegration::saveConfig(const QString &configFile) const
{
    QSettings settings(configFile, QSettings::IniFormat);
    
    // 保存基本设置
    settings.setValue("General/Mode", m_mode);
    settings.setValue("Server/Address", m_serverAddress.toString());
    settings.setValue("Server/Port", m_serverPort);
    
    // 保存采样设置
    if (m_adaptiveSampler) {
        settings.setValue("Sampling/Interval", m_adaptiveSampler->getBaseSamplingInterval());
        settings.setValue("Sampling/Strategy", m_adaptiveSampler->getSamplingStrategy());
        settings.setValue("Sampling/Compression", m_adaptiveSampler->getCompressionAlgorithm());
    }
    
    // 保存远程节点设置（服务器模式）
    if (m_mode == ServerMode && m_distributedMonitor) {
        QVector<MonitorNode> nodes = m_distributedMonitor->getAllNodes();
        settings.setValue("Nodes/Count", nodes.size());
        
        for (int i = 0; i < nodes.size(); ++i) {
            const MonitorNode &node = nodes[i];
            QString prefix = QString("Nodes/Node%1/").arg(i);
            settings.setValue(prefix + "Id", node.nodeId);
            settings.setValue(prefix + "Name", node.nodeName);
            settings.setValue(prefix + "Address", node.address.toString());
            settings.setValue(prefix + "Port", node.port);
        }
    }
    
    qDebug() << "保存设置到配置文件:" << configFile;
    return true;
}