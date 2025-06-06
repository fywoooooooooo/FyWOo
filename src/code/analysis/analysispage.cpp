void AnalysisPage::handleBottleneckDetected(PerformanceAnalyzer::BottleneckType type, const QString& details) {
    QString typeStr;
    QString iconPath;
    
    qDebug() << "异常检测: 检测到瓶颈类型 " << static_cast<int>(type);
    
    switch (type) {
        case PerformanceAnalyzer::BottleneckType::None:
            typeStr = "无瓶颈";
            iconPath = ":/images/icon_ok.png";
            break;
        case PerformanceAnalyzer::BottleneckType::CPU:
            typeStr = "CPU瓶颈";
            iconPath = ":/images/icon_cpu_warning.png";
            break;
        case PerformanceAnalyzer::BottleneckType::Memory:
            typeStr = "内存瓶颈";
            iconPath = ":/images/icon_memory_warning.png";
            break;
        case PerformanceAnalyzer::BottleneckType::Disk:
            typeStr = "磁盘I/O瓶颈";
            iconPath = ":/images/icon_disk_warning.png";
            break;
        case PerformanceAnalyzer::BottleneckType::Network:
            typeStr = "网络瓶颈";
            iconPath = ":/images/icon_network_warning.png";
            break;
        case PerformanceAnalyzer::BottleneckType::Multiple:
            typeStr = "多重瓶颈";
            iconPath = ":/images/icon_warning.png";
            break;
        default:
            typeStr = "未知瓶颈";
            iconPath = ":/images/icon_warning.png";
            break;
    }
    
    m_bottleneckTypeLabel->setText(typeStr);
    m_bottleneckDetailsTextEdit->setText(details);
    
    // 更新警告图标
    QPixmap pixmap(iconPath);
    if (pixmap.isNull()) {
        qWarning() << "警告: 无法加载图标 " << iconPath;
        pixmap = QPixmap(":/images/icon_warning.png"); // 备用图标
    }
    m_bottleneckIconLabel->setPixmap(pixmap.scaled(32, 32, Qt::KeepAspectRatio, Qt::SmoothTransformation));
    
    // 如果是严重瓶颈，闪烁提示
    if (type != PerformanceAnalyzer::BottleneckType::None) {
        startWarningAnimation();
    } else {
        stopWarningAnimation();
    }
} 