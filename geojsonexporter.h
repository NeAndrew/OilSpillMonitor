#ifndef GEOJSONEXPORTER_H
#define GEOJSONEXPORTER_H

#include <QString>
#include <QJsonArray>
#include <QVector>
#include <QSize>
#include <QFile>
#include <QJsonObject>
#include <QImage>

/**
 * @brief Структура для хранения параметров геотрансформации
 */
struct GeoTransform 
{
    // [0] OriginX - X-координата начала системы координат
    // [1] PixelWidth - Ширина пикселя в единицах карты
    // [2] RotationX - Поворот по оси X (обычно 0)
    // [3] OriginY - Y-координата начала системы координат
    // [4] RotationY - Поворот по оси Y (обычно 0)
    // [5] PixelHeight - Высота пикселя в единицах карты (обычно отрицательная)
    QVector<double> coefficients; // Коэффициенты геотрансформации (6 элементов)
    
    QSize rasterSize; // Размер исходного растра
    
    GeoTransform() : coefficients(6, 0.0) {}
    GeoTransform(const QVector<double> &coeffs, const QSize &size)
        : coefficients(coeffs), rasterSize(size) {}
    
    /**
     * @brief Проверка корректности геотрансформации
     * @return true если геотрансформация корректна
     */
    bool isValid() const
    {
        return coefficients.size() == 6 && !rasterSize.isEmpty();
    }
};

/**
 * @brief Класс для экспорта результатов обнаружения нефтяных пятен в GeoJSON формат
 *
 * Класс реализует преобразование пиксельных координат обнаруженных нефтяных пятен
 * в географические координаты и сохранение результатов в формате GeoJSON.
 */
class GeoJsonExporter
{
public:
    /**
     * @brief Конструктор
     */
    GeoJsonExporter();

    /**
     * @brief Экспорт предсказаний в GeoJSON файл
     * @param filePath Путь для сохранения файла
     * @param predictions Массив предсказаний
     * @param geoTransform Параметры геотрансформации
     * @param targetClass Целевой класс для экспорта (по умолчанию "Oil-Spill")
     * @return true если экспорт выполнен успешно
     */
    bool exportToGeoJson(const QString &filePath,
                         const QJsonArray &predictions,
                         const GeoTransform &geoTransform,
                         const QString &targetClass = "Oil-Spill") const;

    /**
     * @brief Преобразование пиксельных координат в географические
     * @param pixel Пиксельные координаты
     * @param geoTransform Параметры геотрансформации
     * @return Географические координаты
     */
    QPointF pixelToGeo(const QPointF &pixel, const GeoTransform &geoTransform) const;
    
private:
    /**
     * @brief Генерация GeoJSON feature для предсказания
     * @param prediction Объект предсказания
     * @param geoTransform Параметры геотрансформации
     * @return Строка с GeoJSON feature
     */
    QString generateFeature(const QJsonObject &prediction, const GeoTransform &geoTransform) const;
    
    /**
     * @brief Форматирование координаты для вывода
     * @param value Значение координаты
     * @return Отформатированная строка
     */
    QString formatCoordinate(double value) const;
};

#endif // GEOJSONEXPORTER_H
