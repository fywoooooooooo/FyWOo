#include "src/include/distributed/monitornode.h"
#include <QDebug>
#include <QNetworkInterface>
#include <QProcess>
#include <QSysInfo>

MonitorNode::MonitorNode(QObject *parent)
    : QObject(parent)
    , m_socket(new QTcpSocket(this))
    , m_samplingTimer(new QTimer(this))
    , m_samplingInterval(5000) // 默认5秒
    , m_state(Disconnected)
    , m_isSampling(false)
{
    // 连接信号槽
    connect(m_socket, &QTcpSocket::connected, this, &MonitorNode::handleConnected);
    connect(m_socket, &QTcpSocket::readyRead, this, &MonitorNode::handleDataReceived);
    connect(m_socket, QOverload<QAbstractSocket::SocketError>::of(&QTcpSocket::error),
            this, &MonitorNode::handleError);
    connect(m_samplingTimer, &QTimer::timeout, this, &MonitorNode::sendMetricsToServer);
    
    // 设置默认节点信息
    m_nodeId = QSysInfo::machineHostName() + "-" + QString::number(qrand() % 10000);
    m_nodeName = QSysInfo::prettyProductName();
}

MonitorNode::~MonitorNode()
{
    // 停止采样
    stopSampling();
    
    // 断开连接
    disconnectFromServer();
}

void MonitorNode::setNodeInfo(const QString &nodeId, const QString &nodeName)
{
    m_nodeId = nodeId;
    m_nodeName = nodeName;
    qDebug() << "设置节点信息:" << nodeId << nodeName;
}

bool MonitorNode::connectToServer(const QHostAddress &serverAddress, quint16 serverPort)
{
    // 如果已连接，先断开
    if (m_socket->state() == QAbstractSocket::ConnectedState) {
        disconnectFromServer();
    }
    
    // 更新状态
    m_state = Connecting;
    emit stateChanged(m_state);
    
    // 连接到服务器
    m_socket->connectToHost(serverAddress, serverPort);
    qDebug() << "正在连接到监控服务器:" << serverAddress.toString() << ":" << serverPort;
    
    // 等待连接建立
    return m_socket->waitForConnected(5000);
}

void MonitorNode::disconnectFromServer()
{
    // 如果已连接，断开连接
    if (m_socket->state() == QAbstractSocket::ConnectedState) {
        m_socket->disconnectFromHost();
    }
    
    // 更新状态
    m_state = Disconnected;
    emit stateChanged(m_state);
    qDebug() << "已断开与监控服务器的连接";
}

MonitorNode::NodeState MonitorNode::getState() const
{
    return m_state;
}

void MonitorNode::setSamplingInterval(int msec)
{
    if (msec >= 1000) { // 最小1秒
        m_samplingInterval = msec;
        
        // 如果定时器正在运行，重新启动
        if (m_samplingTimer->isActive()) {
            m_samplingTimer->stop();
            m_samplingTimer->start(m_samplingInterval);
        }
        
        qDebug() << "设置采样间隔为" << msec << "毫秒";
    }
}

int MonitorNode::getSamplingInterval() const
{
    return m_samplingInterval;
}

void MonitorNode::addMetric(const QString &metricName, double value)
{
    m_metrics[metricName] = value;
}

void MonitorNode::removeMetric(const QString &metricName)
{
    m_metrics.remove(metricName);
}

QMap<QString, double> MonitorNode::getAllMetrics() const
{
    return m_metrics;
}

void MonitorNode::startSampling()
{
    if (!m_isSampling) {
        // 收集初始系统指标
        collectSystemMetrics();
        
        // 启动定时器
        m_samplingTimer->start(m_samplingInterval);
        m_isSampling = true;
        qDebug() << "开始数据采集";
        
        // 立即发送一次数据
        sendMetricsToServer();
    }
}

void MonitorNode::stopSampling()
{
    if (m_isSampling) {
        // 停止定时器
        m_samplingTimer->stop();
        m_isSampling = false;
        qDebug() << "停止数据采集";
    }
}

bool MonitorNode::isSampling() const
{
    return m_isSampling;
}

void MonitorNode::handleConnected()
{
    // 更新状态
    m_state = Connected;
    emit stateChanged(m_state);
    qDebug() << "已连接到监控服务器";
    
    // 发送节点信息
    QJsonObject infoObj;
    infoObj["command"] = "registerNode";
    infoObj["nodeId"] = m_nodeId;
    infoObj["nodeName"] = m_nodeName;
    infoObj["osInfo"] = QSysInfo::prettyProductName();
    infoObj["timestamp"] = QDateTime::currentDateTime().toString(Qt::ISODate);
    
    QJsonDocument infoDoc(infoObj);
    QByteArray infoData = infoDoc.toJson();
    
    m_socket->write(infoData);
    m_socket->flush();
}

void MonitorNode::handleError(QAbstractSocket::SocketError error)
{
    // 更新状态
    m_state = Error;
    emit stateChanged(m_state);
    qDebug() << "连接错误:" << m_socket->errorString() << "错误代码:" << error;
}

void MonitorNode::handleDataReceived()
{
    // 读取所有可用数据
    QByteArray data = m_socket->readAll();
    
    // 解析JSON数据
    QJsonParseError error;
    QJsonDocument doc = QJsonDocument::fromJson(data, &error);
    
    if (error.error != QJsonParseError::NoError) {
        qDebug() << "JSON解析错误:" << error.errorString();
        return;
    }
    
    if (!doc.isObject()) {
        qDebug() << "JSON不是对象";
        return;
    }
    
    QJsonObject obj = doc.object();
    
    // 检查是否包含命令
    if (!obj.contains("command")) {
        qDebug() << "JSON缺少命令字段";
        return;
    }
    
    QString command = obj["command"].toString();
    QJsonObject params;
    
    if (obj.contains("params") && obj["params"].isObject()) {
        params = obj["params"].toObject();
    }
    
    // 处理命令
    if (command == "getMetrics") {
        // 服务器请求指标数据，立即发送
        sendMetricsToServer();
    } else if (command == "setSamplingInterval") {
        // 设置采样间隔
        if (params.contains("interval")) {
            int interval = params["interval"].toInt();
            setSamplingInterval(interval);
        }
    } else if (command == "startSampling") {
        // 开始采样
        startSampling();
    } else if (command == "stopSampling") {
        // 停止采样
        stopSampling();
    }
    
    // 发出命令接收信号
    emit commandReceived(command, params);
}

void MonitorNode::sendMetricsToServer()
{
    // 如果未连接，不发送数据
    if (m_state != Connected) {
        return;
    }
    
    // 更新系统指标
    collectSystemMetrics();
    
    // 创建指标数据JSON
    QJsonObject metricsObj;
    metricsObj["command"] = "metrics";
    metricsObj["nodeId"] = m_nodeId;
    metricsObj["timestamp"] = QDateTime::currentDateTime().toString(Qt::ISODate);
    metricsObj["metrics"] = createMetricsJson();
    
    QJsonDocument metricsDoc(metricsObj);
    QByteArray metricsData = metricsDoc.toJson();
    
    // 发送数据
    m_socket->write(metricsData);
    m_socket->flush();
}

void MonitorNode::collectSystemMetrics()
{
#ifdef Q_OS_WINDOWS
    // Windows系统下获取CPU使用率
    QProcess process;
    process.start("wmic", QStringList() << "cpu" << "get" << "loadpercentage");
    process.waitForFinished();
    QString output = process.readAllStandardOutput();
    QStringList lines = output.split("\n", QString::SkipEmptyParts);
    if (lines.size() >= 2) {
        bool ok;
        int cpuUsage = lines[1].trimmed().toInt(&ok);
        if (ok) {
            addMetric("cpu", cpuUsage);
        }
    }
    
    // 获取内存使用率
    process.start("wmic", QStringList() << "OS" << "get" << "FreePhysicalMemory,TotalVisibleMemorySize");
    process.waitForFinished();
    output = process.readAllStandardOutput();
    lines = output.split("\n", QString::SkipEmptyParts);
    if (lines.size() >= 2) {
        QStringList values = lines[1].trimmed().split(QRegExp("\\s+"));
        if (values.size() >= 2) {
            bool ok1, ok2;
            qint64 freeMemory = values[0].toLongLong(&ok1);
            qint64 totalMemory = values[1].toLongLong(&ok2);
            if (ok1 && ok2 && totalMemory > 0) {
                double memoryUsage = 100.0 * (1.0 - static_cast<double>(freeMemory) / totalMemory);
                addMetric("memory", memoryUsage);
            }
        }
    }
    
    // 获取磁盘使用率
    process.start("wmic", QStringList() << "logicaldisk" << "get" << "size,freespace,caption");
    process.waitForFinished();
    output = process.readAllStandardOutput();
    lines = output.split("\n", QString::SkipEmptyParts);
    double totalDiskUsage = 0.0;
    int diskCount = 0;
    
    for (int i = 1; i < lines.size(); ++i) {
        QStringList values = lines[i].trimmed().split(QRegExp("\\s+"));
        if (values.size() >= 3) {
            bool ok1, ok2;
            qint64 freeSpace = values[1].toLongLong(&ok1);
            qint64 totalSize = values[2].toLongLong(&ok2);
            if (ok1 && ok2 && totalSize > 0) {
                double diskUsage = 100.0 * (1.0 - static_cast<double>(freeSpace) / totalSize);
                totalDiskUsage += diskUsage;
                diskCount++;
                addMetric("disk." + values[0], diskUsage);
            }
        }
    }
    
    if (diskCount > 0) {
        addMetric("disk", totalDiskUsage / diskCount);
    }
    
    // 获取网络接口信息
    QList<QNetworkInterface> interfaces = QNetworkInterface::allInterfaces();
    for (const QNetworkInterface &interface : interfaces) {
        if (interface.flags().testFlag(QNetworkInterface::IsUp) && 
            !interface.flags().testFlag(QNetworkInterface::IsLoopBack)) {
            addMetric("network." + interface.name() + ".active", 1.0);
            
            // 这里只能获取接口是否活跃，无法直接获取流量
            // 实际应用中可以使用系统特定API获取网络流量
        }
    }
#else
    // 非Windows系统下的简化实现
    // 在实际应用中，应该根据不同操作系统使用相应的API获取系统指标
    
    // 添加一些模拟数据
    addMetric("cpu", qrand() % 100);
    addMetric("memory", qrand() % 100);
    addMetric("disk", qrand() % 100);
    addMetric("network", qrand() % 1000);
#endif
}

QJsonObject MonitorNode::createMetricsJson() const
{
    QJsonObject metricsObj;
    
    // 将所有指标添加到JSON对象
    for (auto it = m_metrics.begin(); it != m_metrics.end(); ++it) {
        metricsObj[it.key()] = it.value();
    }
    
    return metricsObj;
}