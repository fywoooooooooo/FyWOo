// infopanel.cpp
#include "src/include/ui/infopanel.h"

InfoPanel::InfoPanel(const QString& title, QWidget *parent)
    : QGroupBox(parent)
{
    setTitle(title);
    
    // 创建主布局
    m_mainLayout = new QGridLayout(this);
    m_mainLayout->setSpacing(10);
    
    // 创建图标标签
    m_iconLabel = new QLabel(this);
    m_mainLayout->addWidget(m_iconLabel, 0, 0, 2, 1);
    
    // 创建指标区域
    m_metricsWidget = new QWidget(this);
    m_metricsLayout = new QGridLayout(m_metricsWidget);
    m_metricsLayout->setContentsMargins(0, 0, 0, 0);
    m_mainLayout->addWidget(m_metricsWidget, 0, 1);
    
    // 创建详细信息区域
    m_detailsWidget = new QWidget(this);
    m_detailsLayout = new QGridLayout(m_detailsWidget);
    m_detailsLayout->setContentsMargins(0, 0, 0, 0);
    m_mainLayout->addWidget(m_detailsWidget, 1, 1);
    
    // 设置列拉伸
    m_mainLayout->setColumnStretch(1, 1);
}

InfoPanel::~InfoPanel()
{
}

void InfoPanel::setIcon(const QIcon& icon)
{
    m_iconLabel->setPixmap(icon.pixmap(32, 32));
}

void InfoPanel::addMetric(const QString& name, const QString& value, int progressValue)
{
    int row = m_metricLabels.size();
    
    // 创建标签
    QLabel* label = new QLabel(name + ": " + value, this);
    m_metricLabels.append(label);
    m_metricsLayout->addWidget(label, row, 0);
    
    // 如果提供了进度值，则创建进度条
    if (progressValue >= 0) {
        QProgressBar* progressBar = new QProgressBar(this);
        progressBar->setFixedHeight(15);
        progressBar->setValue(progressValue);
        m_progressBars.append(progressBar);
        m_metricsLayout->addWidget(progressBar, row, 1);
    } else {
        m_progressBars.append(nullptr);
    }
}

void InfoPanel::updateMetric(int index, const QString& value, int progressValue)
{
    if (index < 0 || index >= m_metricLabels.size())
        return;
    
    // 获取当前标签文本，并更新值部分
    QString labelText = m_metricLabels[index]->text();
    int colonPos = labelText.indexOf(":");
    if (colonPos >= 0) {
        QString name = labelText.left(colonPos);
        m_metricLabels[index]->setText(name + ": " + value);
    }
    
    // 更新进度条（如果存在）
    if (progressValue >= 0 && m_progressBars[index]) {
        m_progressBars[index]->setValue(progressValue);
    }
}

void InfoPanel::addDetailItem(const QString& name, const QString& value)
{
    int row = m_detailLabels.size();
    
    // 创建标签
    QLabel* label = new QLabel(name + ": " + value, this);
    m_detailLabels.append(label);
    m_detailsLayout->addWidget(label, row, 0);
}

void InfoPanel::updateDetailItem(int index, const QString& value)
{
    if (index < 0 || index >= m_detailLabels.size())
        return;
    
    // 获取当前标签文本，并更新值部分
    QString labelText = m_detailLabels[index]->text();
    int colonPos = labelText.indexOf(":");
    if (colonPos >= 0) {
        QString name = labelText.left(colonPos);
        m_detailLabels[index]->setText(name + ": " + value);
    }
}

void InfoPanel::clear()
{
    // 清除所有指标
    for (QLabel* label : m_metricLabels) {
        m_metricsLayout->removeWidget(label);
        delete label;
    }
    m_metricLabels.clear();
    
    for (QProgressBar* bar : m_progressBars) {
        if (bar) {
            m_metricsLayout->removeWidget(bar);
            delete bar;
        }
    }
    m_progressBars.clear();
    
    // 清除所有详细信息
    for (QLabel* label : m_detailLabels) {
        m_detailsLayout->removeWidget(label);
        delete label;
    }
    m_detailLabels.clear();
}