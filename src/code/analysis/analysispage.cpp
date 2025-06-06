void AnalysisPage::handleBottleneckDetected(PerformanceAnalyzer::BottleneckType type, const QString& details) {
    QString typeStr;
    QString iconPath;
    
    qDebug() << "�쳣���: ��⵽ƿ������ " << static_cast<int>(type);
    
    switch (type) {
        case PerformanceAnalyzer::BottleneckType::None:
            typeStr = "��ƿ��";
            iconPath = ":/images/icon_ok.png";
            break;
        case PerformanceAnalyzer::BottleneckType::CPU:
            typeStr = "CPUƿ��";
            iconPath = ":/images/icon_cpu_warning.png";
            break;
        case PerformanceAnalyzer::BottleneckType::Memory:
            typeStr = "�ڴ�ƿ��";
            iconPath = ":/images/icon_memory_warning.png";
            break;
        case PerformanceAnalyzer::BottleneckType::Disk:
            typeStr = "����I/Oƿ��";
            iconPath = ":/images/icon_disk_warning.png";
            break;
        case PerformanceAnalyzer::BottleneckType::Network:
            typeStr = "����ƿ��";
            iconPath = ":/images/icon_network_warning.png";
            break;
        case PerformanceAnalyzer::BottleneckType::Multiple:
            typeStr = "����ƿ��";
            iconPath = ":/images/icon_warning.png";
            break;
        default:
            typeStr = "δ֪ƿ��";
            iconPath = ":/images/icon_warning.png";
            break;
    }
    
    m_bottleneckTypeLabel->setText(typeStr);
    m_bottleneckDetailsTextEdit->setText(details);
    
    // ���¾���ͼ��
    QPixmap pixmap(iconPath);
    if (pixmap.isNull()) {
        qWarning() << "����: �޷�����ͼ�� " << iconPath;
        pixmap = QPixmap(":/images/icon_warning.png"); // ����ͼ��
    }
    m_bottleneckIconLabel->setPixmap(pixmap.scaled(32, 32, Qt::KeepAspectRatio, Qt::SmoothTransformation));
    
    // ���������ƿ������˸��ʾ
    if (type != PerformanceAnalyzer::BottleneckType::None) {
        startWarningAnimation();
    } else {
        stopWarningAnimation();
    }
} 