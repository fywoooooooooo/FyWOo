// modelinterface.cpp
#include "src/include/model/modelinterface.h"
#include <QUrlQuery>
#include <QTimer>

// ???????
const QString ModelInterface::DEFAULT_MODEL = "deepseek-chat";
const QString ModelInterface::DEFAULT_ENDPOINT = "https://api.deepseek.com/v1/chat/completions";

ModelInterface::ModelInterface(QObject *parent)
    : QObject(parent)
    , m_networkManager(new QNetworkAccessManager(this))
    , m_currentReply(nullptr)
    , m_modelName(DEFAULT_MODEL)
    , m_apiEndpoint(DEFAULT_ENDPOINT)
{
}

ModelInterface::~ModelInterface()
{
    cancelRequest();
}

void ModelInterface::setModel(const QString &modelName)
{
    m_modelName = modelName;
}

void ModelInterface::setApiKey(const QString &apiKey)
{
    m_apiKey = apiKey;
}

void ModelInterface::sendRequest(const QString &prompt)
{
    // ???API??????????
    if (m_apiKey.isEmpty()) {
        emit errorOccurred(tr("API???¦Ä????"));
        return;
    }
    
    // ????¦Ê???????§Ö?????
    cancelRequest();
    
    // ???????????
    QJsonDocument requestData = prepareRequestData(prompt);
    
    // ????????????
    QNetworkRequest request;
    request.setUrl(QUrl(m_apiEndpoint));
    request.setHeader(QNetworkRequest::ContentTypeHeader, "application/json");
    
    // ?????????????Authorization?????????
    QByteArray authHeader = QString("Bearer %1").arg(m_apiKey).toUtf8();
    request.setRawHeader("Authorization", authHeader);
    
    // ???????????????????????
    // Qt 6?·Ú??FollowRedirectsAttribute???????????RedirectPolicyAttribute???
    request.setAttribute(QNetworkRequest::RedirectPolicyAttribute, QNetworkRequest::NoLessSafeRedirectPolicy);
    
    // ???¨®?????
    request.setAttribute(QNetworkRequest::CacheLoadControlAttribute, QNetworkRequest::AlwaysNetwork);
    
    // ????????
    m_currentReply = m_networkManager->post(request, requestData.toJson());
    
    // ?????????
    connect(m_currentReply, &QNetworkReply::finished, this, &ModelInterface::handleNetworkReply);
    connect(m_currentReply, &QNetworkReply::errorOccurred,
            this, &ModelInterface::handleNetworkError);
    
    // ????????????
    emit requestStarted();
}

void ModelInterface::cancelRequest()
{
    if (m_currentReply && m_currentReply->isRunning()) {
        m_currentReply->abort();
        m_currentReply->deleteLater();
        m_currentReply = nullptr;
    }
}

QJsonDocument ModelInterface::prepareRequestData(const QString &prompt)
{
    QJsonObject requestObj;
    requestObj["model"] = m_modelName;
    
    // ???????????
    QJsonArray messagesArray;
    
    // ?????????
    QJsonObject systemMessage;
    systemMessage["role"] = "system";
    systemMessage["content"] = "???????????????????????????????????????????????î•";
    messagesArray.append(systemMessage);
    
    // ??????????
    QJsonObject userMessage;
    userMessage["role"] = "user";
    userMessage["content"] = prompt;
    messagesArray.append(userMessage);
    
    // ????????????????
    requestObj["messages"] = messagesArray;
    
    // ????????????
    requestObj["temperature"] = 0.7;
    requestObj["max_tokens"] = 2000;
    
    return QJsonDocument(requestObj);
}

void ModelInterface::handleNetworkReply()
{
    // ?????????????
    if (!m_currentReply) {
        return;
    }
    
    // ???????
    if (m_currentReply->error() == QNetworkReply::NoError) {
        // ??????????
        QByteArray responseData = m_currentReply->readAll();
        
        // ???????
        QString response = parseResponse(responseData);
        
        // ??????????
        emit responseReceived(response);
    }
    
    // ????
    m_currentReply->deleteLater();
    m_currentReply = nullptr;
    
    // ??????????????
    emit requestFinished();
}

void ModelInterface::handleNetworkError(QNetworkReply::NetworkError error)
{
    if (m_currentReply) {
        QString errorMessage = tr("???????: %1").arg(m_currentReply->errorString());
        emit errorOccurred(errorMessage);
    } else {
        emit errorOccurred(tr("¦Ä????????"));
    }
}

QString ModelInterface::parseResponse(const QByteArray &responseData)
{
    // ????JSON???
    QJsonDocument doc = QJsonDocument::fromJson(responseData);
    if (doc.isNull() || !doc.isObject()) {
        return tr("??????????????");
    }
    
    QJsonObject responseObj = doc.object();
    
    // ???????§Õ???
    if (responseObj.contains("error")) {
        QJsonObject errorObj = responseObj["error"].toObject();
        return tr("API????: %1").arg(errorObj["message"].toString());
    }
    
    // ??????????
    if (responseObj.contains("choices") && responseObj["choices"].isArray()) {
        QJsonArray choices = responseObj["choices"].toArray();
        if (!choices.isEmpty()) {
            QJsonObject choice = choices.at(0).toObject();
            if (choice.contains("message") && choice["message"].isObject()) {
                QJsonObject message = choice["message"].toObject();
                return message["content"].toString();
            }
        }
    }
    
    return tr("?????????????????");
}