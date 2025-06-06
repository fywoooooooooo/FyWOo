#include "src/include/chart/chartwidget.h"
#include <QVBoxLayout>
#include <QPainter>
#include <QResizeEvent>
#include <cmath>

ChartWidget::ChartWidget(const QString &title, QWidget *parent)
    : QWidget(parent)
    , m_chart(new QChart())
    , m_chartView(new QChartView(m_chart, this))
    , m_series(new QSplineSeries(this))
    , m_axisX(new QValueAxis(this))
    , m_axisY(new QValueAxis(this))
    , m_title(title)
    , m_seriesName(QString())
    , m_maxPoints(60)
    , m_minY(0)
    , m_maxY(100)
    , m_currentMaxY(0)
    , m_data()
{
    setupChart();

    QVBoxLayout *layout = new QVBoxLayout(this);
    layout->setContentsMargins(0, 0, 0, 0);
    layout->addWidget(m_chartView);
    setLayout(layout);
}

ChartWidget::~ChartWidget() {}

void ChartWidget::setupChart()
{
    m_chart->setTitle(m_title);
    m_chart->legend()->hide();
    m_chart->addSeries(m_series);

    m_axisX->setRange(0, m_maxPoints);
    m_axisX->setLabelFormat("%d");
    m_axisX->setTitleText("时间 (秒)");
    m_chart->addAxis(m_axisX, Qt::AlignBottom);
    m_series->attachAxis(m_axisX);

    m_axisY->setRange(0, m_maxY);
    m_axisY->setLabelFormat("%.1f");
    m_axisY->setTitleText(m_title);
    m_chart->addAxis(m_axisY, Qt::AlignLeft);
    m_series->attachAxis(m_axisY);

    m_chartView->setRenderHint(QPainter::Antialiasing);
}

void ChartWidget::setTitle(const QString &title)
{
    m_title = title;
    m_chart->setTitle(m_title);
}

void ChartWidget::setYRange(qreal min, qreal max)
{
    m_minY = min;
    m_maxY = max;
    m_axisY->setRange(m_minY, m_maxY);
}

void ChartWidget::setSeriesName(const QString &name)
{
    m_seriesName = name;
    m_series->setName(m_seriesName);
    if (!m_seriesName.isEmpty()) {
        m_chart->legend()->show();
    } else {
        m_chart->legend()->hide();
    }
}

void ChartWidget::setMaxPoints(int points)
{
    m_maxPoints = points;
    m_axisX->setRange(0, points);
}

void ChartWidget::updateValue(qreal value)
{
    if (m_data.size() >= m_maxPoints) {
        m_data.removeFirst();
        for (int i = 0; i < m_data.size(); ++i) {
            m_series->replace(i, i, m_data[i]);
        }}

    m_data.append(value);
    m_series->append(m_data.size() - 1, value);

    // 动态调整Y轴范围
    // 1. 当当前值超过Y轴最大值的80%时，增加Y轴上限
    if (value > m_maxY * 0.8) {
        // 新的上限为当前值乘以1.5，并向上取整到最接近的10的倍数
        m_maxY = std::ceil(value * 1.5 / 10.0) * 10.0;
        m_axisY->setRange(m_minY, m_maxY);
    }
    
    // 2. 定期检查是否需要缩小Y轴上限
    static int checkCount = 0;
    checkCount++;
    
    // 每10次更新检查一次
    if (checkCount >= 10) {
        checkCount = 0;
        
        // 找到当前数据中的最大值
        qreal maxDataValue = 0;
        for (qreal dataPoint : m_data) {
            maxDataValue = qMax(maxDataValue, dataPoint);
        }
        
        // 如果当前最大值小于Y轴上限的50%，并且Y轴上限大于100，则缩小Y轴上限
        // 新的上限为当前最大值乘以1.5，并向上取整到最接近的10的倍数
        if (maxDataValue < m_maxY * 0.5 && m_maxY > 100) {
            // 新的上限为当前最大值乘以1.5，并向上取整到最接近的10的倍数
            qreal newMaxY = std::ceil(maxDataValue * 1.5 / 10.0) * 10.0;
            // 确保新的上限不小于100
            newMaxY = qMax(100.0, newMaxY);
            
            if (newMaxY < m_maxY) {
                m_maxY = newMaxY;
                m_axisY->setRange(m_minY, m_maxY);
            }
        }
    }
}

void ChartWidget::resetZoom()
{
    m_minY = 0;
    m_maxY = 100;
    m_axisY->setRange(m_minY, m_maxY);
    m_axisX->setRange(0, m_maxPoints);
}

void ChartWidget::clear()
{
    m_series->clear();
    m_currentMaxY = 0;
}

void ChartWidget::resizeEvent(QResizeEvent *event)
{
    if (m_chart) {
        m_chart->resize(event->size());
    }
    QWidget::resizeEvent(event);
}

void ChartWidget::addDataPoint(qreal value)
{
    if (m_data.size() >= m_maxPoints) {
        m_data.removeFirst();
    }
    m_data.append(value);
    m_series->clear();
    for (int i = 0; i < m_data.size(); ++i) {
        m_series->append(i, m_data[i]);
    }

    if (value > m_maxY) {
        m_maxY = value * 1.2;
        m_axisY->setRange(0, m_maxY);
    }
}
