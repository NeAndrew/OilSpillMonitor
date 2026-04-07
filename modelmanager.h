#ifndef MODELMANAGER_H
#define MODELMANAGER_H

#include <QObject>
#include <QNetworkReply>
#include <QJsonArray>
#include <QHttpMultiPart>
#include <QJsonDocument>
#include <QJsonObject>

/**
 * @brief Менеджер для работы с моделями детекции нефтяных пятен
 *
 * Класс реализует отправку запросов к API Roboflow для различных моделей
 * детекции, обработку ответов и управление моделеями.
 */
class ModelManager : public QObject
{
    Q_OBJECT

public:
    /**
     * @brief Конструктор менеджера моделей
     * @param apiKey API ключ пользователя для доступа к Roboflow
     * @param parent Родительский объект
     */
    explicit ModelManager(const QString &apiKey, QObject *parent = nullptr);
    
    /**
     * @brief Деструктор
     */
    ~ModelManager();

    /**
     * @brief Отправка запросов ко всем моделям
     * @param imageData Данные изображения в формате QByteArray
     */
    void sendRequestsToAllModels(const QByteArray &imageData);
    
    /**
     * @brief Получение ответа от модели
     * @param modelIndex Индекс модели
     * @return Массив предсказаний QJsonArray
     */
    QJsonArray getModelResponse(int modelIndex) const;
    
    /**
     * @brief Проверка готовности модели
     * @param modelIndex Индекс модели
     * @return true если модель готова, false в противном случае
     */
    bool isModelReady(int modelIndex) const;
    
    /**
     * @brief Получение списка имен моделей
     * @return Список имен моделей QStringList
     */
    QStringList getModelNames() const;
    
    /**
     * @brief Получение количества моделей
     * @return Количество моделей
     */
    int getModelCount() const;

signals:
    /**
     * @brief Сигнал получения ответа от модели
     * @param modelIndex Индекс модели
     * @param predictions Массив предсказаний
     */
    void modelResponseReceived(int modelIndex, const QJsonArray &predictions);
    
    /**
     * @brief Сигнал ошибки модели
     * @param modelIndex Индекс модели
     * @param error Текст ошибки
     */
    void modelErrorOccurred(int modelIndex, const QString &error);
    
    /**
     * @brief Сигнал завершения всех запросов
     */
    void allModelsCompleted();

private slots:
    /**
     * @brief Слот обработки завершения сетевого запроса
     */
    void onNetworkReplyFinished();

private:
    /**
     * @brief Структура информации о модели
     */
    struct ModelInfo
    {
        QString name;        // Имя модели
        QString url;         // URL API модели
        QJsonArray response; // Ответ от модели
        bool isReady;        // Флаг готовности модели
    };

    /**
     * @brief Настройка моделей
     */
    void setupModels();
    
    /**
     * @brief Отправка запроса к конкретной модели
     * @param modelIndex Индекс модели
     * @param imageData Данные изображения
     */
    void sendRequestToModel(int modelIndex, const QByteArray &imageData);

    QString m_apiKey;                        // API ключ Roboflow
    QNetworkAccessManager *m_networkManager; // Менеджер сетевых запросов
    QVector<ModelInfo> m_models;             // Информация о моделях
    int m_completedRequests;                 // Количество завершенных запросов
    int m_totalRequests;                     // Общее количество запросов
};

#endif // MODELMANAGER_H
