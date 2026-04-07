#ifndef DRAWPOINTTOOL_H
#define DRAWPOINTTOOL_H

#include <qgsmaptool.h>
#include <qgsmapcanvas.h>
#include <qgsmapmouseevent.h>

// Класс для создания и добавления "Точки" на QgsMapCanvas
class DrawPointTool : public QgsMapTool
{
    Q_OBJECT

public:
    DrawPointTool(QgsMapCanvas* canvas);

    // Обработка нажатия клавишы на QgsMapCanvas (карта)
    void canvasPressEvent(QgsMapMouseEvent* e);

    // Функция установки слоя
    void setLayer(QgsVectorLayer* layer);

    // Текущий слой
    QgsVectorLayer* mLayer = nullptr;

private:
    QgsMapCanvas* mCanvas;  // Карта
    int featureId = 1;      // Id
};
#endif // DRAWPOINTTOOL_H
