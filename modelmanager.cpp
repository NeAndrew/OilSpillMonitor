#include "modelmanager.h"

ModelManager::ModelManager(const QString &apiKey, QObject *parent)
    : QObject(parent)
    , m_apiKey(apiKey)
    , m_networkManager(new QNetworkAccessManager(this))
    , m_completedRequests(0)
    , m_totalRequests(0)
{
    setupModels();
}

ModelManager::~ModelManager()
{
}

void ModelManager::setupModels()
{
    // Модели детекции нефтяных пятен Roboflow
    m_models =
    {
        {"YOLOv26", QString("https://detect.roboflow.com/oilspilldetector-fcmeo-eyspc/8?api_key=%1").arg(m_apiKey), QJsonArray(), false},
        {"YOLOv11", QString("https://detect.roboflow.com/oilspilldetector-fcmeo-eyspc/1?api_key=%1").arg(m_apiKey), QJsonArray(), false},
        {"RF-DETR-Seg (tiled)", QString("https://detect.roboflow.com/oilspilldetector-fcmeo-eyspc/7?api_key=%1").arg(m_apiKey), QJsonArray(), false},
        {"RF-DETR-Seg", QString("https://detect.roboflow.com/oilspilldetector-fcmeo-eyspc/2?api_key=%1").arg(m_apiKey), QJsonArray(), false}
    };
}

void ModelManager::sendRequestsToAllModels(const QByteArray &imageData)
{
    // Сбрасываем состояние
    for (auto &model : m_models)
    {
        model.isReady = false;
        model.response = QJsonArray();
    }
    
    m_completedRequests = 0;
    m_totalRequests = m_models.size();

    // Отправляем запросы ко всем моделям
    for (int i = 0; i < m_models.size(); ++i)
    {
        sendRequestToModel(i, imageData);
    }
}

void ModelManager::sendRequestToModel(int modelIndex, const QByteArray &imageData)
{
    if (modelIndex < 0 || modelIndex >= m_models.size())
    {
        emit modelErrorOccurred(modelIndex, "Некорректный индекс модели");
        return;
    }

    QHttpMultiPart *multiPart = new QHttpMultiPart(QHttpMultiPart::FormDataType);
    
    // Добавляем изображение
    QHttpPart imagePart;
    imagePart.setHeader(QNetworkRequest::ContentDispositionHeader, "form-data; name=\"file\"; filename=\"image.png\"");
    imagePart.setHeader(QNetworkRequest::ContentTypeHeader, "image/png");
    imagePart.setBody(imageData);
    multiPart->append(imagePart);

    // Добавляем параметры
    QHttpPart confPart;
    confPart.setHeader(QNetworkRequest::ContentDispositionHeader, "form-data; name=\"confidence\"");
    confPart.setBody("0.5");
    multiPart->append(confPart);

    QHttpPart overlapPart;
    overlapPart.setHeader(QNetworkRequest::ContentDispositionHeader, "form-data; name=\"overlap\"");
    overlapPart.setBody("0.5");
    multiPart->append(overlapPart);

    QHttpPart formatPart;
    formatPart.setHeader(QNetworkRequest::ContentDispositionHeader, "form-data; name=\"format\"");
    formatPart.setBody("json");
    multiPart->append(formatPart);

    QNetworkRequest request(QUrl(m_models[modelIndex].url));
    QNetworkReply *reply = m_networkManager->post(request, multiPart);
    multiPart->setParent(reply);

    connect(reply, &QNetworkReply::finished, this, [this, reply, modelIndex]()
    {
        onNetworkReplyFinished();
        
        if (reply->error() == QNetworkReply::NoError)
        {
            QByteArray resp = reply->readAll();
            QJsonDocument doc = QJsonDocument::fromJson(resp);
            if (!doc.isNull() && doc.isObject())
            {
                m_models[modelIndex].response = doc.object().value("predictions").toArray();
                m_models[modelIndex].isReady = true;
                emit modelResponseReceived(modelIndex, m_models[modelIndex].response);
            }
            else
            {
                emit modelErrorOccurred(modelIndex, "Некорректный JSON ответ.");
            }
        }
        else
        {
            emit modelErrorOccurred(modelIndex, reply->errorString());
        }
        
        reply->deleteLater();
    });
}

void ModelManager::onNetworkReplyFinished()
{
    m_completedRequests++;
    if (m_completedRequests >= m_totalRequests)
    {
        emit allModelsCompleted();
    }
}

QJsonArray ModelManager::getModelResponse(int modelIndex) const
{
    if (modelIndex >= 0 && modelIndex < m_models.size())
    {
        return m_models[modelIndex].response;
    }
    return QJsonArray();
}

bool ModelManager::isModelReady(int modelIndex) const
{
    if (modelIndex >= 0 && modelIndex < m_models.size())
    {
        return m_models[modelIndex].isReady;
    }
    return false;
}

QStringList ModelManager::getModelNames() const
{
    QStringList names;
    for (const auto &model : m_models)
    {
        names.append(model.name);
    }
    return names;
}

int ModelManager::getModelCount() const
{
    return m_models.size();
}
