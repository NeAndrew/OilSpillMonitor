#ifndef MEASUREMENTTOOL_H
#define MEASUREMENTTOOL_H

#include <QObject>

#include <qgsmapcanvas.h>
#include <qgsmaptoolemitpoint.h>
#include <qgsmapmouseevent.h>
#include <qgsrubberband.h>
#include <qgsdistancearea.h>

/**
 * @brief Инструмент измерения расстояния, площади и угла на карте
 *
 * Класс реализует пользовательский инструмент для интерактивных измерений.
 * Поддерживаются три режима:
 *  - Distance (измерение длины ломаной)
 *  - Area     (измерение площади полигона)
 *  - Angle    (измерение угла по трём точкам)
 */
class MeasurementTool : public QgsMapToolEmitPoint
{
    Q_OBJECT

public:
    /**
     * @brief Режим измерения
     */
    enum Mode
    {
        Distance, // Измерение расстояния
        Area,     // Измерение площади
        Angle     // Измерение угла
    };

    /**
     * @brief Конструктор MeasurementTool
     * @param canvas Указатель на QgsMapCanvas
     * @param mode Начальный режим измерения
     */
    explicit MeasurementTool(QgsMapCanvas* canvas, Mode mode = Distance);

    /**
     * @brief Деструктор
     */
    ~MeasurementTool() override;

    /**
     * @brief Функция установки режима измерения
     * @param mode Новый режим
     */
    void setMode(Mode mode);

    /**
     * @brief Функция получения текущего режима измерения
     * @return Текущий режим
     */
    Mode mode() const;

    /**
     * @brief Функция сброса текущего измерения
     *
     * Очищает точки, удаляет rubber band и возвращает инструмент
     * в исходное состояние.
     */
    void reset();

protected:
    // Обработка отпускания кнопки мыши
    void canvasReleaseEvent(QgsMapMouseEvent* e) override;

    // Обработка перемещения мыши
    void canvasMoveEvent(QgsMapMouseEvent* e) override;

    // Деактивация инструмента
    void deactivate() override;

signals:
    /**
     * @brief Сигнал изменения расстояния
     * @param totalKm Общая длина, км
     * @param lastSegmentKm Длина последнего сегмента, км
     */
    void distanceChanged(double totalKm, double lastSegmentKm);

    /**
     * @brief Сигнал изменения площади
     * @param squareMeters Площадь в квадратных метрах
     */
    void areaChanged(double squareMeters);

    /**
     * @brief Сигнал изменения угла
     * @param degrees Угол в градусах
     */
    void angleChanged(double degrees);

    /**
     * @brief Сигнал сброса измерений
     */
    void measurementReset();

private:
    // Функции завершения измерения
    void finalizeDistance();
    void finalizeArea();
    void finalizeAngle();

    // Функции динамического обновления
    void updateDistancePreview();
    void updateAreaPreview();
    void updateAnglePreview();

    // Создает объект для измерений
    QgsDistanceArea createDistanceArea() const;

    // Функция вычисления угла по трем точками
    double calculateAngleDeg(const QgsPointXY& A, const QgsPointXY& B, const QgsPointXY& C);

private:
    QgsMapCanvas* mCanvas = nullptr;   // Карта
    QgsRubberBand* mRubberBand = nullptr; // Визуализация геометрии

    Mode mMode = Distance;             // Текущий режим

    QVector<QgsPointXY> mPoints;       // Зафиксированные точки
    QgsPointXY mTempPoint;             // Временная точка (мышь)
    bool mHasTempPoint = false;        // Признак наличия временной точки
    bool mMeasurementFinished = false; // Завершено ли измерение

};

#endif // MEASUREMENTTOOL_H
