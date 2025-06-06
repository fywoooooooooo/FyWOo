#pragma once

#include <QObject>
#include <QTcpSocket>
#include <QTimer>
#include <QHostAddress>
#include <QJsonObject>
#include <QJsonDocument>
#include <QDateTime>
#include <QMap>
#include <QString>

// 分布式监控节点类 - 作为客户端节点与监控管理器通信
class MonitorNode : public QObject {
    Q_OBJECT

public:
    // 节点状态枚举
    enum NodeState {
        Disconnected,   // 已断开连接
        Connecting,     // 正在连接
        Connected,      // 已连接
        Error           // 连接错误
    };

    explicit MonitorNode(QObject *parent = nullptr);
    ~MonitorNode();

    // 设置节点信息
    void setNodeInfo(const QString &nodeId, const QString &nodeName);
    
    // 连接到监控服务器
    bool connectToServer(const QHostAddress &serverAddress, quint16 serverPort);
    
    // 断开连接
    void disconnectFromServer();
    
    // 获取当前连接状态
    NodeState getState() const;
    
    // 设置数据采集间隔（毫秒）
    void setSamplingInterval(int msec);
    
    // 获取数据采集间隔
    int getSamplingInterval() const;
    
    // 添加指标数据
    void addMetric(const QString &metricName, double value);
    
    // 移除指标数据
    void removeMetric(const QString &metricName);
    
    // 获取所有指标数据
    QMap<QString, double> getAllMetrics() const;
    
    // 开始数据采集
    void startSampling();
    
    // 停止数据采集
    void stopSampling();
    
    // 是否正在采集数据
    bool isSampling() const;

signals:
    // 连接状态变化信号
    void stateChanged(NodeState state);
    
    // 收到服务器命令信号
    void commandReceived(const QString &command, const QJsonObject &params);

private slots:
    // 处理连接成功
    void handleConnected();
    
    // 处理连接错误
    void handleError(QAbstractSocket::SocketError error);
    
    // 处理接收到的数据
    void handleDataReceived();
    
    // 发送数据到服务器
    void sendMetricsToServer();

private:
    // 收集系统指标数据
    void collectSystemMetrics();
    
    // 创建指标数据JSON
    QJsonObject createMetricsJson() const;

private:
    QString m_nodeId;               // 节点ID
    QString m_nodeName;             // 节点名称
    QTcpSocket *m_socket;           // 网络连接
    QTimer *m_samplingTimer;        // 采样定时器
    int m_samplingInterval;         // 采样间隔（毫秒）
    NodeState m_state;              // 当前状态
    QMap<QString, double> m_metrics; // 指标数据
    bool m_isSampling;              // 是否正在采集数据
};