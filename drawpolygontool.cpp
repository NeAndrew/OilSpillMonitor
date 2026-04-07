#include "drawpolygontool.h"

// Конструктор
DrawPolygonTool::DrawPolygonTool(QgsMapCanvas* canvas)
    : QgsMapTool(canvas)
    , mCanvas(canvas)
{
    setCursor(Qt::CrossCursor); // Устанавливает перекрестный курсор
}

// Устанавливаем векторный слой, в который будет добавляться полигон
void DrawPolygonTool::setLayer(QgsVectorLayer* layer)
{
    mLayer = layer;
    if (mLayer)
    {
        m_tempFeature.setGeometry(QgsGeometry());
        mLayer->dataProvider()->addFeature(m_tempFeature);
    }
}

// Обработка нажатия кнопки мыши — добавление вершин полигона
void DrawPolygonTool::canvasPressEvent(QgsMapMouseEvent* e)
{
    if (e->button() != Qt::LeftButton) // Обрабатываем только левую кнопку
    {
        return;
    }

    if (!mInProgress)
    {
        mInProgress = true;
        m_points.clear();
        m_currentFeature.setGeometry(QgsGeometry());
    }

    // Получаем точку в координатах карты
    QgsPointXY point = e->mapPoint();
    m_points.append(point); // Добавляем точку в список вершин

    if (m_points.size() >= 2)
    {
        QgsGeometry geom = QgsGeometry::fromPolygonXY({m_points}); // Создаём временную геометрию
        m_tempFeature.setGeometry(geom);
        mLayer->dataProvider()->changeGeometryValues({ {m_tempFeature.id(), geom} });
    }

    mCanvas->refresh();
    qDebug() << "Добавлена точка:" << point.x() << "," << point.y();
}

// Обработка отпускания кнопки мыши
void DrawPolygonTool::canvasReleaseEvent(QgsMapMouseEvent* e)
{
    if (e->button() == Qt::RightButton && mInProgress) // Правая кнопка — завершение рисования
    {
        finishPolygon(); // Создаём и сохраняем полигон

        // Сбрасываем данные для нового полигона
        m_points.clear();
        m_currentFeature.setGeometry(QgsGeometry());
        mInProgress = false;

        mCanvas->refresh();
    }
}

// Функция завершает рисование полигона
void DrawPolygonTool::finishPolygon()
{
    if (m_points.size() < 3)
    {
        qDebug() << "Недостаточно точек для полигона (минимум 3)";
        return;
    }

    // Замыкаем полигон, добавляя первую точку в конец
    if (m_points.first() != m_points.last())
    {
        m_points.append(m_points.first());
    }

    // Создаём геометрию полигона из точек
    QgsGeometry polygonGeom = QgsGeometry::fromPolygonXY({m_points});

    if (!polygonGeom.isGeosValid())
    {
        qDebug() << "Геометрия полигона недействительна!";
        return;
    }

    m_currentFeature.setGeometry(polygonGeom);

    m_currentFeature.initAttributes(2);
    m_currentFeature.setAttribute("id", nextFeatureId++);   // Уникальный ID
    m_currentFeature.setAttribute("name", "Polygon");       // Имя объекта

    if (mLayer->dataProvider()->addFeature(m_currentFeature))
    {
        qDebug() << "Полигон добавлен, ID:" << m_currentFeature.id();
    }
    else
    {
        qDebug() << "Ошибка добавления полигона.";
    }

    mLayer->dataProvider()->deleteFeatures({m_tempFeature.id()});
    m_tempFeature.setGeometry(QgsGeometry());
    mLayer->dataProvider()->addFeature(m_tempFeature);

    // Обновляем границы слоя и перерисовываем его
    mLayer->updateExtents();
    mLayer->triggerRepaint();
}
