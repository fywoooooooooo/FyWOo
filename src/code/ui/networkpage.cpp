#include "src/include/ui/networkpage.h"
#include "src/include/monitor/networkmonitor.h"
#include <windows.h>
#include <iphlpapi.h>
#include <netioapi.h>
#include <cmath>

#pragma comment(lib, "iphlpapi.lib")

QT_USE_NAMESPACE

NetworkPage::NetworkPage(QWidget *parent)
    : QWidget(parent)
    , m_interfaceCombo(new QComboBox(this))
    , m_uploadSpeedLabel(new QLabel(this))
    , m_downloadSpeedLabel(new QLabel(this))
    , m_uploadChartWidget(new ChartWidget(tr("上传速度历史"), this))
    , m_downloadChartWidget(new ChartWidget(tr("下载速度历史"), this))
    , m_lastUploadBytes(0)
    , m_lastDownloadBytes(0)
{
    setupUI();
    refreshInterfaces();

    QTimer *timer = new QTimer(this);
    connect(timer, &QTimer::timeout, this, &NetworkPage::updateNetworkData);
    timer->start(1000);

    connect(m_interfaceCombo, &QComboBox::currentTextChanged,
            this, &NetworkPage::onInterfaceChanged);
}

NetworkPage::~NetworkPage()
{
}

void NetworkPage::setupUI()
{
    QVBoxLayout *mainLayout = new QVBoxLayout(this);
    mainLayout->setSpacing(16);
    mainLayout->setContentsMargins(16, 16, 16, 16);

    // 网络接口选择和速度显示区域（移到页面左上角）
    QHBoxLayout *topLayout = new QHBoxLayout;
    
    // 创建带有更大字体的标签
    QLabel* interfaceLabel = new QLabel(tr("网络接口:"));
    interfaceLabel->setStyleSheet("QLabel { font-size: 12pt; font-weight: bold; }");
    
    // 设置下拉框样式
    m_interfaceCombo->setStyleSheet("QComboBox { font-size: 11pt; min-width: 280px; padding: 4px; }");
    
    // 设置上传下载标签样式
    QLabel* uploadLabel = new QLabel(tr("上传速度:"));
    uploadLabel->setStyleSheet("QLabel { font-size: 12pt; font-weight: bold; color: #4CAF50; }");
    
    m_uploadSpeedLabel->setStyleSheet("QLabel { font-size: 12pt; font-weight: bold; min-width: 100px; }");
    m_uploadSpeedLabel->setAlignment(Qt::AlignLeft | Qt::AlignVCenter);
    
    QLabel* downloadLabel = new QLabel(tr("下载速度:"));
    downloadLabel->setStyleSheet("QLabel { font-size: 12pt; font-weight: bold; color: #2196F3; }");
    
    m_downloadSpeedLabel->setStyleSheet("QLabel { font-size: 12pt; font-weight: bold; min-width: 100px; }");
    m_downloadSpeedLabel->setAlignment(Qt::AlignLeft | Qt::AlignVCenter);
    
    // 添加到顶部布局
    topLayout->addWidget(interfaceLabel);
    topLayout->addWidget(m_interfaceCombo);
    topLayout->addStretch();
    topLayout->addWidget(uploadLabel);
    topLayout->addWidget(m_uploadSpeedLabel);
    topLayout->addWidget(downloadLabel);
    topLayout->addWidget(m_downloadSpeedLabel);
    mainLayout->addLayout(topLayout);

    // 图表区域
    QHBoxLayout *chartLayout = new QHBoxLayout;
    m_uploadChartWidget->setYRange(0, 1024); // 初始范围设为 0-1024 KB/s
    m_uploadChartWidget->setTitle(tr("上传速度历史"));
    chartLayout->addWidget(m_uploadChartWidget);
    
    m_downloadChartWidget->setYRange(0, 1024); // 初始范围设为 0-1024 KB/s
    m_downloadChartWidget->setTitle(tr("下载速度历史"));
    chartLayout->addWidget(m_downloadChartWidget);
    
    mainLayout->addLayout(chartLayout);

    // 详细信息面板
    QGroupBox *infoPanel = new QGroupBox(this);
    infoPanel->setTitle(tr("详细信息"));
    infoPanel->setStyleSheet("QGroupBox { font-size: 12pt; font-weight: bold; }");
    
    QVBoxLayout *infoLayout = new QVBoxLayout(infoPanel);
    m_detailLabel = new QLabel(tr("请选择网络接口以查看详细信息"), this);
    m_detailLabel->setStyleSheet("QLabel { font-size: 11pt; }");
    infoLayout->addWidget(m_detailLabel);
    mainLayout->addWidget(infoPanel);

    connect(m_interfaceCombo, &QComboBox::currentTextChanged, this, [=](const QString &text){
        int idx = m_interfaceCombo->currentIndex();
        if(idx >= 0) {
            QString detail = tr("接口: %1\n上传速度: %2\n下载速度: %3")
                .arg(m_interfaceCombo->currentText())
                .arg(m_uploadSpeedLabel->text())
                .arg(m_downloadSpeedLabel->text());
            m_detailLabel->setText(detail);
        }
    });
}

void NetworkPage::refreshInterfaces()
{
    m_interfaceCombo->clear();

    ULONG bufferSize = 0;
    GetAdaptersInfo(nullptr, &bufferSize);
    
    if (bufferSize == 0) {
        return;
    }

    std::vector<BYTE> buffer(bufferSize);
    PIP_ADAPTER_INFO adapterInfo = reinterpret_cast<PIP_ADAPTER_INFO>(buffer.data());
    
    if (GetAdaptersInfo(adapterInfo, &bufferSize) == ERROR_SUCCESS) {
        while (adapterInfo) {
            // 排除回环接口
            if (adapterInfo->Type != MIB_IF_TYPE_LOOPBACK) {
                QString description = QString::fromLocal8Bit(adapterInfo->Description);
                m_interfaceCombo->addItem(description, QVariant::fromValue(adapterInfo->Index));
            }
            adapterInfo = adapterInfo->Next;
        }
    }

    if (m_interfaceCombo->count() > 0) {
        // 获取当前活跃的网络接口
        NetworkMonitor monitor;
        DWORD activeInterface = monitor.getActiveNetworkInterface();
        
        // 如果找到活跃接口，则选择它
        if (activeInterface > 0) {
            for (int i = 0; i < m_interfaceCombo->count(); i++) {
                if (m_interfaceCombo->itemData(i).toUInt() == activeInterface) {
                    m_interfaceCombo->setCurrentIndex(i);
                    break;
                }
            }
        } else {
            // 如果没有找到活跃接口，则默认选择第一个
            m_interfaceCombo->setCurrentIndex(0);
        }
        
        onInterfaceChanged(m_interfaceCombo->currentText());
    }
}

void NetworkPage::onInterfaceChanged(const QString &interfaceName)
{
    m_lastUploadBytes = 0;
    m_lastDownloadBytes = 0;
    updateNetworkData();
}

QString NetworkPage::formatSpeed(qint64 bytes)
{
    const double kb = bytes / 1024.0;
    if (kb < 1024) {
        return QString("%1 KB/s").arg(kb, 0, 'f', 2);
    }
    const double mb = kb / 1024.0;
    if (mb < 1024) {
        return QString("%1 MB/s").arg(mb, 0, 'f', 2);
    }
    const double gb = mb / 1024.0;
    return QString("%1 GB/s").arg(gb, 0, 'f', 2);
}

void NetworkPage::updateNetworkData()
{
    if (m_interfaceCombo->currentData().isValid()) {
        DWORD index = m_interfaceCombo->currentData().toUInt();
        MIB_IFROW ifRow = { 0 };
        ifRow.dwIndex = index;

        if (GetIfEntry(&ifRow) == NO_ERROR) {
            qint64 currentUploadBytes = ifRow.dwOutOctets;
            qint64 currentDownloadBytes = ifRow.dwInOctets;

            if (m_lastUploadBytes > 0 && m_lastDownloadBytes > 0) {
                qint64 uploadSpeed = currentUploadBytes - m_lastUploadBytes;
                qint64 downloadSpeed = currentDownloadBytes - m_lastDownloadBytes;

                // 如果当前接口没有流量，尝试切换到活跃的接口
                static int noTrafficCount = 0;
                if (uploadSpeed == 0 && downloadSpeed == 0) {
                    noTrafficCount++;
                    
                    // 连续5秒没有流量，尝试切换到活跃接口
                    if (noTrafficCount >= 5) {
                        NetworkMonitor monitor;
                        DWORD activeInterface = monitor.getActiveNetworkInterface();
                        
                        if (activeInterface > 0 && activeInterface != index) {
                            // 查找活跃接口在下拉框中的索引
                            for (int i = 0; i < m_interfaceCombo->count(); i++) {
                                if (m_interfaceCombo->itemData(i).toUInt() == activeInterface) {
                                    m_interfaceCombo->setCurrentIndex(i);
                                    noTrafficCount = 0;
                                    return; // 切换接口后退出，等待下一次更新
                                }
                            }
                        }
                    }
                } else {
                    // 有流量时重置计数器
                    noTrafficCount = 0;
                }

                // 更新速度标签
                m_uploadSpeedLabel->setText(formatSpeed(uploadSpeed));
                m_downloadSpeedLabel->setText(formatSpeed(downloadSpeed));

                // 更新图表（转换为 KB/s）
                double uploadSpeedKB = uploadSpeed / 1024.0;
                double downloadSpeedKB = downloadSpeed / 1024.0;

                m_uploadChartWidget->updateValue(uploadSpeedKB);
                m_downloadChartWidget->updateValue(downloadSpeedKB);
                
                // 更新详细信息
                updateDetailInfo(ifRow, uploadSpeed, downloadSpeed);
            }

            m_lastUploadBytes = currentUploadBytes;
            m_lastDownloadBytes = currentDownloadBytes;
        }
    }
}

void NetworkPage::updateDetailInfo(const MIB_IFROW& ifRow, qint64 uploadSpeed, qint64 downloadSpeed)
{
    QString status;
    switch (ifRow.dwOperStatus) {
        case IF_OPER_STATUS_OPERATIONAL:
            status = tr("连接");
            break;
        case IF_OPER_STATUS_NON_OPERATIONAL:
            status = tr("断开");
            break;
        default:
            status = tr("未知状态");
    }
    
    QString detail = tr("接口: %1\n"
                      "状态: %2\n"
                      "上传速度: %3\n"
                      "下载速度: %4\n"
                      "MAC地址: %5\n"
                      "MTU: %6\n"
                      "总上传: %7\n"
                      "总下载: %8")
        .arg(m_interfaceCombo->currentText())
        .arg(status)
        .arg(formatSpeed(uploadSpeed))
        .arg(formatSpeed(downloadSpeed))
        .arg(formatMacAddress(ifRow.bPhysAddr, ifRow.dwPhysAddrLen))
        .arg(ifRow.dwMtu)
        .arg(formatDataSize(ifRow.dwOutOctets))
        .arg(formatDataSize(ifRow.dwInOctets));
        
    m_detailLabel->setText(detail);
}

QString NetworkPage::formatMacAddress(const BYTE* addr, DWORD len)
{
    if (!addr || len == 0) return tr("未知");
    
    QString macAddr;
    for (DWORD i = 0; i < len; i++) {
        if (i > 0) macAddr += ":";
        macAddr += QString("%1").arg(addr[i], 2, 16, QChar('0')).toUpper();
    }
    return macAddr;
}

QString NetworkPage::formatDataSize(qint64 bytes)
{
    const double kb = bytes / 1024.0;
    if (kb < 1024) {
        return QString("%1 KB").arg(kb, 0, 'f', 2);
    }
    const double mb = kb / 1024.0;
    if (mb < 1024) {
        return QString("%1 MB").arg(mb, 0, 'f', 2);
    }
    const double gb = mb / 1024.0;
    return QString("%1 GB").arg(gb, 0, 'f', 2);
}
