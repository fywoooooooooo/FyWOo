void Sampler::checkGpuAvailability() {
    m_gpuAvailable = false;
    m_gpuName = "�?";
    m_gpuDriverVersion = "�?";
    
    try {
        // ���?��GPU
        #ifdef Q_OS_WIN
            // Windows??GPU����?�
            QProcess process;
            process.start("wmic path win32_VideoController get name, driverversion");
            if (process.waitForFinished(3000)) {
                QString output = process.readAllStandardOutput();
                if (!output.isEmpty() && !output.contains("No Instance(s) Available")) {
                    QStringList lines = output.split("\n", QString::SkipEmptyParts);
                    if (lines.size() > 1) {  // ��?���?���
                        QString gpuInfo = lines[1].trimmed();
                        if (!gpuInfo.isEmpty()) {
                            m_gpuAvailable = true;
                            if (gpuInfo.contains(" ")) {
                                m_gpuName = gpuInfo.left(gpuInfo.lastIndexOf(" ")).trimmed();
                                m_gpuDriverVersion = gpuInfo.mid(gpuInfo.lastIndexOf(" ")).trimmed();
                            } else {
                                m_gpuName = gpuInfo;
                            }
                        }
                    }
                }
            }
        #elif defined(Q_OS_LINUX)
            // Linux??GPU����?�
            QProcess process;
            process.start("lspci | grep -i vga");
            if (process.waitForFinished(3000)) {
                QString output = process.readAllStandardOutput();
                if (!output.isEmpty()) {
                    m_gpuAvailable = true;
                    int nameStart = output.indexOf(": ") + 2;
                    if (nameStart > 1) {
                        m_gpuName = output.mid(nameStart).trimmed();
                    }
                    
                    // ���?�?�����?
                    process.start("glxinfo | grep \"OpenGL version\"");
                    if (process.waitForFinished(3000)) {
                        output = process.readAllStandardOutput();
                        if (!output.isEmpty()) {
                            int versionStart = output.indexOf("OpenGL version string:") + 23;
                            if (versionStart > 22) {
                                m_gpuDriverVersion = output.mid(versionStart).trimmed();
                            }
                        }
                    }
                }
            }
        #elif defined(Q_OS_MAC)
            // Mac??GPU����?�
            QProcess process;
            process.start("system_profiler SPDisplaysDataType");
            if (process.waitForFinished(3000)) {
                QString output = process.readAllStandardOutput();
                if (!output.isEmpty() && output.contains("Chipset Model:")) {
                    m_gpuAvailable = true;
                    int nameStart = output.indexOf("Chipset Model:") + 15;
                    int nameEnd = output.indexOf("\n", nameStart);
                    if (nameStart > 14 && nameEnd > nameStart) {
                        m_gpuName = output.mid(nameStart, nameEnd - nameStart).trimmed();
                    }
                }
            }
        #endif
        
        // �����?GPU�������?�
        if (m_gpuAvailable) {
            emit gpuAvailabilityChanged(true, m_gpuName, m_gpuDriverVersion);
        } else {
            // ?��GPU���?��
            emit gpuAvailabilityChanged(false, "�?", "�?");
            
            // ��?GPU��?��?���
            QMetaObject::invokeMethod(this, "showGpuNotFoundDialog", Qt::QueuedConnection);
        }
    } catch (const std::exception& e) {
        emit gpuAvailabilityChanged(false, "����?", "�?");
        
        // ��?GPU��?��?���
        QMetaObject::invokeMethod(this, "showGpuNotFoundDialog", Qt::QueuedConnection);
    }
}

// ���?����public slots����
public slots:
    void showGpuNotFoundDialog() {
        QMessageBox msgBox;
        msgBox.setWindowTitle("GPU���");
        msgBox.setText("���?���?�GPU�?");
        msgBox.setInformativeText("??�?�?��GPU�?��GPU��?��?������?�\n\n���?�?��\n- ??���?GPU\n- GPU�����������?��?\n- ??��?�?�GPU�?�");
        
        // ���?���?��
        msgBox.setIcon(QMessageBox::Warning);
        
        // ������?��?
        msgBox.setDetailedText("??���������CPU���??���?��������?������?��??����GPU���?�?��������������������?�?���");
        
        // ���?�?
        msgBox.addButton("����", QMessageBox::AcceptRole);
        
        // ��?�?���
        msgBox.exec();
    }