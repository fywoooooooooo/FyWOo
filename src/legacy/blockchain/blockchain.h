#ifndef BLOCKCHAIN_H
#define BLOCKCHAIN_H

#include <QObject>
#include <QVector>
#include <QDateTime>
#include <QCryptographicHash>
#include <QJsonObject>
#include <QJsonDocument>
#include <QDebug>

/**
 * @brief 区块类，表示区块链中的单个区块
 */
class Block {
public:
    Block(int index, const QDateTime &timestamp, const QString &data, const QString &previousHash = "");

    QString calculateHash() const;
    
    // Getters
    int getIndex() const { return m_index; }
    QDateTime getTimestamp() const { return m_timestamp; }
    QString getData() const { return m_data; }
    QString getHash() const { return m_hash; }
    QString getPreviousHash() const { return m_previousHash; }

private:
    int m_index;                // 区块索引
    QDateTime m_timestamp;      // 区块创建时间戳
    QString m_data;             // 区块数据
    QString m_hash;             // 区块哈希值
    QString m_previousHash;     // 前一个区块的哈希值
};

/**
 * @brief 区块链类，管理区块的集合
 */
class Blockchain : public QObject {
    Q_OBJECT
public:
    explicit Blockchain(QObject *parent = nullptr);

    /**
     * @brief 添加新区块到区块链
     * @param data 要存储的数据
     * @return 新添加的区块
     */
    Block addBlock(const QString &data);

    /**
     * @brief 验证区块链的完整性
     * @return 如果区块链有效返回true，否则返回false
     */
    bool isChainValid() const;

    /**
     * @brief 获取区块链中的所有区块
     * @return 区块链中的所有区块
     */
    QVector<Block> getChain() const { return m_chain; }

    /**
     * @brief 获取最新区块
     * @return 区块链中的最新区块
     */
    Block getLatestBlock() const;

    /**
     * @brief 将区块链导出为JSON格式
     * @return 包含区块链数据的JSON文档
     */
    QJsonDocument toJson() const;

    /**
     * @brief 从JSON导入区块链数据
     * @param json 包含区块链数据的JSON文档
     * @return 导入是否成功
     */
    bool fromJson(const QJsonDocument &json);

private:
    QVector<Block> m_chain;  // 存储区块的容器

    /**
     * @brief 创建创世区块（区块链中的第一个区块）
     * @return 创世区块
     */
    Block createGenesisBlock();
};

#endif // BLOCKCHAIN_H