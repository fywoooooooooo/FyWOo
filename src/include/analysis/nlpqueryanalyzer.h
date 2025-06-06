#pragma once

#include <QObject>
#include <QVariant>
#include <QDateTime>
#include <QMap>
#include "src/include/analysis/performanceanalyzer.h"
#include "src/include/storage/datastorage.h"

// 自然语言查询分析器 - 支持用户通过自然语言查询系统性能数据
class NLPQueryAnalyzer : public QObject {
    Q_OBJECT

public:
    // 统计类型枚举
    enum StatType {
        Maximum,   // 最大值
        Minimum,   // 最小值
        Average,   // 平均值
        Median,    // 中位数
        Trend      // 趋势
    };

    explicit NLPQueryAnalyzer(QObject *parent = nullptr);
    ~NLPQueryAnalyzer();

    // 设置性能分析器
    void setPerformanceAnalyzer(PerformanceAnalyzer *analyzer);

    // 设置数据存储
    void setDataStorage(DataStorage *storage);

    // 处理自然语言查询
    QVariant processQuery(const QString &query);

private:
    // 初始化关键词映射
    void initializeKeywords();

    // 从查询中提取时间范围
    void extractTimeRange(const QString &query, QDateTime &startTime, QDateTime &endTime);

    // 从查询中提取统计类型
    StatType extractStatType(const QString &query);

    // 处理CPU相关查询
    QVariant processCpuQuery(const QString &query, const QDateTime &startTime, const QDateTime &endTime);

    // 处理内存相关查询
    QVariant processMemoryQuery(const QString &query, const QDateTime &startTime, const QDateTime &endTime);

    // 处理磁盘相关查询
    QVariant processDiskQuery(const QString &query, const QDateTime &startTime, const QDateTime &endTime);

    // 处理网络相关查询
    QVariant processNetworkQuery(const QString &query, const QDateTime &startTime, const QDateTime &endTime);

    // 处理进程相关查询
    QVariant processProcessQuery(const QString &query, const QDateTime &startTime, const QDateTime &endTime);

    // 处理系统整体查询
    QVariant processSystemQuery(const QString &query, const QDateTime &startTime, const QDateTime &endTime);

    // 将趋势类型转换为字符串
    QString trendTypeToString(PerformanceAnalyzer::TrendType trend);

private:
    PerformanceAnalyzer *m_performanceAnalyzer;
    DataStorage *m_dataStorage;

    // 关键词映射
    QMap<QString, int> m_timeKeywords;      // 时间关键词
    QMap<QString, int> m_timeUnits;         // 时间单位
    QMap<QString, StatType> m_statKeywords; // 统计类型关键词
};