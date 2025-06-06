#include "src/include/ui/processpage.h"
#include <QHeaderView>
#include <QMessageBox>
#include <QProcess>
#include <windows.h>
#include <psapi.h>
#include <tlhelp32.h>

ProcessPage::ProcessPage(QWidget *parent)
    : QWidget(parent)
    , m_processTable(new QTableWidget(this))
    , m_updateTimer(new QTimer(this))
    , m_searchEdit(new QLineEdit(this))
    , m_terminateButton(new QPushButton("结束进程", this))
    , m_refreshButton(new QPushButton("刷新", this))
    , m_totalProcessesLabel(new QLabel(this))
    , m_cpuUsageLabel(new QLabel(this))
    , m_memoryUsageLabel(new QLabel(this))
{
    setupUI();
    
    // 连接信号和槽
    connect(m_updateTimer, &QTimer::timeout, this, &ProcessPage::updateProcessList);
    connect(m_searchEdit, &QLineEdit::textChanged, this, &ProcessPage::filterProcesses);
    connect(m_terminateButton, &QPushButton::clicked, this, &ProcessPage::terminateSelectedProcess);
    connect(m_refreshButton, &QPushButton::clicked, this, &ProcessPage::refreshProcesses);
    
    m_updateTimer->start(2000); // 每2秒更新一次
    updateProcessList(); // 初始更新
}

ProcessPage::~ProcessPage()
{
}

void ProcessPage::setupUI()
{
    QVBoxLayout *mainLayout = new QVBoxLayout(this);
    
    // 创建搜索和控制面板
    QHBoxLayout *controlLayout = new QHBoxLayout();
    m_searchEdit->setPlaceholderText("搜索进程...");
    controlLayout->addWidget(m_searchEdit);
    controlLayout->addWidget(m_refreshButton);
    controlLayout->addWidget(m_terminateButton);
    
    // 创建状态面板
    QHBoxLayout *statsLayout = new QHBoxLayout();
    statsLayout->addWidget(m_totalProcessesLabel);
    statsLayout->addWidget(m_cpuUsageLabel);
    statsLayout->addWidget(m_memoryUsageLabel);
    
    // 设置进程表格
    m_processTable->setColumnCount(6); // 增加一列用于复选框
    m_processTable->setHorizontalHeaderLabels({"选择", "PID", "进程名", "CPU使用率", "内存使用", "状态"});
    m_processTable->horizontalHeader()->setSectionResizeMode(QHeaderView::Stretch);
    m_processTable->horizontalHeader()->setSectionResizeMode(0, QHeaderView::Fixed); // 固定复选框列宽
    m_processTable->setColumnWidth(0, 50); // 设置复选框列宽
    m_processTable->setSelectionBehavior(QAbstractItemView::SelectRows);
    m_processTable->setSelectionMode(QAbstractItemView::MultiSelection); // 支持多选
    m_processTable->setSortingEnabled(true);
    
    // 默认按CPU使用率降序排序
    m_processTable->sortByColumn(3, Qt::DescendingOrder);
    
    // 添加所有组件到主布局
    mainLayout->addLayout(controlLayout);
    mainLayout->addLayout(statsLayout);
    mainLayout->addWidget(m_processTable);
    
    // 设置终止按钮样式
    m_terminateButton->setStyleSheet(
        "QPushButton { background-color: #f44336; color: white; border: none; border-radius: 4px; "
        "padding: 8px 16px; font-weight: bold; } "
        "QPushButton:hover { background-color: #d32f2f; } "
        "QPushButton:pressed { background-color: #b71c1c; }"
    );
    m_terminateButton->setIcon(QIcon(":/icons/icons/process.png"));
    m_terminateButton->setText("结束选中进程");
}

QString ProcessPage::formatMemorySize(quint64 bytes)
{
    if (bytes >= 1024 * 1024 * 1024) {
        return QString::number(bytes / (1024.0 * 1024 * 1024), 'f', 2) + " GB";
    } else if (bytes >= 1024 * 1024) {
        return QString::number(bytes / (1024.0 * 1024), 'f', 2) + " MB";
    } else if (bytes >= 1024) {
        return QString::number(bytes / 1024.0, 'f', 2) + " KB";
    } else {
        return QString::number(bytes) + " B";
    }
}

void ProcessPage::updateSystemStats()
{
    PERFORMANCE_INFORMATION perfInfo;
    perfInfo.cb = sizeof(PERFORMANCE_INFORMATION);
    GetPerformanceInfo(&perfInfo, sizeof(PERFORMANCE_INFORMATION));
    
    // 更新进程总数
    m_totalProcessesLabel->setText(QString("进程总数: %1").arg(perfInfo.ProcessCount));
    
    // 更新内存使用情况
    MEMORYSTATUSEX memInfo;
    memInfo.dwLength = sizeof(MEMORYSTATUSEX);
    GlobalMemoryStatusEx(&memInfo);
    m_memoryUsageLabel->setText(QString("内存使用: %1%").arg(memInfo.dwMemoryLoad));
    
    // 更新CPU使用率
    FILETIME idleTime, kernelTime, userTime;
    GetSystemTimes(&idleTime, &kernelTime, &userTime);
    static FILETIME lastIdleTime = {0}, lastKernelTime = {0}, lastUserTime = {0};
    
    ULARGE_INTEGER idle, kernel, user, lastIdle, lastKernel, lastUser;
    idle.LowPart = idleTime.dwLowDateTime;
    idle.HighPart = idleTime.dwHighDateTime;
    kernel.LowPart = kernelTime.dwLowDateTime;
    kernel.HighPart = kernelTime.dwHighDateTime;
    user.LowPart = userTime.dwLowDateTime;
    user.HighPart = userTime.dwHighDateTime;
    
    lastIdle.LowPart = lastIdleTime.dwLowDateTime;
    lastIdle.HighPart = lastIdleTime.dwHighDateTime;
    lastKernel.LowPart = lastKernelTime.dwLowDateTime;
    lastKernel.HighPart = lastKernelTime.dwHighDateTime;
    lastUser.LowPart = lastUserTime.dwLowDateTime;
    lastUser.HighPart = lastUserTime.dwHighDateTime;
    
    ULONGLONG idleDiff = idle.QuadPart - lastIdle.QuadPart;
    ULONGLONG kernelDiff = kernel.QuadPart - lastKernel.QuadPart;
    ULONGLONG userDiff = user.QuadPart - lastUser.QuadPart;
    ULONGLONG totalDiff = kernelDiff + userDiff;
    ULONGLONG cpuUsage = totalDiff > 0 ? ((totalDiff - idleDiff) * 100) / totalDiff : 0;
    
    m_cpuUsageLabel->setText(QString("CPU使用: %1%").arg(cpuUsage));
    
    lastIdleTime = idleTime;
    lastKernelTime = kernelTime;
    lastUserTime = userTime;
}

void ProcessPage::updateProcessList()
{
    updateSystemStats();
    
    // 保存当前选中的复选框状态
    QMap<DWORD, bool> checkedPids;
    for (int row = 0; row < m_processTable->rowCount(); ++row) {
        QTableWidgetItem* checkItem = m_processTable->item(row, 0);
        QTableWidgetItem* pidItem = m_processTable->item(row, 1);
        if (checkItem && pidItem) {
            DWORD pid = pidItem->text().toULong();
            bool checked = checkItem->checkState() == Qt::Checked;
            checkedPids[pid] = checked;
        }
    }
    
    HANDLE snapshot = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
    if (snapshot == INVALID_HANDLE_VALUE) {
        return;
    }
    
    m_processTable->setSortingEnabled(false);
    m_processTable->setRowCount(0);
    
    PROCESSENTRY32W pe32;
    pe32.dwSize = sizeof(pe32);
    
    if (Process32FirstW(snapshot, &pe32)) {
        do {
            HANDLE hProcess = OpenProcess(PROCESS_QUERY_INFORMATION | PROCESS_VM_READ, FALSE, pe32.th32ProcessID);
            if (hProcess) {
                PROCESS_MEMORY_COUNTERS pmc;
                if (GetProcessMemoryInfo(hProcess, &pmc, sizeof(pmc))) {
                    int row = m_processTable->rowCount();
                    m_processTable->insertRow(row);
                    
                    // 复选框
                    QTableWidgetItem *checkItem = new QTableWidgetItem();
                    checkItem->setFlags(checkItem->flags() | Qt::ItemIsUserCheckable);
                    checkItem->setCheckState(checkedPids.contains(pe32.th32ProcessID) && checkedPids.value(pe32.th32ProcessID) ? Qt::Checked : Qt::Unchecked);
                    m_processTable->setItem(row, 0, checkItem);
                    
                    // PID
                    QTableWidgetItem *pidItem = new QTableWidgetItem(QString::number(pe32.th32ProcessID));
                    pidItem->setFlags(pidItem->flags() & ~Qt::ItemIsEditable);
                    m_processTable->setItem(row, 1, pidItem);
                    
                    // 进程名
                    QString processName = QString::fromWCharArray(pe32.szExeFile);
                    QTableWidgetItem *nameItem = new QTableWidgetItem(processName);
                    nameItem->setFlags(nameItem->flags() & ~Qt::ItemIsEditable);
                    m_processTable->setItem(row, 2, nameItem);
                    
                    // CPU使用率（简化计算）
                    FILETIME creation, exit, kernel, user;
                    double cpuTime = 0.0;
                    if (GetProcessTimes(hProcess, &creation, &exit, &kernel, &user)) {
                        ULARGE_INTEGER kernelTime, userTime;
                        kernelTime.LowPart = kernel.dwLowDateTime;
                        kernelTime.HighPart = kernel.dwHighDateTime;
                        userTime.LowPart = user.dwLowDateTime;
                        userTime.HighPart = user.dwHighDateTime;
                        
                        cpuTime = (kernelTime.QuadPart + userTime.QuadPart) / 10000000.0;
                        QTableWidgetItem *cpuItem = new QTableWidgetItem(QString::number(cpuTime, 'f', 1) + "%");
                        cpuItem->setFlags(cpuItem->flags() & ~Qt::ItemIsEditable);
                        cpuItem->setData(Qt::UserRole, cpuTime); // 存储原始值用于排序
                        m_processTable->setItem(row, 3, cpuItem);
                    } else {
                        QTableWidgetItem *cpuItem = new QTableWidgetItem("0.0%");
                        cpuItem->setFlags(cpuItem->flags() & ~Qt::ItemIsEditable);
                        m_processTable->setItem(row, 3, cpuItem);
                    }
                    
                    // 内存使用
                    QTableWidgetItem *memItem = new QTableWidgetItem(formatMemorySize(pmc.WorkingSetSize));
                    memItem->setFlags(memItem->flags() & ~Qt::ItemIsEditable);
                    memItem->setData(Qt::UserRole, (qulonglong)pmc.WorkingSetSize); // 存储原始值用于排序
                    m_processTable->setItem(row, 4, memItem);
                    
                    // 状态
                    QTableWidgetItem *statusItem = new QTableWidgetItem("运行中");
                    statusItem->setFlags(statusItem->flags() & ~Qt::ItemIsEditable);
                    m_processTable->setItem(row, 5, statusItem);
                    
                    // 高亮显示高CPU使用率的进程
                    if (cpuTime > 5.0) {
                        for (int col = 0; col < m_processTable->columnCount(); ++col) {
                            if (m_processTable->item(row, col)) {
                                m_processTable->item(row, col)->setBackground(QColor(255, 235, 235));
                            }
                        }
                    }
                }
                CloseHandle(hProcess);
            }
        } while (Process32NextW(snapshot, &pe32));
    }
    
    CloseHandle(snapshot);
    m_processTable->setSortingEnabled(true);
}

void ProcessPage::filterProcesses(const QString &filter)
{
    for (int i = 0; i < m_processTable->rowCount(); ++i) {
        bool match = false;
        for (int j = 0; j < m_processTable->columnCount(); ++j) {
            QTableWidgetItem *item = m_processTable->item(i, j);
            if (item && item->text().contains(filter, Qt::CaseInsensitive)) {
                match = true;
                break;
            }
        }
        m_processTable->setRowHidden(i, !match);
    }
}

void ProcessPage::terminateSelectedProcess()
{
    QList<DWORD> selectedPids;
    QStringList selectedNames;
    
    // 收集所有被选中的进程
    for (int row = 0; row < m_processTable->rowCount(); ++row) {
        QTableWidgetItem *checkItem = m_processTable->item(row, 0);
        if (checkItem && checkItem->checkState() == Qt::Checked) {
            QTableWidgetItem *pidItem = m_processTable->item(row, 1);
            QTableWidgetItem *nameItem = m_processTable->item(row, 2);
            
            if (pidItem && nameItem) {
                selectedPids.append(pidItem->text().toULong());
                selectedNames.append(nameItem->text());
            }
        }
    }
    
    if (selectedPids.isEmpty()) {
        QMessageBox::warning(this, tr("警告"), tr("请先选择至少一个进程"));
        return;
    }
    
    // 构建确认消息
    QString confirmMessage;
    if (selectedPids.size() == 1) {
        confirmMessage = tr("确定要结束进程 %1 (PID: %2) 吗?").arg(selectedNames[0]).arg(selectedPids[0]);
    } else {
        confirmMessage = tr("确定要结束以下 %1 个进程吗?\n").arg(selectedPids.size());
        for (int i = 0; i < qMin(5, selectedPids.size()); ++i) {
            confirmMessage += tr("%1 (PID: %2)\n").arg(selectedNames[i]).arg(selectedPids[i]);
        }
        if (selectedPids.size() > 5) {
            confirmMessage += tr("...以及其他 %1 个进程").arg(selectedPids.size() - 5);
        }
    }
    
    int result = QMessageBox::question(this, tr("确认"), confirmMessage,
                                      QMessageBox::Yes | QMessageBox::No);
    
    if (result == QMessageBox::Yes) {
        int successCount = 0;
        QStringList failedProcesses;
        
        for (int i = 0; i < selectedPids.size(); ++i) {
            DWORD pid = selectedPids[i];
            
            // 在终止进程前发射信号
            emit processTerminationRequested(pid);
            
            HANDLE hProcess = OpenProcess(PROCESS_TERMINATE, FALSE, pid);
            if (hProcess) {
                if (TerminateProcess(hProcess, 0)) {
                    successCount++;
                } else {
                    failedProcesses.append(tr("%1 (PID: %2, 错误: %3)").arg(selectedNames[i]).arg(pid).arg(GetLastError()));
                }
                CloseHandle(hProcess);
            } else {
                failedProcesses.append(tr("%1 (PID: %2, 错误: 无法打开进程)").arg(selectedNames[i]).arg(pid));
            }
        }
        
        // 显示结果
        if (failedProcesses.isEmpty()) {
            QMessageBox::information(this, tr("成功"), tr("已成功终止 %1 个进程").arg(successCount));
        } else {
            QString message = tr("已成功终止 %1 个进程，但以下进程终止失败:\n").arg(successCount);
            for (const QString &failedProcess : failedProcesses) {
                message += failedProcess + "\n";
            }
            QMessageBox::warning(this, tr("部分失败"), message);
        }
        
        refreshProcesses();
    }
}

void ProcessPage::refreshProcesses()
{
    updateProcessList();
}

QList<ProcessPage::ProcessData> ProcessPage::getCurrentProcessData() const
{
    QList<ProcessData> result;
    
    for (int row = 0; row < m_processTable->rowCount(); ++row) {
        if (!m_processTable->isRowHidden(row)) {
            ProcessData data;
            
            // 获取PID
            QTableWidgetItem *pidItem = m_processTable->item(row, 1);
            if (pidItem) {
                data.pid = pidItem->text().toULongLong();
            }
            
            // 获取进程名
            QTableWidgetItem *nameItem = m_processTable->item(row, 2);
            if (nameItem) {
                data.name = nameItem->text();
            }
            
            // 获取CPU使用率
            QTableWidgetItem *cpuItem = m_processTable->item(row, 3);
            if (cpuItem) {
                data.cpuUsage = cpuItem->data(Qt::UserRole).toDouble();
            }
            
            // 获取内存使用量
            QTableWidgetItem *memItem = m_processTable->item(row, 4);
            if (memItem) {
                data.memoryUsage = memItem->data(Qt::UserRole).toULongLong();
            }
            
            result.append(data);
        }
    }
    
    return result;
}
