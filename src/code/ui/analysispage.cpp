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
    , m_runAnomalyButton(new QPushButton("ִ���쳣���", this))
    , m_performanceTab(new QWidget(this))
    , m_bottleneckText(new QTextEdit(this))
    , m_trendText(new QTextEdit(this))
    , m_suggestionsText(new QTextEdit(this))
    , m_timeWindowCombo(new QComboBox(this))
    , m_runAnalysisButton(new QPushButton("ִ�����ܷ���", this))
    , m_currentAnomalyThreshold(2) // Default value
    , m_performCpuCheck(true)      // Default value
    , m_performMemoryCheck(true)   // Default value
    , m_performDiskCheck(true)     // Default value
    , m_performNetworkCheck(true)  // Default value
    // ����NLP��ѯ�����ʼ��
    , m_nlpQueryTab(new QWidget(this))
    , m_queryInput(new QLineEdit(this))
    , m_queryButton(new QPushButton(tr("��ѯ"), this))
    , m_queryResultText(new QTextEdit(this))
    , m_queryHistoryList(new QListWidget(this))
    , m_showHistoryButton(new QPushButton(tr("��ʷ"), this))
{
    setupUI();
    setupConnections();
    // Initialization for m_anomalyThresholdSpin, m_autoCpuCheck, etc. moved or handled by defaults/slots
    m_timeWindowCombo->addItem("���30����", 30);
    m_timeWindowCombo->addItem("���1Сʱ", 60);
    m_timeWindowCombo->addItem("���3Сʱ", 180);
    m_timeWindowCombo->addItem("���6Сʱ", 360);
    m_timeWindowCombo->addItem("���12Сʱ", 720);
    m_timeWindowCombo->addItem("���24Сʱ", 1440);
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
    anomalyLayout->addWidget(new QLabel("�쳣���:"));
    anomalyLayout->addWidget(m_anomalyResultsText);
    m_tabWidget->addTab(anomalyPageWidget, "�쳣���");

    // Performance Tab
    QWidget *performancePageWidget = new QWidget;
    QVBoxLayout *performanceLayout = new QVBoxLayout(performancePageWidget);
    performanceLayout->addWidget(new QLabel("ʱ�䴰��:"));
    performanceLayout->addWidget(m_timeWindowCombo);
    performanceLayout->addWidget(m_runAnalysisButton);
    performanceLayout->addWidget(new QLabel("ƿ������:"));
    performanceLayout->addWidget(m_bottleneckText);
    performanceLayout->addWidget(new QLabel("���Ʒ���:"));
    performanceLayout->addWidget(m_trendText);
    performanceLayout->addWidget(new QLabel("�Ż�����:"));
    performanceLayout->addWidget(m_suggestionsText);
    m_tabWidget->addTab(performancePageWidget, "���ܷ���");
    
    // ����NLP��ѯ��ǩҳ
    setupNlpQueryTab();

    setLayout(mainLayout);
}

// ����������NLP��ѯ��ǩҳ
void AnalysisPage::setupNlpQueryTab()
{
    QVBoxLayout *nlpLayout = new QVBoxLayout(m_nlpQueryTab);
    
    // ������ѯ��������
    QGroupBox *queryBox = new QGroupBox("��Ȼ���Բ�ѯ");
    QVBoxLayout *queryBoxLayout = new QVBoxLayout(queryBox);
    
    // ��Ӱ�����ʾ��ǩ
    QLabel *helpLabel = new QLabel("ʾ����ѯ: 'CPUʹ������ߵĽ�����ʲô?', '��ȥһСʱ�ڵ����������������?'");
    helpLabel->setWordWrap(true);
    helpLabel->setStyleSheet("color: #666666; font-style: italic;");
    queryBoxLayout->addWidget(helpLabel);
    
    // ��ѯ������
    QHBoxLayout *inputLayout = new QHBoxLayout();
    m_queryInput->setPlaceholderText("���������Ĳ�ѯ...");
    m_queryInput->setClearButtonEnabled(true);
    inputLayout->addWidget(m_queryInput, 1);
    
    // ��ѯ��ť
    // ֱ��ʹ����Դ�е�PNGͼ��
    m_queryButton->setIcon(QIcon(":/icons/icons/search.png"));
    m_queryButton->setCursor(Qt::PointingHandCursor);
    m_queryButton->setStyleSheet(
        "QPushButton { background-color: #2196F3; color: white; border: none; border-radius: 4px; "
        "padding: 8px 16px; } "
        "QPushButton:hover { background-color: #1976D2; } "
        "QPushButton:pressed { background-color: #0D47A1; }"
    );
    inputLayout->addWidget(m_queryButton);

    // ��ʷ��ѯ��ť
    // ֱ��ʹ����Դ�е�PNGͼ��
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
    
    // ��ӽ����ʾ����
    QGroupBox *resultBox = new QGroupBox("��ѯ���");
    QVBoxLayout *resultLayout = new QVBoxLayout(resultBox);
    m_queryResultText->setStyleSheet(
        "QTextEdit { background-color: #f8f8f8; border: 1px solid #e0e0e0; border-radius: 4px; "
        "font-family: 'Segoe UI', sans-serif; padding: 8px; }"
    );
    resultLayout->addWidget(m_queryResultText);
    
    // �����ʷ��¼�б�Ĭ�����أ�
    m_queryHistoryList->setVisible(false);
    m_queryHistoryList->setAlternatingRowColors(true);
    m_queryHistoryList->setStyleSheet(
        "QListWidget { background-color: #f5f5f5; border: 1px solid #e0e0e0; border-radius: 4px; "
        "alternate-background-color: #e9e9e9; }"
        "QListWidget::item { padding: 4px; }"
        "QListWidget::item:selected { background-color: #2196F3; color: white; }"
    );
    
    // �����������ӵ�������
    nlpLayout->addWidget(queryBox, 0);
    nlpLayout->addWidget(resultBox, 1);
    nlpLayout->addWidget(m_queryHistoryList, 1);
    
    // ��ӵ�����ǩҳ
    m_tabWidget->addTab(m_nlpQueryTab, "��Ȼ���Բ�ѯ");
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
    
    // ������NLP��ѯ�������
    connect(m_queryButton, &QPushButton::clicked, this, &AnalysisPage::executeNlpQuery);
    connect(m_queryInput, &QLineEdit::returnPressed, this, &AnalysisPage::executeNlpQuery);
    // �����ѯ��ʷ
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
        // ��˫����ʷ��¼ʱ������Ϊ��ǰ��ѯ
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

// ɾ��δ�����ķ�������Щ������ͷ�ļ���û������

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

// ������ִ����Ȼ���Բ�ѯ
void AnalysisPage::executeNlpQuery()
{
    QString query = m_queryInput->text().trimmed();
    if (query.isEmpty()) {
        // �����ѯΪ�գ���ʾ��ʾ
        m_queryResultText->setHtml("<span style='color:red'>��������Ч�Ĳ�ѯ</span>");
        return;
    }
    
    // ��ѯǰ��ʾ����״̬
    m_queryResultText->setHtml("<i>���ڴ����ѯ...</i>");
    QApplication::processEvents();
    
    // �����ѯ
    QString result = processNlpQuery(query);
    
    // ��ʾ���
    m_queryResultText->setHtml(result);
    
    // ��ӵ���ʷ��¼
    addQueryToHistory(query, result);
    
    // ��������
    m_queryInput->clear();
    
    // ������ʷ��¼�б�����ɼ���
    if (m_queryHistoryList->isVisible()) {
        m_queryHistoryList->setVisible(false);
    }
    
    // ���Ͳ�ѯ�����ź�
    emit naturalLanguageQueryRequested(query);
}

// ������������Ȼ���Բ�ѯ
QString AnalysisPage::processNlpQuery(const QString &query)
{
    // �������棺ƥ���ѯ����Ӧ֮����߼�
    
    // CPU��ز�ѯ
    if (query.contains(QRegularExpression("\\bCPU\\b.*ʹ����", QRegularExpression::CaseInsensitiveOption)) ||
        query.contains(QRegularExpression("������.*ռ��", QRegularExpression::CaseInsensitiveOption))) {
        
        double cpuUsage = m_performanceAnalyzer->getLastCpuUsage();
        QString trend;
        switch (m_performanceAnalyzer->analyzeCpuTrend()) {
            case PerformanceAnalyzer::TrendType::Increasing: trend = "����"; break;
            case PerformanceAnalyzer::TrendType::Decreasing: trend = "�½�"; break;
            case PerformanceAnalyzer::TrendType::Fluctuating: trend = "����"; break;
            default: trend = "�ȶ�";
        }
        
        return QString("<h3>CPUʹ���ʷ���</h3>"
                      "<p>��ǰCPUʹ����: <b>%1%</b></p>"
                      "<p>����: <b>%2</b></p>"
                      "<p>�ڹ�ȥ��30�����ڣ�CPUʹ����%3��</p>")
                      .arg(cpuUsage, 0, 'f', 1)
                      .arg(trend)
                      .arg(trend == "����" ? "�����������ƣ��������Ƿ�����Դ�ܼ��ͽ���" : 
                           trend == "�½�" ? "�𽥽��ͣ�ϵͳ�������ڼ���" :
                           trend == "����" ? "�����Բ����������Ƕ�ʱ������" : "�����ȶ�");
    }
    
    // �ڴ���ز�ѯ
    if (query.contains(QRegularExpression("\\b�ڴ�\\b", QRegularExpression::CaseInsensitiveOption)) ||
        query.contains(QRegularExpression("\\bRAM\\b", QRegularExpression::CaseInsensitiveOption)) ||
        query.contains(QRegularExpression("\\b�ڴ�ʹ��\\b", QRegularExpression::CaseInsensitiveOption))) {
        
        double memUsage = m_performanceAnalyzer->getLastMemoryUsage();
        QString riskLevel;
        
        if (memUsage > 90) riskLevel = "����";
        else if (memUsage > 80) riskLevel = "��";
        else if (memUsage > 60) riskLevel = "�е�";
        else riskLevel = "��";
        
        return QString("<h3>�ڴ�ʹ�÷���</h3>"
                      "<p>��ǰ�ڴ�ʹ����: <b>%1%</b></p>"
                      "<p>���ռ���: <b>%2</b></p>"
                      "<p>%3</p>")
                      .arg(memUsage, 0, 'f', 1)
                      .arg(riskLevel)
                      .arg(riskLevel == "����" ? "�ڴ����ز��㣬���������رղ���Ҫ�ĳ������ͷ��ڴ�" : 
                           riskLevel == "��" ? "�ڴ�ʹ�ýϸߣ����ܻ�Ӱ��ϵͳ���ܣ�����رղ���Ҫ�ĳ���" :
                           riskLevel == "�е�" ? "�ڴ�ʹ�ô���������Χ���������Ż��ռ�" : 
                           "�ڴ�ʹ�ô��ڽ���ˮƽ�����赣��");
    }
    
    // ������ز�ѯ
    if (query.contains(QRegularExpression("\\b����\\b", QRegularExpression::CaseInsensitiveOption)) ||
        query.contains(QRegularExpression("\\bIO\\b", QRegularExpression::CaseInsensitiveOption)) ||
        query.contains(QRegularExpression("�洢", QRegularExpression::CaseInsensitiveOption))) {
        
        double diskIO = m_performanceAnalyzer->getLastDiskIO();
        
        return QString("<h3>���̻����</h3>"
                      "<p>��ǰ����I/O�: <b>%1 MB/s</b></p>"
                      "<p>%2</p>"
                      "<p>����ʹ�����:</p>")
                      .arg(diskIO, 0, 'f', 2)
                      .arg(diskIO > 50 ? "���̻�ϸߣ����ܴ��ڴ��ļ���������ݿ����" : 
                           diskIO > 20 ? "���̻�����е�ˮƽ" : "���̻�ϵͣ�ϵͳ��������");
        
        // ���������Ӵ���ʹ������ı��
    }
    
    // ������ز�ѯ
    if (query.contains(QRegularExpression("\\b����\\b", QRegularExpression::CaseInsensitiveOption)) ||
        query.contains(QRegularExpression("\\b����\\b", QRegularExpression::CaseInsensitiveOption)) ||
        query.contains(QRegularExpression("\\b����\\b", QRegularExpression::CaseInsensitiveOption))) {
        
        double networkUsage = m_performanceAnalyzer->getLastNetworkUsage();
        
        return QString("<h3>��������</h3>"
                      "<p>��ǰ����: <b>%1 MB/s</b></p>"
                      "<p>%2</p>")
                      .arg(networkUsage, 0, 'f', 2)
                      .arg(networkUsage > 50 ? "����ʹ���ʽϸߣ��������ڽ��д��ļ����ػ���ý�岥��" : 
                           networkUsage > 10 ? "����ʹ�ô����е�ˮƽ" : "����ʹ�ýϵͣ������ȶ�");
    }
    
    // ϵͳ�����ѯ
    if (query.contains(QRegularExpression("\\bϵͳ\\b.*\\b״̬\\b", QRegularExpression::CaseInsensitiveOption)) ||
        query.contains(QRegularExpression("\\b����\\b.*\\b����\\b", QRegularExpression::CaseInsensitiveOption)) ||
        query.contains(QRegularExpression("\\b�ſ�\\b", QRegularExpression::CaseInsensitiveOption))) {
        
        double cpuUsage = m_performanceAnalyzer->getLastCpuUsage();
        double memUsage = m_performanceAnalyzer->getLastMemoryUsage();
        double diskIO = m_performanceAnalyzer->getLastDiskIO();
        double networkUsage = m_performanceAnalyzer->getLastNetworkUsage();
        
        PerformanceAnalyzer::BottleneckType bottleneck = m_performanceAnalyzer->analyzeBottleneck();
        QString bottleneckStr;
        
        switch (bottleneck) {
            case PerformanceAnalyzer::BottleneckType::CPU: 
                bottleneckStr = "CPU���ܳ�Ϊϵͳƿ��������رղ���Ҫ�ĸ�CPUռ�ó���"; 
                break;
            case PerformanceAnalyzer::BottleneckType::Memory: 
                bottleneckStr = "�ڴ���ܳ�Ϊϵͳƿ��������ر��ڴ�ռ�ô��Ӧ��"; 
                break;
            case PerformanceAnalyzer::BottleneckType::Disk: 
                bottleneckStr = "����I/O���ܳ�Ϊϵͳƿ����������ٴ��ļ�����"; 
                break;
            case PerformanceAnalyzer::BottleneckType::Network: 
                bottleneckStr = "������ܳ�Ϊϵͳƿ�����������Ƹߴ���Ӧ��"; 
                break;
            case PerformanceAnalyzer::BottleneckType::Multiple: 
                bottleneckStr = "ϵͳ���ڶ������ƿ��������ȫ���Ż�"; 
                break;
            default: 
                bottleneckStr = "ϵͳ�������ã�û�����Ե�����ƿ��";
        }
        
        return QString("<h3>ϵͳ����״̬</h3>"
                      "<table width='100%' border='0' cellspacing='4'>"
                      "<tr><td>CPUʹ����:</td><td><b>%1%</b></td></tr>"
                      "<tr><td>�ڴ�ʹ����:</td><td><b>%2%</b></td></tr>"
                      "<tr><td>����I/O:</td><td><b>%3 MB/s</b></td></tr>"
                      "<tr><td>����ʹ����:</td><td><b>%4 MB/s</b></td></tr>"
                      "</table>"
                      "<p><b>����ƿ������:</b> %5</p>")
                      .arg(cpuUsage, 0, 'f', 1)
                      .arg(memUsage, 0, 'f', 1)
                      .arg(diskIO, 0, 'f', 2)
                      .arg(networkUsage, 0, 'f', 2)
                      .arg(bottleneckStr);
    }
    
    // ���û��ƥ��Ĳ�ѯ
    return QString("<p>��Ǹ�����޷�������Ĳ�ѯ���볢������ʾ����ѯ:</p>"
                  "<ul>"
                  "<li>ϵͳ״̬���?</li>"
                  "<li>CPUʹ�����Ƕ���?</li>"
                  "<li>�ڴ�ʹ���������?</li>"
                  "<li>��ǰ���̻�Ƕ���?</li>"
                  "<li>��������������?</li>"
                  "</ul>");
}

// ��������Ӳ�ѯ����ʷ��¼
void AnalysisPage::addQueryToHistory(const QString &query, const QString &result)
{
    // ��ӵ��ڲ��洢
    m_queryHistory.prepend(qMakePair(query, result));
    
    // ������ʷ��¼��С
    if (m_queryHistory.size() > 20) {
        m_queryHistory.removeLast();
    }
}

// ��������ʾ��ѯ��ʷ
void AnalysisPage::showQueryHistory()
{
    // �����ʷ��¼�б��Ѿ��ɼ�����������
    if (m_queryHistoryList->isVisible()) {
        m_queryHistoryList->setVisible(false);
        return;
    }
    
    // �����ǰ�б�
    m_queryHistoryList->clear();
    
    // �����ʷ��¼
    for (const auto &pair : m_queryHistory) {
        QListWidgetItem *item = new QListWidgetItem(pair.first);
        item->setData(Qt::UserRole, pair.first); // �洢ԭʼ��ѯ
        m_queryHistoryList->addItem(item);
    }
    
    // ���û����ʷ��¼
    if (m_queryHistoryList->count() == 0) {
        QListWidgetItem *item = new QListWidgetItem("���޲�ѯ��ʷ");
        item->setFlags(item->flags() & ~Qt::ItemIsEnabled);
        m_queryHistoryList->addItem(item);
    }
    
    // ��ʾ��ʷ��¼�б�
    m_queryHistoryList->setVisible(true);
}

// ����������NLP��ѯ���
void AnalysisPage::handleNlpQueryResult(const QString &result)
{
    // ��ʾ��ģ�ͽӿڷ��صĽ��
    m_queryResultText->setHtml(result);
}
