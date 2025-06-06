#pragma once

#include <QObject>
#include <QString>
#include <QHostAddress>
#include "src/include/distributed/distributedmonitor.h"
#include "src/include/distributed/monitornode.h"
#include "src/include/storage/adaptivesampler.h"
#include "src/include/analysis/performanceanalyzer.h"
#include "src/include/storage/datastorage.h"

// 监控集成类 - 展示如何集成分布式监控和自适应采样功能到应用中
class MonitoringIntegration : public QObject {
    Q_OBJECT

public:
    // 运行模式枚举
    enum RunMode {
        ServerMode,      // 服务器模式
        ClientMode,      // 客户端模式
        StandaloneMode   // 独立模式
    };

    explicit MonitoringIntegration(QObject *parent = nullptr);
    ~MonitoringIntegration();

    // 初始化监控系统
    bool initialize(RunMode mode, const QString &configFile = QString());
    
    // 启动监控
    bool start();
    
    // 停止监控
    void stop();
    
    // 是否正在运行
    bool isRunning() const;
    
    // 设置服务器地址（客户端模式）
    void setServerAddress(const QHostAddress &address, quint16 port);
    
    // 设置监听端口（服务器模式）
    void setServerPort(quint16 port);
    
    // 添加远程节点（服务器模式）
    bool addRemoteNode(const QString &nodeId, const QString &nodeName, const QHostAddress &address, quint16 port);
    
    // 设置采样策略
    void setSamplingStrategy(AdaptiveSampler::SamplingStrategy strategy);
    
    // 设置压缩算法
    void setCompressionAlgorithm(AdaptiveSampler::CompressionAlgorithm algorithm);
    
    // 设置采样间隔
    void setSamplingInterval(int msec);
    
    // 导出监控报告
    bool exportReport(const QString &filePath) const;

signals:
    // 状态变化信号
    void statusChanged(const QString &status);
    
    // 异常检测信号
    void anomalyDetected(const QString &source, const QString &metricName, double value);

private slots:
    // 处理分布式监控异常
    void handleDistributedAnomaly(const QString &nodeId, const QString &metricName, double value);
    
    // 处理节点状态变化
    void handleNodeStatusChanged(const QString &nodeId, DistributedMonitor::NodeStatus status);
    
    // 处理采样间隔变化
    void handleSamplingIntervalChanged(const QString &metricName, int newInterval);

private:
    // 从配置文件加载设置
    bool loadConfig(const QString &configFile);
    
    // 保存设置到配置文件
    bool saveConfig(const QString &configFile) const;

private:
    RunMode m_mode;                         // 运行模式
    bool m_isRunning;                       // 是否正在运行
    DistributedMonitor *m_distributedMonitor; // 分布式监控管理器
    MonitorNode *m_monitorNode;             // 监控节点
    AdaptiveSampler *m_adaptiveSampler;     // 自适应采样器
    PerformanceAnalyzer *m_performanceAnalyzer; // 性能分析器
    DataStorage *m_dataStorage;             // 数据存储
    RunMode m_runMode;                      // 运行模式
    QHostAddress m_serverAddress;           // 服务器地址
    quint16 m_serverPort;                   // 服务器端口
};