#include <QCoreApplication>
#include <QCommandLineParser>
#include <QDebug>
#include <QHostAddress>
#include <QTimer>
#include <QThread>
#include "src/include/distributed/monitoringintegration.h"

int main(int argc, char *argv[])
{
    QCoreApplication app(argc, argv);
    app.setApplicationName("性能监控示例程序");
    app.setApplicationVersion("1.0");
    
    // 命令行参数解析
    QCommandLineParser parser;
    parser.setApplicationDescription("分布式性能监控系统示例程序");
    parser.addHelpOption();
    parser.addVersionOption();
    
    // 添加模式选项
    QCommandLineOption modeOption(QStringList() << "m" << "mode",
                                "运行模式 (server, client, standalone)",
                                "mode", "standalone");
    parser.addOption(modeOption);
    
    // 添加服务器地址选项
    QCommandLineOption serverOption(QStringList() << "s" << "server",
                                  "服务器地址",
                                  "address", "127.0.0.1");
    parser.addOption(serverOption);
    
    // 添加端口选项
    QCommandLineOption portOption(QStringList() << "p" << "port",
                                "服务器端口号",
                                "port", "8765");
    parser.addOption(portOption);
    
    // 添加采样间隔选项
    QCommandLineOption intervalOption(QStringList() << "i" << "interval",
                                    "采样间隔（毫秒）",
                                    "interval", "5000");
    parser.addOption(intervalOption);
    
    // 添加采样策略选项
    QCommandLineOption strategyOption(QStringList() << "strategy",
                                    "采样策略 (0=固定, 1=自适应, 2=智能)",
                                    "strategy", "0");
    parser.addOption(strategyOption);
    
    // 添加压缩算法选项
    QCommandLineOption compressionOption(QStringList() << "compression",
                                       "压缩算法 (0=无压缩, 1=LZ4, 2=ZLIB)",
                                       "compression", "0");
    parser.addOption(compressionOption);
    
    // 添加配置文件选项
    QCommandLineOption configOption(QStringList() << "c" << "config",
                                  "配置文件路径",
                                  "config");
    parser.addOption(configOption);
    
    // 解析命令行参数
    parser.process(app);
    
    // 获取运行模式
    QString modeStr = parser.value(modeOption);
    MonitoringIntegration::RunMode mode = MonitoringIntegration::StandaloneMode;
    if (modeStr == "server") {
        mode = MonitoringIntegration::ServerMode;
    } else if (modeStr == "client") {
        mode = MonitoringIntegration::ClientMode;
    }
    
    // 获取其他参数
    QString serverAddress = parser.value(serverOption);
    quint16 port = parser.value(portOption).toUShort();
    int interval = parser.value(intervalOption).toInt();
    int strategy = parser.value(strategyOption).toInt();
    int compression = parser.value(compressionOption).toInt();
    QString configFile = parser.value(configOption);
    
    // 创建监控集成对象
    MonitoringIntegration *monitoring = new MonitoringIntegration(&app);
    
    // 连接信号槽
    QObject::connect(monitoring, &MonitoringIntegration::statusChanged,
                    [](const QString &status) {
                        qDebug() << "[状态]" << status;
                    });
    
    QObject::connect(monitoring, &MonitoringIntegration::anomalyDetected,
                    [](const QString &source, const QString &metricName, double value) {
                        qDebug() << "[异常]" << source << metricName << value;
                    });
    
    // 初始化监控系统
    bool initialized = false;
    if (!configFile.isEmpty()) {
        // 使用配置文件初始化
        initialized = monitoring->initialize(mode, configFile);
    } else {
        // 使用命令行参数初始化
        initialized = monitoring->initialize(mode);
        
        // 设置参数
        if (mode == MonitoringIntegration::ClientMode) {
            monitoring->setServerAddress(QHostAddress(serverAddress), port);
        } else if (mode == MonitoringIntegration::ServerMode) {
            monitoring->setServerPort(port);
        }
        
        monitoring->setSamplingInterval(interval);
        monitoring->setSamplingStrategy(static_cast<AdaptiveSampler::SamplingStrategy>(strategy));
        monitoring->setCompressionAlgorithm(static_cast<AdaptiveSampler::CompressionAlgorithm>(compression));
    }
    
    if (!initialized) {
        qDebug() << "初始化失败！";
        return 1;
    }
    
    // 启动监控
    if (!monitoring->start()) {
        qDebug() << "启动失败！";}]}
        return 1;
    }
    
    qDebug() << "监控系统已启动，按Ctrl+C退出...";
    
    // 如果是服务器模式，添加一些测试节点
    if (mode == MonitoringIntegration::ServerMode) {
        // 延迟1秒后添加测试节点
        QTimer::singleShot(1000, [monitoring]() {
            monitoring->addRemoteNode("test-node-1", "测试节点1", QHostAddress("192.168.1.100"), 8766);
            monitoring->addRemoteNode("test-node-2", "测试节点2", QHostAddress("192.168.1.101"), 8766);
        });
    }
    
    // 定期导出报告（每60秒）
    QTimer reportTimer;
    QObject::connect(&reportTimer, &QTimer::timeout, [monitoring]() {
        QString reportPath = "monitoring_report_" + 
                            QDateTime::currentDateTime().toString("yyyyMMdd_hhmmss") + ".txt";
        if (monitoring->exportReport(reportPath)) {
            qDebug() << "报告已生成：" << reportPath;
        }
    });
    reportTimer.start(60000);
    
    // 运行应用程序事件循环
    return app.exec();
}