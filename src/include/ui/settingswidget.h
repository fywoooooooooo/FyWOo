// settingswidget.h - 设置窗口头文件
#pragma once

// 引入必要的Qt控件头文件
#include <QWidget>
#include <QPushButton>
#include <QLineEdit>
#include <QSpinBox>
#include <QComboBox>
#include <QTabWidget>
#include <QTextEdit>
#include <QCheckBox>
#include <QGroupBox>
#include <QFormLayout>
#include "../storage/exporter.h" // 引入导出器头文件
#include <QLabel>
#include <QSlider>

// SettingsWidget 类定义
// 继承自 QWidget，用于创建应用程序的设置界面
class SettingsWidget : public QWidget {
    Q_OBJECT // 启用Qt的元对象系统，支持信号和槽

public:
    // 构造函数
    explicit SettingsWidget(QWidget *parent = nullptr);
    // 析构函数
    ~SettingsWidget();
    
    // 获取当前选定的模型名称
    QString getCurrentModelName() const;
    // 获取当前输入的API密钥
    QString getApiKey() const;
    
    // 设置模型运行结果输出文本框的内容
    void setOutputText(const QString &text);

signals:
    // 请求导出数据的信号，参数为导出路径
    void requestExport(const QString &path);
    // 采样间隔改变时发出的信号，参数为新的间隔（毫秒）
    void samplingIntervalChanged(int interval);
    // 模型设置（名称和API密钥）改变时发出的信号
    void modelSettingsChanged(const QString &modelName, const QString &apiKey);
    // 请求运行模型进行分析的信号，参数为输入数据
    void runModelRequested(const QString &inputData);
    // 分析设置改变时发出的信号，包括异常阈值和自动检测项
    void analysisSettingsChanged(int anomalyThreshold, bool cpuAuto, bool memAuto, bool diskAuto, bool netAuto);
    // 历史数据保留天数改变时发出的信号
    void historyRetentionDaysChanged(int days);
    // 窗口关闭行为改变时发出的信号 (0: 询问, 1: 最小化, 2: 退出)
    void closeBehaviorChanged(int behavior);
    
    // Removed: void languageChanged(const QString &locale);

private slots:
    // 导出按钮点击槽函数
    void onExportClicked();
    // 采样间隔滑块值改变槽函数
    void onSliderValueChanged(int value);
    // 采样间隔微调框值改变槽函数
    void onSpinBoxValueChanged(int value);
    // 采样设置应用按钮点击槽函数
    void onApplyClicked();
    // 模型设置应用按钮点击槽函数
    void onModelApplyClicked();
    // 运行模型按钮点击槽函数
    void onRunModelClicked();
    // 模型选择器选项改变槽函数
    void onModelSelectionChanged(int index);
    // 分析设置应用按钮点击槽函数
    void onAnalysisSettingsApplyClicked();
    // 数据设置应用按钮点击槽函数
    void onDataSettingsApplyClicked();
    // 关闭行为设置应用按钮点击槽函数
    void onCloseBehaviorApplyClicked();

private:
    // 设置UI界面的函数
    void setupUI();
    // 设置采样设置选项卡的函数
    void setupSamplingTab();
    // 设置模型设置选项卡的函数
    void setupModelTab();
    // 设置分析设置选项卡的函数
    void setupAnalysisSettingsTab();
    // 设置数据设置选项卡的函数
    void setupDataSettingsTab();
    // 设置关闭行为选项卡的函数
    void setupCloseTab();
    // 格式化采样间隔显示文本的函数
    QString formatInterval(int ms);

    // 通用UI组件成员变量
    QTabWidget *m_tabWidget; // 主选项卡控件
    QWidget *m_samplingTab; // 采样设置选项卡页面
    QWidget *m_modelTab; // 模型设置选项卡页面
    QWidget *m_analysisSettingsTab; // 分析设置选项卡页面
    QWidget *m_dataSettingsTab; // 数据设置选项卡页面
    QLineEdit *pathEdit; // 导出路径编辑框
    QPushButton *exportButton; // 导出按钮
    QLabel *m_titleLabel; // 采样设置标题标签
    QLabel *m_intervalLabel; // 采样间隔显示标签
    QLabel *m_rangeLabel; // 采样间隔范围标签
    QSlider *m_intervalSlider; // 采样间隔滑块
    QSpinBox *m_intervalSpinBox; // 采样间隔微调框
    QPushButton *m_applyButton; // 采样设置应用按钮
    
    // 模型设置相关组件成员变量
    QLabel *m_modelTitleLabel; // 模型设置标题标签
    QComboBox *m_modelSelector; // 模型选择下拉框
    QLabel *m_apiKeyLabel; // API密钥标签
    QLineEdit *m_apiKeyEdit; // API密钥编辑框
    QPushButton *m_modelApplyButton; // 模型设置应用按钮
    QLabel *m_modelStatusLabel; // 模型状态标签
    
    // 模型运行相关组件成员变量
    QLabel *m_inputLabel; // 输入标签
    QTextEdit *m_inputTextEdit; // 输入文本框
    QLabel *m_outputLabel; // 输出标签
    QTextEdit *m_outputTextEdit; // 输出文本框
    QPushButton *m_runModelButton; // 运行模型按钮
    QCheckBox *m_saveResultCheckBox; // 保存结果复选框

    // 分析设置选项卡UI元素成员变量
    QLabel *m_analysisTitleLabel; // 分析设置标题标签
    QLabel *m_anomalyThresholdLabel; // 异常阈值标签
    QSpinBox *m_anomalyThresholdSpin; // 异常阈值微调框
    QGroupBox *m_autoDetectGroup; // 自动检测分组框
    QCheckBox *m_autoCpuCheck; // 自动检测CPU复选框
    QCheckBox *m_autoMemoryCheck; // 自动检测内存复选框
    QCheckBox *m_autoDiskCheck; // 自动检测磁盘复选框
    QCheckBox *m_autoNetworkCheck; // 自动检测网络复选框
    QPushButton *m_analysisApplyButton; // 分析设置应用按钮

    // 数据设置选项卡UI元素成员变量
    QLabel *m_dataSettingsTitleLabel; // 数据设置标题标签
    QLabel *m_historyDaysLabel; // 历史数据保留天数标签
    QSpinBox *m_historyDaysSpinBox; // 历史数据保留天数微调框
    QPushButton *m_dataSettingsApplyButton; // 数据设置应用按钮

    // 关闭行为设置选项卡UI元素成员变量
    QWidget *m_closeTab; // 关闭行为选项卡页面
    QLabel *m_closeBehaviorLabel; // 关闭行为标签
    QComboBox *m_closeBehaviorSelector; // 关闭行为选择下拉框
    QPushButton *m_closeBehaviorApplyButton; // 关闭行为应用按钮

    // 常量定义
    static const int MIN_INTERVAL = 100;    // 最小采样间隔(ms)
    static const int MAX_INTERVAL = 5000;   // 最大采样间隔(ms)
    static const int DEFAULT_INTERVAL = 1000; // 默认采样间隔(ms)
    static const int DEFAULT_HISTORY_DAYS = 30; // 默认历史数据保留天数
};
