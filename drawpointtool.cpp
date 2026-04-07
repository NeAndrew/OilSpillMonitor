#include "drawpointtool.h"

// Конструктор
DrawPointTool::DrawPointTool(QgsMapCanvas *canvas)
    : QgsMapTool(canvas), mCanvas(canvas) {}

// Обработка события нажатия кнопки мыши на карте
void DrawPointTool::canvasPressEvent(QgsMapMouseEvent *e)
{
    // Обрабатываем только левую кнопку мыши, остальные игнорируем
    if (!(e->button() == Qt::LeftButton))
    {
        return;
    }

    // Преобразуем экранные координаты клика в координаты карты
    QgsPointXY mapPoint = mCanvas->getCoordinateTransform()->toMapCoordinates(e->pos());

    qDebug() << "Точка Широта (Latitude):" << mapPoint.y() << ", Долгота (Longitude):" << mapPoint.x();


    QgsGeometry pointGeometry = QgsGeometry::fromPointXY(mapPoint);

    // Создаём новый объект для добавления в слой
    QgsFeature feature;
    feature.initAttributes(2);
    feature.setFields(mLayer->fields());
    feature.setAttribute("id", featureId++); // ID
    feature.setAttribute("name", "New Point"); // Имя точки
    feature.setGeometry(pointGeometry);

    mLayer->dataProvider()->addFeature(feature);

    mCanvas->refresh();
}

// Функция устанавливает векторный слой, в который будут добавляться точки
void DrawPointTool::setLayer(QgsVectorLayer *layer)
{
    mLayer = layer;
}
