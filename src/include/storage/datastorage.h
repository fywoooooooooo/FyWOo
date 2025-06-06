// datastorage.h
#ifndef DATASTORAGE_H
#define DATASTORAGE_H

#include <QObject>
#include <QVector>
#include <QDateTime>
#include <QString>
#include <QSqlDatabase>
#include <QSettings>

class DataStorage : public QObject
{
    Q_OBJECT

public:
    explicit DataStorage(QObject *parent = nullptr);
    ~DataStorage();
    void storeSample(const QString &type, double value);
    void storeSample(const QString &type, double value, const QDateTime &timestamp);
    // 存储系统数据的结构体
    struct SystemData {
        QDateTime timestamp;
        double cpuUsage;
        double memoryUsage;
        double diskUsage;
        double networkUpload;
        double networkDownload;
        double gpuUsage; // 添加GPU使用率字段
    };

    // 初始化存储系统
    bool initialize(const QString &dbPath);

public slots:
    // 添加新的系统数据
    void storeData(double cpuUsage, double memoryUsage, double diskUsage,
                  double networkUpload, double networkDownload);
    
    // 导出系统数据到CSV文件
    bool exportSystemData(const QString &filename);
    bool exportSystemData(const QString &filename, const QDateTime &startTime, const QDateTime &endTime);

    // 清除所有存储的数据
    void clear();
    
    // 保存采样率设置
    void saveSamplingRate(int rate);
    
    // 获取上次保存的采样率设置
    int getSavedSamplingRate();

protected:
    bool openDatabase(const QString &dbPath);
    void closeDatabase();
    bool createTables();

private:
    QVector<SystemData> m_systemData;
    static const int MAX_DATA_POINTS = 3600; // 存储1小时的数据（每秒一个数据点）
    QString m_dbPath;
    QSqlDatabase db;
    bool m_isInitialized;
};

#endif // DATASTORAGE_H
