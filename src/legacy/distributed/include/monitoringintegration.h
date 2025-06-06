#pragma once

#include <QObject>
#include <QString>
#include <QHostAddress>
#include "src/include/distributed/distributedmonitor.h"
#include "src/include/distributed/monitornode.h"
#include "src/include/storage/adaptivesampler.h"
#include "src/include/analysis/performanceanalyzer.h"
#include "src/include/storage/datastorage.h"

// ��ؼ����� - չʾ��μ��ɷֲ�ʽ��غ�����Ӧ�������ܵ�Ӧ����
class MonitoringIntegration : public QObject {
    Q_OBJECT

public:
    // ����ģʽö��
    enum RunMode {
        ServerMode,      // ������ģʽ
        ClientMode,      // �ͻ���ģʽ
        StandaloneMode   // ����ģʽ
    };

    explicit MonitoringIntegration(QObject *parent = nullptr);
    ~MonitoringIntegration();

    // ��ʼ�����ϵͳ
    bool initialize(RunMode mode, const QString &configFile = QString());
    
    // �������
    bool start();
    
    // ֹͣ���
    void stop();
    
    // �Ƿ���������
    bool isRunning() const;
    
    // ���÷�������ַ���ͻ���ģʽ��
    void setServerAddress(const QHostAddress &address, quint16 port);
    
    // ���ü����˿ڣ�������ģʽ��
    void setServerPort(quint16 port);
    
    // ���Զ�̽ڵ㣨������ģʽ��
    bool addRemoteNode(const QString &nodeId, const QString &nodeName, const QHostAddress &address, quint16 port);
    
    // ���ò�������
    void setSamplingStrategy(AdaptiveSampler::SamplingStrategy strategy);
    
    // ����ѹ���㷨
    void setCompressionAlgorithm(AdaptiveSampler::CompressionAlgorithm algorithm);
    
    // ���ò������
    void setSamplingInterval(int msec);
    
    // ������ر���
    bool exportReport(const QString &filePath) const;

signals:
    // ״̬�仯�ź�
    void statusChanged(const QString &status);
    
    // �쳣����ź�
    void anomalyDetected(const QString &source, const QString &metricName, double value);

private slots:
    // ����ֲ�ʽ����쳣
    void handleDistributedAnomaly(const QString &nodeId, const QString &metricName, double value);
    
    // ����ڵ�״̬�仯
    void handleNodeStatusChanged(const QString &nodeId, DistributedMonitor::NodeStatus status);
    
    // �����������仯
    void handleSamplingIntervalChanged(const QString &metricName, int newInterval);

private:
    // �������ļ���������
    bool loadConfig(const QString &configFile);
    
    // �������õ������ļ�
    bool saveConfig(const QString &configFile) const;

private:
    RunMode m_mode;                         // ����ģʽ
    bool m_isRunning;                       // �Ƿ���������
    DistributedMonitor *m_distributedMonitor; // �ֲ�ʽ��ع�����
    MonitorNode *m_monitorNode;             // ��ؽڵ�
    AdaptiveSampler *m_adaptiveSampler;     // ����Ӧ������
    PerformanceAnalyzer *m_performanceAnalyzer; // ���ܷ�����
    DataStorage *m_dataStorage;             // ���ݴ洢
    RunMode m_runMode;                      // ����ģʽ
    QHostAddress m_serverAddress;           // ��������ַ
    quint16 m_serverPort;                   // �������˿�
};