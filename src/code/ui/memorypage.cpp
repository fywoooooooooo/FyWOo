#include "src/include/ui/memorypage.h"
#include <QPainter>
#include <windows.h>
#include <QGroupBox>

QT_USE_NAMESPACE

MemoryPage::MemoryPage(QWidget *parent)
    : QWidget(parent)
    , m_totalMemLabel(new QLabel("总内存: ", this))
    , m_usedMemLabel(new QLabel("已用: ", this))
    , m_freeMemLabel(new QLabel("可用: ", this))
    , m_memoryBar(new QProgressBar(this))
    , m_chart(new QChart())
    , m_chartView(new QChartView(m_chart))
    , m_series(new QLineSeries())
    , m_updateTimer(new QTimer(this))
    , m_usagePercentLabel(new QLabel("内存使用率: 0%", this))
    , m_memTypeLabel(new QLabel("内存类型: DDR4", this))
    , m_pageFileLabel(new QLabel("页面文件: 0 GB", this))
{
    setupUI();
    connect(m_updateTimer, &QTimer::timeout, this, &MemoryPage::updateMemoryData);
    m_updateTimer->start(1000);
    
    // 获取内存类型信息（简化处理，实际应从WMI或其他系统API获取）
    m_memTypeLabel->setText("内存类型: DDR4");
    
    // 获取页面文件信息
    MEMORYSTATUSEX memInfo;
    memInfo.dwLength = sizeof(MEMORYSTATUSEX);
    GlobalMemoryStatusEx(&memInfo);
    quint64 pageFileTotal = memInfo.ullTotalPageFile;
    m_pageFileLabel->setText(QString("页面文件: %1").arg(formatSize(pageFileTotal)));
}

MemoryPage::~MemoryPage()
{
    m_updateTimer->stop();
}

void MemoryPage::setupUI()
{
    QVBoxLayout *mainLayout = new QVBoxLayout(this);
    mainLayout->setSpacing(10);
    mainLayout->setContentsMargins(10, 10, 10, 10);
    
    // 内存使用率图表框
    QGroupBox *chartGroup = new QGroupBox(this);
    chartGroup->setTitle(tr("内存使用率历史"));
    chartGroup->setStyleSheet("QGroupBox { background-color: white; border-radius: 8px; border: 1px solid #e0e0e0; font-weight: bold; }");
    QVBoxLayout *chartLayout = new QVBoxLayout(chartGroup);
    
    m_series->setName("内存使用率");
    m_chart->addSeries(m_series);
    
    QValueAxis *axisX = new QValueAxis;
    axisX->setRange(0, 60);
    axisX->setTitleText("时间 (秒)");
    m_chart->addAxis(axisX, Qt::AlignBottom);
    m_series->attachAxis(axisX);
    
    QValueAxis *axisY = new QValueAxis;
    axisY->setRange(0, 100);
    axisY->setTitleText("使用率 (%)");
    m_chart->addAxis(axisY, Qt::AlignLeft);
    m_series->attachAxis(axisY);
    
    m_chartView->setRenderHint(QPainter::Antialiasing);
    chartLayout->addWidget(m_chartView);
    
    // 内存使用率框
    QFrame *memUsageFrame = new QFrame(this);
    memUsageFrame->setStyleSheet("QFrame { background-color: white; border-radius: 8px; border: 1px solid #e0e0e0; }");
    QVBoxLayout *usageLayout = new QVBoxLayout(memUsageFrame);
    
    QLabel *usageTitle = new QLabel("内存使用率:", this);
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
        "   background-color: #2196F3;"
        "   border-radius: 5px;"
        "}";
    
    m_memoryBar->setStyleSheet(progressBarStyle);
    m_memoryBar->setTextVisible(true);
    usageLayout->addWidget(m_memoryBar);
    
    // 内存详细信息框
    QFrame *memInfoFrame = new QFrame(this);
    memInfoFrame->setStyleSheet("QFrame { background-color: white; border-radius: 8px; border: 1px solid #e0e0e0; }");
    QVBoxLayout *infoLayout = new QVBoxLayout(memInfoFrame);
    
    QLabel *infoTitle = new QLabel("内存详细信息:", this);
    infoTitle->setStyleSheet("QLabel { color: #333333; font-size: 12pt; font-weight: bold; }");
    infoLayout->addWidget(infoTitle);
    
    QGridLayout *detailLayout = new QGridLayout();
    detailLayout->setContentsMargins(0, 0, 0, 0);
    
    QLabel *totalLabel = new QLabel("总内存:", this);
    QLabel *usedLabel = new QLabel("已用:", this);
    QLabel *freeLabel = new QLabel("可用:", this);
    
    totalLabel->setStyleSheet("QLabel { color: #333333; font-size: 10pt; }");
    usedLabel->setStyleSheet("QLabel { color: #333333; font-size: 10pt; }");
    freeLabel->setStyleSheet("QLabel { color: #333333; font-size: 10pt; }");
    m_totalMemLabel->setStyleSheet("QLabel { color: #333333; font-size: 10pt; font-weight: bold; }");
    m_usedMemLabel->setStyleSheet("QLabel { color: #333333; font-size: 10pt; font-weight: bold; }");
    m_freeMemLabel->setStyleSheet("QLabel { color: #333333; font-size: 10pt; font-weight: bold; }");
    m_memTypeLabel->setStyleSheet("QLabel { color: #333333; font-size: 10pt; }");
    m_pageFileLabel->setStyleSheet("QLabel { color: #333333; font-size: 10pt; }");
    
    detailLayout->addWidget(totalLabel, 0, 0);
    detailLayout->addWidget(m_totalMemLabel, 0, 1);
    detailLayout->addWidget(usedLabel, 1, 0);
    detailLayout->addWidget(m_usedMemLabel, 1, 1);
    detailLayout->addWidget(freeLabel, 2, 0);
    detailLayout->addWidget(m_freeMemLabel, 2, 1);
    
    infoLayout->addLayout(detailLayout);
    infoLayout->addWidget(m_memTypeLabel);
    infoLayout->addWidget(m_pageFileLabel);
    
    // 添加所有组件到主布局
    mainLayout->addWidget(chartGroup);
    mainLayout->addWidget(memUsageFrame);
    mainLayout->addWidget(memInfoFrame);
}

void MemoryPage::updateMemoryData()
{
    MEMORYSTATUSEX memInfo;
    memInfo.dwLength = sizeof(MEMORYSTATUSEX);
    GlobalMemoryStatusEx(&memInfo);
    
    // 更新标签
    updateLabels(memInfo.ullTotalPhys, memInfo.ullTotalPhys - memInfo.ullAvailPhys, memInfo.ullAvailPhys);
    
    // 更新内存使用率
    int usagePercentage = static_cast<int>(memInfo.dwMemoryLoad);
    m_memoryBar->setValue(usagePercentage);
    m_usagePercentLabel->setText(QString("内存使用率: %1%").arg(usagePercentage));
    
    // 根据内存使用率设置不同的颜色
    QString memStyle = 
        "QProgressBar {"
        "   border: 1px solid #e0e0e0;"
        "   border-radius: 5px;"
        "   background-color: #f5f5f5;"
        "   text-align: center;"
        "}"
        "QProgressBar::chunk {";
    
    if (usagePercentage < 50) {
        // 低使用率 - 蓝色
        memStyle += "background-color: #2196F3;";
    } else if (usagePercentage < 80) {
        // 中等使用率 - 紫色
        memStyle += "background-color: #9C27B0;";
    } else {
        // 高使用率 - 红色
        memStyle += "background-color: #F44336;";
    }
    
    memStyle += "border-radius: 5px;}";
    m_memoryBar->setStyleSheet(memStyle);
    
    // 更新图表
    static QVector<qreal> data;
    data.append(usagePercentage);
    if (data.size() > 60) {
        data.removeFirst();
    }
    
    m_series->clear();
    for (int i = 0; i < data.size(); ++i) {
        m_series->append(i, data[i]);
    }
}

void MemoryPage::updateLabels(quint64 total, quint64 used, quint64 free)
{
    m_totalMemLabel->setText(formatSize(total));
    m_usedMemLabel->setText(formatSize(used));
    m_freeMemLabel->setText(formatSize(free));
}

QString MemoryPage::formatSize(quint64 bytes)
{
    const quint64 kb = 1024;
    const quint64 mb = 1024 * kb;
    const quint64 gb = 1024 * mb;
    
    if (bytes >= gb) {
        return QString::number(bytes / static_cast<double>(gb), 'f', 2) + " GB";
    } else if (bytes >= mb) {
        return QString::number(bytes / static_cast<double>(mb), 'f', 2) + " MB";
    } else if (bytes >= kb) {
        return QString::number(bytes / static_cast<double>(kb), 'f', 2) + " KB";
    } else {
        return QString::number(bytes) + " B";
    }
}
