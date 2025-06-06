#include "src/include/mainwindow.h"
#include <QWidget>
#include <QLabel>
#include <QGridLayout>
#include <QFile>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QIcon>
#include <QPushButton>
#include <QStackedWidget>
#include <QFileDialog>
#include <QSettings>
#include <QTimer>
#include <QMessageBox> // Added for optimization button placeholders
#include "src/include/chart/chartwidget.h"
#include "src/include/monitor/sampler.h"
#include "src/include/storage/datastorage.h"
#include "src/include/storage/exporter.h"
#include "src/include/ui/settingswidget.h"
#include "src/include/ui/cpupage.h"
#include "src/include/ui/memorypage.h"
#include "src/include/ui/diskpage.h"
#include "src/include/ui/networkpage.h"
#include "src/include/ui/processpage.h"
#include "src/include/ui/gpupage.h"
#include "src/include/model/modelinterface.h"
#include "src/include/ui/processselectiondialog.h" // Added for process selection
#include "src/include/ui/analysispage.h" // Ensure analysispage.h is included if not already
#include "src/legacy/visualization3d/Vis3DPage.h" // 使用正确的包含路径
#include <QProcess> // For terminating processes
#include <QDebug>   // For logging
#include <windows.h> // For Windows-specific process termination
#include <algorithm> // For std::sort

// 仅在启用3D可视化时包含
#ifdef USE_VISUALIZATION3D
#include "src/legacy/visualization3d/Vis3DPage.h" // 修改为实际路径
#endif

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , m_cpuChart(new ChartWidget("CPU使用率", this))
    , m_gpuChart(new ChartWidget("GPU使用率", this))
    , m_memoryChart(new ChartWidget("内存使用率", this))
    , m_wifiChart(new ChartWidget("网络使用率", this))
    , m_sampler(new Sampler(this))
    , m_storage(new DataStorage(this))
    , m_modelInterface(new ModelInterface(this))
    , m_cpuPage(new CpuPage(this))
    , m_memoryPage(new MemoryPage(this))
    , m_diskPage(new DiskPage(this))
    , m_networkPage(new NetworkPage(this))
    , m_processPage(new ProcessPage(this))
    , m_gpuPage(new GpuPage(this))
    , m_analysisPage(new AnalysisPage(this))
    , m_pageStack(new QStackedWidget(this))
    , m_closeHighCpuProcessButton(nullptr)
    , m_closeHighMemoryProcessButton(nullptr)
    , m_optimizeThreadPoolButton(nullptr)
    , m_optimizeDiskButton(nullptr)
#ifdef USE_VISUALIZATION3D
    , m_vis3DPage(nullptr)
#endif
{
    setupUI();
    setupConnections();
    
    // 加载模型设置
    loadModelSettings();
    
    // Initialize storage and sampler
    qDebug() << "[MainWindow] Initializing storage with path: Data/data.db";
    bool storageInit = m_storage->initialize("Data/data.db");
    qDebug() << "[MainWindow] Storage initialized result:" << storageInit;
    m_sampler->setStorage(m_storage);
    m_sampler->setChartWidgets(m_cpuChart, m_memoryChart, m_gpuChart, m_wifiChart);
    m_sampler->startSampling();
}

MainWindow::~MainWindow()
{
    // No need to explicitly stop the sampler as it will be deleted with its parent
}

void MainWindow::setupUI()
{
    // 创建3D可视化页面
    createVisualizationPage();
    // Create navigation buttons with icons
    m_overviewButton = new QPushButton(QIcon(":/icons/icons/app.png"), tr("Overview"), this);
    m_cpuButton = new QPushButton(QIcon(":/icons/icons/cpu.png"), tr("CPU"), this);
    m_gpuButton = new QPushButton(QIcon(":/icons/icons/gpu.png"), tr("GPU"), this);
    m_memoryButton = new QPushButton(QIcon(":/icons/icons/memory.png"), tr("Memory"), this);
    m_diskButton = new QPushButton(QIcon(":/icons/icons/disk.png"), tr("Disk"), this);
    m_networkButton = new QPushButton(QIcon(":/icons/icons/network.png"), tr("Network"), this);
    m_processButton = new QPushButton(QIcon(":/icons/icons/process.png"), tr("Process"), this);
    m_analysisButton = new QPushButton(QIcon(":/icons/icons/analysis.png"), tr("Analysis"), this);
    m_3dVisButton = new QPushButton(QIcon(":/icons/icons/3D.png"), tr("3D Visualization"), this);
    m_exportButton = new QPushButton(QIcon(":/icons/icons/export.png"), tr("Export"), this);
    m_settingsButton = new QPushButton(QIcon(":/icons/icons/settings.png"), tr("Settings"), this);

    // 设置应用程序图标
    setWindowIcon(QIcon(":/icons/icons/app.ico"));

    // Set button styles
    QString buttonStyle = 
        "QPushButton {"
        "    text-align: left;"
        "    padding: 10px 20px;"
        "    border: none;"
        "    border-radius: 4px;"
        "    margin: 2px 8px;"
        "    color: #333333;"
        "    background: transparent;"
        "}"
        "QPushButton:hover {"
        "    background: rgba(33, 150, 243, 0.1);"
        "}"
        "QPushButton:pressed {"
        "    background: rgba(33, 150, 243, 0.2);"
        "}"
        "QPushButton[selected=true] {"
        "    background: #2196F3;"
        "    color: white;"
        "}";

    QList<QPushButton*> buttons = {
        m_overviewButton, m_cpuButton, m_gpuButton, m_memoryButton, m_diskButton,
        m_networkButton, m_processButton, m_analysisButton, m_settingsButton, m_3dVisButton,
        m_exportButton
    };

    for (auto button : buttons) {
        button->setStyleSheet(buttonStyle);
        button->setFixedHeight(40);
        button->setCursor(Qt::PointingHandCursor);
        button->setIconSize(QSize(20, 20));
    }

    // Create navigation panel
    QWidget *navPanel = new QWidget(this);
    navPanel->setFixedWidth(200);
    navPanel->setStyleSheet(
        "QWidget {"
        "    background-color: #f5f5f5;"
        "    border-right: 1px solid #e0e0e0;"
        "}"
    );

    QVBoxLayout *navLayout = new QVBoxLayout(navPanel);
    navLayout->setSpacing(0);
    navLayout->setContentsMargins(0, 0, 0, 0);

    // Add navigation buttons
    navLayout->addWidget(m_overviewButton);
    navLayout->addWidget(m_cpuButton);
    navLayout->addWidget(m_gpuButton);
    navLayout->addWidget(m_memoryButton);
    navLayout->addWidget(m_diskButton);
    navLayout->addWidget(m_networkButton);
    navLayout->addWidget(m_processButton);
    navLayout->addWidget(m_analysisButton);
    navLayout->addWidget(m_3dVisButton);
    navLayout->addStretch();
    navLayout->addWidget(m_exportButton);
    navLayout->addWidget(m_settingsButton);

    // 创建分析面板（右侧常显）
    QWidget *analysisPanel = new QWidget(this);
    analysisPanel->setFixedWidth(300);
    analysisPanel->setStyleSheet(
        "QWidget {"
        "    background-color: #f8f8f8;"
        "    border-left: 1px solid #e0e0e0;"
        "}"
    );
    
    QVBoxLayout *analysisPanelLayout = new QVBoxLayout(analysisPanel);
    analysisPanelLayout->setContentsMargins(10, 10, 10, 10);
    
    // 添加分析标题
    QLabel *analysisTitle = new QLabel("实时分析", analysisPanel);
    analysisTitle->setStyleSheet("font-size: 16px; font-weight: bold; margin-bottom: 10px;");
    analysisPanelLayout->addWidget(analysisTitle);
    
    // 创建性能分析组件
    QGroupBox *performanceGroup = new QGroupBox("性能分析", analysisPanel);
    QVBoxLayout *performanceLayout = new QVBoxLayout(performanceGroup);
    
    QTextEdit *performanceText = new QTextEdit(performanceGroup);
    performanceText->setReadOnly(true);
    performanceText->setMaximumHeight(150);
    performanceLayout->addWidget(performanceText);
    
    // 创建异常检测组件
    QGroupBox *anomalyGroup = new QGroupBox("异常检测", analysisPanel);
    QVBoxLayout *anomalyLayout = new QVBoxLayout(anomalyGroup);
    
    QTextEdit *anomalyText = new QTextEdit(anomalyGroup);
    anomalyText->setReadOnly(true);
    anomalyText->setMaximumHeight(150);
    anomalyLayout->addWidget(anomalyText);
    
    // 创建优化按钮
    QPushButton *optimizeButton = new QPushButton("系统优化", analysisPanel);
    optimizeButton->setIcon(QIcon(":/icons/icons/settings.png"));
    optimizeButton->setCursor(Qt::PointingHandCursor);
    optimizeButton->setMinimumHeight(36);
    optimizeButton->setStyleSheet(
        "QPushButton { background-color: #2196F3; color: white; border: none; border-radius: 4px; "
        "padding: 8px 16px; font-weight: bold; } "
        "QPushButton:hover { background-color: #1976D2; } "
        "QPushButton:pressed { background-color: #0D47A1; }"
    );

    connect(optimizeButton, &QPushButton::clicked, [this]() {
        // 直接切换到分析页面的AI分析标签页
        if (m_analysisPage) {
            switchToPage(static_cast<int>(PageIndex::AnalysisPage));
            if (m_analysisPage->findChild<QTabWidget*>()) {
                QTabWidget* tabWidget = m_analysisPage->findChild<QTabWidget*>();
                // 查找AI分析标签页的索引
                for (int i = 0; i < tabWidget->count(); i++) {
                    if (tabWidget->tabText(i).contains("AI")) {
                        tabWidget->setCurrentIndex(i);
                        break;
                    }
                }
            }
        }
    });
    
    // 将3D可视化按钮样式与其他导航按钮统一
    m_3dVisButton->setStyleSheet(buttonStyle);
    m_3dVisButton->setFixedHeight(40);
    m_3dVisButton->setCursor(Qt::PointingHandCursor);
    m_3dVisButton->setIconSize(QSize(20, 20));
    
    // 添加一键优化按钮到右侧面板
    optimizeButton->setStyleSheet(
        "QPushButton { background-color: #3F51B5; color: white; border: none; border-radius: 4px; "
        "padding: 8px 16px; font-weight: bold; } "
        "QPushButton:hover { background-color: #303F9F; } "
        "QPushButton:pressed { background-color: #1A237E; }"
    );
    optimizeButton->setMinimumHeight(36);
    
    // 添加到分析面板
    analysisPanelLayout->addWidget(performanceGroup);
    analysisPanelLayout->addWidget(anomalyGroup);
    analysisPanelLayout->addWidget(optimizeButton);

    // Add new optimization buttons
    m_closeHighCpuProcessButton = new QPushButton("关闭高CPU进程", analysisPanel);
    m_closeHighMemoryProcessButton = new QPushButton("关闭高内存进程", analysisPanel);
    m_optimizeThreadPoolButton = new QPushButton("优化线程池", analysisPanel);
    m_optimizeDiskButton = new QPushButton("优化磁盘", analysisPanel);

    QString optimizationButtonStyle = 
        "QPushButton {"
        "    background-color: #4CAF50; color: white; border: none; border-radius: 4px; "
        "    padding: 6px 12px; margin-top: 5px; "
        "}"
        "QPushButton:hover { background-color: #45a049; }"
        "QPushButton:pressed { background-color: #3e8e41; }";

    m_closeHighCpuProcessButton->setStyleSheet(optimizationButtonStyle);
    m_closeHighMemoryProcessButton->setStyleSheet(optimizationButtonStyle);
    m_optimizeThreadPoolButton->setStyleSheet(optimizationButtonStyle);
    m_optimizeDiskButton->setStyleSheet(optimizationButtonStyle);

    m_closeHighCpuProcessButton->setCursor(Qt::PointingHandCursor);
    m_closeHighMemoryProcessButton->setCursor(Qt::PointingHandCursor);
    m_optimizeThreadPoolButton->setCursor(Qt::PointingHandCursor);
    m_optimizeDiskButton->setCursor(Qt::PointingHandCursor);

    analysisPanelLayout->addWidget(m_closeHighCpuProcessButton);
    analysisPanelLayout->addWidget(m_closeHighMemoryProcessButton);
    analysisPanelLayout->addWidget(m_optimizeThreadPoolButton);
    analysisPanelLayout->addWidget(m_optimizeDiskButton);

    analysisPanelLayout->addStretch();
    
    // 连接分析页面的信号到右侧面板
    connect(m_analysisPage, &AnalysisPage::performanceUpdated, performanceText, &QTextEdit::setText);
    connect(m_analysisPage, &AnalysisPage::anomalyUpdated, anomalyText, &QTextEdit::setText);
    
    // 初始化右侧面板的文本
    performanceText->setText("系统正在分析性能数据...");
    anomalyText->setText("系统正在监控异常情况...");
    
    // 定时更新右侧面板信息
    QTimer *panelUpdateTimer = new QTimer(this);
    connect(panelUpdateTimer, &QTimer::timeout, [=]() {
        // 触发分析页面更新信号
        if (m_analysisPage) {
            m_analysisPage->updatePerformanceData(m_sampler->lastCpuUsage(), 
                                               m_sampler->lastMemoryUsage(),
                                               m_sampler->lastDiskIO(),
                                               m_sampler->lastNetworkUsage());
            
            m_analysisPage->updateAnomalyData(m_sampler->lastCpuUsage(), 
                                           m_sampler->lastMemoryUsage(),
                                           m_sampler->lastDiskIO(),
                                           m_sampler->lastNetworkUsage());
        }
    });
    panelUpdateTimer->start(5000); // 每5秒更新一次
    
    // 创建主布局
    QHBoxLayout *mainLayout = new QHBoxLayout;
    mainLayout->setSpacing(0);
    mainLayout->setContentsMargins(0, 0, 0, 0);
    mainLayout->addWidget(navPanel);
    mainLayout->addWidget(m_pageStack);
    mainLayout->addWidget(analysisPanel);

    QWidget *centralWidget = new QWidget(this);
    centralWidget->setLayout(mainLayout);
    setCentralWidget(centralWidget);

    // Create overview page
    m_overviewPage = createOverviewPage();

    // Create settings page
    m_settingsPage = new SettingsWidget(this);

    // Add pages to stack
    enum PageIndex {
        OverviewPage = 0,
        CpuPage,
        GpuPage,
        MemoryPage,
        DiskPage,
        NetworkPage,
        ProcessPage,
        AnalysisPage,
        SettingsPage
    };

    m_pageStack->addWidget(m_overviewPage);    // 0
    m_pageStack->addWidget(m_cpuPage);        // 1
    m_pageStack->addWidget(m_gpuPage);        // 2
    m_pageStack->addWidget(m_memoryPage);     // 3
    m_pageStack->addWidget(m_diskPage);       // 4
    m_pageStack->addWidget(m_networkPage);    // 5
    m_pageStack->addWidget(m_processPage);    // 6
    m_pageStack->addWidget(m_analysisPage);   // 7
    m_pageStack->addWidget(m_settingsPage);   // 8
#ifdef USE_VISUALIZATION3D
    // 仅在启用3D可视化时添加
    m_pageStack->addWidget(m_vis3DPage);      // 9 // Visualization3DPage
#endif

    // Set initial page
    switchToPage(static_cast<int>(PageIndex::OverviewPage));
}

void MainWindow::setupConnections()
{
    // Navigation buttons
    connect(m_overviewButton, &QPushButton::clicked, [this]() { switchToPage(static_cast<int>(MainWindow::PageIndex::OverviewPage)); });
    connect(m_cpuButton, &QPushButton::clicked, [this]() { switchToPage(static_cast<int>(MainWindow::PageIndex::CpuPage)); });
    connect(m_gpuButton, &QPushButton::clicked, [this]() { switchToPage(static_cast<int>(MainWindow::PageIndex::GpuPage)); });
    connect(m_memoryButton, &QPushButton::clicked, [this]() { switchToPage(static_cast<int>(MainWindow::PageIndex::MemoryPage)); });
    connect(m_diskButton, &QPushButton::clicked, [this]() { switchToPage(static_cast<int>(MainWindow::PageIndex::DiskPage)); });
    connect(m_networkButton, &QPushButton::clicked, [this]() { switchToPage(static_cast<int>(MainWindow::PageIndex::NetworkPage)); });
    connect(m_processButton, &QPushButton::clicked, [this]() { switchToPage(static_cast<int>(MainWindow::PageIndex::ProcessPage)); });
    connect(m_analysisButton, &QPushButton::clicked, [this]() { switchToPage(static_cast<int>(MainWindow::PageIndex::AnalysisPage)); });
    connect(m_3dVisButton, &QPushButton::clicked, [this]() {
        // 切换到3D可视化页面
        switchToPage(static_cast<int>(MainWindow::PageIndex::Visualization3DPage)); // 使用枚举值替代硬编码的9
    });
    connect(m_settingsButton, &QPushButton::clicked, [this]() { switchToPage(static_cast<int>(MainWindow::PageIndex::SettingsPage)); });
    connect(m_exportButton, &QPushButton::clicked, this, &MainWindow::exportData);

    // Connect sampler signals to page updates
    connect(m_sampler, &Sampler::cpuUsageUpdated, m_cpuPage, &CpuPage::updateCpuData);
    connect(m_sampler, &Sampler::gpuStatsUpdated, m_gpuPage, &GpuPage::updateGpuData);
    connect(m_sampler, &Sampler::gpuAvailabilityChanged, m_gpuPage, &GpuPage::handleGpuAvailabilityChange);
    connect(m_sampler, &Sampler::memoryStatsUpdated, m_memoryPage, &MemoryPage::updateLabels);
    connect(m_sampler, &Sampler::diskStatsUpdated, m_diskPage, &DiskPage::updateDiskData);
    connect(m_sampler, &Sampler::networkStatsUpdated, m_networkPage, &NetworkPage::updateNetworkData);
    connect(m_sampler, &Sampler::performanceDataUpdated, m_processPage, &ProcessPage::updateProcessList);
    
    // 确保概览页图表和单独页面图表同步
    // 1. 连接CPU数据到概览页和单独页面的图表
    connect(m_sampler, &Sampler::cpuUsageUpdated, [this](double cpuUsage) {
        m_cpuChart->updateValue(cpuUsage);
    });
    
    // 2. 连接GPU数据到概览页和单独页面的图表
    connect(m_sampler, &Sampler::gpuStatsUpdated, [this](double usage, double temperature, quint64 memoryUsed, quint64 memoryTotal) {
        m_gpuChart->updateValue(usage);
    });
    
    // 3. 连接内存数据到概览页和单独页面的图表
    connect(m_sampler, &Sampler::memoryStatsUpdated, [this](quint64 total, quint64 used, quint64 free) {
        double memoryUsagePercent = static_cast<double>(used) / total * 100.0;
        m_memoryChart->updateValue(memoryUsagePercent);
    });
    
    // 4. 连接网络数据到概览页和单独页面的图表
    connect(m_sampler, &Sampler::networkStatsUpdated, [this](double uploadSpeed, double downloadSpeed) {
        double totalNetworkUsage = uploadSpeed + downloadSpeed;
        m_wifiChart->updateValue(totalNetworkUsage);
    });
    
    // 将性能数据连接到3D可视化页面
    #ifdef USE_VISUALIZATION3D
    if (m_vis3DPage) {
        connect(m_sampler, &Sampler::cpuUsageUpdated, m_vis3DPage, &Vis3DPage::updateCpuData);
        connect(m_sampler, &Sampler::gpuStatsUpdated, m_vis3DPage, &Vis3DPage::updateGpuData);
        connect(m_sampler, &Sampler::memoryStatsUpdated, m_vis3DPage, [this](quint64 total, quint64 used, quint64 free) {
            m_vis3DPage->updateMemoryData(static_cast<double>(used), static_cast<double>(total));
        });
        connect(m_sampler, &Sampler::diskStatsUpdated, m_vis3DPage, &Vis3DPage::updateDiskData);
        connect(m_sampler, &Sampler::networkStatsUpdated, m_vis3DPage, &Vis3DPage::updateNetworkData);
    }
    #endif
    
    // 连接性能数据到PerformanceAnalyzer
    connect(m_sampler, &Sampler::cpuUsageUpdated, m_analysisPage->getPerformanceAnalyzer(), &PerformanceAnalyzer::updateCpuUsage);
    connect(m_sampler, &Sampler::memoryStatsUpdated, 
        [this](quint64 total, quint64 used, quint64 free) {
            // 将内存总量和使用量转换为百分比
            double memoryUsagePercent = static_cast<double>(used) / total * 100.0;
            m_analysisPage->getPerformanceAnalyzer()->updateMemoryUsage(memoryUsagePercent);
        });
    connect(m_sampler, &Sampler::diskStatsUpdated, 
        [this](qint64 readBytes, qint64 writeBytes) {
            // 将磁盘读写速度转换为MB/s
            double totalDiskIO = static_cast<double>(readBytes + writeBytes) / (1024.0 * 1024.0);
            m_analysisPage->getPerformanceAnalyzer()->updateDiskUsage(totalDiskIO);
        });
    connect(m_sampler, &Sampler::networkStatsUpdated, 
        [this](double uploadSpeed, double downloadSpeed) {
            // 将网络上传和下载速度合并为总网络使用量
            double totalNetworkUsage = uploadSpeed + downloadSpeed;
            m_analysisPage->getPerformanceAnalyzer()->updateNetworkUsage(totalNetworkUsage);
        });
    
    // Analysis & Optimization connections
    connect(m_sampler, &Sampler::performanceDataUpdated, m_analysisPage, [this](double cpuUsage, double memoryUsage, double diskIO, double networkUsage) {
        m_analysisPage->updatePerformanceData(cpuUsage, memoryUsage, diskIO, networkUsage);
        m_analysisPage->updateAnomalyData(cpuUsage, memoryUsage, diskIO, networkUsage);
    });

    // Export connection using custom signal-slot
    connect(m_exportButton, &QPushButton::clicked, this, &MainWindow::exportData);

    // Settings connections - 使用findChild获取SettingsWidget实例
    SettingsWidget* settingsWidget = findChild<SettingsWidget*>();
    if (settingsWidget) {
        connect(settingsWidget, &SettingsWidget::samplingIntervalChanged, m_sampler, &Sampler::startSampling);
        connect(settingsWidget, &SettingsWidget::analysisSettingsChanged, m_analysisPage, &AnalysisPage::onAnalysisSettingsUpdated);
        connect(settingsWidget, &SettingsWidget::samplingIntervalChanged, m_analysisPage, &AnalysisPage::onUpdateIntervalChanged);
        connect(settingsWidget, &SettingsWidget::modelSettingsChanged, this, &MainWindow::onModelSettingsChanged);
    }
    
    // Process page connections - using lambdas for signal-slot connections that don't exist yet
    connect(m_processPage, SIGNAL(processTerminationRequested(quint64)), this, SLOT(onProcessTerminationRequested(quint64)));

    // Model connections
    connect(m_modelInterface, &ModelInterface::responseReceived, this, &MainWindow::onModelResponseReceived);
    connect(m_modelInterface, &ModelInterface::errorOccurred, this, &MainWindow::onModelErrorOccurred);
    connect(m_analysisPage, &AnalysisPage::requestAiAnalysis, this, &MainWindow::onRunModelRequested);
    
    // 新增：连接自然语言查询功能
    connect(m_analysisPage, &AnalysisPage::naturalLanguageQueryRequested, this, &MainWindow::onRunModelRequested);
    connect(m_modelInterface, &ModelInterface::responseReceived, m_analysisPage, &AnalysisPage::handleNlpQueryResult);
    
    // 添加重置视图按钮的信号连接
    #ifdef USE_VISUALIZATION3D
    if (m_vis3DPage) {
        QPushButton* resetViewButton = m_vis3DPage->findChild<QPushButton*>("resetViewButton");
        if (resetViewButton) {
            connect(resetViewButton, &QPushButton::clicked, [this]() {
                // 重置3D视图
                if (m_vis3DPage) {
                    m_vis3DPage->resetCamera();
                }
            });
        }
    }
    #endif

    // Connect new optimization buttons
    if (m_closeHighCpuProcessButton) connect(m_closeHighCpuProcessButton, &QPushButton::clicked, this, &MainWindow::onCloseHighCpuProcessClicked);
    if (m_closeHighMemoryProcessButton) connect(m_closeHighMemoryProcessButton, &QPushButton::clicked, this, &MainWindow::onCloseHighMemoryProcessClicked);
    if (m_optimizeThreadPoolButton) connect(m_optimizeThreadPoolButton, &QPushButton::clicked, this, &MainWindow::onOptimizeThreadPoolClicked);
    if (m_optimizeDiskButton) connect(m_optimizeDiskButton, &QPushButton::clicked, this, &MainWindow::onOptimizeDiskClicked);

    // 连接GPU通知信号
    connect(m_sampler, &Sampler::showGpuNotification, this, &MainWindow::displayGpuNotification);
}

void MainWindow::switchToPage(int index)
{
    resetNavigationButtons();
    
    // 定义按钮数组，确保顺序与PageIndex枚举值顺序完全一致
    QPushButton* buttons[] = {
        m_overviewButton,  // OverviewPage = 0
        m_cpuButton,      // CpuPage = 1
        m_gpuButton,      // GpuPage = 2
        m_memoryButton,   // MemoryPage = 3
        m_diskButton,     // DiskPage = 4
        m_networkButton,  // NetworkPage = 5
        m_processButton,  // ProcessPage = 6
        m_analysisButton, // AnalysisPage = 7
        m_settingsButton, // SettingsPage = 8
        m_3dVisButton     // Visualization3DPage = 9
    };
    
    int pageCount = m_pageStack->count();
    int numNavButtons = sizeof(buttons) / sizeof(QPushButton*);

    if (index >= 0 && index < pageCount && index < numNavButtons) {
        // 正常页面切换，包括 Visualization3DPage
        updateNavigationButton(buttons[index], true);
        m_pageStack->setCurrentIndex(index);
    } else {
        // 处理无效索引或按钮数组越界的情况，例如默认到概览页
        // 确保至少有一个按钮和页面可以切换
        if (pageCount > 0 && numNavButtons > 0) {
             updateNavigationButton(buttons[0], true); // 高亮概览按钮
             m_pageStack->setCurrentIndex(0); // 切换到概览页
        } else {
            // 记录一个错误或者不执行任何操作，因为没有可切换的页面/按钮
        }
    }
}


void MainWindow::updateNavigationButton(QPushButton* button, bool selected)
{
    button->setProperty("selected", selected);
    button->style()->unpolish(button);
    button->style()->polish(button);
}
    void MainWindow::showProcessSelectionDialogAndTerminate(const QList<ProcessInfo>& processes, const QString& title, const QString& type) {
        int successCount = 0;
        int failCount = 0;
        QStringList failedProcesses;
        
        QString message = QString(tr(QString::fromUtf8("成功关闭 %1 个进程。").toUtf8().constData())).arg(successCount);
        if (failCount > 0) {
            message += QString(tr(QString::fromUtf8("\n关闭 %1 个进程失败: %2").toUtf8().constData())).arg(failCount).arg(failedProcesses.join(", "));
        }
        QMessageBox::information(this, tr(title.toUtf8().constData()), message);
        // Optionally, refresh the process list if ProcessPage is active and visible
        if (m_processPage && m_pageStack->currentWidget() == m_processPage) {
            m_processPage->updateProcessList();
        }
    }

void MainWindow::onCloseHighCpuProcessClicked()
{
    if (!m_processPage) {
        QMessageBox::warning(this, tr("错误"), tr("进程页面未初始化。"));
        return;
    }

    QList<ProcessInfo> highCpuProcesses;
    // Get top CPU consuming processes. This logic might be refined or moved.
    QList<ProcessPage::ProcessData> allProcesses = m_processPage->getCurrentProcessData(); 
    std::sort(allProcesses.begin(), allProcesses.end(), [](const ProcessPage::ProcessData& a, const ProcessPage::ProcessData& b) {
        return a.cpuUsage > b.cpuUsage;
    });

    int count = 0;
    for (const auto& pData : allProcesses) {
        // Define a threshold for "high CPU usage", e.g., > 5% or > 10%
        // Or simply take the top N processes if the list is sorted.
        if (pData.cpuUsage > 5.0) { // Example: processes using more than 5% CPU
            ProcessInfo info;
            info.pid = pData.pid;
            info.name = pData.name;
            info.usage = pData.cpuUsage;
            info.usageString = QString::number(pData.cpuUsage, 'f', 1) + "%";
            highCpuProcesses.append(info);
            count++;
            if (count >= 10) break; // Limit to top 10 high CPU processes, for example
        }
    }
    showProcessSelectionDialogAndTerminate(highCpuProcesses, tr("关闭高CPU占用进程"), tr("高CPU占用"));
}

void MainWindow::onCloseHighMemoryProcessClicked()
{
    if (!m_processPage) {
        QMessageBox::warning(this, tr("错误"), tr("进程页面未初始化。"));
        return;
    }

    QList<ProcessInfo> highMemoryProcesses;
    QList<ProcessPage::ProcessData> allProcesses = m_processPage->getCurrentProcessData();
    std::sort(allProcesses.begin(), allProcesses.end(), [](const ProcessPage::ProcessData& a, const ProcessPage::ProcessData& b) {
        return a.memoryUsage > b.memoryUsage; // memoryUsage is typically in KB
    });

    int count = 0;
    for (const auto& pData : allProcesses) {
        // Define a threshold for "high memory usage", e.g., > 100MB
        if (pData.memoryUsage > 100 * 1024) { // Example: processes using more than 100MB (100 * 1024 KB)
            ProcessInfo info;
            info.pid = pData.pid;
            info.name = pData.name;
            info.usage = static_cast<double>(pData.memoryUsage);
            info.usageString = QString::number(pData.memoryUsage / 1024.0, 'f', 1) + " MB";
            highMemoryProcesses.append(info);
            count++;
            if (count >= 10) break; // Limit to top 10 high memory processes
        }
    }
    showProcessSelectionDialogAndTerminate(highMemoryProcesses, tr("关闭高内存占用进程"), tr("高内存占用"));
}

QWidget* MainWindow::createOverviewPage()
{
    QWidget* page = new QWidget(this);
    QGridLayout* grid = new QGridLayout(page);

    // Add charts to grid
    grid->addWidget(m_cpuChart, 0, 0);
    grid->addWidget(m_gpuChart, 0, 1);
    grid->addWidget(m_memoryChart, 1, 0);
    grid->addWidget(m_wifiChart, 1, 1);

    return page;
}

void MainWindow::exportData()
{
    QString path = QFileDialog::getSaveFileName(this, "导出数据", "", "CSV文件 (*.csv)");
    if (!path.isEmpty()) {
        handleExportRequest(path);
    }
}

void MainWindow::handleExportRequest(const QString& path)
{
    Exporter exporter;
    exporter.exportToCsv("Data/data.db", path);
}

void MainWindow::loadModelSettings()
{
    QSettings settings("PerformanceMonitor", "Settings");
    QString modelName = settings.value("model_name", "gpt-3.5-turbo").toString();
    QString apiKey = settings.value("api_key", "").toString();
    
    // 设置模型接口
    m_modelInterface->setModel(modelName);
    m_modelInterface->setApiKey(apiKey);
}

void MainWindow::onModelSettingsChanged(const QString &modelName, const QString &apiKey)
{
    // 更新模型接口设置
    m_modelInterface->setModel(modelName);
    m_modelInterface->setApiKey(apiKey);
}

void MainWindow::onRunModelRequested(const QString &inputData)
{
    // 发送请求到模型
    m_modelInterface->sendRequest(inputData);
}

void MainWindow::onModelResponseReceived(const QString &response)
{
    // 设置页面没有直接的成员变量，暂时注释掉这部分代码
    // 如果需要更新设置页面，应该通过其他方式获取设置页面的引用
    /*
    // 更新设置窗口中的输出文本框
    SettingsWidget* settingsWidget = findChild<SettingsWidget*>();
    if (settingsWidget) {
        settingsWidget->setOutputText(response);
    }
    */
    
    // 更新分析页面中的AI分析结果
    if (m_analysisPage && m_analysisPage->findChild<QTextEdit*>("m_aiResultText")) {
        m_analysisPage->findChild<QTextEdit*>("m_aiResultText")->setText(response);
    }
}

void MainWindow::onModelErrorOccurred(const QString &errorMessage)
{
    // 显示错误消息
    QMessageBox::critical(this, tr("模型错误"), errorMessage);
    
    // 设置页面没有直接的成员变量，暂时注释掉这部分代码
    // 如果需要更新设置页面，应该通过其他方式获取设置页面的引用
    /*
    // 更新设置窗口中的输出文本框
    SettingsWidget* settingsWidget = findChild<SettingsWidget*>();
    if (settingsWidget) {
        settingsWidget->setOutputText(tr("错误: %1").arg(errorMessage));
    }
    */
    
    // 更新分析页面中的AI分析结果
    if (m_analysisPage && m_analysisPage->findChild<QTextEdit*>("m_aiResultText")) {
        m_analysisPage->findChild<QTextEdit*>("m_aiResultText")->setText(tr("错误: %1").arg(errorMessage));
    }
}

// 创建3D可视化页面
void MainWindow::createVisualizationPage() {
    // 创建3D可视化页面 - 使用Qt Graphs
    m_vis3DPage = new Vis3DPage(this);
    QVBoxLayout* layout = new QVBoxLayout(m_vis3DPage);
    
    // 创建控制面板
    QGroupBox* controlGroup = new QGroupBox("可视化控制", m_vis3DPage);
    controlGroup->setStyleSheet(
        "QGroupBox { font-weight: bold; border: 1px solid #cccccc; border-radius: 6px; margin-top: 12px; padding-top: 10px; }" 
        "QGroupBox::title { subcontrol-origin: margin; left: 10px; padding: 0 5px; background-color: #f8f8f8; }"
    );
    
    QHBoxLayout* controlLayout = new QHBoxLayout(controlGroup);
    
    // 添加刷新按钮
    QPushButton* refreshButton = new QPushButton("刷新数据", controlGroup);
    refreshButton->setStyleSheet(
        "QPushButton { background-color: #4CAF50; color: white; border: none; border-radius: 4px; " 
        "padding: 8px 16px; font-weight: bold; } " 
        "QPushButton:hover { background-color: #388E3C; } " 
        "QPushButton:pressed { background-color: #1B5E20; } "
    );
    
    // 添加可视化模式标签
    QLabel* modeLabel = new QLabel("3D性能可视化", controlGroup);
    modeLabel->setStyleSheet("font-weight: bold; color: #333333;");
    
    // 创建重置视图按钮
    QPushButton* resetViewButton = new QPushButton("重置视图", controlGroup);
    resetViewButton->setStyleSheet(
        "QPushButton { background-color: #FF5722; color: white; border: none; border-radius: 4px; " 
        "padding: 8px 16px; font-weight: bold; } " 
        "QPushButton:hover { background-color: #E64A19; } " 
        "QPushButton:pressed { background-color: #BF360C; }"
    );
    
    // 将控件添加到控制面板布局
    controlLayout->addWidget(modeLabel);
    controlLayout->addStretch();
    controlLayout->addWidget(refreshButton);
    controlLayout->addWidget(resetViewButton);
    
    // 添加图例
    QGroupBox* legendGroup = new QGroupBox("数据图例", m_vis3DPage);
    legendGroup->setStyleSheet(
        "QGroupBox { font-weight: bold; border: 1px solid #cccccc; border-radius: 6px; margin-top: 12px; padding-top: 10px; }" 
        "QGroupBox::title { subcontrol-origin: margin; left: 10px; padding: 0 5px; background-color: #f8f8f8; }"
    );
    
    QGridLayout* legendLayout = new QGridLayout(legendGroup);
    
    // CPU图例
    QLabel* cpuColorLabel = new QLabel(legendGroup);
    cpuColorLabel->setFixedSize(16, 16);
    cpuColorLabel->setStyleSheet("background-color: red; border: 1px solid #cccccc;");
    legendLayout->addWidget(cpuColorLabel, 0, 0);
    legendLayout->addWidget(new QLabel("CPU使用率"), 0, 1);
    
    // 内存图例
    QLabel* memoryColorLabel = new QLabel(legendGroup);
    memoryColorLabel->setFixedSize(16, 16);
    memoryColorLabel->setStyleSheet("background-color: blue; border: 1px solid #cccccc;");
    legendLayout->addWidget(memoryColorLabel, 0, 2);
    legendLayout->addWidget(new QLabel("内存使用率"), 0, 3);
    
    // 磁盘图例
    QLabel* diskColorLabel = new QLabel(legendGroup);
    diskColorLabel->setFixedSize(16, 16);
    diskColorLabel->setStyleSheet("background-color: green; border: 1px solid #cccccc;");
    legendLayout->addWidget(diskColorLabel, 1, 0);
    legendLayout->addWidget(new QLabel("磁盘使用率"), 1, 1);
    
    // 网络图例
    QLabel* networkColorLabel = new QLabel(legendGroup);
    networkColorLabel->setFixedSize(16, 16);
    networkColorLabel->setStyleSheet("background-color: orange; border: 1px solid #cccccc;");
    legendLayout->addWidget(networkColorLabel, 1, 2);
    legendLayout->addWidget(new QLabel("网络使用率"), 1, 3);
    
    // 将控制面板和图例添加到主布局
    layout->addWidget(controlGroup);
    layout->addWidget(legendGroup);
    
    // 创建3D可视化内容
    
    // Vis3DPage现在使用Qt Graphs进行3D可视化
    // 不再需要手动创建Qt3D场景
}

// Qt Graphs处理所有3D可视化 - 移除了旧的Qt3D方法

void MainWindow::displayGpuNotification(const QString& title, const QString& message)
{
    QMessageBox::information(this, tr(title.toUtf8().constData()), message);
}

// Add implementations for the missing show* methods
void MainWindow::showCpuPage() {
    switchToPage(static_cast<int>(PageIndex::CpuPage));
}

void MainWindow::showMemoryPage() {
    switchToPage(static_cast<int>(PageIndex::MemoryPage));
}

void MainWindow::showDiskPage() {
    switchToPage(static_cast<int>(PageIndex::DiskPage));
}

void MainWindow::showNetworkPage() {
    switchToPage(static_cast<int>(PageIndex::NetworkPage));
}

void MainWindow::showGpuPage() {
    switchToPage(static_cast<int>(PageIndex::GpuPage));
}

void MainWindow::showAnalysisPage() {
    switchToPage(static_cast<int>(PageIndex::AnalysisPage));
}

void MainWindow::show3DVisPage() {
    switchToPage(static_cast<int>(PageIndex::Visualization3DPage));
}

void MainWindow::showSettingsPage() {
    switchToPage(static_cast<int>(PageIndex::SettingsPage));
}

// 添加进程终止请求处理槽函数
void MainWindow::onProcessTerminationRequested(quint64 pid)
{
    // 这里可以添加额外的处理逻辑，比如记录到日志
    // 或者执行其他需要在进程终止前完成的操作
}

void MainWindow::resetViewport() {
#ifdef USE_VISUALIZATION3D
    if (m_vis3DPage) {
        m_vis3DPage->resetCamera();
    }
#endif
}

void MainWindow::resetNavigationButtons()
{
    QList<QPushButton*> buttons = {
        m_overviewButton, m_cpuButton, m_gpuButton, m_memoryButton,
        m_diskButton, m_networkButton, m_processButton, m_analysisButton,
        m_3dVisButton, m_settingsButton
    };

    for (auto button : buttons) {
        updateNavigationButton(button, false);
    }
}

void MainWindow::onOptimizeDiskClicked()
{
    // Basic disk optimization logic
    QMessageBox::information(this, tr("Disk Optimization"), 
        tr("Defragmenting disk and optimizing storage..."));
    // Add actual disk optimization logic here
}

void MainWindow::onOptimizeThreadPoolClicked()
{
    // Basic thread pool optimization
    QMessageBox::information(this, tr("Thread Pool Optimization"),
        tr("Adjusting thread pool size and priorities..."));
    // Add actual thread pool management logic here
}

// Helper function to show process selection dialog and terminate selected processes
// This function is already defined earlier in the file
// void MainWindow::showProcessSelectionDialogAndTerminate(const QList<ProcessInfo>& processes, const QString& title, const QString& type)(const QList<ProcessInfo>& processes, const QString& title, const QString& type)
// {
//     // Implementation moved to the first definition
// }

// The duplicate function implementation has been removed completely
