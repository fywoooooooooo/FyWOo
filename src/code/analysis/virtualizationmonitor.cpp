// virtualizationmonitor.cpp
#include "src/include/analysis/virtualizationmonitor.h"
#include <QDebug>
#include <QFile>
#include <QDir>
#include <QRegularExpression>
#include <QJsonDocument>
#include <QJsonObject>

VirtualizationMonitor::VirtualizationMonitor(QObject *parent)
    : QObject(parent), m_detectedType(None) {
    // 在构造时执行一次检测并缓存结果
    detectVirtualizationType();
}

VirtualizationMonitor::~VirtualizationMonitor() {
    // 清理资源（如果需要）
}

// 检测当前环境的虚拟化类型
VirtualizationMonitor::VirtualizationType VirtualizationMonitor::detectVirtualizationType() const {
    // 如果已经检测过并且不是None或Other（Other可能需要重新评估），直接返回缓存值
    // 注意：m_detectedType 是 mutable 的，所以可以在 const 方法中修改
    if (m_detectedType != None && m_detectedType != Other) {
        return m_detectedType;
    }

    // 依次检测各种虚拟化技术
    // 优先级可以根据常见程度或者检测效率来定
    if (detectVMware()) {
        m_detectedType = VMware;
        return m_detectedType;
    }
    if (detectVirtualBox()) {
        m_detectedType = VirtualBox;
        return m_detectedType;
    }
    if (detectHyperV()) {
        m_detectedType = HyperV;
        return m_detectedType;
    }
    if (detectKVM()) {
        m_detectedType = KVM;
        return m_detectedType;
    }
    if (detectXen()) {
        m_detectedType = Xen;
        return m_detectedType;
    }
    // 容器检测通常依赖于宿主机环境，或者特定的文件/环境变量
    // Docker, Kubernetes, LXC 的检测通常意味着当前进程运行在这些容器内部
    if (detectDocker()) {
        m_detectedType = Docker;
        return m_detectedType;
    }
    if (detectKubernetes()) {
        // Kubernetes通常运行在Docker等容器之上，所以Docker检测可能先命中
        // 如果需要区分，Kubernetes的检测逻辑应更具体
        m_detectedType = Kubernetes;
        return m_detectedType;
    }
    if (detectLXC()) {
        m_detectedType = LXC;
        return m_detectedType;
    }

    // 如果没有检测到任何已知的虚拟化技术
    m_detectedType = None;
    return m_detectedType;
}

// 获取虚拟化类型的字符串描述
QString VirtualizationMonitor::getVirtualizationTypeString(VirtualizationType type) const {
    switch (type) {
        case None: return QStringLiteral("None");
        case VMware: return QStringLiteral("VMware");
        case VirtualBox: return QStringLiteral("VirtualBox");
        case HyperV: return QStringLiteral("Hyper-V");
        case KVM: return QStringLiteral("KVM");
        case Xen: return QStringLiteral("Xen");
        case Docker: return QStringLiteral("Docker");
        case Kubernetes: return QStringLiteral("Kubernetes");
        case LXC: return QStringLiteral("LXC");
        case Other: return QStringLiteral("Other");
        default: return QStringLiteral("Unknown");
    }
}

// 执行命令并返回输出
QString VirtualizationMonitor::executeCommand(const QString &command, const QStringList &arguments) const {
    QProcess process;
    process.start(command, arguments);
    process.waitForFinished(); // 设置超时以避免永久阻塞
    QString output = process.readAllStandardOutput();
    QString errorOutput = process.readAllStandardError();
    if (process.exitStatus() != QProcess::NormalExit || process.exitCode() != 0) {
        qWarning() << "Command execution failed:" << command << arguments.join(" ");
        qWarning() << "Stdout:" << output;
        qWarning() << "Stderr:" << errorOutput;
        return QString(); // 返回空字符串表示失败
    }
    return output.trimmed();
}

// --- 私有检测方法实现 --- 

// 检测VMware虚拟机
bool VirtualizationMonitor::detectVMware() const {
#if defined(Q_OS_WIN)
    // 检查特定的注册表项或服务
    // 例如: HKEY_LOCAL_MACHINE\SYSTEM\CurrentControlSet\Services\vmdebug
    // 或者检查VMware Tools服务: executeCommand("sc", {"query", "VMTools"}).contains("RUNNING")
    // 检查特定的文件路径
    if (QFile::exists("C:/Program Files/VMware/VMware Tools/vmtoolsd.exe")) {
        return true;
    }
    // 检查驱动文件
    if (QFile::exists("C:/Windows/System32/drivers/vmci.sys") || QFile::exists("C:/Windows/System32/drivers/vmmouse.sys")) {
        return true;
    }
#elif defined(Q_OS_LINUX)
    // 检查 /sys/class/dmi/id/product_name 或 /sys/class/dmi/id/sys_vendor
    QString productName = executeCommand("cat", {"/sys/class/dmi/id/product_name"});
    QString sysVendor = executeCommand("cat", {"/sys/class/dmi/id/sys_vendor"});
    if (productName.contains("VMware", Qt::CaseInsensitive) || sysVendor.contains("VMware", Qt::CaseInsensitive)) {
        return true;
    }
    // 检查内核模块
    if (executeCommand("lsmod").contains("vmw_balloon")) {
        return true;
    }
#elif defined(Q_OS_MACOS)
    // macOS上的VMware Fusion检测
    // 检查ioreg输出
    if (executeCommand("ioreg", {"-l"}).contains("VMware", Qt::CaseInsensitive)) {
        return true;
    }
#endif
    return false;
}

// 检测VirtualBox虚拟机
bool VirtualizationMonitor::detectVirtualBox() const {
#if defined(Q_OS_WIN)
    // 检查注册表项: HKEY_LOCAL_MACHINE\HARDWARE\ACPI\DSDT\VBOX__
    // 检查服务: VBoxService
    if (executeCommand("sc", {"query", "VBoxService"}).contains("RUNNING")) {
        return true;
    }
    // 检查文件路径
    if (QFile::exists("C:/Program Files/Oracle/VirtualBox Guest Additions/VBoxControl.exe")) {
        return true;
    }
    // 检查驱动文件
    if (QFile::exists("C:/Windows/System32/drivers/VBoxGuest.sys")) {
        return true;
    }
#elif defined(Q_OS_LINUX)
    QString productName = executeCommand("cat", {"/sys/class/dmi/id/product_name"});
    QString sysVendor = executeCommand("cat", {"/sys/class/dmi/id/sys_vendor"});
    if (productName.contains("VirtualBox", Qt::CaseInsensitive) || sysVendor.contains("innotek GmbH", Qt::CaseInsensitive) || sysVendor.contains("Oracle Corporation", Qt::CaseInsensitive)) {
        return true;
    }
    if (executeCommand("lsmod").contains("vboxguest")) {
        return true;
    }
#elif defined(Q_OS_MACOS)
    // 检查ioreg输出
    if (executeCommand("ioreg", {"-l"}).contains("VirtualBox", Qt::CaseInsensitive)) {
        return true;
    }
#endif
    return false;
}

// 检测Hyper-V虚拟机
bool VirtualizationMonitor::detectHyperV() const {
#if defined(Q_OS_WIN)
    // 检查注册表项: HKEY_LOCAL_MACHINE\SOFTWARE\Microsoft\Virtual Machine\Guest\Parameters
    // 检查服务: vmicheartbeat, vmicvss, etc.
    // 更可靠的方式是检查 WMI
    // QString result = executeCommand("powershell", {"-Command", "(Get-WmiObject -q \"SELECT * FROM Win32_ComputerSystem\").Model"});
    // if (result.contains("Virtual Machine", Qt::CaseInsensitive) && result.contains("Microsoft", Qt::CaseInsensitive)) {
    //    return true;
    // }
    // 检查特定的文件或驱动
    if (QFile::exists("C:/Windows/System32/drivers/vmbus.sys")) {
        return true;
    }
#elif defined(Q_OS_LINUX)
    // Hyper-V在Linux客户机中通常会通过DMI信息暴露
    QString productName = executeCommand("cat", {"/sys/class/dmi/id/product_name"});
    QString sysVendor = executeCommand("cat", {"/sys/class/dmi/id/sys_vendor"});
    if (productName.contains("Virtual Machine", Qt::CaseInsensitive) && sysVendor.contains("Microsoft Corporation", Qt::CaseInsensitive)) {
        return true;
    }
    // 检查内核模块
    if (executeCommand("lsmod").contains("hv_vmbus")) {
        return true;
    }
#endif
    // macOS一般不作为Hyper-V的Guest
    return false;
}

// 检测KVM虚拟机
bool VirtualizationMonitor::detectKVM() const {
#if defined(Q_OS_LINUX)
    // 检查 /sys/class/dmi/id/product_name 或 /sys/class/dmi/id/sys_vendor
    QString productName = executeCommand("cat", {"/sys/class/dmi/id/product_name"});
    if (productName.contains("KVM", Qt::CaseInsensitive) || productName.contains("QEMU", Qt::CaseInsensitive)) {
        return true;
    }
    // 检查 /proc/cpuinfo 是否包含 kvm 或 qemu
    QString cpuInfo = executeCommand("cat", {"/proc/cpuinfo"});
    if (cpuInfo.contains("kvm", Qt::CaseInsensitive) || cpuInfo.contains("qemu", Qt::CaseInsensitive)) {
        return true;
    }
    // 检查内核模块
    if (executeCommand("lsmod").contains("kvm")) {
        return true;
    }
#endif
    // KVM主要用于Linux宿主机，Windows和macOS作为KVM Guest的情况较少，检测方式也不同
    return false;
}

// 检测Xen虚拟机
bool VirtualizationMonitor::detectXen() const {
#if defined(Q_OS_LINUX)
    // 检查 /sys/hypervisor/type
    if (QFile::exists("/sys/hypervisor/type")) {
        QString hypervisorType = executeCommand("cat", {"/sys/hypervisor/type"});
        if (hypervisorType.trimmed().toLower() == "xen") {
            return true;
        }
    }
    // 检查 /proc/xen/capabilities
    if (QFile::exists("/proc/xen/capabilities")) {
        return true;
    }
    // 检查DMI信息
    QString productName = executeCommand("cat", {"/sys/class/dmi/id/product_name"});
    if (productName.contains("HVM domU", Qt::CaseInsensitive)) {
        return true;
    }
#endif
    return false;
}

// 检测Docker容器
bool VirtualizationMonitor::detectDocker() const {
#if defined(Q_OS_LINUX)
    // 检查 /.dockerenv 文件是否存在
    if (QFile::exists("/.dockerenv")) {
        return true;
    }
    // 检查 /proc/1/cgroup 是否包含 "docker"
    QString cgroupContent = executeCommand("cat", {"/proc/1/cgroup"});
    if (cgroupContent.contains("docker", Qt::CaseInsensitive)) {
        return true;
    }
#elif defined(Q_OS_WIN)
    // Windows容器检测相对复杂，可能需要检查特定的服务或API
    // Docker Desktop for Windows 使用WSL2或Hyper-V
    // 如果是WSL2，可能被识别为KVM或Hyper-V
    // 检查Docker服务
    // if (executeCommand("sc", {"query", "Docker Engine"}).contains("RUNNING") || executeCommand("sc", {"query", "com.docker.service"}).contains("RUNNING")) {
    //    return true; // 这更多是检测宿主机是否有Docker服务，而不是当前是否在容器内
    // }
    // 在Windows容器内，环境变量 `ProgramData=C:\ProgramData` 通常不变，但这不是唯一标识
    // 更可靠的方式是检查特定的挂载点或进程，但这依赖于容器的配置
#endif
    // macOS上Docker Desktop使用虚拟机，容器运行在Linux VM内
    return false;
}

// 检测Kubernetes容器
bool VirtualizationMonitor::detectKubernetes() const {
#if defined(Q_OS_LINUX)
    // 检查环境变量，Kubernetes会自动注入一些 KUBERNETES_SERVICE_HOST 等
    if (!qgetenv("KUBERNETES_SERVICE_HOST").isEmpty()) {
        return true;
    }
    // 检查 /var/run/secrets/kubernetes.io/serviceaccount/token 文件是否存在
    if (QFile::exists("/var/run/secrets/kubernetes.io/serviceaccount/token")) {
        return true;
    }
#endif
    // Kubernetes Pod通常运行在Docker等容器运行时之上
    return false;
}

// 检测LXC容器
bool VirtualizationMonitor::detectLXC() const {
#if defined(Q_OS_LINUX)
    // 检查 /proc/1/environ 是否包含 lxc
    // QString environContent = executeCommand("cat", {"/proc/1/environ"});
    // if (environContent.contains("container=lxc")) { // 这个环境变量不一定存在
    //    return true;
    // }
    // 检查cgroup
    QString cgroupContent = executeCommand("cat", {"/proc/1/cgroup"});
    if (cgroupContent.contains("/lxc/")) {
        return true;
    }
    // 检查 `/.lxcenv` 或 `/.lxc-keep` (不标准)
#endif
    return false;
}

// --- 公有方法实现占位 --- 

// 获取虚拟机/容器列表
QStringList VirtualizationMonitor::getVirtualMachineList() const {
    // 根据检测到的虚拟化类型调用相应的私有方法
    // 确保虚拟化类型已被检测和缓存
    // detectVirtualizationType() 会处理缓存逻辑
    VirtualizationType currentType = detectVirtualizationType();

    switch (currentType) {
        case VMware: return getVMwareVMs();
        case VirtualBox: return getVirtualBoxVMs();
        case HyperV: return getHyperVVMs();
        // KVM, Xen 通常是宿主机技术，获取其上的VM列表需要特定工具和权限，这里简化
        default: break;
    }
    return QStringList();
}

QStringList VirtualizationMonitor::getContainerList() const {
    // 确保虚拟化类型已被检测和缓存
    VirtualizationType currentType = detectVirtualizationType();

    switch (currentType) {
        case Docker: return getDockerContainers();
        case Kubernetes: return getKubernetesPods();
        // LXC 列表获取也需要特定命令
        default: break;
    }
    return QStringList();
}

// 获取虚拟机/容器的资源使用情况
QMap<QString, QVariant> VirtualizationMonitor::getVirtualMachineResources(const QString &vmName, ResourceType type) const {
    Q_UNUSED(type);
    // 确保虚拟化类型已被检测和缓存
    VirtualizationType currentType = detectVirtualizationType();

    switch (currentType) {
        case VMware: return getVMwareVMResources(vmName);
        case VirtualBox: return getVirtualBoxVMResources(vmName);
        case HyperV: return getHyperVVMResources(vmName);
        default: break;
    }
    return QMap<QString, QVariant>();
}

QMap<QString, QVariant> VirtualizationMonitor::getContainerResources(const QString &containerName, ResourceType type) const {
    // 确保虚拟化类型已被检测和缓存
    VirtualizationType currentType = detectVirtualizationType();

    switch (currentType) {
        case Docker: return getDockerContainerResources(containerName, type);
        // TODO: Update getKubernetesPodResources to accept and use ResourceType similar to getDockerContainerResources
        case Kubernetes: return getKubernetesPodResources(containerName); // Needs update for type filtering
        default: break;
    }
    return QMap<QString, QVariant>();
}

// 获取虚拟机/容器的性能历史数据 (占位)
QList<QPair<QString, QVariant>> VirtualizationMonitor::getVirtualMachineHistory(const QString &vmName, ResourceType type, int hours) const {
    Q_UNUSED(vmName);
    Q_UNUSED(type);
    Q_UNUSED(hours);
    qWarning() << "getVirtualMachineHistory not implemented yet.";
    return QList<QPair<QString, QVariant>>();
}

QList<QPair<QString, QVariant>> VirtualizationMonitor::getContainerHistory(const QString &containerName, ResourceType type, int hours) const {
    Q_UNUSED(containerName);
    Q_UNUSED(type);
    Q_UNUSED(hours);
    qWarning() << "getContainerHistory not implemented yet.";
    return QList<QPair<QString, QVariant>>();
}

// 优化虚拟机/容器资源分配 (占位)
bool VirtualizationMonitor::optimizeVirtualMachineResources(const QString &vmName) const {
    Q_UNUSED(vmName);
    qWarning() << "optimizeVirtualMachineResources not implemented yet.";
    return false;
}

bool VirtualizationMonitor::optimizeContainerResources(const QString &containerName) const {
    Q_UNUSED(containerName);
    qWarning() << "optimizeContainerResources not implemented yet.";
    return false;
}

// 获取虚拟化环境的总体健康状况 (占位)
QMap<QString, QVariant> VirtualizationMonitor::getVirtualizationHealth() const {
    qWarning() << "getVirtualizationHealth not implemented yet.";
    return QMap<QString, QVariant>();
}

// 获取虚拟化优化建议 (占位)
QString VirtualizationMonitor::getVirtualizationOptimizationSuggestions() const {
    qWarning() << "getVirtualizationOptimizationSuggestions not implemented yet.";
    return QStringLiteral("No suggestions available yet.");
}


// --- 私有获取列表和资源方法实现 (主要是Docker的初步实现) ---

QStringList VirtualizationMonitor::getDockerContainers() const {
    QStringList containerIds;
#if defined(Q_OS_LINUX) || defined(Q_OS_MACOS) // Docker CLI在Linux和macOS上行为相似
    // 使用 docker ps 命令获取正在运行的容器ID
    // docker ps -q --no-trunc
    QString output = executeCommand("docker", {"ps", "-q", "--no-trunc"});
    if (!output.isEmpty()) {
        containerIds = output.split('\n', Qt::SkipEmptyParts);
    }
#elif defined(Q_OS_WIN)
    // Windows上Docker Desktop可能使用PowerShell命令
    // Get-Container -State Running | Select-Object -ExpandProperty ID
    // 或者 docker ps
    QString output = executeCommand("docker", {"ps", "-q", "--no-trunc"});
    if (!output.isEmpty()) {
        containerIds = output.split('\n', Qt::SkipEmptyParts);
    }
    // 如果是Windows原生容器，命令可能是 Get-Container
#endif
    return containerIds;
}

QMap<QString, QVariant> VirtualizationMonitor::getDockerContainerResources(const QString &containerNameOrId, ResourceType requestedType) const {
    QMap<QString, QVariant> allResources;
#if defined(Q_OS_LINUX) || defined(Q_OS_MACOS) || defined(Q_OS_WIN)
    // 使用 docker stats 命令获取容器资源使用情况
    // docker stats <container_id_or_name> --no-stream --format "{{.CPUPerc}},{{.MemUsage}},{{.NetIO}},{{.BlockIO}}"
    // 输出格式: CPU%, MEM USAGE / LIMIT, NET I/O, BLOCK I/O
    QString output = executeCommand("docker", {"stats", containerNameOrId, "--no-stream", "--format", 
                                          "{{.Name}},{{.CPUPerc}},{{.MemUsage}},{{.MemPerc}},{{.NetIO}},{{.BlockIO}},{{.PIDs}}"});
    
    if (output.isEmpty()) {
        qWarning() << "Failed to get stats for container:" << containerNameOrId;
        return allResources;
    }

    // 解析输出, 格式: container_name,cpu_percent,mem_usage_limit,mem_percent,net_io,block_io,pids
    // Example: test_container,0.05%,10.5MiB / 1.944GiB,0.53%,1.2kB / 0B,0B / 0B,5
    QStringList lines = output.split('\n', Qt::SkipEmptyParts);
    if (lines.isEmpty()) return allResources; // Should be at least one line for the specified container

    QStringList parts = lines.first().split(',');
    if (parts.size() >= 7) {
        allResources["Name"] = parts[0].trimmed();
        allResources["CPUPercRaw"] = parts[1].trimmed(); // Renamed from CPUPerc to avoid conflict with parsed value
        allResources["MemUsageRaw"] = parts[2].trimmed();
        allResources["MemPercRaw"] = parts[3].trimmed(); // Renamed from MemPerc
        allResources["NetIORaw"] = parts[4].trimmed();
        allResources["BlockIORaw"] = parts[5].trimmed();
        allResources["PIDs"] = parts[6].trimmed().toInt();

        // CPU Percentage
        QString cpuPercStr = parts[1].trimmed();
        if (cpuPercStr.endsWith('%')) cpuPercStr.chop(1);
        allResources["CPUUsagePercentage"] = cpuPercStr.toDouble();

        // Memory Usage
        QString memUsageRaw = parts[2].trimmed();
        QStringList memParts = memUsageRaw.split(" / ");
        if (memParts.size() == 2) {
            allResources["MemUsedBytes"] = parseByteString(memParts[0].trimmed());
            allResources["MemLimitBytes"] = parseByteString(memParts[1].trimmed());
        }

        // Memory Percentage
        QString memPercStr = parts[3].trimmed();
        if (memPercStr.endsWith('%')) memPercStr.chop(1);
        allResources["MemoryUsagePercentage"] = memPercStr.toDouble();

        // Network I/O
        QString netIORaw = parts[4].trimmed();
        QStringList netParts = netIORaw.split(" / ");
        if (netParts.size() == 2) {
            allResources["NetInputBytes"] = parseByteString(netParts[0].trimmed());
            allResources["NetOutputBytes"] = parseByteString(netParts[1].trimmed());
        }

        // Block I/O
        QString blockIORaw = parts[5].trimmed();
        QStringList blockParts = blockIORaw.split(" / ");
        if (blockParts.size() == 2) {
            allResources["BlockInputBytes"] = parseByteString(blockParts[0].trimmed());
            allResources["BlockOutputBytes"] = parseByteString(blockParts[1].trimmed());
        }

    } else {
        qWarning() << "Unexpected format from docker stats for" << containerNameOrId << ":" << output;
        return allResources; // Return whatever was parsed if format is unexpected
    }
#endif

    if (requestedType == All) {
        return allResources;
    }

    QMap<QString, QVariant> filteredResources;
    if (allResources.contains("Name")) filteredResources["Name"] = allResources["Name"];

    if (requestedType == CPU) {
        if (allResources.contains("CPUUsagePercentage")) filteredResources["CPUUsagePercentage"] = allResources["CPUUsagePercentage"];
        if (allResources.contains("CPUPercRaw")) filteredResources["CPUPercRaw"] = allResources["CPUPercRaw"];
    } else if (requestedType == Memory) {
        if (allResources.contains("MemUsedBytes")) filteredResources["MemUsedBytes"] = allResources["MemUsedBytes"];
        if (allResources.contains("MemLimitBytes")) filteredResources["MemLimitBytes"] = allResources["MemLimitBytes"];
        if (allResources.contains("MemoryUsagePercentage")) filteredResources["MemoryUsagePercentage"] = allResources["MemoryUsagePercentage"];
        if (allResources.contains("MemUsageRaw")) filteredResources["MemUsageRaw"] = allResources["MemUsageRaw"];
        if (allResources.contains("MemPercRaw")) filteredResources["MemPercRaw"] = allResources["MemPercRaw"];
    } else if (requestedType == Network) {
        if (allResources.contains("NetInputBytes")) filteredResources["NetInputBytes"] = allResources["NetInputBytes"];
        if (allResources.contains("NetOutputBytes")) filteredResources["NetOutputBytes"] = allResources["NetOutputBytes"];
        if (allResources.contains("NetIORaw")) filteredResources["NetIORaw"] = allResources["NetIORaw"];
    } else if (requestedType == Disk) { // Disk corresponds to BlockIO for containers
        if (allResources.contains("BlockInputBytes")) filteredResources["BlockInputBytes"] = allResources["BlockInputBytes"];
        if (allResources.contains("BlockOutputBytes")) filteredResources["BlockOutputBytes"] = allResources["BlockOutputBytes"];
        if (allResources.contains("BlockIORaw")) filteredResources["BlockIORaw"] = allResources["BlockIORaw"];
    }
    // Always include PIDs if available
    if (allResources.contains("PIDs")) filteredResources["PIDs"] = allResources["PIDs"]; 

    return filteredResources;

}

// --- 辅助方法实现 ---

qint64 VirtualizationMonitor::parseByteString(const QString &byteString) const {
    QString numStr = byteString;
    qint64 multiplier = 1;

    if (numStr.isEmpty()) return 0;

    // 移除末尾的 'B' (如 KB, MB, KiB, MiB)
    if (numStr.endsWith('B', Qt::CaseInsensitive)) {
        numStr.chop(1);
    }

    if (numStr.endsWith('K', Qt::CaseInsensitive)) {
        multiplier = 1024; // Default to KiB
        if (byteString.contains("KB", Qt::CaseSensitive) && !byteString.contains("KiB", Qt::CaseSensitive)) multiplier = 1000;
        numStr.chop(1);
    } else if (numStr.endsWith('M', Qt::CaseInsensitive)) {
        multiplier = 1024 * 1024; // Default to MiB
        if (byteString.contains("MB", Qt::CaseSensitive) && !byteString.contains("MiB", Qt::CaseSensitive)) multiplier = 1000 * 1000;
        numStr.chop(1);
    } else if (numStr.endsWith('G', Qt::CaseInsensitive)) {
        multiplier = 1024 * 1024 * 1024; // Default to GiB
        if (byteString.contains("GB", Qt::CaseSensitive) && !byteString.contains("GiB", Qt::CaseSensitive)) multiplier = 1000 * 1000 * 1000;
        numStr.chop(1);
    } else if (numStr.endsWith('T', Qt::CaseInsensitive)) {
        multiplier = 1024LL * 1024LL * 1024LL * 1024LL; // Default to TiB
        if (byteString.contains("TB", Qt::CaseSensitive) && !byteString.contains("TiB", Qt::CaseSensitive)) multiplier = 1000LL * 1000LL * 1000LL * 1000LL;
        numStr.chop(1);
    } else if (numStr.endsWith('P', Qt::CaseInsensitive)) { // Petabytes
        multiplier = 1024LL * 1024LL * 1024LL * 1024LL * 1024LL;
        if (byteString.contains("PB", Qt::CaseSensitive) && !byteString.contains("PiB", Qt::CaseSensitive)) multiplier = 1000LL * 1000LL * 1000LL * 1000LL * 1000LL;
        numStr.chop(1);
    }

    // 移除可能存在的 'i' (如 Ki, Mi)
    if (numStr.endsWith('i', Qt::CaseInsensitive)) {
        numStr.chop(1);
    }

    bool ok;
    double value = numStr.trimmed().toDouble(&ok);
    if (!ok) {
        qWarning() << "Failed to parse number from byte string:" << byteString << "(parsed as: " << numStr << ")";
        return 0;
    }

    return static_cast<qint64>(value * multiplier);
}

// --- 其他私有获取列表和资源方法实现 (初步或占位) ---
QStringList VirtualizationMonitor::getKubernetesPods() const {
    QStringList podNames;
#if defined(Q_OS_LINUX) || defined(Q_OS_MACOS) // kubectl CLI
    // kubectl get pods -o jsonpath='{.items[*].metadata.name}'
    // 或者 kubectl get pods --no-headers -o custom-columns=":metadata.name"
    QString output = executeCommand("kubectl", {"get", "pods", "--no-headers", "-o", "custom-columns=:metadata.name"});
    if (!output.isEmpty()) {
        podNames = output.split('\n', Qt::SkipEmptyParts);
    }
#elif defined(Q_OS_WIN)
    // Windows上kubectl行为类似
    QString output = executeCommand("kubectl", {"get", "pods", "--no-headers", "-o", "custom-columns=:metadata.name"});
    if (!output.isEmpty()) {
        podNames = output.split('\n', Qt::SkipEmptyParts);
    }
#endif
    if (podNames.isEmpty()){
        qWarning() << "getKubernetesPods: No pods found or kubectl not configured/available.";
    }
    return podNames;
}

QStringList VirtualizationMonitor::getVMwareVMs() const {
    // 获取VMware虚拟机列表通常需要使用VMware特定的命令行工具，如vmrun (VMware Workstation/Fusion) 或govc (vSphere)
    // 这是一个复杂的操作，高度依赖于具体的VMware产品和配置
    // 此处仅为占位符
#if defined(Q_OS_WIN)
    // 示例：尝试使用 vmrun list (如果安装了VMware Workstation且vmrun在PATH中)
    // QString output = executeCommand("vmrun", {"list"});
    // if (!output.isEmpty() && !output.contains("Total running VMs: 0")) {
    //    // 解析输出以获取VM列表，输出格式可能为 "Total running VMs: N\n/path/to/vm1.vmx\n/path/to/vm2.vmx"
    //    QStringList lines = output.split('\n', Qt::SkipEmptyParts);
    //    if (lines.size() > 1) {
    //        lines.removeFirst(); // 移除 "Total running VMs: N"
    //        return lines;
    //    }
    // }
#elif defined(Q_OS_LINUX)
    // Linux上的vmrun行为类似
#elif defined(Q_OS_MACOS)
    // macOS上的vmrun行为类似
#endif
    qWarning() << "getVMwareVMs: Not fully implemented. Requires VMware specific tools and configuration.";
    return QStringList();
}

QStringList VirtualizationMonitor::getVirtualBoxVMs() const {
    QStringList vmNames;
    // 使用 VBoxManage list vms
#if defined(Q_OS_WIN) || defined(Q_OS_LINUX) || defined(Q_OS_MACOS)
    QString output = executeCommand("VBoxManage", {"list", "vms"});
    // 输出格式: "VM Name 1" {uuid1}\n"VM Name 2" {uuid2}
    if (!output.isEmpty()) {
        QStringList lines = output.split('\n', Qt::SkipEmptyParts);
        for (const QString &line : lines) {
            int firstQuote = line.indexOf('"');
            int lastQuote = line.lastIndexOf('"');
            if (firstQuote != -1 && lastQuote != -1 && firstQuote < lastQuote) {
                vmNames.append(line.mid(firstQuote + 1, lastQuote - firstQuote - 1));
            }
        }
    }
#endif
    if (vmNames.isEmpty()) {
         qWarning() << "getVirtualBoxVMs: No VMs found or VBoxManage not in PATH.";
    }
    return vmNames;
}

QStringList VirtualizationMonitor::getHyperVVMs() const {
    QStringList vmNames;
#if defined(Q_OS_WIN)
    // 使用 PowerShell Get-VM 命令
    // Get-VM | Select-Object -ExpandProperty VMName
    // 注意：直接执行 powershell 命令可能需要特殊处理执行策略
    QString command = "powershell";
    QStringList args = {"-NoProfile", "-ExecutionPolicy", "Bypass", "-Command", "Get-VM | Select-Object -ExpandProperty VMName"};
    QString output = executeCommand(command, args);
    if (!output.isEmpty()) {
        vmNames = output.split(QRegularExpression("[\r\n]+"), Qt::SkipEmptyParts);
    }
#else
    // Hyper-V管理主要在Windows上
    qWarning() << "getHyperVVMs: Primarily a Windows feature. Not implemented for this OS.";
#endif
    if (vmNames.isEmpty() && defined(Q_OS_WIN)) {
        qWarning() << "getHyperVVMs: No VMs found or PowerShell Get-VM failed.";
    }
    return vmNames;
}

QMap<QString, QVariant> VirtualizationMonitor::getKubernetesPodResources(const QString &podName) const {
    QMap<QString, QVariant> resources;
    // kubectl top pod <pod_name> --no-headers
    // 输出: POD_NAME CPU(cores) MEMORY(bytes)
    // e.g., my-pod 100m 200Mi
#if defined(Q_OS_LINUX) || defined(Q_OS_MACOS) || defined(Q_OS_WIN)
    QString output = executeCommand("kubectl", {"top", "pod", podName, "--no-headers"});
    if (output.isEmpty()) {
        qWarning() << "Failed to get resources for pod:" << podName;
        return resources;
    }
    QStringList parts = output.split(QRegularExpression("\\s+"), Qt::SkipEmptyParts);
    if (parts.size() >= 3) {
        resources["Name"] = parts[0];
        resources["CPUCoresRaw"] = parts[1]; // e.g., "100m"
        resources["MemoryBytesRaw"] = parts[2]; // e.g., "200Mi"

        // TODO: Parse CPU (e.g., '100m' to 0.1 cores) and Memory (e.g., '200Mi' to bytes)
        // For simplicity, storing raw values for now.
        // Example CPU parsing:
        QString cpuStr = parts[1];
        double cpuCores = 0.0;
        if (cpuStr.endsWith('m')) {
            cpuStr.chop(1);
            cpuCores = cpuStr.toDouble() / 1000.0;
        } else {
            cpuCores = cpuStr.toDouble();
        }
        resources["CPUCores"] = cpuCores;

        resources["MemoryBytes"] = parseByteString(parts[2]);

    } else {
        qWarning() << "Unexpected format from kubectl top pod for" << podName << ":" << output;
    }
#else
    Q_UNUSED(podName);
#endif
    return resources;
}

QMap<QString, QVariant> VirtualizationMonitor::getVMwareVMResources(const QString &vmName) const {
    // Getting specific resource usage for a VMware VM via CLI (e.g., vmrun, govc) is complex
    // and often requires the VM to be running and VMware Tools to be installed and operational.
    // The output format varies significantly.
    // This is a placeholder.
    Q_UNUSED(vmName);
    qWarning() << "getVMwareVMResources: Not fully implemented. Highly dependent on VMware tools and product.";
    return QMap<QString, QVariant>();
}

QMap<QString, QVariant> VirtualizationMonitor::getVirtualBoxVMResources(const QString &vmName) const {
    QMap<QString, QVariant> resources;
    // VBoxManage guestproperty get <vmname> /VirtualBox/GuestInfo/OS/Load
    // VBoxManage metrics query <vmname> CPU/Load/User,RAM/Usage/Total
    // Example: VBoxManage metrics query "<vm_name_or_uuid>" Guest/CPU/Load/User,Guest/RAM/Usage/Total
    // Output is complex and needs parsing.
    // For simplicity, this is a placeholder.
#if defined(Q_OS_WIN) || defined(Q_OS_LINUX) || defined(Q_OS_MACOS)
    // Example for CPU load (very simplified, VBoxManage metrics is more comprehensive but harder to parse generally)
    // QString cpuLoadOutput = executeCommand("VBoxManage", {"guestproperty", "get", vmName, "/VirtualBox/GuestInfo/CPU/Load"});
    // // Value: 5 (example for 5% load)
    // if (cpuLoadOutput.startsWith("Value: ")) {
    //     resources["CPULoad"] = cpuLoadOutput.mid(7).trimmed().toDouble();
    // }
#endif
    Q_UNUSED(vmName);
    qWarning() << "getVirtualBoxVMResources: Not fully implemented. VBoxManage metrics query is complex.";
    return resources;
}

QMap<QString, QVariant> VirtualizationMonitor::getHyperVVMResources(const QString &vmName) const {
    QMap<QString, QVariant> resources;
#if defined(Q_OS_WIN)
    // Get-VM <vmname> | Measure-VM
    // Or Get-Counter for specific metrics if Hyper-V counters are enabled
    // Example: (Measure-VM -VMName <vmName>).ProcessorUsage (percentage)
    //          (Measure-VM -VMName <vmName>).MemoryAssigned (bytes)
    //          (Measure-VM -VMName <vmName>).NetworkUsage (bytes)
    //          (Measure-VM -VMName <vmName>).DiskUsage (bytes)
    QString psCommand = QString("(Measure-VM -VMName '%1' | Select-Object ProcessorUsage, MemoryAssigned, TotalDiskSpace, NetworkUsageData) | ConvertTo-Json -Compress").arg(vmName);
    QString output = executeCommand("powershell", {"-NoProfile", "-ExecutionPolicy", "Bypass", "-Command", psCommand});

    if (!output.isEmpty()) {
        QJsonDocument doc = QJsonDocument::fromJson(output.toUtf8());
        if (!doc.isNull() && doc.isObject()) {
            QJsonObject obj = doc.object();
            if (obj.contains("ProcessorUsage")) resources["CPUUsagePercent"] = obj["ProcessorUsage"].toDouble();
            if (obj.contains("MemoryAssigned")) resources["MemoryAssignedBytes"] = obj["MemoryAssigned"].toDouble(); // This is assigned, not necessarily usage
            if (obj.contains("TotalDiskSpace")) resources["DiskAllocatedBytes"] = obj["TotalDiskSpace"].toDouble(); // This is allocated, not usage
            // NetworkUsageData is complex, often 0 if not actively measured or VM off.
            // For actual usage, Get-VMNetworkAdapter and Get-VMHardDiskDrive might be needed with performance counters.
        } else {
            qWarning() << "getHyperVVMResources: Failed to parse JSON from PowerShell for" << vmName << output;
        }
    } else {
        qWarning() << "getHyperVVMResources: Measure-VM failed or returned no output for" << vmName;
    }
#else
    Q_UNUSED(vmName);
    qWarning() << "getHyperVVMResources: Primarily a Windows feature.";
#endif
    return resources;
}