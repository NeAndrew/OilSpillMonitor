#ifndef REPORTGENERATOR_H
#define REPORTGENERATOR_H

#include <QObject>
#include <QString>
#include <QImage>
#include <QDateTime>
#include <QPdfWriter>
#include <QPainter>
#include <QFile>

/**
 * @brief Структура для хранения детальной информации о пятне разлива
 *
 * Содержит основные параметры обнаруженного пятна, включая временные метки,
 * геометрические характеристики и идентификаторы связанных изображений.
 */
struct SpotDetails
{
    QDateTime timestamp;    // Временная метка обнаружения пятна
    double perimeter = 0.0; // Периметр пятна в метрах
    double area = 0.0;      // Площадь пятна в квадратных метрах
    QString coordinates;    // Координаты пятна
    QString centerCoordinates; // Координаты центра пятна
    QString imageId;        // Идентификатор связанного спутникового снимка
};

/**
 * @brief Структура флагов включения различных секций отчёта
 *
 * Позволяет гибко настраивать содержимое отчёта, включая или исключая
 * различные типы информации в зависимости от требований пользователя.
 */
struct ReportIncludeFlags 
{
    bool includeImageId = true;            // Включать ID снимка в отчёт
    bool includeCoordinates = true;        // Включать координаты пятна
    bool includePerimeter = true;          // Включать периметр пятна
    bool includeArea = true;               // Включать площадь пятна
    bool includeTimestamp = true;          // Включать временную метку
    bool includePhotos = true;             // Включать фотографии (vv.png, rgb.png)
    bool includeSpillScreenshots = true;   // Включать изображения прогноза разлива (spill_*)
    bool includeDetectionOverlay = true;   // Включать изображение с обнаруженным пятном
};

/**
 * @brief Структура для хранения всех входных данных генератора отчётов
 *
 * Содержит всю необходимую информацию для создания PDF отчёта:
 * пути к файлам, геометрические данные, детальную информацию о пятне
 * и флаги включения различных секций.
 */
struct ReportData
{
    QString outputFilePath;             // Путь для сохранения PDF файла
    QString appName;                    // Название приложения для заголовка
    QString logoImagePath;              // Путь к логотипу приложения
    QString sentinelImageOriginalPath;  // Путь к оригинальному снимку Sentinel (vv.png)
    QString sentinelImagePolyPath;      // Путь к снимку с полигоном (rgb.png)
    QString detectionOverlayImagePath;  // Путь к изображению с детекцией
    QStringList spillForecastImages;    // Пути к изображениям прогноза (spill_0, 24, 48, 72)
    QPolygonF polygon;                  //  Полигон нефтяного пятна
    SpotDetails details;                // Детальная информация о пятне
    ReportIncludeFlags includeFlags;    //  Флаги включения секций отчёта
};

/**
 * @brief Класс для генерации PDF отчётов о разливах нефти
 *
 * Класс реализует создание PDF файлов с детальной информацией
 * об обнаруженных разливах нефти. Включает в отчёт изображения нефтяного пятна,
 * прогнозы распространения и другую информацию.
 */
class ReportGenerator : public QObject {
    Q_OBJECT

public:
    /**
     * @brief Конструктор генератора отчётов
     *
     * @param data Структура со всеми необходимыми данными для отчёта
     * @param parent Родительский объект QObject (по умолчанию nullptr)
     */
    explicit ReportGenerator(const ReportData& data, QObject *parent = nullptr);
    
    /**
     * @brief Основной метод генерации PDF отчёта
     *
     * @return true если отчёт успешно создан, false в случае ошибки
     */
    bool generate();

private:
    ReportData m_data;  // Входные данные для генерации отчёта

    /**
     * @brief Отрисовка заголовка отчёта с логотипом и названием
     *
     * @param painter Объект для отрисовки
     * @param yOffset Текущая вертикальная позиция (обновляется)
     * @param margin Отступ от краёв страницы
     * @param contentW Ширина области содержимого
     * @param pageH Высота страницы
     * @param bottomMargin Нижний отступ
     * @param writer Объект PDF Writer для управления страницами
     */
    void drawHeader(QPainter& painter,
                    double& yOffset,
                    double margin,
                    double contentW,
                    double pageH,
                    double bottomMargin,
                    QPdfWriter* writer);

    /**
     * @brief Отрисовка двух изображений в одном ряду (оригинал и rdb)
     *
     * @param painter Объект для отрисовки
     * @param yOffset Текущая вертикальная позиция (обновляется)
     * @param margin Отступ от краёв страницы
     * @param contentW Ширина области содержимого
     * @param pageH Высота страницы
     * @param bottomMargin Нижний отступ
     * @param writer Объект PDF Writer для управления страницами
     */
    void drawTwoImagesRow(QPainter& painter,
                          double& yOffset,
                          double margin,
                          double contentW,
                          double pageH,
                          double bottomMargin,
                          QPdfWriter* writer);

    /**
     * @brief Отрисовка изображений прогноза разлива (spill_0, 24, 48, 72)
     *
     * @param painter Объект для отрисовки
     * @param yOffset Текущая вертикальная позиция (обновляется)
     * @param margin Отступ от краёв страницы (обновляется)
     * @param contentW Ширина области содержимого (обновляется)
     * @param pageH Высота страницы (обновляется)
     * @param bottomMargin Нижний отступ (обновляется)
     * @param writer Объект PDF Writer для управления страницами
     */
    void drawSpillScreenshots(QPainter& painter,
                              double& yOffset,
                              double& margin,
                              double& contentW,
                              double& pageH,
                              double& bottomMargin,
                              QPdfWriter* writer);

    /**
     * @brief Отрисовка изображения с обнаруженным пятном (детекция)
     *
     * @param painter Объект для отрисовки
     * @param yOffset Текущая вертикальная позиция (обновляется)
     * @param margin Отступ от краёв страницы (обновляется)
     * @param contentW Ширина области содержимого (обновляется)
     * @param pageH Высота страницы (обновляется)
     * @param bottomMargin Нижний отступ (обновляется)
     * @param writer Объект PDF Writer для управления страницами
     */
    void drawDetectionOverlay(QPainter& painter,
                              double& yOffset,
                              double& margin,
                              double& contentW,
                              double& pageH,
                              double& bottomMargin,
                              QPdfWriter* writer);

    /**
     * @brief Отрисовка таблицы с детальной информацией о пятне
     *
     * @param painter Объект для отрисовки
     * @param yOffset Текущая вертикальная позиция (обновляется)
     * @param margin Отступ от краёв страницы
     * @param contentW Ширина области содержимого
     * @param pageH Высота страницы
     * @param bottomMargin Нижний отступ
     * @param writer Объект PDF Writer для управления страницами
     */
    void drawDetailsTable(QPainter& painter,
                          double& yOffset,
                          double margin,
                          double contentW,
                          double pageH,
                          double bottomMargin,
                          QPdfWriter* writer);
    
    /**
     * @brief Конвертация миллиметров в пункты (единицы PDF)
     *
     * @param mm Значение в миллиметрах
     * @return Значение в пунктах
     */
    static double mmToPoints(double mm);
};

#endif // REPORTGENERATOR_H
