#ifndef CHARTWIDGET_H
#define CHARTWIDGET_H

#include <QWidget>
#include <QtCharts/QChart>
#include <QtCharts/QChartView>
#include <QtCharts/QSplineSeries>
#include <QtCharts/QValueAxis>
#include <QVBoxLayout>
#include <QQueue>

QT_BEGIN_NAMESPACE
namespace QtCharts {}
QT_END_NAMESPACE
class QResizeEvent;
using namespace QtCharts;

class ChartWidget : public QWidget
{
    Q_OBJECT

public:
    explicit ChartWidget(const QString &title = QString(), QWidget *parent = nullptr);
    ~ChartWidget();

    qreal yRange() const { return m_maxY; }

    void addDataPoint(qreal value); // <-- ???????????

public slots:
    void setTitle(const QString &title);
    void setYRange(qreal min, qreal max);
    void setSeriesName(const QString &name);
    void setMaxPoints(int points);
    void updateValue(qreal value);
    void clear();
    void resetZoom(); // Added resetZoom declaration

protected:
    void resizeEvent(QResizeEvent *event) override;

private:
    void setupChart();
    void updateAxisRange();

    QChart *m_chart;
    QChartView *m_chartView;
    QSplineSeries *m_series;
    QValueAxis *m_axisX;
    QValueAxis *m_axisY;
    QString m_title;
    QString m_seriesName;
    int m_maxPoints;
    qreal m_minY;
    qreal m_maxY;
    qreal m_currentMaxY;
    QVector<qreal> m_data;
};

#endif // CHARTWIDGET_H
