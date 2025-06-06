#ifndef NETWORKPAGE_H
#define NETWORKPAGE_H

#include <QWidget>
#include <QVBoxLayout>
#include <QComboBox>
#include <QLabel>
#include <QtCharts>
#include "src/include/chart/chartwidget.h"
#include <windows.h>
#include <iphlpapi.h>

QT_BEGIN_NAMESPACE
namespace Ui { class NetworkPage; }
QT_END_NAMESPACE

class NetworkPage : public QWidget
{
    Q_OBJECT

public:
    explicit NetworkPage(QWidget *parent = nullptr);
    ~NetworkPage();

public slots:
    void updateNetworkData();

private slots:
    void refreshInterfaces();
    void onInterfaceChanged(const QString &interfaceName);

private:
    void setupUI();
    QString formatSpeed(qint64 bytes);
    void updateDetailInfo(const MIB_IFROW& ifRow, qint64 uploadSpeed, qint64 downloadSpeed);
    QString formatMacAddress(const BYTE* addr, DWORD len);
    QString formatDataSize(qint64 bytes);

    QComboBox *m_interfaceCombo;
    QLabel *m_uploadSpeedLabel;
    QLabel *m_downloadSpeedLabel;
    ChartWidget *m_uploadChartWidget;
    ChartWidget *m_downloadChartWidget;
    QString m_currentInterface;
    qint64 m_lastUploadBytes;
    qint64 m_lastDownloadBytes;
    QLabel *m_detailLabel;
};

#endif // NETWORKPAGE_H
