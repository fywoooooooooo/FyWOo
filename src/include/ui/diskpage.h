#ifndef DISKPAGE_H
#define DISKPAGE_H

#include <QWidget>
#include <QVBoxLayout>
#include <QTableWidget>
#include <QtCharts>
#include <QTimer>
#include <QLabel>
#include <QProgressBar>
#include <QFrame>
#include "src/include/chart/chartwidget.h"

QT_BEGIN_NAMESPACE
namespace Ui { class DiskPage; }
QT_END_NAMESPACE

class DiskPage : public QWidget
{
    Q_OBJECT

public:
    explicit DiskPage(QWidget *parent = nullptr);
    ~DiskPage();

public slots:
    void updateDiskData();

private slots:
    void updateDiskTable();
    void updateDiskChart();
    void updateDiskProgressBar(double usagePercent);

private:
    void setupUI();
    QString formatSize(qint64 bytes);

    QTableWidget *m_diskTable;
    QChart *m_chart;
    QChartView *m_chartView;
    QLineSeries *m_series;
    QTimer *m_updateTimer;
    ChartWidget *m_chartWidget;
    QVector<double> m_diskUsage;
    QLabel *m_detailLabel;
    QFrame *m_progressFrame;
    QProgressBar *m_diskProgressBar;
};

#endif // DISKPAGE_H
