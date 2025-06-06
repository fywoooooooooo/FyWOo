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
    app.setApplicationName("���ܼ��ʾ������");
    app.setApplicationVersion("1.0");
    
    // �����в�������
    QCommandLineParser parser;
    parser.setApplicationDescription("�ֲ�ʽ���ܼ��ϵͳʾ������");
    parser.addHelpOption();
    parser.addVersionOption();
    
    // ���ģʽѡ��
    QCommandLineOption modeOption(QStringList() << "m" << "mode",
                                "����ģʽ (server, client, standalone)",
                                "mode", "standalone");
    parser.addOption(modeOption);
    
    // ��ӷ�������ַѡ��
    QCommandLineOption serverOption(QStringList() << "s" << "server",
                                  "��������ַ",
                                  "address", "127.0.0.1");
    parser.addOption(serverOption);
    
    // ��Ӷ˿�ѡ��
    QCommandLineOption portOption(QStringList() << "p" << "port",
                                "�������˿ں�",
                                "port", "8765");
    parser.addOption(portOption);
    
    // ��Ӳ������ѡ��
    QCommandLineOption intervalOption(QStringList() << "i" << "interval",
                                    "������������룩",
                                    "interval", "5000");
    parser.addOption(intervalOption);
    
    // ��Ӳ�������ѡ��
    QCommandLineOption strategyOption(QStringList() << "strategy",
                                    "�������� (0=�̶�, 1=����Ӧ, 2=����)",
                                    "strategy", "0");
    parser.addOption(strategyOption);
    
    // ���ѹ���㷨ѡ��
    QCommandLineOption compressionOption(QStringList() << "compression",
                                       "ѹ���㷨 (0=��ѹ��, 1=LZ4, 2=ZLIB)",
                                       "compression", "0");
    parser.addOption(compressionOption);
    
    // ��������ļ�ѡ��
    QCommandLineOption configOption(QStringList() << "c" << "config",
                                  "�����ļ�·��",
                                  "config");
    parser.addOption(configOption);
    
    // ���������в���
    parser.process(app);
    
    // ��ȡ����ģʽ
    QString modeStr = parser.value(modeOption);
    MonitoringIntegration::RunMode mode = MonitoringIntegration::StandaloneMode;
    if (modeStr == "server") {
        mode = MonitoringIntegration::ServerMode;
    } else if (modeStr == "client") {
        mode = MonitoringIntegration::ClientMode;
    }
    
    // ��ȡ��������
    QString serverAddress = parser.value(serverOption);
    quint16 port = parser.value(portOption).toUShort();
    int interval = parser.value(intervalOption).toInt();
    int strategy = parser.value(strategyOption).toInt();
    int compression = parser.value(compressionOption).toInt();
    QString configFile = parser.value(configOption);
    
    // ������ؼ��ɶ���
    MonitoringIntegration *monitoring = new MonitoringIntegration(&app);
    
    // �����źŲ�
    QObject::connect(monitoring, &MonitoringIntegration::statusChanged,
                    [](const QString &status) {
                        qDebug() << "[״̬]" << status;
                    });
    
    QObject::connect(monitoring, &MonitoringIntegration::anomalyDetected,
                    [](const QString &source, const QString &metricName, double value) {
                        qDebug() << "[�쳣]" << source << metricName << value;
                    });
    
    // ��ʼ�����ϵͳ
    bool initialized = false;
    if (!configFile.isEmpty()) {
        // ʹ�������ļ���ʼ��
        initialized = monitoring->initialize(mode, configFile);
    } else {
        // ʹ�������в�����ʼ��
        initialized = monitoring->initialize(mode);
        
        // ���ò���
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
        qDebug() << "��ʼ��ʧ�ܣ�";
        return 1;
    }
    
    // �������
    if (!monitoring->start()) {
        qDebug() << "����ʧ�ܣ�";}]}
        return 1;
    }
    
    qDebug() << "���ϵͳ����������Ctrl+C�˳�...";
    
    // ����Ƿ�����ģʽ�����һЩ���Խڵ�
    if (mode == MonitoringIntegration::ServerMode) {
        // �ӳ�1�����Ӳ��Խڵ�
        QTimer::singleShot(1000, [monitoring]() {
            monitoring->addRemoteNode("test-node-1", "���Խڵ�1", QHostAddress("192.168.1.100"), 8766);
            monitoring->addRemoteNode("test-node-2", "���Խڵ�2", QHostAddress("192.168.1.101"), 8766);
        });
    }
    
    // ���ڵ������棨ÿ60�룩
    QTimer reportTimer;
    QObject::connect(&reportTimer, &QTimer::timeout, [monitoring]() {
        QString reportPath = "monitoring_report_" + 
                            QDateTime::currentDateTime().toString("yyyyMMdd_hhmmss") + ".txt";
        if (monitoring->exportReport(reportPath)) {
            qDebug() << "���������ɣ�" << reportPath;
        }
    });
    reportTimer.start(60000);
    
    // ����Ӧ�ó����¼�ѭ��
    return app.exec();
}