#ifndef OILSPREADANALYSIS_H
#define OILSPREADANALYSIS_H

// Библиотеки Qt
#include <QNetworkReply>
#include <QJsonArray>
#include <QUrlQuery>
#include <QtConcurrentRun>
#include <QtMath>

// Библиотеки QGIS API
#include <qgsvectorlayer.h>
#include <qgsvectorfilewriter.h>
#include <qgsspatialindex.h>
#include <qgsprocessingcontext.h>

/**
 * @struct ParticleState
 * @brief Состояние частицы нефти в определенный момент времени
 *
 * Хранит информацию о положении частицы, времени симуляции
 * и статусе (на суше или в воде)
 */
struct ParticleState
{
    QPointF position; // Положение частицы в координатах EPSG:3857
    int hour;         // Час с начала симуляции
    bool onShore;     // Флаг нахождения на суше
};

/**
 * @struct WeatherHour
 * @brief Метеорологические данные для одного часа
 *
 * Содержит информацию о скорости и направлении ветра,
 * а также о скорости и направлении морских течений
 */
struct WeatherHour 
{
    double windSpeed = 0.0;         // Скорость ветра (м/с)
    double windDirection = 0.0;     // Направление ветра (градусы)
    double currentSpeed = 0.0;      // Скорость течения (м/с)
    double currentDirection = 0.0;  // Направление течения (градусы)
};

/**
 * @struct OilSpreadAnalysisConfig
 * @brief Конфигурация параметров анализа распространения нефти
 *
 * Содержит все необходимые параметры для запуска симуляции
 * распространения нефтяного пятна
 */
struct OilSpreadAnalysisConfig 
{
    QString spillGeoJsonPath;       // Путь к файлу с разливом в формате GeoJSON
    QDateTime startTime;            // Время начала разлива
    double bufferMeters = 100000.0; // Размер буферной зоны для метеоданных (метры)
    bool highResLand = false;       // Использовать высокое разрешение береговой линии
    double weatherLatStep = 0.1;    // Шаг метеоданных по широте (градусы)
    double weatherLonStep = 0.1;    // Шаг метеоданных по долготе (градусы)
    
    /**
     * @brief Проверяет валидность конфигурации
     * @return true если конфигурация корректна, иначе false
     */
    bool isValid() const;
    
    /**
     * @brief Возвращает сообщение об ошибке
     * @return Текстовое описание ошибки или пустая строка
     */
    QString errorMessage() const;
};

/**
 * @class OilSpreadAnalysis
 * @brief Основной класс для моделирования распространения нефти
 *
 * Класс выполняет симуляцию распространения нефтяного пятна с учетом
 * метеорологических условий, морских течений и береговой линии.
 * Использует частицы для моделирования движения нефти.
 */
class OilSpreadAnalysis : public QObject
{
    Q_OBJECT
public:
    /**
     * @brief Конструктор класса анализа распространения нефти
     * @param config Конфигурация параметров анализа
     * @param parent Родительский QObject
     */
    explicit OilSpreadAnalysis(const OilSpreadAnalysisConfig &config, QObject *parent = nullptr);

    /**
     * @brief Проверяет валидность объекта анализа
     * @return true если объект корректно инициализирован, иначе false
     */
    bool isValid() const;
    
    /**
     * @brief Возвращает сообщение об ошибке инициализации
     * @return Текстовое описание ошибки
     */
    QString errorMessage() const;

    /**
     * @brief Запускает симуляцию распространения нефти
     * @param outputPath Путь для сохранения результатов в формате GeoJSON
     * @param density Плотность частиц (точек на км²)
     */
    void run(const QString &outputPath, int density);

signals:
    /**
     * @brief Сигнал завершения симуляции
     * @param ok true если симуляция успешна, иначе false
     * @param error Сообщение об ошибке (если есть)
     */
    void finished(bool ok, const QString &error);

private:
    /**
     * @brief Загружает слой с разливом нефти
     * @return true если слой успешно загружен, иначе false
     */
    bool loadSpillLayer();
    
    /**
     * @brief Загружает слой береговой линии
     * @return true если слой успешно загружен, иначе false
     */
    bool loadLandLayer();
    
    /**
     * @brief Проверяет пересечение разлива с сушей
     * @return true если разлив не пересекает сушу, иначе false
     */
    bool checkNoIntersectionWithLand();

    /**
     * @brief Генерирует начальные точки внутри полигона разлива
     * @param density Плотность точек (точек на км²)
     * @return Список начальных точек
     */
    QList<QPointF> generateInitialPoints(int density);

    /**
     * @brief Генерирует кэш метеорологических данных
     * @param latStep Шаг по широте для сетки погодных данных
     * @param lonStep Шаг по долготе для сетки погодных данных
     * @return true если кэш успешно сгенерирован, иначе false
     */
    bool generateWeatherCache(double latStep, double lonStep);
    
    /**
     * @brief Получает погодные данные для указанных координат и времени
     * @param x_epsg3857 Координата X в EPSG:3857
     * @param y_epsg3857 Координата Y в EPSG:3857
     * @param hour Час с начала симуляции
     * @return Погодные данные для указанного времени
     */
    WeatherHour weatherAt(double x_epsg3857, double y_epsg3857, int hour) const;

    /**
     * @brief Симулирует движение одной частицы
     * @param startPt Начальная точка частицы
     * @param trajectory Вектор для сохранения траектории
     */
    void simulateParticle(const QPointF &startPt, QList<ParticleState> &trajectory);
    
    /**
     * @brief Запускает полную симуляцию
     * @param outputPath Путь для сохранения результатов
     * @param density Плотность частиц
     * @return true если симуляция успешна, иначе false
     */
    bool runSimulation(const QString &outputPath, int density);
    
    /**
     * @brief Сохраняет траектории в файл GeoJSON
     * @param all Список всех траекторий частиц
     * @param path Путь для сохранения файла
     */
    void saveTrajectoriesToGeoJSON(const QList<QList<ParticleState>> &all, const QString &path);

    // Конфигурация параметров анализа
    OilSpreadAnalysisConfig m_config;
    
    // Слой с геометрией разлива нефти
    std::unique_ptr<QgsVectorLayer> m_spillLayer;
    
    // Слой с береговой линией
    std::unique_ptr<QgsVectorLayer> m_landLayer;
    
    // Пространственный индекс для быстрого поиска пересечений с сушей
    std::unique_ptr<QgsSpatialIndex> m_landSpatialIndex;

    // Флаг валидности объекта
    bool m_valid = false;
    
    // Сообщение об ошибке инициализации
    QString m_errorMessage;

    // Контекст обработки QGIS
    std::unique_ptr<QgsProcessingContext> m_processingContext;
    
    // Объект обратной связи для операций обработки
    std::unique_ptr<QgsProcessingFeedback> m_processingFeedback;

    // Кэш погодных данных: ключ = координаты узла (lat, lon)
    QMap<QPair<double,double>, QList<WeatherHour>> m_weatherCache;

    // Продолжительность симуляции в часах
    static constexpr int SIMULATION_HOURS = 72;

    // Трансформатор координат из EPSG:3857 в EPSG:4326
    mutable std::unique_ptr<QgsCoordinateTransform> m_coordTransform;
};

#endif // OILSPREADANALYSIS_H
