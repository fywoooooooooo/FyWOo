#include "src/include/ui/analysispage.h"
#include <QSqlQuery>
#include <QSqlError>
#include <QJsonDocument>
#include <QGridLayout>
#include <QMessageBox>
#include <QDateTime>
#include <QDebug>
#include <QTextEdit>
#include <QGroupBox>
#include <QSplitter>
#include <QThread>
#include <QListWidget>
#include <QFormLayout>
#include <QProgressBar>
#include <QProgressDialog>
#include <QApplication>
#include <QListWidgetItem>
#include <QtCharts/QChartView>
#include <QtCharts/QBarSeries>
#include <QtCharts/QBarSet>
#include <QtCharts/QLineSeries>
#include <QtCharts/QPieSeries>
#include <QtCharts/QBarCategoryAxis>
#include <QVBoxLayout>
#include <QWidget>
#include <QRandomGenerator>
#include <QRegularExpression>
#include <QScrollBar>

AnalysisPage::AnalysisPage(QWidget *parent)
    : QWidget(parent)
    , m_anomalyDetector(new AnomalyDetector(this))
    , m_performanceAnalyzer(new PerformanceAnalyzer(this))
    , m_updateTimer(new QTimer(this))
    , m_tabWidget(new QTabWidget(this))
    , m_anomalyTab(new QWidget(this))
    , m_anomalyResultsText(new QTextEdit(this))
    // m_anomalyThresholdSpin, m_autoCpuCheck, etc. are moved to SettingsWidget
    , m_runAnomalyButton(new QPushButton("执行异常检测", this))
    , m_performanceTab(new QWidget(this))
    , m_bottleneckText(new QTextEdit(this))
    , m_trendText(new QTextEdit(this))
    , m_suggestionsText(new QTextEdit(this))
    , m_timeWindowCombo(new QComboBox(this))
    , m_runAnalysisButton(new QPushButton("执行性能分析", this))
    , m_currentAnomalyThreshold(2) // Default value
    , m_performCpuCheck(true)      // Default value
    , m_performMemoryCheck(true)   // Default value
    , m_performDiskCheck(true)     // Default value
    , m_performNetworkCheck(true)  // Default value
    // 新增NLP查询组件初始化
    , m_nlpQueryTab(new QWidget(this))
    , m_queryInput(new QLineEdit(this))
    , m_queryButton(new QPushButton(tr("查询"), this))
    , m_queryResultText(new QTextEdit(this))
    , m_queryHistoryList(new QListWidget(this))
    , m_showHistoryButton(new QPushButton(tr("历史"), this))
{
    setupUI();
    setupConnections();
    // Initialization for m_anomalyThresholdSpin, m_autoCpuCheck, etc. moved or handled by defaults/slots
    m_timeWindowCombo->addItem("最近30分钟", 30);
    m_timeWindowCombo->addItem("最近1小时", 60);
    m_timeWindowCombo->addItem("最近3小时", 180);
    m_timeWindowCombo->addItem("最近6小时", 360);
    m_timeWindowCombo->addItem("最近12小时", 720);
    m_timeWindowCombo->addItem("最近24小时", 1440);
    m_anomalyResultsText->setReadOnly(true);
    m_bottleneckText->setReadOnly(true);
    m_trendText->setReadOnly(true);
    m_suggestionsText->setReadOnly(true);
    m_queryResultText->setReadOnly(true);
    m_updateTimer->start(1000); // Default interval, will be updated by onUpdateIntervalChanged
}

AnalysisPage::~AnalysisPage()
{
    m_updateTimer->stop();
}

void AnalysisPage::setupUI()
{
    // TODO: Implement UI setup
    // UI setup
    QVBoxLayout *mainLayout = new QVBoxLayout(this);
    mainLayout->addWidget(m_tabWidget);

    // Anomaly Tab
    QWidget *anomalyPageWidget = new QWidget;
    QVBoxLayout *anomalyLayout = new QVBoxLayout(anomalyPageWidget);
    // UI elements for threshold and auto-detect moved to SettingsWidget
    anomalyLayout->addWidget(m_runAnomalyButton);
    anomalyLayout->addWidget(new QLabel("异常结果:"));
    anomalyLayout->addWidget(m_anomalyResultsText);
    m_tabWidget->addTab(anomalyPageWidget, "异常检测");

    // Performance Tab
    QWidget *performancePageWidget = new QWidget;
    QVBoxLayout *performanceLayout = new QVBoxLayout(performancePageWidget);
    performanceLayout->addWidget(new QLabel("时间窗口:"));
    performanceLayout->addWidget(m_timeWindowCombo);
    performanceLayout->addWidget(m_runAnalysisButton);
    performanceLayout->addWidget(new QLabel("瓶颈分析:"));
    performanceLayout->addWidget(m_bottleneckText);
    performanceLayout->addWidget(new QLabel("趋势分析:"));
    performanceLayout->addWidget(m_trendText);
    performanceLayout->addWidget(new QLabel("优化建议:"));
    performanceLayout->addWidget(m_suggestionsText);
    m_tabWidget->addTab(performancePageWidget, "性能分析");
    
    // 设置NLP查询标签页
    setupNlpQueryTab();

    setLayout(mainLayout);
}

// 新增：设置NLP查询标签页
void AnalysisPage::setupNlpQueryTab()
{
    QVBoxLayout *nlpLayout = new QVBoxLayout(m_nlpQueryTab);
    
    // 创建查询输入区域
    QGroupBox *queryBox = new QGroupBox("自然语言查询");
    QVBoxLayout *queryBoxLayout = new QVBoxLayout(queryBox);
    
    // 添加帮助提示标签
    QLabel *helpLabel = new QLabel("示例查询: 'CPU使用率最高的进程是什么?', '过去一小时内的网络流量趋势如何?'");
    helpLabel->setWordWrap(true);
    helpLabel->setStyleSheet("color: #666666; font-style: italic;");
    queryBoxLayout->addWidget(helpLabel);
    
    // 查询输入行
    QHBoxLayout *inputLayout = new QHBoxLayout();
    m_queryInput->setPlaceholderText("请输入您的查询...");
    m_queryInput->setClearButtonEnabled(true);
    inputLayout->addWidget(m_queryInput, 1);
    
    // 查询按钮
    // 直接使用资源中的PNG图标
    m_queryButton->setIcon(QIcon(":/icons/icons/search.png"));
    m_queryButton->setCursor(Qt::PointingHandCursor);
    m_queryButton->setStyleSheet(
        "QPushButton { background-color: #2196F3; color: white; border: none; border-radius: 4px; "
        "padding: 8px 16px; } "
        "QPushButton:hover { background-color: #1976D2; } "
        "QPushButton:pressed { background-color: #0D47A1; }"
    );
    inputLayout->addWidget(m_queryButton);

    // 历史查询按钮
    // 直接使用资源中的PNG图标
    m_showHistoryButton->setIcon(QIcon(":/icons/icons/history.png"));
    m_showHistoryButton->setCursor(Qt::PointingHandCursor);
    m_showHistoryButton->setStyleSheet(
        "QPushButton { background-color: #4CAF50; color: white; border: none; border-radius: 4px; "
        "padding: 8px 16px; } "
        "QPushButton:hover { background-color: #388E3C; } "
        "QPushButton:pressed { background-color: #1B5E20; }"
    );
    inputLayout->addWidget(m_showHistoryButton);
    queryBoxLayout->addLayout(inputLayout);
    
    // 添加结果显示区域
    QGroupBox *resultBox = new QGroupBox("查询结果");
    QVBoxLayout *resultLayout = new QVBoxLayout(resultBox);
    m_queryResultText->setStyleSheet(
        "QTextEdit { background-color: #f8f8f8; border: 1px solid #e0e0e0; border-radius: 4px; "
        "font-family: 'Segoe UI', sans-serif; padding: 8px; }"
    );
    resultLayout->addWidget(m_queryResultText);
    
    // 添加历史记录列表（默认隐藏）
    m_queryHistoryList->setVisible(false);
    m_queryHistoryList->setAlternatingRowColors(true);
    m_queryHistoryList->setStyleSheet(
        "QListWidget { background-color: #f5f5f5; border: 1px solid #e0e0e0; border-radius: 4px; "
        "alternate-background-color: #e9e9e9; }"
        "QListWidget::item { padding: 4px; }"
        "QListWidget::item:selected { background-color: #2196F3; color: white; }"
    );
    
    // 将所有组件添加到主布局
    nlpLayout->addWidget(queryBox, 0);
    nlpLayout->addWidget(resultBox, 1);
    nlpLayout->addWidget(m_queryHistoryList, 1);
    
    // 添加到主标签页
    m_tabWidget->addTab(m_nlpQueryTab, "自然语言查询");
}

void AnalysisPage::setupConnections()
{
    // TODO: Implement signal-slot connections
    // Setup connections
    connect(m_runAnomalyButton, &QPushButton::clicked, this, &AnalysisPage::runAnomalyDetection);
    connect(m_runAnalysisButton, &QPushButton::clicked, this, &AnalysisPage::runPerformanceAnalysis);
    connect(m_anomalyDetector, &AnomalyDetector::anomalyDetected, this, &AnalysisPage::handleAnomalyDetected);
    connect(m_performanceAnalyzer, &PerformanceAnalyzer::bottleneckDetected, this, &AnalysisPage::handleBottleneckDetected);
    connect(m_performanceAnalyzer, &PerformanceAnalyzer::trendChanged, this, &AnalysisPage::handleTrendChanged);
    connect(m_updateTimer, &QTimer::timeout, this, [this](){
        // Simulate data updates
        double cpu = QRandomGenerator::global()->bounded(100.0);
        double mem = QRandomGenerator::global()->bounded(100.0);
        double disk = QRandomGenerator::global()->bounded(100.0);
        double net = QRandomGenerator::global()->bounded(100.0);
        updateAnomalyData(cpu, mem, disk, net);
        updatePerformanceData(cpu, mem, disk, net);
        QVector<double> cpuData, memData, diskData, netData;
        for(int i=0; i<10; ++i) {
            cpuData.append(QRandomGenerator::global()->bounded(100.0));
            memData.append(QRandomGenerator::global()->bounded(100.0));
            diskData.append(QRandomGenerator::global()->bounded(100.0));
            netData.append(QRandomGenerator::global()->bounded(100.0));
        }
        updateVisualization(cpuData, memData, diskData, netData);
    });
    
    // 新增：NLP查询相关连接
    connect(m_queryButton, &QPushButton::clicked, this, &AnalysisPage::executeNlpQuery);
    connect(m_queryInput, &QLineEdit::returnPressed, this, &AnalysisPage::executeNlpQuery);
    // 保存查询历史
auto saveQueryHistory = [this](const QString &type, const QVariantMap &params, int count) {
    QSqlQuery query(QSqlDatabase::database("history_connection"));
    query.prepare("INSERT INTO query_history (query_type, parameters, result_count) "
                  "VALUES (:type, :params, :count)");
    query.bindValue(":type", type);
    query.bindValue(":params", QJsonDocument::fromVariant(params).toJson(QJsonDocument::Compact));
    query.bindValue(":count", count);
    
    if (!query.exec()) {
        qWarning() << "Save query failed:" << query.lastError().text();
    }
};

connect(m_showHistoryButton, &QPushButton::clicked, this, [=]() {
    showQueryHistory();
});
    connect(m_queryHistoryList, &QListWidget::itemDoubleClicked, this, [this](QListWidgetItem *item) {
        // 当双击历史记录时，设置为当前查询
        m_queryInput->setText(item->data(Qt::UserRole).toString());
        m_queryHistoryList->setVisible(false);
        executeNlpQuery();
    });
}

void AnalysisPage::updateVisualization(const QVector<double>& cpuData, const QVector<double>& memoryData, const QVector<double>& diskData, const QVector<double>& networkData)
{
    // Update visualization logic without debug logging
    // Example: emit performanceUpdated(QString("CPU: %1, Mem: %2").arg(cpuData.last()).arg(memoryData.last()));
}

void AnalysisPage::updateAnomalyData(double cpuUsage, double memoryUsage, double diskIO, double networkUsage)
{
    // Anomaly data update without debug logging
    m_anomalyDetector->setThresholdFactor(m_currentAnomalyThreshold); // Use stored threshold
    if (m_performCpuCheck) {
        m_anomalyDetector->addCpuDataPoint(cpuUsage);
        m_anomalyDetector->detectCpuAnomaly();
    }
    if (m_performMemoryCheck) {
        m_anomalyDetector->addMemoryDataPoint(memoryUsage);
        m_anomalyDetector->detectMemoryAnomaly();
    }
    if (m_performDiskCheck) {
        m_anomalyDetector->addDiskDataPoint(diskIO);
        m_anomalyDetector->detectDiskAnomaly();
    }
    if (m_performNetworkCheck) {
        m_anomalyDetector->addNetworkDataPoint(networkUsage);
        m_anomalyDetector->detectNetworkAnomaly();
    }
    emit anomalyUpdated(QString("Anomaly Data Updated: CPU %1%, Mem %2%").arg(cpuUsage).arg(memoryUsage));
}

void AnalysisPage::updatePerformanceData(double cpuUsage, double memoryUsage, double diskIO, double networkUsage)
{
    // Performance data update without debug logging
    m_performanceAnalyzer->addDataPoint(cpuUsage, memoryUsage, diskIO, networkUsage);
    // Trigger analysis, e.g., for bottleneck or trends
    m_performanceAnalyzer->analyzeBottleneck();
    m_performanceAnalyzer->analyzeCpuTrend();
    m_performanceAnalyzer->analyzeMemoryTrend();
    m_performanceAnalyzer->analyzeDiskTrend();
    m_performanceAnalyzer->analyzeNetworkTrend();
    emit performanceUpdated(QString("Performance Data Updated: CPU %1%, Mem %2%").arg(cpuUsage).arg(memoryUsage));
}

void AnalysisPage::handleAnomalyDetected(const QString& source, double value, double threshold, const QDateTime& timestamp)
{
    // Handle anomaly without debug logging
    QString message = QString("%1: %2 detected. Value: %3, Threshold: %4")
                          .arg(timestamp.toString(Qt::ISODate))
                          .arg(source)
                          .arg(value)
                          .arg(threshold);
    m_anomalyResultsText->append(message);
}

void AnalysisPage::handleBottleneckDetected(PerformanceAnalyzer::BottleneckType type, const QString& details)
{
    // Handle bottleneck without debug logging
    QString typeStr;
    switch (type) {
        case PerformanceAnalyzer::BottleneckType::CPU: typeStr = "CPU Bottleneck"; break;
        case PerformanceAnalyzer::BottleneckType::Memory: typeStr = "Memory Bottleneck"; break;
        case PerformanceAnalyzer::BottleneckType::Disk: typeStr = "Disk Bottleneck"; break;
        case PerformanceAnalyzer::BottleneckType::Network: typeStr = "Network Bottleneck"; break;
        case PerformanceAnalyzer::BottleneckType::Multiple: typeStr = "Multiple Bottlenecks"; break;
        default: typeStr = "Unknown Bottleneck";
    }
    m_bottleneckText->append(QString("%1: %2").arg(typeStr).arg(details));
}

void AnalysisPage::handleTrendChanged(const QString& component, PerformanceAnalyzer::TrendType trend, const QString& details)
{
    // Handle trend change without debug logging
    QString trendStr;
    switch (trend) {
        case PerformanceAnalyzer::TrendType::Increasing: trendStr = "Increasing"; break;
        case PerformanceAnalyzer::TrendType::Decreasing: trendStr = "Decreasing"; break;
        case PerformanceAnalyzer::TrendType::Stable: trendStr = "Stable"; break;
        default: trendStr = "Unknown Trend";
    }
    m_trendText->append(QString("Component: %1, Trend: %2. Details: %3").arg(component).arg(trendStr).arg(details));
}

void AnalysisPage::runAnomalyDetection()
{
    // TODO: Implement anomaly detection logic
    // Run anomaly detection
    m_anomalyResultsText->append("Manual anomaly detection triggered.");
    // Simulate checking current values
    updateAnomalyData(QRandomGenerator::global()->bounded(100.0), QRandomGenerator::global()->bounded(100.0), QRandomGenerator::global()->bounded(100.0), QRandomGenerator::global()->bounded(100.0));
}

void AnalysisPage::runPerformanceAnalysis()
{
    // TODO: Implement performance analysis logic
    // Run performance analysis
    m_bottleneckText->append("Manual performance analysis triggered.");
    m_trendText->append("Manual trend analysis triggered.");
    m_suggestionsText->append("Suggestions: Consider optimizing X, Y, Z.");
    // Simulate checking current values
    updatePerformanceData(QRandomGenerator::global()->bounded(100.0), QRandomGenerator::global()->bounded(100.0), QRandomGenerator::global()->bounded(100.0), QRandomGenerator::global()->bounded(100.0));
}

// 删除未声明的方法，这些方法在头文件中没有声明

// Slots to receive settings from SettingsWidget
void AnalysisPage::onAnalysisSettingsUpdated(int anomalyThreshold, bool cpuAuto, bool memAuto, bool diskAuto, bool netAuto)
{
    // Analysis settings updated
    m_currentAnomalyThreshold = anomalyThreshold;
    m_performCpuCheck = cpuAuto;
    m_performMemoryCheck = memAuto;
    m_performDiskCheck = diskAuto;
    m_performNetworkCheck = netAuto;
    // Optionally, re-run detection or inform user
    // m_anomalyDetector->setThresholdFactor(m_currentAnomalyThreshold); // Already set in updateAnomalyData
}

void AnalysisPage::onUpdateIntervalChanged(int intervalMs)
{
    // Update interval changed
    if (m_updateTimer) {
        m_updateTimer->setInterval(intervalMs);
    }
}

// 新增：执行自然语言查询
void AnalysisPage::executeNlpQuery()
{
    QString query = m_queryInput->text().trimmed();
    if (query.isEmpty()) {
        // 如果查询为空，显示提示
        m_queryResultText->setHtml("<span style='color:red'>请输入有效的查询</span>");
        return;
    }
    
    // 查询前显示加载状态
    m_queryResultText->setHtml("<i>正在处理查询...</i>");
    QApplication::processEvents();
    
    // 处理查询
    QString result = processNlpQuery(query);
    
    // 显示结果
    m_queryResultText->setHtml(result);
    
    // 添加到历史记录
    addQueryToHistory(query, result);
    
    // 清除输入框
    m_queryInput->clear();
    
    // 隐藏历史记录列表（如果可见）
    if (m_queryHistoryList->isVisible()) {
        m_queryHistoryList->setVisible(false);
    }
    
    // 发送查询请求信号
    emit naturalLanguageQueryRequested(query);
}

// 新增：处理自然语言查询
QString AnalysisPage::processNlpQuery(const QString &query)
{
    // 规则引擎：匹配查询与响应之间的逻辑
    
    // CPU相关查询
    if (query.contains(QRegularExpression("\\bCPU\\b.*使用率", QRegularExpression::CaseInsensitiveOption)) ||
        query.contains(QRegularExpression("处理器.*占用", QRegularExpression::CaseInsensitiveOption))) {
        
        double cpuUsage = m_performanceAnalyzer->getLastCpuUsage();
        QString trend;
        switch (m_performanceAnalyzer->analyzeCpuTrend()) {
            case PerformanceAnalyzer::TrendType::Increasing: trend = "上升"; break;
            case PerformanceAnalyzer::TrendType::Decreasing: trend = "下降"; break;
            case PerformanceAnalyzer::TrendType::Fluctuating: trend = "波动"; break;
            default: trend = "稳定";
        }
        
        return QString("<h3>CPU使用率分析</h3>"
                      "<p>当前CPU使用率: <b>%1%</b></p>"
                      "<p>趋势: <b>%2</b></p>"
                      "<p>在过去的30分钟内，CPU使用率%3。</p>")
                      .arg(cpuUsage, 0, 'f', 1)
                      .arg(trend)
                      .arg(trend == "上升" ? "呈现上升趋势，建议检查是否有资源密集型进程" : 
                           trend == "下降" ? "逐渐降低，系统负载正在减轻" :
                           trend == "波动" ? "有明显波动，可能是短时任务导致" : "保持稳定");
    }
    
    // 内存相关查询
    if (query.contains(QRegularExpression("\\b内存\\b", QRegularExpression::CaseInsensitiveOption)) ||
        query.contains(QRegularExpression("\\bRAM\\b", QRegularExpression::CaseInsensitiveOption)) ||
        query.contains(QRegularExpression("\\b内存使用\\b", QRegularExpression::CaseInsensitiveOption))) {
        
        double memUsage = m_performanceAnalyzer->getLastMemoryUsage();
        QString riskLevel;
        
        if (memUsage > 90) riskLevel = "极高";
        else if (memUsage > 80) riskLevel = "高";
        else if (memUsage > 60) riskLevel = "中等";
        else riskLevel = "低";
        
        return QString("<h3>内存使用分析</h3>"
                      "<p>当前内存使用率: <b>%1%</b></p>"
                      "<p>风险级别: <b>%2</b></p>"
                      "<p>%3</p>")
                      .arg(memUsage, 0, 'f', 1)
                      .arg(riskLevel)
                      .arg(riskLevel == "极高" ? "内存严重不足，建议立即关闭不必要的程序以释放内存" : 
                           riskLevel == "高" ? "内存使用较高，可能会影响系统性能，建议关闭不必要的程序" :
                           riskLevel == "中等" ? "内存使用处于正常范围，但仍有优化空间" : 
                           "内存使用处于健康水平，无需担心");
    }
    
    // 磁盘相关查询
    if (query.contains(QRegularExpression("\\b磁盘\\b", QRegularExpression::CaseInsensitiveOption)) ||
        query.contains(QRegularExpression("\\bIO\\b", QRegularExpression::CaseInsensitiveOption)) ||
        query.contains(QRegularExpression("存储", QRegularExpression::CaseInsensitiveOption))) {
        
        double diskIO = m_performanceAnalyzer->getLastDiskIO();
        
        return QString("<h3>磁盘活动分析</h3>"
                      "<p>当前磁盘I/O活动: <b>%1 MB/s</b></p>"
                      "<p>%2</p>"
                      "<p>磁盘使用情况:</p>")
                      .arg(diskIO, 0, 'f', 2)
                      .arg(diskIO > 50 ? "磁盘活动较高，可能存在大文件传输或数据库操作" : 
                           diskIO > 20 ? "磁盘活动处于中等水平" : "磁盘活动较低，系统运行正常");
        
        // 这里可以添加磁盘使用情况的表格
    }
    
    // 网络相关查询
    if (query.contains(QRegularExpression("\\b网络\\b", QRegularExpression::CaseInsensitiveOption)) ||
        query.contains(QRegularExpression("\\b带宽\\b", QRegularExpression::CaseInsensitiveOption)) ||
        query.contains(QRegularExpression("\\b流量\\b", QRegularExpression::CaseInsensitiveOption))) {
        
        double networkUsage = m_performanceAnalyzer->getLastNetworkUsage();
        
        return QString("<h3>网络活动分析</h3>"
                      "<p>当前网络活动: <b>%1 MB/s</b></p>"
                      "<p>%2</p>")
                      .arg(networkUsage, 0, 'f', 2)
                      .arg(networkUsage > 50 ? "网络使用率较高，可能正在进行大文件下载或流媒体播放" : 
                           networkUsage > 10 ? "网络使用处于中等水平" : "网络使用较低，连接稳定");
    }
    
    // 系统整体查询
    if (query.contains(QRegularExpression("\\b系统\\b.*\\b状态\\b", QRegularExpression::CaseInsensitiveOption)) ||
        query.contains(QRegularExpression("\\b整体\\b.*\\b性能\\b", QRegularExpression::CaseInsensitiveOption)) ||
        query.contains(QRegularExpression("\\b概况\\b", QRegularExpression::CaseInsensitiveOption))) {
        
        double cpuUsage = m_performanceAnalyzer->getLastCpuUsage();
        double memUsage = m_performanceAnalyzer->getLastMemoryUsage();
        double diskIO = m_performanceAnalyzer->getLastDiskIO();
        double networkUsage = m_performanceAnalyzer->getLastNetworkUsage();
        
        PerformanceAnalyzer::BottleneckType bottleneck = m_performanceAnalyzer->analyzeBottleneck();
        QString bottleneckStr;
        
        switch (bottleneck) {
            case PerformanceAnalyzer::BottleneckType::CPU: 
                bottleneckStr = "CPU可能成为系统瓶颈，建议关闭不必要的高CPU占用程序"; 
                break;
            case PerformanceAnalyzer::BottleneckType::Memory: 
                bottleneckStr = "内存可能成为系统瓶颈，建议关闭内存占用大的应用"; 
                break;
            case PerformanceAnalyzer::BottleneckType::Disk: 
                bottleneckStr = "磁盘I/O可能成为系统瓶颈，建议减少大文件操作"; 
                break;
            case PerformanceAnalyzer::BottleneckType::Network: 
                bottleneckStr = "网络可能成为系统瓶颈，建议限制高带宽应用"; 
                break;
            case PerformanceAnalyzer::BottleneckType::Multiple: 
                bottleneckStr = "系统存在多个性能瓶颈，建议全面优化"; 
                break;
            default: 
                bottleneckStr = "系统运行良好，没有明显的性能瓶颈";
        }
        
        return QString("<h3>系统整体状态</h3>"
                      "<table width='100%' border='0' cellspacing='4'>"
                      "<tr><td>CPU使用率:</td><td><b>%1%</b></td></tr>"
                      "<tr><td>内存使用率:</td><td><b>%2%</b></td></tr>"
                      "<tr><td>磁盘I/O:</td><td><b>%3 MB/s</b></td></tr>"
                      "<tr><td>网络使用率:</td><td><b>%4 MB/s</b></td></tr>"
                      "</table>"
                      "<p><b>性能瓶颈分析:</b> %5</p>")
                      .arg(cpuUsage, 0, 'f', 1)
                      .arg(memUsage, 0, 'f', 1)
                      .arg(diskIO, 0, 'f', 2)
                      .arg(networkUsage, 0, 'f', 2)
                      .arg(bottleneckStr);
    }
    
    // 如果没有匹配的查询
    return QString("<p>抱歉，我无法理解您的查询。请尝试以下示例查询:</p>"
                  "<ul>"
                  "<li>系统状态如何?</li>"
                  "<li>CPU使用率是多少?</li>"
                  "<li>内存使用情况怎样?</li>"
                  "<li>当前磁盘活动是多少?</li>"
                  "<li>网络流量情况如何?</li>"
                  "</ul>");
}

// 新增：添加查询到历史记录
void AnalysisPage::addQueryToHistory(const QString &query, const QString &result)
{
    // 添加到内部存储
    m_queryHistory.prepend(qMakePair(query, result));
    
    // 限制历史记录大小
    if (m_queryHistory.size() > 20) {
        m_queryHistory.removeLast();
    }
}

// 新增：显示查询历史
void AnalysisPage::showQueryHistory()
{
    // 如果历史记录列表已经可见，则隐藏它
    if (m_queryHistoryList->isVisible()) {
        m_queryHistoryList->setVisible(false);
        return;
    }
    
    // 清除当前列表
    m_queryHistoryList->clear();
    
    // 填充历史记录
    for (const auto &pair : m_queryHistory) {
        QListWidgetItem *item = new QListWidgetItem(pair.first);
        item->setData(Qt::UserRole, pair.first); // 存储原始查询
        m_queryHistoryList->addItem(item);
    }
    
    // 如果没有历史记录
    if (m_queryHistoryList->count() == 0) {
        QListWidgetItem *item = new QListWidgetItem("暂无查询历史");
        item->setFlags(item->flags() & ~Qt::ItemIsEnabled);
        m_queryHistoryList->addItem(item);
    }
    
    // 显示历史记录列表
    m_queryHistoryList->setVisible(true);
}

// 新增：处理NLP查询结果
void AnalysisPage::handleNlpQueryResult(const QString &result)
{
    // 显示从模型接口返回的结果
    m_queryResultText->setHtml(result);
}
