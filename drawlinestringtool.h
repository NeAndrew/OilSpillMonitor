#ifndef DRAWLINESTRINGTOOL_H
#define DRAWLINESTRINGTOOL_H

#include <qgsmaptool.h>
#include <qgsmapcanvas.h>
#include <qgsmapmouseevent.h>

// Класс для создания и добавления "Линии" на QgsMapCanvas
class DrawLineStringTool : public QgsMapTool
{
    Q_OBJECT
public:
    DrawLineStringTool(QgsMapCanvas* canvas);

    // Обработка нажатия клавишы на QgsMapCanvas (карта)
    void canvasPressEvent(QgsMapMouseEvent* e) override;

    // Обработка отпускания клавишы на QgsMapCanvas (карта)
    void canvasReleaseEvent(QgsMapMouseEvent *e) override;

    // Функция установки слоя
    void setLayer(QgsVectorLayer* layer);

    // Текущий слой
    QgsVectorLayer *mLayer = nullptr;

private:
    // Функция завершения рисования линии
    void finishLine();

    QgsMapCanvas *mCanvas;          // Карта
    bool mInProgress;               // Флаг для отслеживания рисуем ли мы линию
    QVector<QgsPointXY> m_points;   // Вектор для хранення точек для линии
    QgsFeature m_currentFeature;    // Атрибуты
    QgsFeature m_tempFeature;       // Атрибуты (временные)
    int nextFeatureId = 1;          // Счетчик id слоя
};

#endif // DRAWLINESTRINGTOOL_H
