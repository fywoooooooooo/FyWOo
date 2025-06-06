#pragma once

#include <QMainWindow>
#include <QStackedWidget>
#include <QPushButton>
#include <QGroupBox>
#include <QLabel>
#include <QGridLayout>
#include <QRandomGenerator>
#include <QTimer>
#include <QSystemTrayIcon>
#include <QMenu>
#include <QAction>
#include <QSettings>
#include <QMap>
#include <QDateTime>
#include <QVector>
#include <QDockWidget>
#include <QListWidget>

// Qt Data Visualization相关头文件已在Vis3DPage中包含

#include "chart/chartwidget.h"
#include "monitor/sampler.h"
#include "storage/datastorage.h"
#include "ui/settingswidget.h"
#include "ui/cpupage.h"
#include "ui/memorypage.h"
#include "ui/diskpage.h"
#include "ui/networkpage.h"
#include "ui/processpage.h"
#include "ui/gpupage.h"
#include "ui/analysispage.h"
#include "model/modelinterface.h"
#include "ui/processselectiondialog.h"
#include "common/processinfo.h"
#include "analysis/anomalydetector.h"

#ifdef USE_VISUALIZATION3D
#include "src/legacy/visualization3d/Vis3DPage.h"
#endif

class MainWindow : public QMainWindow {
    Q_OBJECT

public:
    enum class PageIndex {
        OverviewPage = 0,
        CpuPage,
        GpuPage,
        MemoryPage,
        DiskPage,
        NetworkPage,
        ProcessPage,
        AnalysisPage,
        SettingsPage,
        Visualization3DPage
    };
    explicit MainWindow(QWidget *parent = nullptr);
    ~MainWindow();

public slots:
    void showCpuPage();
    void showMemoryPage();
    void showDiskPage();
    void showNetworkPage();
    void showGpuPage();
    void showAnalysisPage();
    void show3DVisPage();
    void showSettingsPage();
    void exportData();
    void displayGpuNotification(const QString& title, const QString& message);

private slots:
    void switchToPage(int index);
    void handleExportRequest(const QString& path);
    
    // 模型相关槽函数
    void onModelSettingsChanged(const QString &modelName, const QString &apiKey);
    void onRunModelRequested(const QString &inputData);
    void onModelResponseReceived(const QString &response);
    void onModelErrorOccurred(const QString &errorMessage);

    void resetViewport();

private:
    void setupUI();
    void setupConnections();
    void setupNavigation();
    void loadSettings();
    void saveSettings();
    QWidget* createOverviewPage();
    void createVisualizationPage();
    void updateNavigationButton(QPushButton* button, bool selected);
    void resetNavigationButtons();
    void loadModelSettings();
    void handleError(const QString& errorMessage);
    void showProcessSelectionDialogAndTerminate(const QList<ProcessInfo>& processes, const QString& title, const QString& type);

    // Charts
    ChartWidget *m_cpuChart;
    ChartWidget *m_gpuChart;
    ChartWidget *m_memoryChart;
    ChartWidget *m_wifiChart;

    // Core components
    Sampler *m_sampler;
    DataStorage *m_storage;
    ModelInterface *m_modelInterface;
    AnomalyDetector *m_anomalyDetector;

    // Pages
    QWidget *m_overviewPage;
    CpuPage *m_cpuPage;
    GpuPage *m_gpuPage;
    MemoryPage *m_memoryPage;
    DiskPage *m_diskPage;
    NetworkPage *m_networkPage;
    ProcessPage *m_processPage;
    AnalysisPage *m_analysisPage;
    SettingsWidget *m_settingsPage;
    
#ifdef USE_VISUALIZATION3D
    Vis3DPage *m_vis3DPage;
#endif
    
    // UI components
    QStackedWidget *m_pageStack;
    QPushButton *m_overviewButton;
    QPushButton *m_cpuButton;
    QPushButton *m_gpuButton;
    QPushButton *m_memoryButton;
    QPushButton *m_diskButton;
    QPushButton *m_networkButton;
    QPushButton *m_processButton;
    QPushButton *m_analysisButton;
    QPushButton *m_3dVisButton;
    QPushButton *m_exportButton;
    QPushButton *m_settingsButton;

    // Optimization buttons for the right panel
    QPushButton *m_closeHighCpuProcessButton;
    QPushButton *m_closeHighMemoryProcessButton;
    QPushButton *m_optimizeThreadPoolButton;
    QPushButton *m_optimizeDiskButton;

private slots:
    void onCloseHighCpuProcessClicked();
    void onCloseHighMemoryProcessClicked();
    void onOptimizeThreadPoolClicked();
    void onOptimizeDiskClicked();
    void onProcessTerminationRequested(quint64 pid);
};
