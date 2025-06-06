#pragma once
#include <QWidget>
#include <QVector>
#include <QQuickWidget>
#include <QQuickItem>

// 添加前向声明
class QLabel;
class QTimer;

class Vis3DPage : public QWidget {
    Q_OBJECT
public:
    explicit Vis3DPage(QWidget *parent = nullptr);
    ~Vis3DPage();

    // 更新3D场景数据
    void updateCpuData(double usage);
    void updateGpuData(double usage, double temperature, quint64 memoryUsed, quint64 memoryTotal);
    void updateMemoryData(double used, double total);
    void updateDiskData(double usage);
    void updateNetworkData(double usage);
    
    // 重置视图
    void resetCamera();
    
    // 设置不同视角
    void setTopView();
    void setFrontView();
    void setSideView();

private slots:
    void onDataUpdateTimer();

private:
    // QML相关
    QQuickWidget *m_quickWidget;
    QQuickItem *m_barsGraphQml;
    
    // 数据更新定时器
    QTimer *m_updateTimer;
    
    // 性能数据标签
    QLabel *m_cpuLabel;
    QLabel *m_gpuLabel;
    QLabel *m_memoryLabel;
    QLabel *m_diskLabel;
    QLabel *m_networkLabel;
    
    // 性能数据历史记录
    QVector<double> m_cpuHistory;
    QVector<double> m_gpuHistory;
    QVector<double> m_memoryHistory;
    QVector<double> m_diskHistory;
    QVector<double> m_networkHistory;
    
    // 当前性能数据
    double m_cpuUsage;
    double m_gpuUsage;
    double m_gpuTemp;
    quint64 m_gpuMemUsed;
    quint64 m_gpuMemTotal;
    double m_memoryUsage;
    double m_diskUsage;
    double m_networkUsage;
    
    // 常量定义
    static const int MAX_HISTORY_POINTS = 50;
    
    // 私有方法
    void setupUI();
    void updateEntities();
    
    // 辅助函数
    QString formatMemorySize(quint64 bytes);
};