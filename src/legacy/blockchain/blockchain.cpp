#include "../../include/storage/blockchain.h"
#include <QJsonArray>

// Block 实现
Block::Block(int index, const QDateTime &timestamp, const QString &data, const QString &previousHash)
    : m_index(index)
    , m_timestamp(timestamp)
    , m_data(data)
    , m_previousHash(previousHash)
{
    m_hash = calculateHash();
}

QString Block::calculateHash() const {
    QString stringToHash = QString::number(m_index) + 
                          m_timestamp.toString(Qt::ISODate) + 
                          m_data + 
                          m_previousHash;
    
    QByteArray hashData = QCryptographicHash::hash(
        stringToHash.toUtf8(),
        QCryptographicHash::Sha256
    );
    
    return QString(hashData.toHex());
}

// Blockchain 实现
Blockchain::Blockchain(QObject *parent)
    : QObject(parent)
{
    // 初始化区块链，添加创世区块
    m_chain.append(createGenesisBlock());
}

Block Blockchain::createGenesisBlock() {
    // 创建创世区块，索引为0，前一个哈希为0
    return Block(0, QDateTime::currentDateTime(), "创世区块", "0");
}

Block Blockchain::getLatestBlock() const {
    return m_chain.last();
}

Block Blockchain::addBlock(const QString &data) {
    Block latestBlock = getLatestBlock();
    int newIndex = latestBlock.getIndex() + 1;
    QDateTime timestamp = QDateTime::currentDateTime();
    
    // 创建新区块，前一个哈希是最新区块的哈希
    Block newBlock(newIndex, timestamp, data, latestBlock.getHash());
    m_chain.append(newBlock);
    
    return newBlock;
}

bool Blockchain::isChainValid() const {
    // 区块链至少要有一个区块（创世区块）
    if (m_chain.isEmpty()) {
        return false;
    }
    
    // 验证每个区块
    for (int i = 1; i < m_chain.size(); i++) {
        const Block &currentBlock = m_chain[i];
        const Block &previousBlock = m_chain[i-1];
        
        // 检查当前区块的哈希是否正确
        if (currentBlock.getHash() != currentBlock.calculateHash()) {
            qDebug() << "区块" << i << "的哈希无效";
            return false;
        }
        
        // 检查当前区块的前一个哈希是否指向前一个区块的哈希
        if (currentBlock.getPreviousHash() != previousBlock.getHash()) {
            qDebug() << "区块" << i << "与前一个区块的链接无效";
            return false;
        }
    }
    
    return true;
}

QJsonDocument Blockchain::toJson() const {
    QJsonArray blockArray;
    
    for (const Block &block : m_chain) {
        QJsonObject blockObject;
        blockObject["index"] = block.getIndex();
        blockObject["timestamp"] = block.getTimestamp().toString(Qt::ISODate);
        blockObject["data"] = block.getData();
        blockObject["hash"] = block.getHash();
        blockObject["previousHash"] = block.getPreviousHash();
        
        blockArray.append(blockObject);
    }
    
    QJsonObject chainObject;
    chainObject["chain"] = blockArray;
    
    return QJsonDocument(chainObject);
}

bool Blockchain::fromJson(const QJsonDocument &json) {
    if (!json.isObject()) {
        return false;
    }
    
    QJsonObject chainObject = json.object();
    if (!chainObject.contains("chain") || !chainObject["chain"].isArray()) {
        return false;
    }
    
    QJsonArray blockArray = chainObject["chain"].toArray();
    QVector<Block> newChain;
    
    for (const QJsonValue &value : blockArray) {
        if (!value.isObject()) {
            return false;
        }
        
        QJsonObject blockObject = value.toObject();
        
        // 验证必要的字段
        if (!blockObject.contains("index") || !blockObject.contains("timestamp") ||
            !blockObject.contains("data") || !blockObject.contains("hash") ||
            !blockObject.contains("previousHash")) {
            return false;
        }
        
        int index = blockObject["index"].toInt();
        QDateTime timestamp = QDateTime::fromString(blockObject["timestamp"].toString(), Qt::ISODate);
        QString data = blockObject["data"].toString();
        QString hash = blockObject["hash"].toString();
        QString previousHash = blockObject["previousHash"].toString();
        
        // 创建区块并添加到新链中
        Block block(index, timestamp, data, previousHash);
        
        // 验证哈希值是否匹配
        if (block.calculateHash() != hash) {
            return false;
        }
        
        newChain.append(block);
    }
    
    // 验证新链的完整性
    m_chain = newChain;
    if (!isChainValid()) {
        m_chain.clear(); // 如果链无效，清空链
        m_chain.append(createGenesisBlock()); // 重新添加创世区块
        return false;
    }
    
    return true;
}