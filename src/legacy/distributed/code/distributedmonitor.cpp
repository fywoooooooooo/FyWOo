#include "src/include/distributed/distributedmonitor.h"
#include <QDebug>
#include <QJsonArray>
#include <QJsonObject>
#include <QJsonDocument>
#include <QDateTime>
#include <QFile>
#include <QTextStream>
#include <algorithm>
#include <numeric>

DistributedMonitor::DistributedMonitor(QObject *parent)
    : QObject(parent)
    , m_server(new QTcpServer(this))
    , m_samplingTimer(new QTimer(this))
    , m_samplingInterval(5000) // 默认5秒采样间隔
    , m_isMonitoring(false)
    , m_performanceAnalyzer(nullptr)
    , m_dataStorage(nullptr)
{
    // 连接信号槽
    connect(m_server, &QTcpServer::newConnection, this, &DistributedMonitor::handleNewConnection);
    connect(m_samplingTimer, &QTimer::timeout, this, &DistributedMonitor::collectData);
}

DistributedMonitor::~DistributedMonitor()
{
    // 停止监控
    stopMonitoring();
    
    // 断开所有连接
    for (auto it = m_nodeConnections.begin(); it != m_nodeConnections.end(); ++it) {
        if (it.value() && it.value()->isOpen()) {
            it.value()->close();
            it.value()->deleteLater();
        }
    }
    m_nodeConnections.clear();
    
    // 关闭服务器
    if (m_server->isListening()) {
        m_server->close();
    }
}

bool DistributedMonitor::initServer(quint16 port)
{
    // 如果服务器已在监听，先关闭
    if (m_server->isListening()) {
        m_server->close();
    }
    
    // 启动服务器
    bool success = m_server->listen(QHostAddress::Any, port);
    if (success) {
        qDebug() << "分布式监控服务器已启动，监听端口:" << port;
    } else {
        qDebug() << "分布式监控服务器启动失败:" << m_server->errorString();
    }
    
    return success;
}

bool DistributedMonitor::addNode(const QString &nodeId, const QString &nodeName, const QHostAddress &address, quint16 port)
{
    // 检查节点ID是否已存在
    if (m_nodes.contains(nodeId)) {
        qDebug() << "节点ID已存在:" << nodeId;
        return false;
    }
    
    // 创建新节点
    MonitorNode node;
    node.nodeId = nodeId;
    node.nodeName = nodeName;
    node.address = address;
    node.port = port;
    node.isActive = false;
    node.lastSeen = QDateTime::currentDateTime();
    
    // 添加到节点列表
    m_nodes[nodeId] = node;
    qDebug() << "添加监控节点:" << nodeName << "(" << nodeId << ")" << address.toString() << ":" << port;
    
    return true;
}

bool DistributedMonitor::removeNode(const QString &nodeId)
{
    // 检查节点是否存在
    if (!m_nodes.contains(nodeId)) {
        qDebug() << "节点不存在:" << nodeId;
        return false;
    }
    
    // 如果有连接，先断开
    if (m_nodeConnections.contains(nodeId)) {
        QTcpSocket *socket = m_nodeConnections[nodeId];
        if (socket) {
            if (socket->isOpen()) {
                socket->close();
            }
            socket->deleteLater();
            m_nodeConnections.remove(nodeId);
        }
    }
    
    // 从节点列表中移除
    m_nodes.remove(nodeId);
    qDebug() << "移除监控节点:" << nodeId;
    
    return true;
}

DistributedMonitor::NodeStatus DistributedMonitor::connectToNode(const QString &nodeId)
{
    // 检查节点是否存在
    if (!m_nodes.contains(nodeId)) {
        qDebug() << "节点不存在:" << nodeId;
        return Failed;
    }
    
    // 如果已经有连接，先检查状态
    if (m_nodeConnections.contains(nodeId)) {
        QTcpSocket *socket = m_nodeConnections[nodeId];
        if (socket) {
            if (socket->state() == QAbstractSocket::ConnectedState) {
                return Connected;
            } else if (socket->state() == QAbstractSocket::ConnectingState) {
                return Connecting;
            } else {
                // 连接已断开，删除旧连接
                socket->deleteLater();
                m_nodeConnections.remove(nodeId);
            }
        }
    }
    
    // 创建新连接
    QTcpSocket *socket = new QTcpSocket(this);
    
    // 连接信号槽
    connect(socket, &QTcpSocket::readyRead, this, &DistributedMonitor::handleNodeData);
    connect(socket, QOverload<QAbstractSocket::SocketError>::of(&QTcpSocket::error),
            this, &DistributedMonitor::handleConnectionError);
    
    // 获取节点信息
    const MonitorNode &node = m_nodes[nodeId];
    
    // 连接到节点
    socket->connectToHost(node.address, node.port);
    
    // 存储连接
    m_nodeConnections[nodeId] = socket;
    
    // 更新节点状态
    emit nodeStatusChanged(nodeId, Connecting);
    
    // 等待连接建立
    if (socket->waitForConnected(3000)) {
        // 更新节点状态
        MonitorNode &updatedNode = m_nodes[nodeId];
        updatedNode.isActive = true;
        updatedNode.lastSeen = QDateTime::currentDateTime();
        
        emit nodeStatusChanged(nodeId, Connected);
        qDebug() << "已连接到节点:" << node.nodeName << "(" << nodeId << ")";
        return Connected;
    } else {
        emit nodeStatusChanged(nodeId, Failed);
        qDebug() << "连接节点失败:" << node.nodeName << "(" << nodeId << ")" << socket->errorString();
        return Failed;
    }
}

void DistributedMonitor::disconnectFromNode(const QString &nodeId)
{
    // 检查是否有连接
    if (m_nodeConnections.contains(nodeId)) {
        QTcpSocket *socket = m_nodeConnections[nodeId];
        if (socket) {
            if (socket->isOpen()) {
                socket->close();
            }
            socket->deleteLater();
            m_nodeConnections.remove(nodeId);
        }
        
        // 更新节点状态
        if (m_nodes.contains(nodeId)) {
            MonitorNode &node = m_nodes[nodeId];
            node.isActive = false;
            emit nodeStatusChanged(nodeId, Disconnected);
            qDebug() << "已断开与节点的连接:" << node.nodeName << "(" << nodeId << ")";
        }
    }
}

QVector<MonitorNode> DistributedMonitor::getAllNodes() const
{
    QVector<MonitorNode> nodes;
    for (auto it = m_nodes.begin(); it != m_nodes.end(); ++it) {
        nodes.append(it.value());
    }
    return nodes;
}

QVector<MonitorNode> DistributedMonitor::getActiveNodes() const
{
    QVector<MonitorNode> activeNodes;
    for (auto it = m_nodes.begin(); it != m_nodes.end(); ++it) {
        if (it.value().isActive) {
            activeNodes.append(it.value());
        }
    }
    return activeNodes;
}

QMap<QString, double> DistributedMonitor::getNodeMetrics(const QString &nodeId) const
{
    if (m_nodes.contains(nodeId)) {
        return m_nodes[nodeId].lastMetrics;
    }
    return QMap<QString, double>();
}

double DistributedMonitor::getAggregatedMetric(const QString &metricName, AggregationType type) const
{
    QVector<double> values;
    
    // 收集所有活跃节点的指定指标值
    for (auto it = m_nodes.begin(); it != m_nodes.end(); ++it) {
        if (it.value().isActive && it.value().lastMetrics.contains(metricName)) {
            values.append(it.value().lastMetrics[metricName]);
        }
    }
    
    // 计算聚合值
    return calculateAggregatedValue(values, type);
}

void DistributedMonitor::setSamplingInterval(int msec)
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

int DistributedMonitor::getSamplingInterval() const
{
    return m_samplingInterval;
}

void DistributedMonitor::setPerformanceAnalyzer(PerformanceAnalyzer *analyzer)
{
    m_performanceAnalyzer = analyzer;
}

void DistributedMonitor::setDataStorage(DataStorage *storage)
{
    m_dataStorage = storage;
}

void DistributedMonitor::startMonitoring()
{
    if (!m_isMonitoring) {
        // 启动定时器
        m_samplingTimer->start(m_samplingInterval);
        m_isMonitoring = true;
        qDebug() << "开始分布式监控";
        
        // 立即请求一次数据
        requestDataFromAllNodes();
    }
}

void DistributedMonitor::stopMonitoring()
{
    if (m_isMonitoring) {
        // 停止定时器
        m_samplingTimer->stop();
        m_isMonitoring = false;
        qDebug() << "停止分布式监控";
    }
}

bool DistributedMonitor::isMonitoring() const
{
    return m_isMonitoring;
}

bool DistributedMonitor::exportMonitoringReport(const QString &filePath) const
{
    QFile file(filePath);
    if (!file.open(QIODevice::WriteOnly | QIODevice::Text)) {
        qDebug() << "无法打开文件进行写入:" << filePath;
        return false;
    }
    
    QTextStream out(&file);
    out.setCodec("UTF-8");
    
    // 写入报告头
    out << "分布式监控报告\n";
    out << "生成时间: " << QDateTime::currentDateTime().toString("yyyy-MM-dd HH:mm:ss") << "\n\n";
    
    // 写入节点信息
    out << "监控节点信息:\n";
    out << "--------------------\n";
    for (auto it = m_nodes.begin(); it != m_nodes.end(); ++it) {
        const MonitorNode &node = it.value();
        out << "节点ID: " << node.nodeId << "\n";
        out << "节点名称: " << node.nodeName << "\n";
        out << "IP地址: " << node.address.toString() << "\n";
        out << "端口: " << node.port << "\n";
        out << "状态: " << (node.isActive ? "活跃" : "离线") << "\n";
        out << "最后通信时间: " << node.lastSeen.toString("yyyy-MM-dd HH:mm:ss") << "\n";
        
        // 写入最新指标
        if (!node.lastMetrics.isEmpty()) {
            out << "最新指标:\n";
            for (auto metricIt = node.lastMetrics.begin(); metricIt != node.lastMetrics.end(); ++metricIt) {
                out << "  " << metricIt.key() << ": " << metricIt.value() << "\n";
            }
        }
        out << "--------------------\n";
    }
    
    // 写入聚合指标
    out << "\n聚合指标:\n";
    out << "--------------------\n";
    QStringList metricNames;
    
    // 收集所有指标名称
    for (auto it = m_nodes.begin(); it != m_nodes.end(); ++it) {
        const MonitorNode &node = it.value();
        for (auto metricIt = node.lastMetrics.begin(); metricIt != node.lastMetrics.end(); ++metricIt) {
            if (!metricNames.contains(metricIt.key())) {
                metricNames.append(metricIt.key());
            }
        }
    }
    
    // 计算并写入每个指标的聚合值
    for (const QString &metricName : metricNames) {
        out << metricName << ":\n";
        out << "  平均值: " << getAggregatedMetric(metricName, Average) << "\n";
        out << "  最大值: " << getAggregatedMetric(metricName, Maximum) << "\n";
        out << "  最小值: " << getAggregatedMetric(metricName, Minimum) << "\n";
        out << "  中位数: " << getAggregatedMetric(metricName, Median) << "\n";
        out << "  总和: " << getAggregatedMetric(metricName, Sum) << "\n";
        out << "--------------------\n";
    }
    
    file.close();
    qDebug() << "分布式监控报告已导出到:" << filePath;
    return true;
}

void DistributedMonitor::requestDataFromAllNodes()
{
    // 遍历所有活跃节点，请求数据
    for (auto it = m_nodes.begin(); it != m_nodes.end(); ++it) {
        if (it.value().isActive) {
            requestDataFromNode(it.key());
        }
    }
}

void DistributedMonitor::requestDataFromNode(const QString &nodeId)
{
    // 检查节点是否存在且活跃
    if (!m_nodes.contains(nodeId) || !m_nodes[nodeId].isActive) {
        return;
    }
    
    // 检查是否有连接
    if (!m_nodeConnections.contains(nodeId) || !m_nodeConnections[nodeId]) {
        return;
    }
    
    QTcpSocket *socket = m_nodeConnections[nodeId];
    if (!socket->isOpen()) {
        return;
    }
    
    // 创建请求数据的JSON
    QJsonObject requestObj;
    requestObj["command"] = "getMetrics";
    requestObj["timestamp"] = QDateTime::currentDateTime().toString(Qt::ISODate);
    
    QJsonDocument requestDoc(requestObj);
    QByteArray requestData = requestDoc.toJson();
    
    // 发送请求
    socket->write(requestData);
    socket->flush();
}

void DistributedMonitor::handleNewConnection()
{
    // 接受新连接
    QTcpSocket *clientSocket = m_server->nextPendingConnection();
    if (!clientSocket) {
        return;
    }
    
    // 连接信号槽
    connect(clientSocket, &QTcpSocket::readyRead, this, &DistributedMonitor::handleNodeData);
    connect(clientSocket, QOverload<QAbstractSocket::SocketError>::of(&QTcpSocket::error),
            this, &DistributedMonitor::handleConnectionError);
    
    qDebug() << "新的客户端连接:" << clientSocket->peerAddress().toString() << ":" << clientSocket->peerPort();
    
    // 注意：此时我们还不知道这是哪个节点，需要等待节点发送标识信息
}

void DistributedMonitor::handleNodeData()
{
    // 获取发送数据的socket
    QTcpSocket *socket = qobject_cast<QTcpSocket*>(sender());
    if (!socket) {
        return;
    }
    
    // 读取所有可用数据
    QByteArray data = socket->readAll();
    
    // 解析数据
    QString nodeId;
    QMap<QString, double> metrics;
    if (parseNodeData(data, nodeId, metrics)) {
        // 检查节点是否已知
        if (!m_nodes.contains(nodeId)) {
            // 未知节点，可能是自动发现的节点
            qDebug() << "收到未知节点的数据:" << nodeId;
            return;
        }
        
        // 更新节点信息
        MonitorNode &node = m_nodes[nodeId];
        node.isActive = true;
        node.lastSeen = QDateTime::currentDateTime();
        node.lastMetrics = metrics;
        
        // 存储数据
        storeNodeData(nodeId, metrics);
        
        // 发出信号
        emit newDataReceived(nodeId, metrics);
        
        // 检查异常
        for (auto it = metrics.begin(); it != metrics.end(); ++it) {
            // 这里可以添加简单的阈值检查
            if (it.key().contains("cpu", Qt::CaseInsensitive) && it.value() > 90.0) {
                emit anomalyDetected(nodeId, it.key(), it.value());
            } else if (it.key().contains("memory", Qt::CaseInsensitive) && it.value() > 90.0) {
                emit anomalyDetected(nodeId, it.key(), it.value());
            } else if (it.key().contains("disk", Qt::CaseInsensitive) && it.value() > 95.0) {
                emit anomalyDetected(nodeId, it.key(), it.value());
            }
        }
    }
}

void DistributedMonitor::handleConnectionError(QAbstractSocket::SocketError error)
{
    QTcpSocket *socket = qobject_cast<QTcpSocket*>(sender());
    if (!socket) {
        return;
    }
    
    qDebug() << "连接错误:" << socket->errorString() << "错误代码:" << error;
    
    // 查找对应的节点ID
    QString nodeId;
    for (auto it = m_nodeConnections.begin(); it != m_nodeConnections.end(); ++it) {
        if (it.value() == socket) {
            nodeId = it.key();
            break;
        }
    }
    
    if (!nodeId.isEmpty()) {
        // 更新节点状态
        if (m_nodes.contains(nodeId)) {
            MonitorNode &node = m_nodes[nodeId];
            node.isActive = false;
            emit nodeStatusChanged(nodeId, Failed);
        }
        
        // 移除连接
        m_nodeConnections.remove(nodeId);
    }
    
    // 关闭并删除socket
    socket->close();
    socket->deleteLater();
}

void DistributedMonitor::collectData()
{
    // 请求所有节点的数据
    requestDataFromAllNodes();
}

bool DistributedMonitor::parseNodeData(const QByteArray &data, QString &nodeId, QMap<QString, double> &metrics)
{
    QJsonParseError error;
    QJsonDocument doc = QJsonDocument::fromJson(data, &error);
    
    if (error.error != QJsonParseError::NoError) {
        qDebug() << "JSON解析错误:" << error.errorString();
        return false;
    }
    
    if (!doc.isObject()) {
        qDebug() << "JSON不是对象";
        return false;
    }
    
    QJsonObject obj = doc.object();
    
    // 检查必要字段
    if (!obj.contains("nodeId") || !obj.contains("metrics")) {
        qDebug() << "JSON缺少必要字段";
        return false;
    }
    
    nodeId = obj["nodeId"].toString();
    
    // 解析指标数据
    QJsonObject metricsObj = obj["metrics"].toObject();
    for (auto it = metricsObj.begin(); it != metricsObj.end(); ++it) {
        metrics[it.key()] = it.value().toDouble();
    }
    
    return true;
}

double DistributedMonitor::calculateAggregatedValue(const QVector<double> &values, AggregationType type) const
{
    if (values.isEmpty()) {
        return 0.0;
    }
    
    switch (type) {
        case Sum: {
            return std::accumulate(values.begin(), values.end(), 0.0);
        }
        case Average: {
            return std::accumulate(values.begin(), values.end(), 0.0) / values.size();
        }
        case Maximum: {
            return *std::max_element(values.begin(), values.end());
        }
        case Minimum: {
            return *std::min_element(values.begin(), values.end());
        }
        case Median: {
            QVector<double> sortedValues = values;
            std::sort(sortedValues.begin(), sortedValues.end());
            
            if (sortedValues.size() % 2 == 0) {
                // 偶数个元素，取中间两个的平均值
                int middle = sortedValues.size() / 2;
                return (sortedValues[middle - 1] + sortedValues[middle]) / 2.0;
            } else {
                // 奇数个元素，取中间值
                return sortedValues[sortedValues.size() / 2];
            }
        }
        default:
            return 0.0;
    }
}

void DistributedMonitor::storeNodeData(const QString &nodeId, const QMap<QString, double> &metrics)
{
    if (!m_dataStorage) {
        return;
    }
    
    QDateTime timestamp = QDateTime::currentDateTime();
    
    // 存储各项指标
    for (auto it = metrics.begin(); it != metrics.end(); ++it) {
        QString metricKey = nodeId + "." + it.key(); // 使用节点ID作为前缀
        m_dataStorage->storeSample(metricKey, it.value(), timestamp);
    }
    
    // 如果有性能分析器，可以添加数据点
    if (m_performanceAnalyzer) {
        // 这里可以根据需要添加到性能分析器
        if (metrics.contains("cpu")) {
            m_performanceAnalyzer->addDataPoint(
                metrics.value("cpu", 0.0),
                metrics.value("memory", 0.0),
                metrics.value("disk", 0.0),
                metrics.value("network", 0.0),
                timestamp
            );
        }
    }
}