#include "src/include/ui/cpupage.h"
#include <QPainter>
#include <windows.h>
#include <QGroupBox>
#include <QProgressBar>
#include <intrin.h>
#include <QProcess>
#include <QRegularExpression>

QT_USE_NAMESPACE

CpuPage::CpuPage(QWidget *parent)
    : QWidget(parent)
    , m_chart(new QChart())
    , m_chartView(new QChartView(m_chart))
    , m_series(new QLineSeries())
    , m_axisX(new QValueAxis())
    , m_axisY(new QValueAxis())
    , m_updateTimer(new QTimer(this))
    , m_usageBar(new QProgressBar(this))
    , m_usageLabel(new QLabel("CPU使用率: 0%", this))
    , m_coresLabel(new QLabel("CPU核心数: 0", this))
    , m_modelLabel(new QLabel("CPU型号: 未知", this))
    , m_freqLabel(new QLabel("CPU频率: 获取中...", this))
    , m_archLabel(new QLabel("CPU架构: x86-64", this))
{
    setupUI();
    
    connect(m_updateTimer, &QTimer::timeout, this, &CpuPage::updateCpuData);
    m_updateTimer->start(1000);
    
    // 获取CPU信息
    SYSTEM_INFO sysInfo;
    GetSystemInfo(&sysInfo);
    m_coresLabel->setText(QString("CPU核心数: %1").arg(sysInfo.dwNumberOfProcessors));
    
    // 获取CPU型号
    int cpuInfo[4] = {-1};
    char cpuBrandString[0x40] = {0};
    __cpuid(cpuInfo, 0x80000000);
    unsigned int nExIds = cpuInfo[0];
    
    for (unsigned int i = 0x80000000; i <= nExIds; ++i) {
        __cpuid(cpuInfo, i);
        if (i == 0x80000002)
            memcpy(cpuBrandString, cpuInfo, sizeof(cpuInfo));
        else if (i == 0x80000003)
            memcpy(cpuBrandString + 16, cpuInfo, sizeof(cpuInfo));
        else if (i == 0x80000004)
            memcpy(cpuBrandString + 32, cpuInfo, sizeof(cpuInfo));
    }
    
    m_modelLabel->setText(QString("CPU型号: %1").arg(cpuBrandString));
}

void CpuPage::setupUI()
{
    QVBoxLayout *mainLayout = new QVBoxLayout(this);
    mainLayout->setSpacing(10);
    mainLayout->setContentsMargins(10, 10, 10, 10);
    
    // CPU使用率图表框
    QGroupBox *chartGroup = new QGroupBox(this);
    chartGroup->setTitle(tr("CPU使用率历史"));
    chartGroup->setStyleSheet("QGroupBox { background-color: white; border-radius: 8px; border: 1px solid #e0e0e0; font-weight: bold; }");
    QVBoxLayout *chartLayout = new QVBoxLayout(chartGroup);
    
    m_series->setName("CPU使用率");
    m_chart->addSeries(m_series);
    
    m_axisX->setRange(0, 60);
    m_axisX->setTitleText("时间 (秒)");
    m_chart->addAxis(m_axisX, Qt::AlignBottom);
    m_series->attachAxis(m_axisX);
    
    m_axisY->setRange(0, 100);
    m_axisY->setTitleText("使用率 (%)");
    m_chart->addAxis(m_axisY, Qt::AlignLeft);
    m_series->attachAxis(m_axisY);
    
    m_chartView->setRenderHint(QPainter::Antialiasing);
    chartLayout->addWidget(m_chartView);
    
    // CPU使用率框
    QFrame *cpuUsageFrame = new QFrame(this);
    cpuUsageFrame->setStyleSheet("QFrame { background-color: white; border-radius: 8px; border: 1px solid #e0e0e0; }");
    QVBoxLayout *usageLayout = new QVBoxLayout(cpuUsageFrame);
    
    QLabel *usageTitle = new QLabel("CPU使用率:", this);
    usageTitle->setStyleSheet("QLabel { color: #333333; font-size: 12pt; font-weight: bold; }");
    usageLayout->addWidget(usageTitle);
    
    // 设置进度条样式
    QString progressBarStyle = 
        "QProgressBar {"
        "   border: 1px solid #e0e0e0;"
        "   border-radius: 5px;"
        "   background-color: #f5f5f5;"
        "   text-align: center;"
        "}"
        "QProgressBar::chunk {"
        "   background-color: #4CAF50;"
        "   border-radius: 5px;"
        "}";
    
    m_usageBar->setStyleSheet(progressBarStyle);
    m_usageBar->setTextVisible(true);
    m_usageBar->setRange(0, 100);
    usageLayout->addWidget(m_usageBar);
    
    // CPU详细信息框
    QFrame *cpuInfoFrame = new QFrame(this);
    cpuInfoFrame->setStyleSheet("QFrame { background-color: white; border-radius: 8px; border: 1px solid #e0e0e0; }");
    QVBoxLayout *infoLayout = new QVBoxLayout(cpuInfoFrame);
    
    QLabel *infoTitle = new QLabel("CPU信息:", this);
    infoTitle->setStyleSheet("QLabel { color: #333333; font-size: 12pt; font-weight: bold; }");
    infoLayout->addWidget(infoTitle);
    
    m_modelLabel->setStyleSheet("QLabel { color: #333333; font-size: 10pt; }");
    m_coresLabel->setStyleSheet("QLabel { color: #333333; font-size: 10pt; }");
    
    // 添加CPU频率标签
    m_freqLabel = new QLabel("CPU频率: 获取中...", this);
    m_freqLabel->setStyleSheet("QLabel { color: #333333; font-size: 10pt; }");
    
    // 添加CPU架构标签
    m_archLabel = new QLabel("CPU架构: x86-64", this);
    m_archLabel->setStyleSheet("QLabel { color: #333333; font-size: 10pt; }");
    
    infoLayout->addWidget(m_modelLabel);
    infoLayout->addWidget(m_coresLabel);
    infoLayout->addWidget(m_freqLabel);
    infoLayout->addWidget(m_archLabel);
    
    // 添加所有组件到主布局
    mainLayout->addWidget(chartGroup);
    mainLayout->addWidget(cpuUsageFrame);
    mainLayout->addWidget(cpuInfoFrame);
}

CpuPage::~CpuPage()
{
    m_updateTimer->stop();
}

void CpuPage::updateCpuData()
{
    static FILETIME prevIdleTime = {0};
    static FILETIME prevKernelTime = {0};
    static FILETIME prevUserTime = {0};
    
    FILETIME idleTime, kernelTime, userTime;
    if (!GetSystemTimes(&idleTime, &kernelTime, &userTime)) {
        return;
    }
    
    ULONGLONG idle = (((ULONGLONG)idleTime.dwHighDateTime) << 32) | idleTime.dwLowDateTime;
    ULONGLONG kernel = (((ULONGLONG)kernelTime.dwHighDateTime) << 32) | kernelTime.dwLowDateTime;
    ULONGLONG user = (((ULONGLONG)userTime.dwHighDateTime) << 32) | userTime.dwLowDateTime;
    
    ULONGLONG prevIdle = (((ULONGLONG)prevIdleTime.dwHighDateTime) << 32) | prevIdleTime.dwLowDateTime;
    ULONGLONG prevKernel = (((ULONGLONG)prevKernelTime.dwHighDateTime) << 32) | prevKernelTime.dwLowDateTime;
    ULONGLONG prevUser = (((ULONGLONG)prevUserTime.dwHighDateTime) << 32) | prevUserTime.dwLowDateTime;
    
    ULONGLONG idleDiff = idle - prevIdle;
    ULONGLONG kernelDiff = kernel - prevKernel;
    ULONGLONG userDiff = user - prevUser;
    ULONGLONG totalDiff = kernelDiff + userDiff;
    
    double cpuUsage = totalDiff > 0 ? ((totalDiff - idleDiff) * 100.0 / totalDiff) : 0;
    
    prevIdleTime = idleTime;
    prevKernelTime = kernelTime;
    prevUserTime = userTime;
    
    // 更新数据
    m_data.append(cpuUsage);
    if (m_data.size() > 60) {
        m_data.removeFirst();
    }
    
    // 更新图表
    m_series->clear();
    for (int i = 0; i < m_data.size(); ++i) {
        m_series->append(i, m_data[i]);
    }
    
    // 更新CPU使用率进度条
    m_usageBar->setValue(qRound(cpuUsage));
    
    // 根据CPU使用率设置不同的颜色
    QString usageStyle = 
        "QProgressBar {"
        "   border: 1px solid #e0e0e0;"
        "   border-radius: 5px;"
        "   background-color: #f5f5f5;"
        "   text-align: center;"
        "}"
        "QProgressBar::chunk {";
    
    if (cpuUsage < 30) {
        // 低负载 - 绿色
        usageStyle += "background-color: #4CAF50;";
    } else if (cpuUsage < 70) {
        // 中等负载 - 黄色
        usageStyle += "background-color: #FFC107;";
    } else {
        // 高负载 - 红色
        usageStyle += "background-color: #F44336;";
    }
    
    usageStyle += "border-radius: 5px;}";
    m_usageBar->setStyleSheet(usageStyle);
    
    // 尝试获取CPU频率 (示例实现，实际可能需要WMI查询)
    static int counter = 0;
    if (counter++ % 5 == 0) { // 每5秒更新一次
        QProcess process;
        process.start("wmic", QStringList() << "cpu" << "get" << "currentclockspeed" << "/format:list");
        if (process.waitForFinished(3000)) {
            QString output = process.readAllStandardOutput();
            // 解析输出格式: CurrentClockSpeed=xxxx
            QRegularExpression regex("CurrentClockSpeed=(\\d+)");
            QRegularExpressionMatch match = regex.match(output);
            if (match.hasMatch()) {
                int freq = match.captured(1).toInt();
                if (freq > 0) {
                    m_freqLabel->setText(QString("CPU频率: %1 MHz").arg(freq));
                }
            } else {
                // 尝试备用命令，使用PowerShell获取
                QProcess psProcess;
                psProcess.start("powershell", QStringList() << "-Command" << "Get-WmiObject Win32_Processor | Select-Object CurrentClockSpeed");
                if (psProcess.waitForFinished(3000)) {
                    QString psOutput = psProcess.readAllStandardOutput();
                    QRegularExpression psRegex("(\\d+)");
                    QRegularExpressionMatch psMatch = psRegex.match(psOutput);
                    if (psMatch.hasMatch()) {
                        int freq = psMatch.captured(1).toInt();
                        if (freq > 0) {
                            m_freqLabel->setText(QString("CPU频率: %1 MHz").arg(freq));
                        }
                    }
                }
            }
        }
    }
}

void CpuPage::setupCharts() {
    m_chartView->setStyleSheet(
        "QChartView {"
        "    background: #ffffff;"
        "    border-radius: 8px;"
        "    padding: 12px;"
        "    border: 1px solid #d0d0d0;"
        "}"
    );
}

// 应用统一表单控件样式
void CpuPage::setupControls() {
    m_usageLabel->setStyleSheet(
        "QLabel {"
        "    background: qlineargradient(x1:0, y1:0, x2:0, y2:1, stop:0 #f6f7fa, stop:1 #e3e4f2);"
        "    border: 1px solid #c4c7d4;"
        "    border-radius: 6px;"
        "    padding: 8px;"
        "}"
    );
}
