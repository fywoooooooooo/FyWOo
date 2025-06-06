#include "src/include/ui/gpupage.h"
#include <QPainter>
#include <QDebug>

GpuPage::GpuPage(QWidget *parent) :
    QWidget(parent),
    m_gpuAvailable(false)
{
    setupUi();
    
    // 初始化GPU状态为不可用
    gpuInfoLabel->setText("GPU状态：未检测到GPU");
    gpuDriverLabel->setText("驱动版本：N/A");
    gpuStatusFrame->setStyleSheet("QFrame { background-color: #ffeeee; border-radius: 8px; border: 1px solid #ffcccc; }");
    gpuUsageBar->setValue(0);
    gpuTempBar->setValue(0);
    gpuMemoryBar->setValue(0);
    
    // 隐藏图表
    gpuChartFrame->setVisible(false);
    
    // 创建GPU使用率图表
    m_chart = new ChartWidget("GPU使用率", this);
    m_chart->setTitle("GPU使用率");
    m_chart->setYRange(0, 100);
    gpuChartLayout->addWidget(m_chart);
    
    // 重新组织UI布局，将图表放在上方，详细信息放在下方
    QVBoxLayout* layout = qobject_cast<QVBoxLayout*>(this->layout());
    if (layout) {
        // 移除所有部件
        while (layout->count() > 0) {
            QLayoutItem* item = layout->takeAt(0);
            if (item->widget()) {
                layout->removeWidget(item->widget());
            }
            // 不要删除部件，只是从布局中移除
        }
        
        // 按照新的顺序重新添加部件
        layout->addWidget(gpuChartFrame);  // 图表放在最上面
        layout->addWidget(gpuUsageFrame);  // 使用率
        layout->addWidget(gpuTempFrame);   // 温度
        layout->addWidget(gpuMemoryFrame); // 内存
        layout->addWidget(gpuStatusFrame); // 状态信息放在最下面
    }
}

void GpuPage::setupUi()
{
    // 创建主布局
    QVBoxLayout *verticalLayout = new QVBoxLayout(this);
    verticalLayout->setSpacing(16);
    verticalLayout->setContentsMargins(16, 16, 16, 16);
    
    // 创建GPU状态框
    gpuStatusFrame = new QFrame(this);
    gpuStatusFrame->setFrameShape(QFrame::StyledPanel);
    gpuStatusFrame->setFrameShadow(QFrame::Raised);
    gpuStatusFrame->setStyleSheet("QFrame { background-color: #f8f8f8; border-radius: 8px; border: 1px solid #e0e0e0; }");
    
    QVBoxLayout *statusLayout = new QVBoxLayout(gpuStatusFrame);
    
    // 创建GPU信息标签
    gpuInfoLabel = new QLabel(gpuStatusFrame);
    QFont infoFont;
    infoFont.setPointSize(12);
    infoFont.setBold(true);
    gpuInfoLabel->setFont(infoFont);
    gpuInfoLabel->setText("GPU状态: 未检测到GPU");
    gpuInfoLabel->setStyleSheet("QLabel { color: #333333; font-size: 14pt; font-weight: bold; }");
    statusLayout->addWidget(gpuInfoLabel);
    
    // 创建驱动版本标签
    gpuDriverLabel = new QLabel(gpuStatusFrame);
    gpuDriverLabel->setText("驱动版本: N/A");
    gpuDriverLabel->setStyleSheet("QLabel { color: #333333; font-size: 11pt; }");
    statusLayout->addWidget(gpuDriverLabel);
    
    // 添加状态框到主布局
    verticalLayout->addWidget(gpuStatusFrame);
    
    // 创建进度条基础样式
    QString progressBarStyle = 
        "QProgressBar {"
        "   border: 1px solid #e0e0e0;"
        "   border-radius: 5px;"
        "   background-color: #f5f5f5;"
        "   text-align: center;"
        "}"
        "QProgressBar::chunk {"
        "   background-color: #4CAF50;"
        "   border-radius: 5px;"
        "}";
    
    // 创建框架基础样式
    QString frameStyle = "QFrame { background-color: white; border-radius: 8px; border: 1px solid #e0e0e0; }";
    
    // 创建GPU使用率框
    gpuUsageFrame = new QFrame(this);
    gpuUsageFrame->setFrameShape(QFrame::StyledPanel);
    gpuUsageFrame->setFrameShadow(QFrame::Raised);
    gpuUsageFrame->setStyleSheet(frameStyle);
    
    QHBoxLayout *usageLayout = new QHBoxLayout(gpuUsageFrame);
    
    // 创建GPU使用率文本标签
    gpuUsageTextLabel = new QLabel(gpuUsageFrame);
    gpuUsageTextLabel->setMinimumSize(QSize(120, 0));
    gpuUsageTextLabel->setText("GPU使用率:");
    usageLayout->addWidget(gpuUsageTextLabel);
    
    // 创建GPU使用率进度条
    gpuUsageBar = new QProgressBar(gpuUsageFrame);
    gpuUsageBar->setValue(0);
    gpuUsageBar->setStyleSheet(progressBarStyle);
    usageLayout->addWidget(gpuUsageBar);
    
    // 创建GPU使用率数值标签
    gpuUsageLabel = new QLabel(gpuUsageFrame);
    gpuUsageLabel->setMinimumSize(QSize(60, 0));
    gpuUsageLabel->setText("0%");
    gpuUsageLabel->setAlignment(Qt::AlignRight|Qt::AlignTrailing|Qt::AlignVCenter);
    usageLayout->addWidget(gpuUsageLabel);
    
    // 添加使用率框到主布局
    verticalLayout->addWidget(gpuUsageFrame);
    
    // 创建GPU温度框
    gpuTempFrame = new QFrame(this);
    gpuTempFrame->setFrameShape(QFrame::StyledPanel);
    gpuTempFrame->setFrameShadow(QFrame::Raised);
    gpuTempFrame->setStyleSheet(frameStyle);
    
    QHBoxLayout *tempLayout = new QHBoxLayout(gpuTempFrame);
    
    // 创建GPU温度文本标签
    gpuTempTextLabel = new QLabel(gpuTempFrame);
    gpuTempTextLabel->setMinimumSize(QSize(120, 0));
    gpuTempTextLabel->setText("GPU温度:");
    tempLayout->addWidget(gpuTempTextLabel);
    
    // 创建GPU温度进度条
    gpuTempBar = new QProgressBar(gpuTempFrame);
    gpuTempBar->setValue(0);
    gpuTempBar->setStyleSheet(progressBarStyle.replace("#4CAF50", "#FF5722")); // 温度使用橙色
    tempLayout->addWidget(gpuTempBar);
    
    // 创建GPU温度数值标签
    gpuTempLabel = new QLabel(gpuTempFrame);
    gpuTempLabel->setMinimumSize(QSize(60, 0));
    gpuTempLabel->setText("0°C");
    gpuTempLabel->setAlignment(Qt::AlignRight|Qt::AlignTrailing|Qt::AlignVCenter);
    tempLayout->addWidget(gpuTempLabel);
    
    // 添加温度框到主布局
    verticalLayout->addWidget(gpuTempFrame);
    
    // 创建GPU内存框
    gpuMemoryFrame = new QFrame(this);
    gpuMemoryFrame->setFrameShape(QFrame::StyledPanel);
    gpuMemoryFrame->setFrameShadow(QFrame::Raised);
    gpuMemoryFrame->setStyleSheet(frameStyle);
    
    QHBoxLayout *memoryLayout = new QHBoxLayout(gpuMemoryFrame);
    
    // 创建GPU内存文本标签
    gpuMemoryTextLabel = new QLabel(gpuMemoryFrame);
    gpuMemoryTextLabel->setMinimumSize(QSize(120, 0));
    gpuMemoryTextLabel->setText("GPU内存:");
    memoryLayout->addWidget(gpuMemoryTextLabel);
    
    // 创建GPU内存进度条
    gpuMemoryBar = new QProgressBar(gpuMemoryFrame);
    gpuMemoryBar->setValue(0);
    gpuMemoryBar->setStyleSheet(progressBarStyle.replace("#4CAF50", "#2196F3")); // 内存使用蓝色
    memoryLayout->addWidget(gpuMemoryBar);
    
    // 创建GPU内存数值标签
    gpuMemoryLabel = new QLabel(gpuMemoryFrame);
    gpuMemoryLabel->setMinimumSize(QSize(120, 0));
    gpuMemoryLabel->setText("0 / 0");
    gpuMemoryLabel->setAlignment(Qt::AlignRight|Qt::AlignTrailing|Qt::AlignVCenter);
    memoryLayout->addWidget(gpuMemoryLabel);
    
    // 添加内存框到主布局
    verticalLayout->addWidget(gpuMemoryFrame);
    
    // 创建GPU图表框
    gpuChartFrame = new QFrame(this);
    gpuChartFrame->setFrameShape(QFrame::StyledPanel);
    gpuChartFrame->setFrameShadow(QFrame::Raised);
    gpuChartFrame->setStyleSheet(frameStyle);
    QSizePolicy sizePolicy(QSizePolicy::Preferred, QSizePolicy::Expanding);
    gpuChartFrame->setSizePolicy(sizePolicy);
    
    // 创建图表布局
    gpuChartLayout = new QVBoxLayout(gpuChartFrame);
    
    // 添加图表框到主布局
    verticalLayout->addWidget(gpuChartFrame);
    
    // 设置窗口标题
    setWindowTitle("GPU监视器");
}

GpuPage::~GpuPage()
{
    // 不需要删除ui，因为我们现在手动创建了所有UI组件，
    // 它们会随着父组件的销毁而销毁
}

void GpuPage::updateGpuStats(double usage, double temperature, quint64 memoryUsed, quint64 memoryTotal)
{
    if (!m_gpuAvailable) {
        // 如果GPU之前不可用，但现在有数据，将可用性设为true
        m_gpuAvailable = true;
        gpuStatusFrame->setStyleSheet("QFrame { background-color: #f8f8f8; border-radius: 8px; border: 1px solid #e0e0e0; }");
        gpuChartFrame->setVisible(true);
    }
    
    // 更新GPU使用率，根据使用率改变进度条颜色
    gpuUsageBar->setValue(static_cast<int>(usage));
    gpuUsageLabel->setText(QString::number(usage, 'f', 1) + "%");
    
    // 根据GPU使用率设置不同的颜色
    QString usageStyle = 
        "QProgressBar {"
        "   border: 1px solid #e0e0e0;"
        "   border-radius: 5px;"
        "   background-color: #f5f5f5;"
        "   text-align: center;"
        "}"
        "QProgressBar::chunk {";
    
    if (usage < 30) {
        // 低负载 - 绿色
        usageStyle += "background-color: #4CAF50;";
    } else if (usage < 70) {
        // 中等负载 - 黄色
        usageStyle += "background-color: #FFC107;";
    } else {
        // 高负载 - 红色
        usageStyle += "background-color: #F44336;";
    }
    
    usageStyle += "border-radius: 5px;}";
    gpuUsageBar->setStyleSheet(usageStyle);
    
    // 更新温度（如果支持）
    if (temperature > 0) {
        gpuTempBar->setValue(static_cast<int>(temperature));
        gpuTempLabel->setText(QString::number(temperature, 'f', 1) + "°C");
        gpuTempFrame->setVisible(true);
        
        // 根据温度设置不同的颜色
        QString tempStyle = 
            "QProgressBar {"
            "   border: 1px solid #e0e0e0;"
            "   border-radius: 5px;"
            "   background-color: #f5f5f5;"
            "   text-align: center;"
            "}"
            "QProgressBar::chunk {";
        
        if (temperature < 50) {
            // 低温 - 绿色
            tempStyle += "background-color: #4CAF50;";
        } else if (temperature < 75) {
            // 中温 - 橙色
            tempStyle += "background-color: #FF9800;";
        } else {
            // 高温 - 红色
            tempStyle += "background-color: #F44336;";
        }
        
        tempStyle += "border-radius: 5px;}";
        gpuTempBar->setStyleSheet(tempStyle);
    } else {
        gpuTempFrame->setVisible(false);
    }
    
    // 更新内存使用情况
    if (memoryTotal > 0) {
        double memoryPercent = static_cast<double>(memoryUsed) / static_cast<double>(memoryTotal) * 100.0;
        gpuMemoryBar->setValue(static_cast<int>(memoryPercent));
        
        // 格式化内存显示，根据大小选择合适的单位
        QString usedStr = formatMemorySize(memoryUsed);
        QString totalStr = formatMemorySize(memoryTotal);
        gpuMemoryLabel->setText(usedStr + " / " + totalStr);
        gpuMemoryFrame->setVisible(true);
        
        // 根据内存使用率设置不同的颜色
        QString memStyle = 
            "QProgressBar {"
            "   border: 1px solid #e0e0e0;"
            "   border-radius: 5px;"
            "   background-color: #f5f5f5;"
            "   text-align: center;"
            "}"
            "QProgressBar::chunk {";
        
        if (memoryPercent < 50) {
            // 低使用率 - 蓝色
            memStyle += "background-color: #2196F3;";
        } else if (memoryPercent < 80) {
            // 中等使用率 - 紫色
            memStyle += "background-color: #9C27B0;";
        } else {
            // 高使用率 - 红色
            memStyle += "background-color: #F44336;";
        }
        
        memStyle += "border-radius: 5px;}";
        gpuMemoryBar->setStyleSheet(memStyle);
    } else {
        gpuMemoryFrame->setVisible(false);
    }
    
    // 更新图表
    m_chart->updateValue(usage);
}

void GpuPage::handleGpuAvailabilityChange(bool available, const QString &gpuName, const QString &driverVersion)
{
    m_gpuAvailable = available;
    
    if (available) {
        gpuInfoLabel->setText("GPU状态：" + gpuName);
        gpuDriverLabel->setText("驱动版本：" + driverVersion);
        gpuStatusFrame->setStyleSheet("QFrame { background-color: #f1f8e9; border-radius: 8px; border: 1px solid #c5e1a5; }");
        gpuChartFrame->setVisible(true);
    } else {
        gpuInfoLabel->setText("GPU状态：未检测到GPU");
        gpuDriverLabel->setText("驱动版本：N/A");
        gpuStatusFrame->setStyleSheet("QFrame { background-color: #ffeeee; border-radius: 8px; border: 1px solid #ffcccc; }");
        gpuChartFrame->setVisible(false);
        
        // 重置所有显示
        gpuUsageBar->setValue(0);
        gpuUsageLabel->setText("0%");
        gpuTempBar->setValue(0);
        gpuTempLabel->setText("0°C");
        gpuMemoryBar->setValue(0);
        gpuMemoryLabel->setText("0 / 0");
    }
}

QString GpuPage::formatMemorySize(quint64 bytes)
{
    constexpr quint64 KB = 1024;
    constexpr quint64 MB = 1024 * KB;
    constexpr quint64 GB = 1024 * MB;
    
    if (bytes >= GB) {
        return QString::number(bytes / static_cast<double>(GB), 'f', 2) + " GB";
    } else if (bytes >= MB) {
        return QString::number(bytes / static_cast<double>(MB), 'f', 2) + " MB";
    } else if (bytes >= KB) {
        return QString::number(bytes / static_cast<double>(KB), 'f', 2) + " KB";
    } else {
        return QString::number(bytes) + " B";
    }
} 