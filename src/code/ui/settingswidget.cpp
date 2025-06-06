// settingswidget.cpp
#include "src/include/ui/settingswidget.h"
#include <QSettings>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QGroupBox>
#include <QFileDialog>
#include <QStyle>
#include <QIcon>
#include <QMessageBox>
#include <QTimer>
#include <QApplication>

SettingsWidget::SettingsWidget(QWidget *parent)
    : QWidget(parent)
{
    setupUI();
    
    // 加载上次保存的设置
    QSettings settings("PerformanceMonitor", "Settings");
    
    // 加载采样率设置
    int savedRate = settings.value("sampling_rate", DEFAULT_INTERVAL).toInt();
    m_intervalSlider->setValue(savedRate);
    m_intervalSpinBox->setValue(savedRate);
    m_intervalLabel->setText(formatInterval(savedRate));
    
    // 加载模型设置
    QString savedModel = settings.value("model_name", "gpt-3.5-turbo").toString();
    QString savedApiKey = settings.value("api_key", "sk-ea24db1ec4a04d4a9b8ce06b51a1c68c").toString(); // Set default API key
    
    // 设置模型选择器的当前值
    int modelIndex = m_modelSelector->findText(savedModel);
    if (modelIndex >= 0) {
        m_modelSelector->setCurrentIndex(modelIndex);
    }
    
    // 设置API密钥
    m_apiKeyEdit->setText(savedApiKey);

    // 加载历史数据保留天数设置
    int savedHistoryDays = settings.value("history_retention_days", DEFAULT_HISTORY_DAYS).toInt();
    m_historyDaysSpinBox->setValue(savedHistoryDays);
}

SettingsWidget::~SettingsWidget()
{
}

void SettingsWidget::setupUI()
{
    QVBoxLayout *mainLayout = new QVBoxLayout(this);
    mainLayout->setSpacing(20);
    mainLayout->setContentsMargins(20, 20, 20, 20);

    // 创建选项卡控件
    m_tabWidget = new QTabWidget(this);
    m_tabWidget->setStyleSheet(
        "QTabWidget::pane {"
        "    border: 1px solid #cccccc;"
        "    background-color: #f9f9f9;"
        "    border-radius: 4px;"
        "}"
        "QTabBar::tab {"
        "    background-color: #e6e6e6;"
        "    border: 1px solid #cccccc;"
        "    border-bottom-color: #cccccc;"
        "    border-top-left-radius: 4px;"
        "    border-top-right-radius: 4px;"
        "    min-width: 120px;"
        "    padding: 8px;"
        "    font-weight: bold;"
        "}"
        "QTabBar::tab:selected {"
        "    background-color: #f9f9f9;"
        "    border-bottom-color: #f9f9f9;"
        "}"
        "QTabBar::tab:hover {"
        "    background-color: #f0f0f0;"
        "}"
    );
    
    // 创建采样设置选项卡
    m_samplingTab = new QWidget();
    setupSamplingTab();
    
    // 创建模型设置选项卡
    m_modelTab = new QWidget();
    setupModelTab();
    
    // 创建分析设置选项卡
    m_analysisSettingsTab = new QWidget();
    setupAnalysisSettingsTab();

    // 创建数据设置选项卡
    m_dataSettingsTab = new QWidget();
    setupDataSettingsTab();
    
    // 创建关闭行为设置选项卡
    m_closeTab = new QWidget();
    setupCloseTab();

    // 添加选项卡（修复重复添加的问题）
    m_tabWidget->addTab(m_samplingTab, tr("Sampling Settings"));
    m_tabWidget->addTab(m_modelTab, tr("Model Settings"));
    m_tabWidget->addTab(m_analysisSettingsTab, tr("Analysis Settings"));
    m_tabWidget->addTab(m_dataSettingsTab, tr("Data Settings"));
    m_tabWidget->addTab(m_closeTab, tr("Close Behavior"));
    
    // 添加到主布局
    mainLayout->addWidget(m_tabWidget);
}

void SettingsWidget::onSliderValueChanged(int value)
{
    m_intervalSpinBox->setValue(value);
    m_intervalLabel->setText(formatInterval(value));
}

void SettingsWidget::onSpinBoxValueChanged(int value)
{
    m_intervalSlider->setValue(value);
    m_intervalLabel->setText(formatInterval(value));
}

void SettingsWidget::onApplyClicked()
{
    int rate = m_intervalSpinBox->value();
    // 保存采样率设置
    QSettings settings("PerformanceMonitor", "Settings");
    settings.setValue("sampling_rate", rate);
    emit samplingIntervalChanged(rate);
    QMessageBox::information(this, tr("Settings Applied"), tr("Sampling interval updated to %1 ms.").arg(rate));
}

void SettingsWidget::setupAnalysisSettingsTab()
{
    QVBoxLayout *analysisLayout = new QVBoxLayout(m_analysisSettingsTab);
    analysisLayout->setSpacing(15);

    // Title
    m_analysisTitleLabel = new QLabel(tr("Analysis Settings"), this);
    m_analysisTitleLabel->setStyleSheet("QLabel { font-size: 14px; font-weight: bold; }");

    // Anomaly Threshold
    QGroupBox *anomalyGroup = new QGroupBox(tr("Anomaly Detection"), this);
    QFormLayout *anomalyFormLayout = new QFormLayout(anomalyGroup);

    m_anomalyThresholdLabel = new QLabel(tr("Detection Threshold (σ):"), this);
    m_anomalyThresholdSpin = new QSpinBox(this);
    m_anomalyThresholdSpin->setRange(1, 10);
    m_anomalyThresholdSpin->setValue(2); // Default value from AnalysisPage
    m_anomalyThresholdSpin->setSingleStep(1);
    m_anomalyThresholdSpin->setSuffix(" σ");
    anomalyFormLayout->addRow(m_anomalyThresholdLabel, m_anomalyThresholdSpin);

    // Auto Detect Items
    m_autoDetectGroup = new QGroupBox(tr("Automatic Detection Items"), this);
    QHBoxLayout *autoDetectLayout = new QHBoxLayout(m_autoDetectGroup);
    m_autoCpuCheck = new QCheckBox(tr("CPU"), this);
    m_autoMemoryCheck = new QCheckBox(tr("Memory"), this);
    m_autoDiskCheck = new QCheckBox(tr("Disk"), this);
    m_autoNetworkCheck = new QCheckBox(tr("Network"), this);
    // Set default checked states from AnalysisPage
    m_autoCpuCheck->setChecked(true);
    m_autoMemoryCheck->setChecked(true);
    m_autoDiskCheck->setChecked(true);
    m_autoNetworkCheck->setChecked(true);
    autoDetectLayout->addWidget(m_autoCpuCheck);
    autoDetectLayout->addWidget(m_autoMemoryCheck);
    autoDetectLayout->addWidget(m_autoDiskCheck);
    autoDetectLayout->addWidget(m_autoNetworkCheck);
    autoDetectLayout->addStretch();

    // Apply Button
    m_analysisApplyButton = new QPushButton(tr("Apply"), this);
    m_analysisApplyButton->setIcon(QIcon(":/icons/icons/settings.png"));
    m_analysisApplyButton->setCursor(Qt::PointingHandCursor);
    m_analysisApplyButton->setStyleSheet(
        "QPushButton {"
        "    background-color: #2196F3;"
        "    color: white;"
        "    border: none;"
        "    padding: 8px 16px;"
        "    border-radius: 4px;"
        "    font-weight: bold;"
        "}"
        "QPushButton:hover {"
        "    background-color: #1976D2;"
        "}"
        "QPushButton:pressed {"
        "    background-color: #1565C0;"
        "}"
    );

    analysisLayout->addWidget(m_analysisTitleLabel);
    analysisLayout->addWidget(anomalyGroup);
    analysisLayout->addWidget(m_autoDetectGroup);
    analysisLayout->addStretch();
    analysisLayout->addWidget(m_analysisApplyButton, 0, Qt::AlignRight);

    // Connect signals
    connect(m_analysisApplyButton, &QPushButton::clicked, this, &SettingsWidget::onAnalysisSettingsApplyClicked);
}

void SettingsWidget::setupDataSettingsTab()
{

    QVBoxLayout *dataLayout = new QVBoxLayout(m_dataSettingsTab);
    dataLayout->setSpacing(15);

    // Title
    m_dataSettingsTitleLabel = new QLabel(tr("Data Settings"), this);
    m_dataSettingsTitleLabel->setStyleSheet("QLabel { font-size: 14px; font-weight: bold; }");

    // History Retention Days
    QGroupBox *historyGroup = new QGroupBox(tr("History Data Retention"), this);
    QFormLayout *historyFormLayout = new QFormLayout(historyGroup);

    m_historyDaysLabel = new QLabel(tr("Retention Period (days):"), this);
    m_historyDaysSpinBox = new QSpinBox(this);
    m_historyDaysSpinBox->setRange(1, 365); // Example range, adjust as needed
    m_historyDaysSpinBox->setValue(DEFAULT_HISTORY_DAYS); 
    m_historyDaysSpinBox->setSingleStep(1);
    m_historyDaysSpinBox->setSuffix(tr(" days"));
    historyFormLayout->addRow(m_historyDaysLabel, m_historyDaysSpinBox);

    // Apply Button
    m_dataSettingsApplyButton = new QPushButton(tr("Apply"), this);
    m_dataSettingsApplyButton->setIcon(QIcon(":/icons/icons/save.png")); // Assuming you have a save icon
    m_dataSettingsApplyButton->setCursor(Qt::PointingHandCursor);
    m_dataSettingsApplyButton->setStyleSheet(
        "QPushButton {"
        "    background-color: #2196F3;"
        "    color: white;"
        "    border: none;"
        "    padding: 8px 16px;"
        "    border-radius: 4px;"
        "    font-weight: bold;"
        "}"
        "QPushButton:hover {"
        "    background-color: #1976D2;"
        "}"
        "QPushButton:pressed {"
        "    background-color: #1565C0;"
        "}"
    );

    dataLayout->addWidget(m_dataSettingsTitleLabel);
    dataLayout->addWidget(historyGroup);
    dataLayout->addStretch();
    dataLayout->addWidget(m_dataSettingsApplyButton, 0, Qt::AlignRight);

    connect(m_dataSettingsApplyButton, &QPushButton::clicked, this, &SettingsWidget::onDataSettingsApplyClicked);

    // Load saved data settings - already handled in constructor for m_historyDaysSpinBox
}

void SettingsWidget::onDataSettingsApplyClicked()
{
    int days = m_historyDaysSpinBox->value();
    QSettings settings("PerformanceMonitor", "Settings");
    settings.setValue("history_retention_days", days);
    emit historyRetentionDaysChanged(days);
    QMessageBox::information(this, tr("Settings Applied"), tr("Data history retention updated to %1 days.").arg(days));
}

void SettingsWidget::setupCloseTab()
{
    QVBoxLayout *closeLayout = new QVBoxLayout(m_closeTab);
    closeLayout->setSpacing(15);

    // 标题
    QLabel *closeTitleLabel = new QLabel(tr("Close Behavior Settings"), this);
    closeTitleLabel->setStyleSheet("QLabel { font-size: 14px; font-weight: bold; }");

    // 关闭行为选择
    QGroupBox *closeGroup = new QGroupBox(tr("When clicking the close button"), this);
    QFormLayout *closeFormLayout = new QFormLayout(closeGroup);

    m_closeBehaviorLabel = new QLabel(tr("Action:"), this);
    m_closeBehaviorSelector = new QComboBox(this);
    m_closeBehaviorSelector->addItem(tr("Ask every time"), 0);
    m_closeBehaviorSelector->addItem(tr("Minimize to system tray"), 1);
    m_closeBehaviorSelector->addItem(tr("Exit application"), 2);

    // 加载保存的设置
    QSettings settings("PerformanceMonitor", "Settings");
    int savedBehavior = settings.value("close_behavior", 0).toInt();
    m_closeBehaviorSelector->setCurrentIndex(savedBehavior);

    closeFormLayout->addRow(m_closeBehaviorLabel, m_closeBehaviorSelector);

    // 应用按钮
    m_closeBehaviorApplyButton = new QPushButton(tr("Apply"), this);
    m_closeBehaviorApplyButton->setIcon(QIcon(":/icons/icons/settings.png"));
    m_closeBehaviorApplyButton->setCursor(Qt::PointingHandCursor);
    m_closeBehaviorApplyButton->setStyleSheet(
        "QPushButton {"
        "    background-color: #2196F3;"
        "    color: white;"
        "    border: none;"
        "    padding: 8px 16px;"
        "    border-radius: 4px;"
        "    font-weight: bold;"
        "}"
        "QPushButton:hover {"
        "    background-color: #1976D2;"
        "}"
        "QPushButton:pressed {"
        "    background-color: #1565C0;"
        "}"
    );

    closeLayout->addWidget(closeTitleLabel);
    closeLayout->addWidget(closeGroup);
    closeLayout->addStretch();
    closeLayout->addWidget(m_closeBehaviorApplyButton, 0, Qt::AlignRight);

    // 连接信号
    connect(m_closeBehaviorApplyButton, &QPushButton::clicked, this, &SettingsWidget::onCloseBehaviorApplyClicked);
}

void SettingsWidget::onCloseBehaviorApplyClicked()
{
    int selectedBehavior = m_closeBehaviorSelector->currentData().toInt();
    // 保存关闭行为设置
    QSettings settings("PerformanceMonitor", "Settings");
    settings.setValue("close_behavior", selectedBehavior);
    emit closeBehaviorChanged(selectedBehavior);
    QMessageBox::information(this, tr("Settings Applied"), tr("Close behavior setting updated."));
}

void SettingsWidget::onAnalysisSettingsApplyClicked()
{
    int threshold = m_anomalyThresholdSpin->value();
    bool cpuAuto = m_autoCpuCheck->isChecked();
    bool memAuto = m_autoMemoryCheck->isChecked();
    bool diskAuto = m_autoDiskCheck->isChecked();
    bool netAuto = m_autoNetworkCheck->isChecked();

    QSettings settings("PerformanceMonitor", "Settings");
    settings.setValue("analysis_anomalyThreshold", threshold);
    settings.setValue("analysis_autoCpuCheck", cpuAuto);
    settings.setValue("analysis_autoMemoryCheck", memAuto);
    settings.setValue("analysis_autoDiskCheck", diskAuto);
    settings.setValue("analysis_autoNetworkCheck", netAuto);

    emit analysisSettingsChanged(threshold, cpuAuto, memAuto, diskAuto, netAuto);
    QMessageBox::information(this, tr("Settings Applied"), tr("Analysis settings have been updated."));
}

void SettingsWidget::setupSamplingTab()
{
    QVBoxLayout *samplingLayout = new QVBoxLayout(m_samplingTab);
    samplingLayout->setSpacing(15);
    
    // 标题和说明
    m_titleLabel = new QLabel(tr("Sampling Interval"), this);
    m_titleLabel->setStyleSheet("QLabel { font-size: 14px; font-weight: bold; }");
    
    m_rangeLabel = new QLabel(tr("Range: %1 - %2 ms")
        .arg(MIN_INTERVAL)
        .arg(MAX_INTERVAL), this);
    m_rangeLabel->setStyleSheet("QLabel { color: #666666; }");

    // 滑块和数值显示布局
    QHBoxLayout *sliderLayout = new QHBoxLayout;
    
    m_intervalSlider = new QSlider(Qt::Horizontal, this);
    m_intervalSlider->setRange(MIN_INTERVAL, MAX_INTERVAL);
    m_intervalSlider->setValue(DEFAULT_INTERVAL);
    m_intervalSlider->setStyleSheet(
        "QSlider::groove:horizontal {"
        "    border: 1px solid #999999;"
        "    height: 8px;"
        "    background: qlineargradient(x1:0, y1:0, x2:1, y2:0, "
        "        stop:0 #B1B1B1, stop:1 #c4c4c4);"
        "    margin: 2px 0;"
        "}"
        "QSlider::handle:horizontal {"
        "    background: qlineargradient(x1:0, y1:0, x2:1, y2:1, "
        "        stop:0 #2196F3, stop:1 #1976D2);"
        "    border: 1px solid #5c5c5c;"
        "    width: 18px;"
        "    margin: -2px 0;"
        "    border-radius: 9px;"
        "}"
    );
    
    m_intervalSpinBox = new QSpinBox(this);
    m_intervalSpinBox->setRange(MIN_INTERVAL, MAX_INTERVAL);
    m_intervalSpinBox->setValue(DEFAULT_INTERVAL);
    m_intervalSpinBox->setSuffix(" ms");
    m_intervalSpinBox->setStyleSheet(
        "QSpinBox {"
        "    padding: 5px;"
        "    border: 1px solid #999999;"
        "    border-radius: 4px;"
        "    min-width: 80px;"
        "}"
    );
    
    sliderLayout->addWidget(m_intervalSlider);
    sliderLayout->addWidget(m_intervalSpinBox);
    
    // 当前值显示
    m_intervalLabel = new QLabel(formatInterval(DEFAULT_INTERVAL), this);
    m_intervalLabel->setStyleSheet(
        "QLabel {"
        "    font-size: 13px;"
        "    color: #2196F3;"
        "    padding: 5px;"
        "}"
    );
    
    // 应用按钮
    m_applyButton = new QPushButton(tr("Apply"), this);
    m_applyButton->setIcon(QIcon(":/icons/icons/settings.png"));
    m_applyButton->setCursor(Qt::PointingHandCursor);
    m_applyButton->setStyleSheet(
        "QPushButton {"
        "    background-color: #2196F3;"
        "    color: white;"
        "    border: none;"
        "    padding: 8px 16px;"
        "    border-radius: 4px;"
        "    font-weight: bold;"
        "}"
        "QPushButton:hover {"
        "    background-color: #1976D2;"
        "}"
        "QPushButton:pressed {"
        "    background-color: #1565C0;"
        "}"
    );

    // 添加所有组件到采样设置布局
    samplingLayout->addWidget(m_titleLabel);
    samplingLayout->addWidget(m_rangeLabel);
    samplingLayout->addLayout(sliderLayout);
    samplingLayout->addWidget(m_intervalLabel);
    samplingLayout->addWidget(m_applyButton);
    samplingLayout->addStretch();
    
    // 连接信号和槽
    connect(m_intervalSlider, &QSlider::valueChanged, this, &SettingsWidget::onSliderValueChanged);
    connect(m_intervalSpinBox, QOverload<int>::of(&QSpinBox::valueChanged), 
            this, &SettingsWidget::onSpinBoxValueChanged);
    connect(m_applyButton, &QPushButton::clicked, this, &SettingsWidget::onApplyClicked);
}

void SettingsWidget::setupModelTab()
{
    QVBoxLayout *modelLayout = new QVBoxLayout(m_modelTab);
    modelLayout->setSpacing(15);
    
    // 模型设置部分
    QGroupBox *modelSettingsGroup = new QGroupBox(tr("Model Configuration"), this);
    QVBoxLayout *modelSettingsLayout = new QVBoxLayout(modelSettingsGroup);
    
    // 模型标题
    m_modelTitleLabel = new QLabel(tr("Select AI Model"), this);
    m_modelTitleLabel->setStyleSheet("QLabel { font-size: 14px; font-weight: bold; }");
    
    // 模型选择器
    m_modelSelector = new QComboBox(this);
    m_modelSelector->addItem("gpt-3.5-turbo");
    m_modelSelector->addItem("gpt-4");
    m_modelSelector->addItem("gpt-4-turbo");
    m_modelSelector->addItem("deepseek-coder");
    m_modelSelector->addItem("deepseek-chat");
    m_modelSelector->addItem("llama3");
    m_modelSelector->addItem("qwen");
    m_modelSelector->setStyleSheet(
        "QComboBox {"
        "    padding: 5px;"
        "    border: 1px solid #999999;"
        "    border-radius: 4px;"
        "    min-width: 200px;"
        "}"
        "QComboBox::drop-down {"
        "    subcontrol-origin: padding;"
        "    subcontrol-position: top right;"
        "    width: 20px;"
        "    border-left: 1px solid #999999;"
        "}"
    );
    
    // API密钥输入
    m_apiKeyLabel = new QLabel(tr("API Key"), this);
    m_apiKeyLabel->setStyleSheet("QLabel { font-size: 14px; font-weight: bold; }");
    
    m_apiKeyEdit = new QLineEdit(this);
    m_apiKeyEdit->setPlaceholderText(tr("Enter your API key here"));
    m_apiKeyEdit->setEchoMode(QLineEdit::Password);
    m_apiKeyEdit->setStyleSheet(
        "QLineEdit {"
        "    padding: 8px;"
        "    border: 1px solid #999999;"
        "    border-radius: 4px;"
        "}"
    );
    
    // 应用按钮
    m_modelApplyButton = new QPushButton(tr("Save Settings"), this);
    m_modelApplyButton->setIcon(QIcon(":/icons/icons/save.png"));
    m_modelApplyButton->setCursor(Qt::PointingHandCursor);
    m_modelApplyButton->setStyleSheet(
        "QPushButton {"
        "    background-color: #2196F3;"
        "    color: white;"
        "    border: none;"
        "    padding: 8px 16px;"
        "    border-radius: 4px;"
        "    font-weight: bold;"
        "}"
        "QPushButton:hover {"
        "    background-color: #1976D2;"
        "}"
        "QPushButton:pressed {"
        "    background-color: #1565C0;"
        "}"
    );
    
    // 状态标签
    m_modelStatusLabel = new QLabel("", this);
    m_modelStatusLabel->setStyleSheet(
        "QLabel {"
        "    font-size: 13px;"
        "    color: #4CAF50;"
        "    padding: 5px;"
        "}"
    );
    
    // 添加组件到模型设置布局
    modelSettingsLayout->addWidget(m_modelTitleLabel);
    modelSettingsLayout->addWidget(m_modelSelector);
    modelSettingsLayout->addWidget(m_apiKeyLabel);
    modelSettingsLayout->addWidget(m_apiKeyEdit);
    modelSettingsLayout->addWidget(m_modelApplyButton);
    modelSettingsLayout->addWidget(m_modelStatusLabel);
    
    // 模型运行部分
    QGroupBox *modelRunGroup = new QGroupBox(tr("Run Model"), this);
    QVBoxLayout *modelRunLayout = new QVBoxLayout(modelRunGroup);
    
    // 输入标签和文本框
    m_inputLabel = new QLabel(tr("Input Data"), this);
    m_inputLabel->setStyleSheet("QLabel { font-size: 14px; font-weight: bold; }");
    
    m_inputTextEdit = new QTextEdit(this);
    m_inputTextEdit->setPlaceholderText(tr("Enter performance data or query here..."));
    m_inputTextEdit->setStyleSheet(
        "QTextEdit {"
        "    padding: 8px;"
        "    border: 1px solid #999999;"
        "    border-radius: 4px;"
        "    min-height: 100px;"
        "}"
    );
    
    // 保存结果复选框
    m_saveResultCheckBox = new QCheckBox(tr("Save result to analysis history"), this);
    m_saveResultCheckBox->setStyleSheet(
        "QCheckBox {"
        "    font-size: 13px;"
        "}"
        "QCheckBox::indicator {"
        "    width: 16px;"
        "    height: 16px;"
        "}"
    );
    
    // 运行按钮
    m_runModelButton = new QPushButton(tr("Run Analysis"), this);
    m_runModelButton->setIcon(QIcon(":/icons/icons/play.png"));
    m_runModelButton->setCursor(Qt::PointingHandCursor);
    m_runModelButton->setStyleSheet(
        "QPushButton {"
        "    background-color: #4CAF50;"
        "    color: white;"
        "    border: none;"
        "    padding: 8px 16px;"
        "    border-radius: 4px;"
        "    font-weight: bold;"
        "}"
        "QPushButton:hover {"
        "    background-color: #388E3C;"
        "}"
        "QPushButton:pressed {"
        "    background-color: #2E7D32;"
        "}"
    );
    
    // 输出标签和文本框
    m_outputLabel = new QLabel(tr("Model Output"), this);
    m_outputLabel->setStyleSheet("QLabel { font-size: 14px; font-weight: bold; }");
    
    m_outputTextEdit = new QTextEdit(this);
    m_outputTextEdit->setReadOnly(true);
    m_outputTextEdit->setStyleSheet(
        "QTextEdit {"
        "    padding: 8px;"
        "    border: 1px solid #999999;"
        "    border-radius: 4px;"
        "    background-color: #f5f5f5;"
        "    min-height: 150px;"
        "}"
    );
    
    // 添加组件到模型运行布局
    modelRunLayout->addWidget(m_inputLabel);
    modelRunLayout->addWidget(m_inputTextEdit);
    modelRunLayout->addWidget(m_saveResultCheckBox);
    modelRunLayout->addWidget(m_runModelButton);
    modelRunLayout->addWidget(m_outputLabel);
    modelRunLayout->addWidget(m_outputTextEdit);
    
    // 添加组到主布局
    modelLayout->addWidget(modelSettingsGroup);
    modelLayout->addWidget(modelRunGroup);
    
    // 连接信号和槽
    connect(m_modelApplyButton, &QPushButton::clicked, this, &SettingsWidget::onModelApplyClicked);
    connect(m_runModelButton, &QPushButton::clicked, this, &SettingsWidget::onRunModelClicked);
    connect(m_modelSelector, QOverload<int>::of(&QComboBox::currentIndexChanged),
            this, &SettingsWidget::onModelSelectionChanged);
}

void SettingsWidget::onExportClicked()
{
    QString path = QFileDialog::getSaveFileName(this, tr("Export Data"), "", tr("CSV Files (*.csv)"));
    if (!path.isEmpty()) {
        emit requestExport(path);
    }
}

QString SettingsWidget::formatInterval(int ms)
{
    if (ms >= 1000) {
        double seconds = ms / 1000.0;
        return tr("Current Interval: %1 seconds").arg(QString::number(seconds, 'f', 1));
    }
    return tr("Current Interval: %1 ms").arg(ms);
}

void SettingsWidget::onModelApplyClicked()
{
    QString modelName = m_modelSelector->currentText();
    QString apiKey = m_apiKeyEdit->text();
    
    // 保存模型设置
    QSettings settings("PerformanceMonitor", "Settings");
    settings.setValue("model_name", modelName);
    settings.setValue("api_key", apiKey);
    
    // 更新状态标签
    m_modelStatusLabel->setText(tr("Settings saved successfully!"));
    
    // 发送信号通知模型设置已更改
    emit modelSettingsChanged(modelName, apiKey);
    
    // 3秒后清除状态消息
    QTimer::singleShot(3000, [this]() {
        m_modelStatusLabel->clear();
    });
}

void SettingsWidget::onRunModelClicked()
{
    // 检查是否设置了API密钥
    if (m_apiKeyEdit->text().isEmpty()) {
        QMessageBox::warning(this, tr("Missing API Key"), 
                           tr("Please enter your API key in the settings before running the model."));
        return;
    }
    
    // 获取输入数据
    QString inputData = m_inputTextEdit->toPlainText();
    if (inputData.isEmpty()) {
        QMessageBox::warning(this, tr("Empty Input"), 
                           tr("Please enter some data for analysis."));
        return;
    }
    
    // 清空输出区域并显示加载消息
    m_outputTextEdit->setText(tr("Processing request..."));
    
    // 发送信号请求运行模型
    emit runModelRequested(inputData);
}

void SettingsWidget::onModelSelectionChanged(int index)
{
    // 可以在这里添加模型选择变更的处理逻辑
    // 例如更新UI或显示模型特定的选项
}

QString SettingsWidget::getCurrentModelName() const
{
    return m_modelSelector->currentText();
}

QString SettingsWidget::getApiKey() const
{
    return m_apiKeyEdit->text();
}

void SettingsWidget::setOutputText(const QString &text)
{
    // 确保输出文本框存在
    if (m_outputTextEdit) {
        m_outputTextEdit->setText(text);
    }
}
