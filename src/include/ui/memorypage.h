#ifndef MEMORYPAGE_H
#define MEMORYPAGE_H

#include <QWidget>
#include <QVBoxLayout>
#include <QProgressBar>
#include <QLabel>
#include <QtCharts>
#include "src/include/chart/chartwidget.h"

QT_BEGIN_NAMESPACE
namespace Ui { class MemoryPage; }
QT_END_NAMESPACE

class MemoryPage : public QWidget
{
    Q_OBJECT

public:
    explicit MemoryPage(QWidget *parent = nullptr);
    ~MemoryPage();

public slots:
    void updateMemoryData();
    void updateLabels(quint64 total, quint64 used, quint64 free);

private:
    void setupUI();
    QString formatSize(quint64 bytes);

    QLabel *m_totalMemLabel;
    QLabel *m_usedMemLabel;
    QLabel *m_freeMemLabel;
    QProgressBar *m_memoryBar;
    QChart *m_chart;
    QChartView *m_chartView;
    QLineSeries *m_series;
    QTimer *m_updateTimer;
    ChartWidget *m_chartWidget;
    
    // 详细信息面板元素
    QLabel *m_usagePercentLabel;
    QLabel *m_memTypeLabel;
    QLabel *m_pageFileLabel;
};


#endif // MEMORYPAGE_H
