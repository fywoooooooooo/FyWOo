#pragma once

#include <QObject>
#include <QMap>
#include <QString>
#include <QVariant>
#include <QList>
#include <QPair>
#include <QProcess>

// 虚拟化监控类 - 提供对虚拟机和容器的性能监控
class VirtualizationMonitor : public QObject {
    Q_OBJECT

public:
    // 虚拟化类型枚举
    enum VirtualizationType {
        None,           // 非虚拟化环境
        VMware,         // VMware虚拟机
        VirtualBox,     // VirtualBox虚拟机
        HyperV,         // Hyper-V虚拟机
        KVM,            // KVM虚拟机
        Xen,            // Xen虚拟机
        Docker,         // Docker容器
        Kubernetes,     // Kubernetes容器
        LXC,            // LXC容器
        OpenVZ,         // OpenVZ容器
        Other           // 其他虚拟化类型
    };
    
    // 虚拟化资源类型枚举
    enum ResourceType {
        CPU,            // CPU资源
        Memory,         // 内存资源
        Disk,           // 磁盘资源
        Network,        // 网络资源
        All             // 所有资源
    };

    explicit VirtualizationMonitor(QObject *parent = nullptr);
    ~VirtualizationMonitor();

    // 检测当前环境的虚拟化类型
    VirtualizationType detectVirtualizationType() const;
    
    // 获取虚拟化类型的字符串描述
    QString getVirtualizationTypeString(VirtualizationType type) const;
    
    // 获取虚拟机/容器列表
    QStringList getVirtualMachineList() const;
    QStringList getContainerList() const;
    
    // 获取虚拟机/容器的资源使用情况
    QMap<QString, QVariant> getVirtualMachineResources(const QString &vmName, ResourceType type = All) const;
    QMap<QString, QVariant> getContainerResources(const QString &containerName, ResourceType type = All) const;
    
    // 获取虚拟机/容器的性能历史数据
    QList<QPair<QString, QVariant>> getVirtualMachineHistory(const QString &vmName, ResourceType type, int hours) const;
    QList<QPair<QString, QVariant>> getContainerHistory(const QString &containerName, ResourceType type, int hours) const;
    
    // 优化虚拟机/容器资源分配
    bool optimizeVirtualMachineResources(const QString &vmName) const;
    bool optimizeContainerResources(const QString &containerName) const;
    
    // 获取虚拟化环境的总体健康状况
    QMap<QString, QVariant> getVirtualizationHealth() const;
    
    // 获取虚拟化优化建议
    QString getVirtualizationOptimizationSuggestions() const;

private:
    // 检测VMware虚拟机
    bool detectVMware() const;
    
    // 检测VirtualBox虚拟机
    bool detectVirtualBox() const;
    
    // 检测Hyper-V虚拟机
    bool detectHyperV() const;
    
    // 检测KVM虚拟机
    bool detectKVM() const;
    
    // 检测Xen虚拟机
    bool detectXen() const;
    
    // 检测Docker容器
    bool detectDocker() const;
    
    // 检测Kubernetes容器
    bool detectKubernetes() const;
    
    // 检测LXC容器
    bool detectLXC() const;

    // 检测OpenVZ容器
    bool detectOpenVZ() const;
    
    // 获取Docker容器列表
    QStringList getDockerContainers() const;
    
    // 获取Kubernetes Pod列表
    QStringList getKubernetesPods() const;
    
    // 获取VMware虚拟机列表
    QStringList getVMwareVMs() const;
    
    // 获取VirtualBox虚拟机列表
    QStringList getVirtualBoxVMs() const;
    
    // 获取Hyper-V虚拟机列表
    QStringList getHyperVVMs() const;

    // 获取KVM虚拟机列表
    QStringList getKVMVMs() const;

    // 获取Xen虚拟机列表
    QStringList getXenVMs() const;

    // 获取LXC容器列表
    QStringList getLXCContainers() const;

    // 获取OpenVZ容器列表
    QStringList getOpenVZContainers() const;
    
    // 获取Docker容器资源使用情况
    QMap<QString, QVariant> getDockerContainerResources(const QString &containerName, ResourceType type = All) const;
    
    // 获取Kubernetes Pod资源使用情况
    QMap<QString, QVariant> getKubernetesPodResources(const QString &podName, ResourceType type = All) const;
    
    // 获取VMware虚拟机资源使用情况
    QMap<QString, QVariant> getVMwareVMResources(const QString &vmName) const;
    
    // 获取VirtualBox虚拟机资源使用情况
    QMap<QString, QVariant> getVirtualBoxVMResources(const QString &vmName) const;
    
    // 获取Hyper-V虚拟机资源使用情况
    QMap<QString, QVariant> getHyperVVMResources(const QString &vmName, ResourceType type = All) const;

    // 获取KVM虚拟机资源使用情况
    QMap<QString, QVariant> getKVMVMResources(const QString &vmName, ResourceType type = All) const;

    // 获取Xen虚拟机资源使用情况
    QMap<QString, QVariant> getXenVMResources(const QString &vmName, ResourceType type = All) const;

    // 获取LXC容器资源使用情况
    QMap<QString, QVariant> getLXCContainerResources(const QString &containerName, ResourceType type = All) const;

    // 获取OpenVZ容器资源使用情况
    QMap<QString, QVariant> getOpenVZContainerResources(const QString &containerName, ResourceType type = All) const;
    
    // 执行命令并返回输出
    QString executeCommand(const QString &command, const QStringList &arguments = QStringList()) const;

    // 辅助方法：解析字节字符串 (例如 "10.5MiB", "1.2kB") 为 qint64
    qint64 parseByteString(const QString &byteString) const;
    
    // 当前检测到的虚拟化类型 (mutable for caching in const methods)
    mutable VirtualizationType m_detectedType;
};