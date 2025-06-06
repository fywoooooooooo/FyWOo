#ifndef CPUPAGE_H
#define CPUPAGE_H

#include <QWidget>
#include <QVBoxLayout>
#include <QtCharts>
#include <QTimer>
#include <QLabel>
#include <QProgressBar>
#include "src/include/chart/chartwidget.h"

QT_BEGIN_NAMESPACE
namespace Ui { class CpuPage; }
QT_END_NAMESPACE

class CpuPage : public QWidget
{
    Q_OBJECT

public:
    explicit CpuPage(QWidget *parent = nullptr);
    ~CpuPage();

public slots:
    void updateCpuData();

private:
    void setupUI();
    void setupCharts();
    void setupControls();
    QChart *m_chart;
    QChartView *m_chartView;
    QLineSeries *m_series;
    QValueAxis *m_axisX;
    QValueAxis *m_axisY;
    QTimer *m_updateTimer;
    QVector<qreal> m_data;
    ChartWidget *m_chartWidget;
    
    // 详细信息面板元素
    QProgressBar *m_usageBar;
    QLabel *m_usageLabel;
    QLabel *m_coresLabel;
    QLabel *m_modelLabel;
    QLabel *m_freqLabel;
    QLabel *m_archLabel;
};


#endif // CPUPAGE_H
