#include "Vis3DPage.h"
#include <QVBoxLayout>
#include <QLabel>
#include <QDebug>
#include <QTimer>
#include <QGroupBox>
#include <QGridLayout>
#include <QPushButton>
#include <QHBoxLayout>
#include <QRandomGenerator>
#include <QQuickWidget>
#include <QQmlContext>
#include <QQmlEngine>

// 静态常量定义
const int Vis3DPage::MAX_HISTORY_POINTS;

Vis3DPage::Vis3DPage(QWidget *parent)
    : QWidget(parent)
    , m_quickWidget(nullptr)
    , m_barsGraphQml(nullptr)
    , m_cpuLabel(nullptr)
    , m_gpuLabel(nullptr)
    , m_memoryLabel(nullptr)
    , m_diskLabel(nullptr)
    , m_networkLabel(nullptr)
    // , m_updateTimer(nullptr) // m_updateTimer is no longer used directly for onDataUpdateTimer
    , m_cpuUsage(0.0)
    , m_gpuUsage(0.0)
    , m_gpuTemp(0.0)
    , m_gpuMemUsed(0)
    , m_gpuMemTotal(0)
    , m_memoryUsage(0.0)
    , m_diskUsage(0.0)
    , m_networkUsage(0.0)
{
    setupUI();
    
    // The QTimer that called onDataUpdateTimer is removed as data updates are now pushed.
    // If a general periodic update for updateEntities is still needed, it can be re-added here.
    // For now, assuming updateEntities is called by individual data update slots.
}

Vis3DPage::~Vis3DPage()
{
    // 清理资源
    // m_quickWidget 会被父QWidget自动删除
}

void Vis3DPage::setupUI()
{
    // 清除已有布局
    if (layout()) {
        delete layout();
    }
    
    // 创建主布局
    QVBoxLayout *mainLayout = new QVBoxLayout(this);
    mainLayout->setContentsMargins(0, 0, 0, 0);
    mainLayout->setSpacing(5);
    
    // 创建性能数据面板
    QGroupBox *dataPanel = new QGroupBox("性能数据监控", this);
    dataPanel->setStyleSheet(
        "QGroupBox { font-weight: bold; border: 1px solid #555555; border-radius: 4px; margin-top: 8px; padding-top: 8px; }" 
        "QGroupBox::title { subcontrol-origin: margin; left: 8px; padding: 0 4px; background-color: #f0f0f0; }"
    );
    
    // 创建QQuickWidget并设置QML源文件
    m_quickWidget = new QQuickWidget(this);
    m_quickWidget->setResizeMode(QQuickWidget::SizeRootObjectToView);
    m_quickWidget->setMinimumSize(500, 400);
    m_quickWidget->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
    
    // 注册QML文件
    m_quickWidget->setSource(QUrl::fromLocalFile("d:/Program/src/legacy/visualization3d/3dchart.qml"));
    
    // 获取QML根对象
    m_barsGraphQml = m_quickWidget->rootObject();
    
    // 添加到主布局
    mainLayout->addWidget(m_quickWidget);
    
    // 创建性能数据标签布局
    QGridLayout *dataPanelLayout = new QGridLayout(dataPanel);
    dataPanelLayout->setContentsMargins(8, 8, 8, 8);
    
    m_cpuLabel = new QLabel("CPU: 0.0%", this);
    m_gpuLabel = new QLabel("GPU: 0% | 0°C | 0/0", this);
    m_memoryLabel = new QLabel("内存: 0.0%", this);
    m_diskLabel = new QLabel("磁盘: 0.0 MB/s", this);
    m_networkLabel = new QLabel("网络: 0.0 MB/s", this);
    
    QString labelStyle = "QLabel { font-size: 14px; font-weight: bold; color: #2196F3; }";
    m_cpuLabel->setStyleSheet(labelStyle);
    m_gpuLabel->setStyleSheet(labelStyle);
    m_memoryLabel->setStyleSheet(labelStyle);
    m_diskLabel->setStyleSheet(labelStyle);
    m_networkLabel->setStyleSheet(labelStyle);
    
    dataPanelLayout->addWidget(m_cpuLabel, 0, 0);
    dataPanelLayout->addWidget(m_gpuLabel, 0, 1);
    dataPanelLayout->addWidget(m_memoryLabel, 1, 0);
    dataPanelLayout->addWidget(m_diskLabel, 1, 1);
    dataPanelLayout->addWidget(m_networkLabel, 2, 0);
    
    mainLayout->addWidget(dataPanel);
    
    // 创建控制按钮组
    QGroupBox *controlBox = new QGroupBox("视图控制", this);
    QHBoxLayout *controlLayout = new QHBoxLayout(controlBox);
    
    QPushButton *resetBtn = new QPushButton("重置视图", this);
    QPushButton *frontBtn = new QPushButton("正视图", this);
    QPushButton *topBtn = new QPushButton("俯视图", this);
    QPushButton *sideBtn = new QPushButton("侧视图", this);
    
    QString buttonStyle = "QPushButton { padding: 5px 15px; background: #2196F3; color: white; border-radius: 3px; } "
                         "QPushButton:hover { background: #1976D2; }";
    resetBtn->setStyleSheet(buttonStyle);
    frontBtn->setStyleSheet(buttonStyle);
    topBtn->setStyleSheet(buttonStyle);
    sideBtn->setStyleSheet(buttonStyle);
    
    // 连接按钮信号到C++槽函数
    connect(resetBtn, &QPushButton::clicked, this, &Vis3DPage::resetCamera);
    connect(frontBtn, &QPushButton::clicked, this, &Vis3DPage::setFrontView);
    connect(topBtn, &QPushButton::clicked, this, &Vis3DPage::setTopView);
    connect(sideBtn, &QPushButton::clicked, this, &Vis3DPage::setSideView);

    controlLayout->addWidget(resetBtn);
    controlLayout->addWidget(frontBtn);
    controlLayout->addWidget(topBtn);
    controlLayout->addWidget(sideBtn);
    
    mainLayout->addWidget(controlBox);

    setLayout(mainLayout);
}

void Vis3DPage::updateCpuData(double usage)
{
    m_cpuUsage = usage;
    m_cpuHistory.append(usage);
    if (m_cpuHistory.size() > 100) {
        m_cpuHistory.removeFirst();
    }
    if (m_cpuLabel) {
        m_cpuLabel->setText(QString("CPU: %1%").arg(QString::number(m_cpuUsage, 'f', 1)));
    }
    updateEntities();
}

void Vis3DPage::updateGpuData(double usage, double temperature, quint64 memoryUsed, quint64 memoryTotal)
{
    m_gpuUsage = usage;
    m_gpuTemp = temperature;
    m_gpuMemUsed = memoryUsed;
    m_gpuMemTotal = memoryTotal;
    m_gpuHistory.append(usage);
    if (m_gpuHistory.size() > 100) {
        m_gpuHistory.removeFirst();
    }
    if (m_gpuLabel) {
        m_gpuLabel->setText(QString("GPU: %1%% | %2°C | %3/%4 MB")
            .arg(QString::number(m_gpuUsage, 'f', 1))
            .arg(QString::number(m_gpuTemp, 'f', 1))
            .arg(m_gpuMemUsed / 1024 / 1024)
            .arg(m_gpuMemTotal / 1024 / 1024));
    }
    updateEntities();
}

void Vis3DPage::updateMemoryData(double used, double total)
{
    m_memoryUsage = (total > 0) ? (used / total * 100.0) : 0.0;
    m_memoryHistory.append(m_memoryUsage);
    if (m_memoryHistory.size() > 100) {
        m_memoryHistory.removeFirst();
    }
    if (m_memoryLabel) {
        m_memoryLabel->setText(QString("内存: %1%").arg(QString::number(m_memoryUsage, 'f', 1)));
    }
    updateEntities();
}

void Vis3DPage::updateDiskData(double usage)
{
    m_diskUsage = usage;
    m_diskHistory.append(usage);
    if (m_diskHistory.size() > 100) {
        m_diskHistory.removeFirst();
    }
    if (m_diskLabel) {
        m_diskLabel->setText(QString("磁盘: %1 MB/s").arg(QString::number(m_diskUsage, 'f', 1)));
    }
    updateEntities();
}

void Vis3DPage::updateNetworkData(double usage)
{
    m_networkUsage = usage;
    m_networkHistory.append(usage);
    if (m_networkHistory.size() > 100) {
        m_networkHistory.removeFirst();
    }
    if (m_networkLabel) {
        m_networkLabel->setText(QString("网络: %1 MB/s").arg(QString::number(m_networkUsage, 'f', 1)));
    }
    updateEntities();
}

void Vis3DPage::updateEntities()
{
    QVariantList dataList;
    int maxPoints = 10;
    int cpuStart = std::max(0, static_cast<int>(m_cpuHistory.size()) - maxPoints);
    int gpuStart = std::max(0, static_cast<int>(m_gpuHistory.size()) - maxPoints);
    int memStart = std::max(0, static_cast<int>(m_memoryHistory.size()) - maxPoints);
    int diskStart = std::max(0, static_cast<int>(m_diskHistory.size()) - maxPoints);

    int historySize = std::min({
        static_cast<int>(m_cpuHistory.size()) - cpuStart,
        static_cast<int>(m_gpuHistory.size()) - gpuStart,
        static_cast<int>(m_memoryHistory.size()) - memStart,
        static_cast<int>(m_diskHistory.size()) - diskStart
    });

    for (int i = 0; i < historySize; ++i) {
        QVariantMap cpuPoint;
        cpuPoint["time"] = QString("T-%1").arg(i);
        cpuPoint["type"] = "CPU";
        cpuPoint["value"] = m_cpuHistory[cpuStart + i];
        dataList.append(cpuPoint);

        QVariantMap gpuPoint;
        gpuPoint["time"] = QString("T-%1").arg(i);
        gpuPoint["type"] = "GPU";
        gpuPoint["value"] = m_gpuHistory[gpuStart + i];
        dataList.append(gpuPoint);

        QVariantMap memPoint;
        memPoint["time"] = QString("T-%1").arg(i);
        memPoint["type"] = "内存";
        memPoint["value"] = m_memoryHistory[memStart + i];
        dataList.append(memPoint);

        QVariantMap diskPoint;
        diskPoint["time"] = QString("T-%1").arg(i);
        diskPoint["type"] = "磁盘";
        diskPoint["value"] = m_diskHistory[diskStart + i];
        dataList.append(diskPoint);
    }
    // 发送到QML
    QQuickItem *rootObject = m_quickWidget->rootObject();
    if (rootObject) {
        QMetaObject::invokeMethod(rootObject, "updateData", Q_ARG(QVariant, QVariant::fromValue(dataList)));
    }
}

void Vis3DPage::onDataUpdateTimer()
{
    // 定时器回调函数 - 可以在这里添加定期更新逻辑
    // 目前数据更新由外部调用各个update函数触发
}

QString Vis3DPage::formatMemorySize(quint64 bytes)
{
    if (bytes >= 1024 * 1024 * 1024) {
        return QString::number(bytes / (1024.0 * 1024.0 * 1024.0), 'f', 2) + " GB";
    } else if (bytes >= 1024 * 1024) {
        return QString::number(bytes / (1024.0 * 1024.0), 'f', 2) + " MB";
    } else if (bytes >= 1024) {
        return QString::number(bytes / 1024.0, 'f', 2) + " KB";
    } else {
        return QString::number(bytes) + " B";
    }
}

void Vis3DPage::resetCamera()
{
    QQuickItem *rootObject = m_quickWidget->rootObject();
    if (rootObject) {
        QMetaObject::invokeMethod(rootObject, "resetCamera");
    }
}

void Vis3DPage::setTopView()
{
    QQuickItem *rootObject = m_quickWidget->rootObject();
    if (rootObject) {
        QMetaObject::invokeMethod(rootObject, "setTopView");
    }
}

void Vis3DPage::setFrontView()
{
    QQuickItem *rootObject = m_quickWidget->rootObject();
    if (rootObject) {
        QMetaObject::invokeMethod(rootObject, "setFrontView");
    }
}

void Vis3DPage::setSideView()
{
    QQuickItem *rootObject = m_quickWidget->rootObject();
    if (rootObject) {
        QMetaObject::invokeMethod(rootObject, "setSideView");
    }
}