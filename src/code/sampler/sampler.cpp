void Sampler::checkGpuAvailability() {
    m_gpuAvailable = false;
    m_gpuName = "ä?";
    m_gpuDriverVersion = "ä?";
    
    try {
        // ÿÿÿ?ÿÿGPU
        #ifdef Q_OS_WIN
            // Windows??GPUÿÿÿÿ?ÿ
            QProcess process;
            process.start("wmic path win32_VideoController get name, driverversion");
            if (process.waitForFinished(3000)) {
                QString output = process.readAllStandardOutput();
                if (!output.isEmpty() && !output.contains("No Instance(s) Available")) {
                    QStringList lines = output.split("\n", QString::SkipEmptyParts);
                    if (lines.size() > 1) {  // ÿÿ?ÿÿÿ?ÿÿÿ
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
            // Linux??GPUÿÿÿÿ?ÿ
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
                    
                    // ÿÿÿ?ÿ?ÿÿÿÿÿ?
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
            // Mac??GPUÿÿÿÿ?ÿ
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
        
        // ÿÿÿÿÿ?GPUÿÿÿÿÿÿÿ?ÿ
        if (m_gpuAvailable) {
            emit gpuAvailabilityChanged(true, m_gpuName, m_gpuDriverVersion);
        } else {
            // ?ÿÿGPUÿÿÿ?ÿÿ
            emit gpuAvailabilityChanged(false, "ä?", "ä?");
            
            // ÿÿ?GPUäÿ?ÿÿ?ÿÿÿ
            QMetaObject::invokeMethod(this, "showGpuNotFoundDialog", Qt::QueuedConnection);
        }
    } catch (const std::exception& e) {
        emit gpuAvailabilityChanged(false, "ÿÿÿÿ?", "ä?");
        
        // ÿÿ?GPUäÿ?ÿÿ?ÿÿÿ
        QMetaObject::invokeMethod(this, "showGpuNotFoundDialog", Qt::QueuedConnection);
    }
}

// ÿÿÿ?ÿÿÿÿpublic slotsÿÿÿÿ
public slots:
    void showGpuNotFoundDialog() {
        QMessageBox msgBox;
        msgBox.setWindowTitle("GPUÿÿÿ");
        msgBox.setText("äÿÿ?ÿÿÿ?ÿGPUÿ?");
        msgBox.setInformativeText("??ÿ?ÿ?ÿÿGPUÿ?ÿÿGPUÿÿ?ÿÿ?ÿÿÿÿÿÿ?ÿ\n\nÿÿÿ?ÿ?ÿÿ\n- ??äÿÿ?GPU\n- GPUÿÿÿÿÿÿÿÿäÿÿ?ÿÿ?\n- ??ÿÿ?ÿ?ÿGPUÿ?ÿ");
        
        // ÿÿÿ?ÿÿÿ?ÿÿ
        msgBox.setIcon(QMessageBox::Warning);
        
        // ÿÿÿÿÿÿ?ÿÿ?
        msgBox.setDetailedText("??ÿÿÿÿÿÿÿÿÿCPUÿÿÿ??ÿÿÿ?ÿÿÿÿÿÿÿÿ?ÿÿÿÿÿÿ?ÿÿ??ÿÿÿÿGPUÿÿÿ?ÿ?ÿÿÿÿÿÿÿÿÿÿÿÿÿÿÿÿÿÿÿÿ?ÿ?ÿÿÿ");
        
        // ÿÿÿ?ÿ?
        msgBox.addButton("ÿÿÿÿ", QMessageBox::AcceptRole);
        
        // ÿÿ?ÿ?ÿÿÿ
        msgBox.exec();
    }