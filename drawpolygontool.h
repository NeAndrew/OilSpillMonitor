#ifndef DRAWPOLYGONTOOL_H
#define DRAWPOLYGONTOOL_H

#include <qgsmaptool.h>
#include <qgsmapcanvas.h>
#include <qgsmapmouseevent.h>

// Класс для создания и добавления "Полигона" на QgsMapCanvas
class DrawPolygonTool : public QgsMapTool
{
    Q_OBJECT

public:
    explicit DrawPolygonTool(QgsMapCanvas* canvas);

    // Обработка нажатия клавишы на QgsMapCanvas (карта)
    void canvasPressEvent(QgsMapMouseEvent* e) override;

    // Обработка отпускания клавишы на QgsMapCanvas (карта)
    void canvasReleaseEvent(QgsMapMouseEvent* e) override;

    // Функция установки слоя
    void setLayer(QgsVectorLayer* layer);

    // Текущий слой
    QgsVectorLayer* mLayer = nullptr;

private:
    void finishPolygon();  // Завершаем рисование полигона

    QgsMapCanvas* mCanvas;          // Карта
    bool mInProgress = false;       // Флаг для отслеживания рисуем ли мы полигон
    QVector<QgsPointXY> m_points;   // Контейнер для хранения точек полигона
    QgsFeature m_currentFeature;    // Финальный полигон
    QgsFeature m_tempFeature;       // Временный (отображается при рисовании)
    int nextFeatureId = 1;          // Id
};

#endif // DRAWPOLYGONTOOL_H
