#pragma once

#include <QObject>
#include <QMap>
#include <QVector>
#include <QDateTime>
#include <QString>
#include <QHostAddress>
#include <QTcpSocket>
#include <QTcpServer>
#include <QJsonObject>
#include <QJsonDocument>
#include <QTimer>
#include "src/include/analysis/performanceanalyzer.h"
#include "src/include/storage/datastorage.h"

// 分布式监控节点信息
struct MonitorNode {
    QString nodeId;           // 节点唯一标识
    QString nodeName;         // 节点名称
    QHostAddress address;     // 节点IP地址
    quint16 port;            // 节点端口
    bool isActive;            // 节点是否活跃
    QDateTime lastSeen;       // 最后一次通信时间
    QMap<QString, double> lastMetrics;  // 最近一次的指标数据
};

// 分布式监控管理器 - 支持多节点数据采集和聚合分析
class DistributedMonitor : public QObject {
    Q_OBJECT

public:
    // 节点连接状态枚举
    enum NodeStatus {
        Connected,      // 已连接
        Disconnected,   // 已断开
        Connecting,     // 连接中
        Failed          // 连接失败
    };
    
    // 数据聚合方式枚举
    enum AggregationType {
        Sum,            // 求和
        Average,        // 平均值
        Maximum,        // 最大值
        Minimum,        // 最小值
        Median          // 中位数
    };

    explicit DistributedMonitor(QObject *parent = nullptr);
    ~DistributedMonitor();

    // 初始化服务器
    bool initServer(quint16 port = 8765);
    
    // 添加监控节点
    bool addNode(const QString &nodeId, const QString &nodeName, const QHostAddress &address, quint16 port);
    
    // 移除监控节点
    bool removeNode(const QString &nodeId);
    
    // 连接到指定节点
    NodeStatus connectToNode(const QString &nodeId);
    
    // 断开与指定节点的连接
    void disconnectFromNode(const QString &nodeId);
    
    // 获取所有节点列表
    QVector<MonitorNode> getAllNodes() const;
    
    // 获取活跃节点列表
    QVector<MonitorNode> getActiveNodes() const;
    
    // 获取指定节点的最新数据
    QMap<QString, double> getNodeMetrics(const QString &nodeId) const;
    
    // 获取聚合数据
    double getAggregatedMetric(const QString &metricName, AggregationType type = Average) const;
    
    // 设置数据采集间隔（毫秒）
    void setSamplingInterval(int msec);
    
    // 获取当前数据采集间隔
    int getSamplingInterval() const;
    
    // 设置性能分析器
    void setPerformanceAnalyzer(PerformanceAnalyzer *analyzer);
    
    // 设置数据存储
    void setDataStorage(DataStorage *storage);
    
    // 开始监控
    void startMonitoring();
    
    // 停止监控
    void stopMonitoring();
    
    // 是否正在监控
    bool isMonitoring() const;
    
    // 导出分布式监控报告
    bool exportMonitoringReport(const QString &filePath) const;

signals:
    // 节点状态变化信号
    void nodeStatusChanged(const QString &nodeId, NodeStatus status);
    
    // 接收到新数据信号
    void newDataReceived(const QString &nodeId, const QMap<QString, double> &metrics);
    
    // 异常检测信号
    void anomalyDetected(const QString &nodeId, const QString &metricName, double value);

public slots:
    // 请求所有节点的最新数据
    void requestDataFromAllNodes();
    
    // 请求指定节点的最新数据
    void requestDataFromNode(const QString &nodeId);

private slots:
    // 处理新的连接
    void handleNewConnection();
    
    // 处理接收到的数据
    void handleNodeData();
    
    // 处理连接错误
    void handleConnectionError(QAbstractSocket::SocketError error);
    
    // 定时采集数据
    void collectData();

private:
    // 解析接收到的JSON数据
    bool parseNodeData(const QByteArray &data, QString &nodeId, QMap<QString, double> &metrics);
    
    // 计算聚合值
    double calculateAggregatedValue(const QVector<double> &values, AggregationType type) const;
    
    // 存储节点数据
    void storeNodeData(const QString &nodeId, const QMap<QString, double> &metrics);

private:
    QMap<QString, MonitorNode> m_nodes;         // 所有监控节点
    QMap<QString, QTcpSocket*> m_nodeConnections; // 节点连接
    QTcpServer *m_server;                       // TCP服务器
    QTimer *m_samplingTimer;                    // 采样定时器
    int m_samplingInterval;                     // 采样间隔（毫秒）
    bool m_isMonitoring;                        // 是否正在监控
    PerformanceAnalyzer *m_performanceAnalyzer; // 性能分析器
    DataStorage *m_dataStorage;                 // 数据存储
};