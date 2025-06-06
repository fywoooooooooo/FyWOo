// infopanel.h
#pragma once

#include <QWidget>
#include <QGroupBox>
#include <QGridLayout>
#include <QLabel>
#include <QProgressBar>
#include <QIcon>

// 可重用的信息面板组件，用于在各个监控页面中显示设备详细信息
class InfoPanel : public QGroupBox
{
    Q_OBJECT

public:
    explicit InfoPanel(const QString& title, QWidget *parent = nullptr);
    ~InfoPanel();

    // 添加图标到面板
    void setIcon(const QIcon& icon);
    
    // 添加带进度条的指标
    void addMetric(const QString& name, const QString& value, int progressValue = -1);
    
    // 更新指标值
    void updateMetric(int index, const QString& value, int progressValue = -1);
    
    // 添加详细信息项（无进度条）
    void addDetailItem(const QString& name, const QString& value);
    
    // 更新详细信息项
    void updateDetailItem(int index, const QString& value);
    
    // 清除所有指标和详细信息
    void clear();

private:
    QGridLayout *m_mainLayout;
    QLabel *m_iconLabel;
    
    // 基本指标区域（带进度条）
    QWidget *m_metricsWidget;
    QGridLayout *m_metricsLayout;
    QList<QLabel*> m_metricLabels;
    QList<QProgressBar*> m_progressBars;
    
    // 详细信息区域
    QWidget *m_detailsWidget;
    QGridLayout *m_detailsLayout;
    QList<QLabel*> m_detailLabels;
};