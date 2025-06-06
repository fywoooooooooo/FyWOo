#include "src/include/ui/diskpage.h"
#include <QPainter>
#include <windows.h>
#include <QVBoxLayout>
#include <QHeaderView>

QT_USE_NAMESPACE

DiskPage::DiskPage(QWidget *parent)
    : QWidget(parent)
    , m_diskTable(new QTableWidget(this))
    , m_chartWidget(new ChartWidget(tr("磁盘使用率"), this))
    , m_updateTimer(new QTimer(this))
    , m_diskUsage(60, 0.0)
    , m_progressFrame(nullptr)
    , m_diskProgressBar(nullptr)
{
    setupUI();
    connect(m_updateTimer, &QTimer::timeout, this, &DiskPage::updateDiskData);
    m_updateTimer->start(1000);
}

DiskPage::~DiskPage()
{
    m_updateTimer->stop();
}

void DiskPage::setupUI()
{
    QVBoxLayout *mainLayout = new QVBoxLayout(this);
    mainLayout->setSpacing(10);
    mainLayout->setContentsMargins(10, 10, 10, 10);

    // 磁盘使用率图表框
    QGroupBox *chartGroup = new QGroupBox(this);
    chartGroup->setTitle(tr("磁盘使用率历史"));
    chartGroup->setStyleSheet("QGroupBox { background-color: white; border-radius: 8px; border: 1px solid #e0e0e0; font-weight: bold; }");
    QVBoxLayout *chartLayout = new QVBoxLayout(chartGroup);
    
    m_chartWidget->setTitle("磁盘使用率");
    m_chartWidget->setYRange(0, 100);
    chartLayout->addWidget(m_chartWidget);
    mainLayout->addWidget(chartGroup);

    // 磁盘使用率详情框
    QFrame *diskInfoFrame = new QFrame(this);
    diskInfoFrame->setStyleSheet("QFrame { background-color: white; border-radius: 8px; border: 1px solid #e0e0e0; }");
    QVBoxLayout *infoLayout = new QVBoxLayout(diskInfoFrame);
    
    QLabel *infoTitle = new QLabel("磁盘详细信息:", this);
    infoTitle->setStyleSheet("QLabel { color: #333333; font-size: 12pt; font-weight: bold; }");
    infoLayout->addWidget(infoTitle);
    
    m_detailLabel = new QLabel(tr("请选择磁盘以查看详细信息"), this);
    m_detailLabel->setStyleSheet("QLabel { color: #333333; font-size: 10pt; }");
    infoLayout->addWidget(m_detailLabel);
    mainLayout->addWidget(diskInfoFrame);

    // 磁盘进度条框
    m_progressFrame = new QFrame(this);
    m_progressFrame->setStyleSheet("QFrame { background-color: white; border-radius: 8px; border: 1px solid #e0e0e0; }");
    QVBoxLayout *progressLayout = new QVBoxLayout(m_progressFrame);
    
    QLabel *progressTitle = new QLabel("所选磁盘使用率:", this);
    progressTitle->setStyleSheet("QLabel { color: #333333; font-size: 12pt; font-weight: bold; }");
    progressLayout->addWidget(progressTitle);
    
    // 磁盘使用率进度条
    m_diskProgressBar = new QProgressBar(this);
    m_diskProgressBar->setRange(0, 100);
    m_diskProgressBar->setValue(0);
    m_diskProgressBar->setTextVisible(true);
    m_diskProgressBar->setFormat("%p%");
    
    // 设置进度条样式
    QString progressBarStyle = 
        "QProgressBar {"
        "   border: 1px solid #e0e0e0;"
        "   border-radius: 5px;"
        "   background-color: #f5f5f5;"
        "   text-align: center;"
        "}"
        "QProgressBar::chunk {"
        "   background-color: #FF9800;"
        "   border-radius: 5px;"
        "}";
    
    m_diskProgressBar->setStyleSheet(progressBarStyle);
    progressLayout->addWidget(m_diskProgressBar);
    mainLayout->addWidget(m_progressFrame);

    // 设置表格
    m_diskTable->setColumnCount(5);
    m_diskTable->setHorizontalHeaderLabels(QStringList() 
        << "盘符" 
        << "总容量" 
        << "已用空间" 
        << "可用空间"
        << "使用率");
    m_diskTable->horizontalHeader()->setSectionResizeMode(QHeaderView::Stretch);
    m_diskTable->setStyleSheet(
        "QTableWidget {"
        "   background-color: white;"
        "   border-radius: 8px;"
        "   border: 1px solid #e0e0e0;"
        "}"
        "QHeaderView::section {"
        "   background-color: #f5f5f5;"
        "   padding: 4px;"
        "   font-weight: bold;"
        "   border: 1px solid #e0e0e0;"
        "}"
    );
    mainLayout->addWidget(m_diskTable);

    connect(m_diskTable, &QTableWidget::cellClicked, this, [=](int row, int){
        QString detail = tr("盘符: %1\n总容量: %2\n已用空间: %3\n可用空间: %4\n使用率: %5")
            .arg(m_diskTable->item(row,0)->text())
            .arg(m_diskTable->item(row,1)->text())
            .arg(m_diskTable->item(row,2)->text())
            .arg(m_diskTable->item(row,3)->text())
            .arg(m_diskTable->item(row,4)->text());
        m_detailLabel->setText(detail);
        
        // 更新所选磁盘的进度条
        QString usageText = m_diskTable->item(row, 4)->text();
        usageText.chop(1); // 移除百分号
        double usagePercent = usageText.toDouble();
        updateDiskProgressBar(usagePercent);
    });
    
    // 选中行高亮和字体加粗
    connect(m_diskTable, &QTableWidget::itemSelectionChanged, this, [=](){
        for (int i = 0; i < m_diskTable->rowCount(); ++i) {
            bool selected = m_diskTable->selectionModel()->isRowSelected(i, QModelIndex());
            for (int j = 0; j < m_diskTable->columnCount(); ++j) {
                QTableWidgetItem *item = m_diskTable->item(i, j);
                if (item) {
                    if (selected) {
                        item->setBackground(QColor("#2196F3"));
                        QFont font = item->font();
                        font.setBold(true);
                        item->setFont(font);
                        item->setForeground(QColor("white"));
                    } else {
                        item->setBackground(Qt::white);
                        QFont font = item->font();
                        font.setBold(false);
                        item->setFont(font);
                        item->setForeground(QColor("#222222"));
                    }
                }
            }
        }
    });
}

QString DiskPage::formatSize(qint64 bytes)
{
    const qint64 kb = 1024;
    const qint64 mb = 1024 * kb;
    const qint64 gb = 1024 * mb;
    const qint64 tb = 1024 * gb;
    
    if (bytes >= tb) {
        return QString::number(bytes / static_cast<double>(tb), 'f', 2) + " TB";
    } else if (bytes >= gb) {
        return QString::number(bytes / static_cast<double>(gb), 'f', 2) + " GB";
    } else if (bytes >= mb) {
        return QString::number(bytes / static_cast<double>(mb), 'f', 2) + " MB";
    } else if (bytes >= kb) {
        return QString::number(bytes / static_cast<double>(kb), 'f', 2) + " KB";
    } else {
        return QString::number(bytes) + " B";
    }
}

void DiskPage::updateDiskData()
{
    updateDiskTable();
    updateDiskChart();
}

void DiskPage::updateDiskTable()
{
    DWORD drives = GetLogicalDrives();
    QList<QPair<QString, ULARGE_INTEGER>> diskInfo;
    
    for (char drive = 'A'; drive <= 'Z'; drive++) {
        if (drives & (1 << (drive - 'A'))) {
            QString drivePath = QString(drive) + ":\\";
            ULARGE_INTEGER totalBytes, freeBytes, totalFreeBytes;
            
            if (GetDiskFreeSpaceExW((LPCWSTR)drivePath.utf16(),
                                   &totalFreeBytes,
                                   &totalBytes,
                                   &freeBytes)) {
                diskInfo.append(qMakePair(drivePath, totalBytes));
            }
        }
    }
    
    m_diskTable->setRowCount(diskInfo.size());
    
    for (int i = 0; i < diskInfo.size(); i++) {
        QString drivePath = diskInfo[i].first;
        ULARGE_INTEGER totalBytes = diskInfo[i].second;
        ULARGE_INTEGER freeBytes;
        GetDiskFreeSpaceExW((LPCWSTR)drivePath.utf16(),
                           nullptr,
                           nullptr,
                           &freeBytes);
        
        quint64 usedBytes = totalBytes.QuadPart - freeBytes.QuadPart;
        double usagePercent = (static_cast<double>(usedBytes) / totalBytes.QuadPart) * 100;
        
        m_diskTable->setItem(i, 0, new QTableWidgetItem(drivePath));
        m_diskTable->setItem(i, 1, new QTableWidgetItem(formatSize(totalBytes.QuadPart)));
        m_diskTable->setItem(i, 2, new QTableWidgetItem(formatSize(usedBytes)));
        m_diskTable->setItem(i, 3, new QTableWidgetItem(formatSize(freeBytes.QuadPart)));
        m_diskTable->setItem(i, 4, new QTableWidgetItem(QString::number(usagePercent, 'f', 1) + "%"));
    }
    
    // 如果有选中的行，更新进度条
    QModelIndexList selectedRows = m_diskTable->selectionModel()->selectedRows();
    if (!selectedRows.isEmpty()) {
        int row = selectedRows.first().row();
        QString usageText = m_diskTable->item(row, 4)->text();
        usageText.chop(1); // 移除百分号
        double usagePercent = usageText.toDouble();
        updateDiskProgressBar(usagePercent);
    }
}

void DiskPage::updateDiskChart()
{
    // 计算所有磁盘的平均使用率
    double totalUsage = 0;
    int diskCount = 0;
    
    for (int i = 0; i < m_diskTable->rowCount(); i++) {
        QString usageText = m_diskTable->item(i, 4)->text();
        usageText.chop(1); // 移除百分号
        totalUsage += usageText.toDouble();
        diskCount++;
    }
    
    double averageUsage = diskCount > 0 ? totalUsage / diskCount : 0;
    
    // 更新图表数据
    m_diskUsage.removeFirst();
    m_diskUsage.append(averageUsage);
    m_chartWidget->updateValue(averageUsage);
}

void DiskPage::updateDiskProgressBar(double usagePercent)
{
    m_diskProgressBar->setValue(static_cast<int>(usagePercent));
    
    // 根据使用率设置不同的颜色
    QString progressStyle = 
        "QProgressBar {"
        "   border: 1px solid #e0e0e0;"
        "   border-radius: 5px;"
        "   background-color: #f5f5f5;"
        "   text-align: center;"
        "}"
        "QProgressBar::chunk {";
    
    if (usagePercent < 50) {
        // 低使用率 - 绿色
        progressStyle += "background-color: #4CAF50;";
    } else if (usagePercent < 85) {
        // 中等使用率 - 橙色
        progressStyle += "background-color: #FF9800;";
    } else {
        // 高使用率 - 红色
        progressStyle += "background-color: #F44336;";
    }
    
    progressStyle += "border-radius: 5px;}";
    m_diskProgressBar->setStyleSheet(progressStyle);
}
