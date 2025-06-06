// datastorage.cpp
#include "src/include/storage/datastorage.h"
#include <QSqlQuery>
#include <QSqlError>
#include <QFile>
#include <QTextStream>
#include <QDebug>
#include <QDir>

DataStorage::DataStorage(QObject *parent)
    : QObject(parent)
    , m_isInitialized(false)
{
    m_systemData.reserve(MAX_DATA_POINTS);
}

// 保存采样率设置到QSettings
void DataStorage::saveSamplingRate(int rate) {
    QSettings settings("PerformanceMonitor", "Settings");
    settings.setValue("sampling_rate", rate);
}

// 获取上次保存的采样率设置
int DataStorage::getSavedSamplingRate() {
    QSettings settings("PerformanceMonitor", "Settings");
    // 如果没有保存过设置，返回默认值1000ms
    return settings.value("sampling_rate", 1000).toInt();
}

DataStorage::~DataStorage()
{
    closeDatabase();
    clear();
}

bool DataStorage::initialize(const QString &dbPath)
{
    qDebug() << "[DataStorage] initialize called with path:" << dbPath;
    if (m_isInitialized) {
        qDebug() << "[DataStorage] Already initialized.";
        return true;
    }

    m_dbPath = dbPath;
    
    // 确保目录存在
    QDir dir = QFileInfo(dbPath).dir();
    if (!dir.exists()) {
        if (!dir.mkpath(".")) {
            qDebug() << "[DataStorage] 无法创建目录：" << dir.path();
            return false;
        }
    }
    
    if (!openDatabase(dbPath)) {
        qWarning() << "[DataStorage] 无法打开数据库:" << db.lastError().text();
        return false;
    }

    if (!createTables()) {
        qWarning() << "[DataStorage] 无法创建数据表:" << db.lastError().text();
        closeDatabase();
        return false;
    }

    m_isInitialized = true;
    qDebug() << "[DataStorage] 初始化成功，路径:" << m_dbPath;
    return true;
}

bool DataStorage::openDatabase(const QString &dbPath)
{
    qDebug() << "[DataStorage] openDatabase called with path:" << dbPath;
    db = QSqlDatabase::addDatabase("QSQLITE");
    db.setDatabaseName(dbPath);
    bool ok = db.open();
    qDebug() << "[DataStorage] openDatabase result:" << ok;
    return ok;
}

void DataStorage::closeDatabase()
{
    if (db.isOpen()) {
        db.close();
    }
    m_isInitialized = false;
}

bool DataStorage::createTables()
{
    qDebug() << "[DataStorage] createTables called.";
    QSqlQuery query(db);
    bool systemTableCreated = query.exec(
        "CREATE TABLE IF NOT EXISTS system_data ("
        "id INTEGER PRIMARY KEY AUTOINCREMENT,"
        "timestamp DATETIME DEFAULT CURRENT_TIMESTAMP,"
        "cpu_usage REAL,"
        "memory_usage REAL,"
        "disk_usage REAL,"
        "network_upload REAL,"
        "network_download REAL"
        ")"
    );
    qDebug() << "[DataStorage] system_data table created:" << systemTableCreated;
    
    bool samplesTableCreated = query.exec(
        "CREATE TABLE IF NOT EXISTS samples ("
        "id INTEGER PRIMARY KEY AUTOINCREMENT,"
        "type TEXT NOT NULL,"
        "value REAL,"
        "timestamp TEXT"
        ")"
    );
    qDebug() << "[DataStorage] samples table created:" << samplesTableCreated;
    
    return systemTableCreated && samplesTableCreated;
}

void DataStorage::storeData(double cpuUsage, double memoryUsage, double diskUsage, double networkUpload, double networkDownload)
{
    SystemData data;
    data.timestamp = QDateTime::currentDateTime();
    data.cpuUsage = cpuUsage;
    data.diskUsage = diskUsage;
    data.memoryUsage = memoryUsage;
    data.networkUpload = networkUpload;
    data.networkDownload = networkDownload;
    data.gpuUsage = 0; // 默认值，将通过storeSample更新

    if (m_systemData.size() >= MAX_DATA_POINTS) {
        m_systemData.removeFirst();
    }
    m_systemData.append(data);
}

bool DataStorage::exportSystemData(const QString &filename)
{
    return exportSystemData(filename, QDateTime(), QDateTime());
}

bool DataStorage::exportSystemData(const QString &filename, const QDateTime &startTime, const QDateTime &endTime)
{
    QFile file(filename);
    if (!file.open(QIODevice::WriteOnly | QIODevice::Text)) {
        qDebug() << "无法打开文件进行写入：" << filename;
        return false;
    }

    QTextStream out(&file);
    
    out << "time,CPU,Memory,Disk,Network,GPU\n";

    // 写入数据
    for (const SystemData &data : m_systemData) {
        // 如果指定了时间范围，检查数据是否在范围内
        if (startTime.isValid() && data.timestamp < startTime) continue;
        if (endTime.isValid() && data.timestamp > endTime) continue;

        // 计算网络总流量 (上传+下载)
        double networkTotal = (data.networkUpload + data.networkDownload) / 1024.0;
        
        out << data.timestamp.toString("yyyy-MM-dd :mm:ss") << ","
            << QString::number(data.cpuUsage, 'f', 4) << ","
            << QString::number(data.memoryUsage, 'f', 4) << ","
            << QString::number(data.diskUsage, 'f', 2) << ","
            << QString::number(networkTotal, 'f', 0) << ","
            << QString::number(data.gpuUsage, 'f', 0) << "\n";
    }

    file.close();
    return true;
}

void DataStorage::clear()
{
    m_systemData.clear();
}

void DataStorage::storeSample(const QString &type, double value)
{
    // 使用当前时间戳调用重载方法
    storeSample(type, value, QDateTime::currentDateTime());
}

void DataStorage::storeSample(const QString &type, double value, const QDateTime &timestamp)
{
    if (!m_isInitialized) {
        qWarning() << "[DataStorage] 数据存储未初始化，无法存储样本";
        return;
    }
    qDebug() << "[DataStorage] storeSample called, type:" << type << ", value:" << value << ", timestamp:" << timestamp;
    QSqlQuery query(db);
    query.prepare("INSERT INTO samples (type, value, timestamp) VALUES (?, ?, ?)");
    query.bindValue(0, type);
    query.bindValue(1, value);
    query.bindValue(2, timestamp.toString(Qt::ISODate));
    if (!query.exec()) {
        qWarning() << "[DataStorage] 无法存储样本:" << query.lastError().text();
    } else {
        qDebug() << "[DataStorage] storeSample success.";
    }
    // 如果是GPU数据，更新最后一条系统数据的GPU使用率
    if (type == "GPU" && !m_systemData.isEmpty()) {
        m_systemData.last().gpuUsage = value;
    }
}
