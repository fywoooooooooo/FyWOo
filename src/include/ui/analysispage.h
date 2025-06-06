#pragma once

#include <QWidget>
#include <QVBoxLayout>
#include <QLabel>
#include <QGroupBox>
#include <QPushButton>
#include <QTabWidget>
#include <QTextEdit>
#include <QComboBox>
#include <QSpinBox>
#include <QCheckBox>
#include <QTimer>
#include <QDateTime>
#include <QLineEdit>
#include <QListWidget>
#include <QIcon>
#include "src/include/analysis/anomalydetector.h"
#include "src/include/analysis/performanceanalyzer.h"
#include "src/include/chart/chartwidget.h"

class AnalysisPage : public QWidget
{
    Q_OBJECT

public slots:
    void updateVisualization(const QVector<double>& cpuData, const QVector<double>& memoryData, 
                          const QVector<double>& diskData, const QVector<double>& networkData);

public:
    explicit AnalysisPage(QWidget *parent = nullptr);
    ~AnalysisPage();
    
    // ?????????
    PerformanceAnalyzer* getPerformanceAnalyzer() { return m_performanceAnalyzer; }

signals:
    // ???????????
    void performanceUpdated(const QString &text);
    void anomalyUpdated(const QString &text);
    void requestAiAnalysis(const QString &prompt);
    // ???????????
    void naturalLanguageQueryRequested(const QString &query);

public slots:
    // ????????
    void updateAnomalyData(double cpuUsage, double memoryUsage, double diskIO, double networkUsage);
    
    // ????????
    void updatePerformanceData(double cpuUsage, double memoryUsage, double diskIO, double networkUsage);
    
    // ????????
    void handleAnomalyDetected(const QString& source, double value, double threshold, const QDateTime& timestamp);
    
    // ????????
    void handleBottleneckDetected(PerformanceAnalyzer::BottleneckType type, const QString& details);
    
    // ??????
    void handleTrendChanged(const QString& component, PerformanceAnalyzer::TrendType trend, const QString& details);

    // Slots to receive settings from SettingsWidget
    void onAnalysisSettingsUpdated(int anomalyThreshold, bool cpuAuto, bool memAuto, bool diskAuto, bool netAuto);
    void onUpdateIntervalChanged(int intervalMs);
    
    // ?????????????
    void handleNlpQueryResult(const QString &result);
    
    // ?????????
    void showQueryHistory();

private slots:
    // ??????
    void runAnomalyDetection();
    
    // ??????
    void runPerformanceAnalysis();
    
    // ???????????
    void executeNlpQuery();

private:
    void setupUI();
    void setupConnections();
    void setupNlpQueryTab(); // ?????NLP?????
    
    // ???????????
    QString processNlpQuery(const QString &query);
    
    // ????????????
    void addQueryToHistory(const QString &query, const QString &result);
    
    AnomalyDetector *m_anomalyDetector;
    PerformanceAnalyzer *m_performanceAnalyzer;
    QTimer *m_updateTimer;
    QTabWidget *m_tabWidget;
    QWidget *m_anomalyTab;
    QTextEdit *m_anomalyResultsText;
    QPushButton *m_runAnomalyButton;

private:
    // Store settings received from SettingsWidget
    int m_currentAnomalyThreshold;
    bool m_performCpuCheck;
    bool m_performMemoryCheck;
    bool m_performDiskCheck;
    bool m_performNetworkCheck;
    QWidget *m_performanceTab;
    QTextEdit *m_bottleneckText;
    QTextEdit *m_trendText;
    QTextEdit *m_suggestionsText;
    QComboBox *m_timeWindowCombo;
    QPushButton *m_runAnalysisButton;
    
    // ?????????UI??
    QWidget *m_nlpQueryTab;
    QLineEdit *m_queryInput;
    QPushButton *m_queryButton;
    QTextEdit *m_queryResultText;
    QListWidget *m_queryHistoryList;
    QList<QPair<QString, QString>> m_queryHistory; // ??????
    QPushButton *m_showHistoryButton;
};