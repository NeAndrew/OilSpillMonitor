#include "drawlinestringtool.h"

// Конструктор
DrawLineStringTool::DrawLineStringTool(QgsMapCanvas *canvas) : QgsMapTool(canvas),
    mCanvas(canvas), mInProgress(false)
{
    setCursor(Qt::CrossCursor); // Устанавливаем перекрестный курсор
}

// Обработка события нажатия кнопки мыши на карте
void DrawLineStringTool::canvasPressEvent(QgsMapMouseEvent *e)
{
    if(e->button() == Qt::LeftButton) // Левая кнопка — добавление точки
    {
        if (!mInProgress)
        {
            mInProgress = true;
            m_points.clear();
        }

        // Преобразуем экранные координаты в координаты карты
        QgsPointXY clickedPoint = mCanvas->getCoordinateTransform()->toMapCoordinates(e->pos());
        m_points.append(clickedPoint);

        // Создаём линию из точек
        QgsGeometry lineGeometry = QgsGeometry::fromPolylineXY(m_points);
        m_currentFeature.setGeometry(lineGeometry);

        mLayer->dataProvider()->deleteFeatures({m_tempFeature.id()});

        m_tempFeature.setGeometry(lineGeometry);
        mLayer->dataProvider()->addFeature(m_tempFeature);

        mCanvas->refresh();

        qDebug() << "Добавлена точка Широта (Latitude):" << clickedPoint.y() << ", Долгота (Longitude):" << clickedPoint.x();
    }
}

// Обработка события отпускания кнопки мыши
void DrawLineStringTool::canvasReleaseEvent(QgsMapMouseEvent *e)
{
    // Правая кнопка мыши — завершить рисование линии
    if (e->button() == Qt::RightButton && !(e->modifiers() & Qt::ControlModifier))
    {
        finishLine(); // Сохраняем линию в слой

        m_points.clear();
        m_currentFeature.setGeometry(QgsGeometry());

        mInProgress = false;
    }
}

// Устанавливает векторный слой, в который будут добавляться линии
void DrawLineStringTool::setLayer(QgsVectorLayer *layer)
{
    mLayer = layer;
    m_tempFeature.setGeometry(QgsGeometry());
    mLayer->dataProvider()->addFeature(m_tempFeature); // Добавляем временную линию в слой
}

// Функция завершает рисование линии
void DrawLineStringTool::finishLine()
{
    if (m_points.size() > 1)
    {
        m_currentFeature.initAttributes(2);
        m_currentFeature.setFields(mLayer->fields());

        // Устанавливаем значения атрибутов
        m_currentFeature.setAttribute("id", nextFeatureId++); // ID
        m_currentFeature.setAttribute("name", "Line");        // Имя линии

        qDebug() << "Текущий ID : " << m_currentFeature.id();
        qDebug() << "Количество атрибутов:" << m_currentFeature.attributeCount();
        qDebug() << "Аттрибут 0 (id):" << m_currentFeature.attribute("id");
        qDebug() << "Аттрибут 1 (name):" << m_currentFeature.attribute("name");

        // Добавляем объект в слой
        mLayer->dataProvider()->addFeature(m_currentFeature);

        mCanvas->refresh();
    }
    else
    {
        qDebug() << "Количество точек не достаточно для создания линии";
    }
}
