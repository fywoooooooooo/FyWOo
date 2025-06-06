// modelinterface.h
#pragma once

#include <QObject>
#include <QString>
#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>

class ModelInterface : public QObject {
    Q_OBJECT

public:
    explicit ModelInterface(QObject *parent = nullptr);
    ~ModelInterface();

    // 设置模型参数
    void setModel(const QString &modelName);
    void setApiKey(const QString &apiKey);
    
    // 发送请求到模型API
    void sendRequest(const QString &prompt);
    
    // 取消当前请求
    void cancelRequest();

signals:
    // 当收到模型响应时发出信号
    void responseReceived(const QString &response);
    
    // 错误信号
    void errorOccurred(const QString &errorMessage);
    
    // 请求状态信号
    void requestStarted();
    void requestFinished();

private slots:
    void handleNetworkReply();
    void handleNetworkError(QNetworkReply::NetworkError error);

private:
    // 准备请求数据
    QJsonDocument prepareRequestData(const QString &prompt);
    
    // 解析响应数据
    QString parseResponse(const QByteArray &responseData);

    QNetworkAccessManager *m_networkManager;
    QNetworkReply *m_currentReply;
    
    QString m_modelName;
    QString m_apiKey;
    QString m_apiEndpoint;
    
    static const QString DEFAULT_MODEL;
    static const QString DEFAULT_ENDPOINT;
};