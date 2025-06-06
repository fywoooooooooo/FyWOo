#pragma once

#include <QObject>
#include <QTimer>
#include <QVector>
#include <QPair>
#include <QDateTime>
#include <QNetworkInterface>
#include <QProcess>
#include <QMessageBox>

#include "src/include/storage/datastorage.h"

class Sampler : public QObject {
    Q_OBJECT
public:
    explicit Sampler(QObject *parent = nullptr);
    ~Sampler();

    // ������ֹͣ������
    void start();
    void stop();
    
    // ���ò������ (����)
    void setInterval(int msec);
    
    // ��ȡ��ǰ�������
    int interval() const;
    
    // ����/�ر����ݴ洢
    void enableDataStorage(bool enable);
    
    // �����Ƿ��¼��ʷ����
    void setLoggingEnabled(bool enabled);
    
    // ��ȡ��ǰ״̬�Ƿ���������
    bool isRunning() const;
    
    // ��ȡ�������Դʹ����
    double getCurrentCpuUsage() const;
    quint64 getTotalMemory() const;
    quint64 getUsedMemory() const;
    quint64 getFreeMemory() const;
    double getCurrentNetworkUpload() const; // ��λ: MB/s
    double getCurrentNetworkDownload() const; // ��λ: MB/s

public slots:
    // ����������Դʹ�����
    void sampleAll();
    
    // GPU���Ի���
    void showGpuNotFoundDialog();

signals:
    // ����CPUʹ�����ź�
    void cpuUsageUpdated(double usage);
    
    // �����ڴ�ͳ���ź�
    void memoryStatsUpdated(quint64 total, quint64 used, quint64 free);
    
    // ��������״̬�ź�
    void networkStatsUpdated(double uploadSpeed, double downloadSpeed);
    
    // ���´���״̬�ź�
    void diskStatsUpdated(qint64 readBytes, qint64 writeBytes);
    
    // ����GPU״̬�ź�
    void gpuStatsUpdated(double usage, double temperature, quint64 memoryUsed, quint64 memoryTotal);
    
    // GPU�����Ա仯�ź�
    void gpuAvailabilityChanged(bool available, const QString& gpuName, const QString& driverVersion);
    
    // �����������ݸ����ź�
    void performanceDataUpdated(double cpuUsage, double memoryUsage, double diskIO, double networkUsage);

private:
    // ����CPUʹ����
    void sampleCpuUsage();
    
    // �����ڴ�ʹ�����
    void sampleMemoryUsage();
    
    // ��������ʹ�����
    void sampleNetworkUsage();
    
    // ��������ʹ�����
    void sampleDiskUsage();
    
    // ����GPUʹ�����
    void sampleGpuUsage();
    
    // ���GPU������
    void checkGpuAvailability();
    
    // ��ʼ����ʱ��
    void initTimer();
    
    // �洢����
    void storeData();
    
    // �洢�������ݵ����ݿ�
    void storePerformanceData();
    
    // ��ȡϵͳ��Ϣ
    void getSystemInfo();
    
    // ��ʱ��
    QTimer *m_timer;
    
    // �Ƿ���������
    bool m_running;
    
    // ������� (����)
    int m_interval;
    
    // �Ƿ��������ݴ洢
    bool m_storageEnabled;
    
    // �Ƿ��¼��ʷ����
    bool m_loggingEnabled;
    
    // CPUʹ����
    double m_cpuUsage;
    quint64 m_lastTotalCpuTime;
    quint64 m_lastIdleCpuTime;
    
    // �ڴ�ʹ�����
    quint64 m_totalMemory;
    quint64 m_usedMemory;
    quint64 m_freeMemory;
    
    // ����ʹ�����
    quint64 m_lastReceivedBytes;
    quint64 m_lastSentBytes;
    QDateTime m_lastNetworkSampleTime;
    double m_uploadSpeed; // ��λ: MB/s
    double m_downloadSpeed; // ��λ: MB/s
    
    // ����ʹ�����
    quint64 m_lastDiskReadBytes;
    quint64 m_lastDiskWriteBytes;
    QDateTime m_lastDiskSampleTime;
    double m_diskReadSpeed; // ��λ: MB/s
    double m_diskWriteSpeed; // ��λ: MB/s
    
    // GPU�����Ϣ
    bool m_gpuAvailable;
    double m_gpuUsage;
    double m_gpuTemperature;
    quint64 m_gpuMemoryUsed;
    quint64 m_gpuMemoryTotal;
    QString m_gpuName;
    QString m_gpuDriverVersion;
    
    // ���ݴ洢����
    DataStorage *m_storage;
}; 