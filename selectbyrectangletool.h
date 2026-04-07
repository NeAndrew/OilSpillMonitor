#ifndef SELECTBYRECTANGLETOOL_H
#define SELECTBYRECTANGLETOOL_H

// Библиотеки Qt
#include <QKeyEvent>

// Библиотеки QGIS API
#include <qgsmapcanvas.h>
#include <qgsmaptool.h>
#include <qgsmapmouseevent.h>
#include <qgsrubberband.h>
#include <qgsvertexmarker.h>

/**
 * @brief Инструмент для выбора объектов на карте с помощью прямоугольника
 */
class SelectByRectangleTool : public QgsMapTool
{
    Q_OBJECT

public:
    /**
     * @brief Конструктор инструмента выбора
     *
     * @param canvas указатель на объект карты QGIS
     */
    explicit SelectByRectangleTool(QgsMapCanvas* canvas);

    /**
     * @brief Деструктор
     */
    ~SelectByRectangleTool() override;

    /**
     * @brief Обработка нажатия кнопки мыши
     *
     * Начинает процесс выделения прямоугольной области. Реагирует на левую
     * кнопку мыши. Инициализирует начальную точку прямоугольника.
     *
     * @param e событие нажатия кнопки мыши на карте
     */
    void canvasPressEvent(QgsMapMouseEvent* e) override;

    /**
     * @brief Обработка движения мыши
     *
     * Обновляет размер и положение прямоугольника выделения во время перетаскивания.
     *
     * @param e событие движения мыши на карте
     */
    void canvasMoveEvent(QgsMapMouseEvent* e) override;

    /**
     * @brief Обработка отпускания кнопки мыши
     *
     * Завершает процесс выделения и выполняет выбор объектов во всех векторных
     * слоях проекта, которые пересекаются с прямоугольной областью.
     *
     * @param e событие отпускания кнопки мыши на карте
     */
    void canvasReleaseEvent(QgsMapMouseEvent* e) override;

    /**
     * @brief Обработка нажатия клавиш
     *
     * Обрабатывает нажатие клавиши Escape для отмены выделения и деактивации инструмента.
     *
     * @param e событие нажатия клавиши
     */
    void keyPressEvent(QKeyEvent* e) override;

protected:
    /**
     * @brief Вызывается при деактивации инструмента
     *
     * Очищает все визуальные элементы (резиновую ленту, маркеры вершин) и
     * освобождает ресурсы перед деактивацией инструмента.
     */
    void deactivate() override;

private:
    /**
     * @brief Обновляет визуализацию прямоугольника выделения
     *
     * Пересчитывает координаты углов прямоугольника на основе начальной и конечной
     * точек и обновляет резиновую ленту.
     *
     * @param startPoint начальная точка прямоугольника
     * @param endPoint конечная точка прямоугольника
     */
    void updateRubberBand(const QgsPointXY& startPoint, const QgsPointXY& endPoint);

    /**
     * @brief Выполняет выбор объектов в прямоугольной области
     *
     * Проходит по всем векторным слоям проекта и выбирает объекты, которые
     * пересекаются с заданным прямоугольником.
     *
     * @param rect прямоугольная область для выбора объектов
     */
    void selectFeaturesInRectangle(const QgsRectangle& rect);

    /**
     * @brief Очищает все маркеры вершин, добавленные на карту
     *
     * Удаляет все объекты QgsVertexMarker, хранящиеся в списке mVertexMarkers,
     * из сцен canvas и освобождает выделенную память. Используется для сброса
     * всех подсветок вершин перед новым выделением или деактивацией инструмента.
     */
    void clearVertexMarkers();


private:
    QgsPointXY mStartPoint;                             // Начальная точка прямоугольника выделения
    QgsPointXY mEndPoint;                               // Конечная точка прямоугольника выделения
    bool mDragging = false;                             // Флаг активного процесса выделения
    QList<QgsVertexMarker*> mVertexMarkers;             // Список маркеров вершин для подсветки
    std::unique_ptr<QgsRubberBand> mRubberBand;         // Указатель на объект прямоугольника
};

#endif // SELECTBYRECTANGLETOOL_H
