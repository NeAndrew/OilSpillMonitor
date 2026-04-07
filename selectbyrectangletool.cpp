#include "selectbyrectangletool.h"

// Константы для настройки внешнего вида
namespace
{
const QColor RUBBER_BAND_COLOR(0, 128, 255, 100);      // Цвет контура прямоугольника
const QColor RUBBER_BAND_FILL_COLOR(0, 128, 255, 50);  // Цвет заливки прямоугольника
const int RUBBER_BAND_WIDTH = 2;                       // Толщина линии прямоугольника
}

SelectByRectangleTool::SelectByRectangleTool(QgsMapCanvas* canvas)
    : QgsMapTool(canvas)
{
    // Проверка валидности указателя на карту
    if (!canvas)
    {
        qDebug() << "Ошибка: передан невалидный указатель на карту";
        return;
    }

    // Устанавливаем перекрёстный курсор для визуального указания режима выделения
    setCursor(Qt::CrossCursor);

    // Создаём резиновую ленту для визуализации прямоугольника выделения
    mRubberBand = std::make_unique<QgsRubberBand>(canvas, Qgis::GeometryType::Polygon);

    // Настраиваем внешний вид резиновой ленты
    mRubberBand->setColor(RUBBER_BAND_COLOR);
    mRubberBand->setFillColor(RUBBER_BAND_FILL_COLOR);
    mRubberBand->setWidth(RUBBER_BAND_WIDTH);
    mRubberBand->hide(); // Изначально скрыта
}

SelectByRectangleTool::~SelectByRectangleTool()
{
    clearVertexMarkers();
}

void SelectByRectangleTool::canvasPressEvent(QgsMapMouseEvent* e)
{
    // Проверка валидности события
    if (!e)
    {
        return;
    }

    // Реагируем только на левую кнопку мыши
    if (e->button() != Qt::LeftButton)
    {
        return;
    }

    // Инициализируем начальную и конечную точки (пока одинаковые)
    mStartPoint = e->mapPoint();
    mEndPoint = e->mapPoint();
    mDragging = true;

    // Сбрасываем и настраиваем резиновую ленту для нового прямоугольника
    mRubberBand->reset(Qgis::GeometryType::Polygon);

    // Создаём начальный прямоугольник (точка) - четыре одинаковые точки
    // Это необходимо для корректной работы резиновой ленты
    mRubberBand->addPoint(mStartPoint, false); // false — не замыкать полигон
    mRubberBand->addPoint(mStartPoint, false);
    mRubberBand->addPoint(mStartPoint, false);
    mRubberBand->addPoint(mStartPoint, true);  // true — замыкаем полигон
    mRubberBand->show();
}

void SelectByRectangleTool::canvasMoveEvent(QgsMapMouseEvent* e)
{
    // Проверка валидности события
    if (!e)
    {
        return;
    }

    // Игнорируем движение мыши, если не идёт процесс выделения
    if (!mDragging)
    {
        return;
    }

    // Обновляем конечную точку
    mEndPoint = e->mapPoint();

    // Обновляем визуализацию прямоугольника
    updateRubberBand(mStartPoint, mEndPoint);

    // Перерисовываем карту для обновления резиновой ленты
    canvas()->refresh();
}

void SelectByRectangleTool::canvasReleaseEvent(QgsMapMouseEvent* e)
{
    // Проверка валидности события
    if (!e)
    {
        return;
    }

    // Реагируем только на левую кнопку мыши и только при активном выделении
    if (e->button() != Qt::LeftButton || !mDragging)
    {
        return;
    }

    // Завершаем процесс выделения
    mDragging = false;

    // Создаём нормализованный прямоугольник из двух точек
    QgsRectangle rect(mStartPoint, mEndPoint);
    rect.normalize(); // Упорядочиваем координаты

    // Выполняем выбор объектов в прямоугольной области
    selectFeaturesInRectangle(rect);

    // Обновляем отображение карты для показа выбранных объектов
    canvas()->refresh();

    // Скрываем оверлей слой после завершения выделения
    mRubberBand->reset(Qgis::GeometryType::Polygon);
    mRubberBand->hide();
}

void SelectByRectangleTool::keyPressEvent(QKeyEvent* e)
{
    // Проверка валидности события
    if (!e)
    {
        return;
    }

    // Нажатие Escape — отмена выделения и выход из инструмента
    if (e->key() == Qt::Key_Escape)
    {
        // Очищаем визуальные элементы
        mVertexMarkers.clear();
        if (mRubberBand)
        {
            mRubberBand->hide();
        }

        // Деактивируем инструмент
        canvas()->unsetMapTool(this);
    }
}

void SelectByRectangleTool::deactivate()
{
    // Вызываем базовую реализацию
    QgsMapTool::deactivate();

    // Очищаем все визуальные элементы
    mVertexMarkers.clear();

    // Скрываем и сбрасываем резиновую ленту
    if (mRubberBand)
    {
        mRubberBand->hide();
        mRubberBand.reset();
    }
}

void SelectByRectangleTool::clearVertexMarkers()
{
    for (QgsVertexMarker* marker : std::as_const(mVertexMarkers))
    {
        if (marker)
        {
            canvas()->scene()->removeItem(marker);
            delete marker;
        }
    }
    mVertexMarkers.clear();
}

void SelectByRectangleTool::updateRubberBand(const QgsPointXY& startPoint, const QgsPointXY& endPoint)
{
    // Проверка валидности резиновой ленты
    if (!mRubberBand)
    {
        return;
    }

    // Сбрасываем текущий прямоугольник
    mRubberBand->reset(Qgis::GeometryType::Polygon);

    // Вычисляем четыре угла прямоугольника
    QgsPointXY topLeft(startPoint.x(), startPoint.y());
    QgsPointXY topRight(endPoint.x(), startPoint.y());
    QgsPointXY bottomRight(endPoint.x(), endPoint.y());
    QgsPointXY bottomLeft(startPoint.x(), endPoint.y());

    // Добавляем углы в резиновую ленту для формирования прямоугольника
    mRubberBand->addPoint(topLeft, false);
    mRubberBand->addPoint(topRight, false);
    mRubberBand->addPoint(bottomRight, false);
    mRubberBand->addPoint(bottomLeft, true); // Замыкаем полигон
}

void SelectByRectangleTool::selectFeaturesInRectangle(const QgsRectangle& rect)
{
    // Проверка валидности прямоугольника
    if (rect.isEmpty())
    {
        qDebug() << "Предупреждение: пустой прямоугольник выделения";
        return;
    }

    // Получаем все слои из текущего проекта
    const auto mapLayers = QgsProject::instance()->mapLayers();
    if (mapLayers.isEmpty())
    {
        qDebug() << "Информация: в проекте нет слоёв";
        return;
    }

    int selectedCount = 0;

    // Перебираем все слои проекта
    for (auto* layer : mapLayers)
    {
        // Приводим слой к векторному типу
        QgsVectorLayer* vectorLayer = qobject_cast<QgsVectorLayer*>(layer);
        
        // Проверяем, что слой векторный и имеет пространственные объекты
        if (!vectorLayer || !vectorLayer->isValid() || !vectorLayer->isSpatial())
        {
            continue;
        }

        // Выполняем выборку объектов, пересекающих прямоугольник
        QgsRectangle rectTmp = rect;
        vectorLayer->selectByRect(rectTmp, Qgis::SelectBehavior::AddToSelection);
        
        // Подсчитываем количество выбранных объектов
        selectedCount += vectorLayer->selectedFeatureCount();
    }

    qDebug() << "Выбрано объектов:" << selectedCount;
}
