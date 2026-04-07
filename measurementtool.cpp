#include "measurementtool.h"

// Константы для стилей инструментов измерения
namespace
{
constexpr QColor RUBBER_LINE_COLOR(0, 255, 0);
constexpr QColor RUBBER_FILL_COLOR(0, 255, 0, 40);
constexpr int RUBBER_WIDTH = 2;
}

// Инициализация инструмента измерения
MeasurementTool::MeasurementTool(QgsMapCanvas* canvas, Mode mode)
    : QgsMapToolEmitPoint(canvas)
    , mCanvas(canvas)
    , mMode(mode)
{
    if (!mCanvas)
    {
        return;
    }

    // Устанавливаем курсор в виде крестика
    setCursor(Qt::CrossCursor);
    reset();
}

MeasurementTool::~MeasurementTool()
{
    delete mRubberBand;
    mRubberBand = nullptr;
}

void MeasurementTool::setMode(Mode mode)
{
    if (mMode == mode)
    {
        return;
    }

    mMode = mode;
    reset();
}

MeasurementTool::Mode MeasurementTool::mode() const
{
    return mMode;
}

// Сброс состояния инструмента и подготовка к новому измерению
void MeasurementTool::reset()
{
    // Очищаем все точки и флаги
    mPoints.clear();
    mHasTempPoint = false;
    mMeasurementFinished = false;

    // Удаляем старый оверлей слой
    delete mRubberBand;
    mRubberBand = nullptr;

    emit measurementReset();

    if (!mCanvas || !mCanvas->scene())
    {
        return;
    }

    // Определяем тип геометрии в зависимости от режима измерения
    Qgis::GeometryType geomType = Qgis::GeometryType::Line;

    if (mMode == Area)
    {
        geomType = Qgis::GeometryType::Polygon;
    }

    // Создаем новый оверлей слой с нужными параметрами стиля
    mRubberBand = new QgsRubberBand(mCanvas, geomType);
    mRubberBand->setColor(RUBBER_LINE_COLOR);
    mRubberBand->setFillColor(RUBBER_FILL_COLOR);
    mRubberBand->setWidth(RUBBER_WIDTH);
}

void MeasurementTool::deactivate()
{
    delete mRubberBand;
    mRubberBand = nullptr;

    QgsMapToolEmitPoint::deactivate();
}

// Обработка нажатия кнопки мыши
void MeasurementTool::canvasReleaseEvent(QgsMapMouseEvent* e)
{
    if (!e)
    {
        return;
    }

    // Второй правый клик — полный сброс измерения
    if (e->button() == Qt::RightButton && mMeasurementFinished)
    {
        reset();
        return;
    }

    // Преобразуем экранные координаты в географические
    QgsPointXY point = toMapCoordinates(e->pos());

    if (e->button() == Qt::LeftButton && !mMeasurementFinished)
    {
        // Добавляем точку при левом клике
        mPoints.append(point);

        // Для измерения угла достаточно 3 точек
        if (mMode == Angle && mPoints.size() == 3)
        {
            finalizeAngle();
            mMeasurementFinished = true;
        }
    }
    else if (e->button() == Qt::RightButton && !mMeasurementFinished)
    {
        // Правый клик завершает измерение
        if (mMode == Angle)
        {
            // Для угла - сброс после первого правого клика
            reset();
            return;
        }
        else if (mMode == Distance && mPoints.size() >= 2)
        {
            finalizeDistance();
        }
        else if (mMode == Area && mPoints.size() >= 3)
        {
            finalizeArea();
        }

        mMeasurementFinished = true;
    }
}

// Обработка движения мыши - обновление предварительного просмотра
void MeasurementTool::canvasMoveEvent(QgsMapMouseEvent* e)
{
    if (!e || mPoints.isEmpty() || mMeasurementFinished)
    {
        return;
    }

    // Сохраняем временную позицию курсора
    mTempPoint = toMapCoordinates(e->pos());
    mHasTempPoint = true;

    // Обновляем preview в зависимости от режима измерения
    switch (mMode)
    {
    case Distance: updateDistancePreview(); break;
    case Area:     updateAreaPreview();     break;
    case Angle:    updateAnglePreview();    break;
    }
}

void MeasurementTool::updateDistancePreview()
{
    if (!mRubberBand)
    {
        return;
    }

    mRubberBand->reset(Qgis::GeometryType::Line);

    for (const auto& p : std::as_const(mPoints))
    {
        mRubberBand->addPoint(p);
    }

    mRubberBand->addPoint(mTempPoint);

    QgsDistanceArea da = createDistanceArea();

    double total = 0.0;
    for (int i = 1; i < mPoints.size(); ++i)
    {
        total += da.measureLine(mPoints[i - 1], mPoints[i]);
    }

    double last = da.measureLine(mPoints.last(), mTempPoint);

    emit distanceChanged(da.convertLengthMeasurement(total + last, Qgis::DistanceUnit::Kilometers), da.convertLengthMeasurement(last, Qgis::DistanceUnit::Kilometers));
}

void MeasurementTool::finalizeDistance()
{
    QgsDistanceArea da = createDistanceArea();

    double total = 0.0;
    for (int i = 1; i < mPoints.size(); ++i)
    {
        total += da.measureLine(mPoints[i - 1], mPoints[i]);
    }

    emit distanceChanged(da.convertLengthMeasurement(total, Qgis::DistanceUnit::Kilometers), 0.0);
}

void MeasurementTool::updateAreaPreview()
{
    if (!mRubberBand || mPoints.size() < 2)
    {
        return;
    }

    QVector<QgsPointXY> pts = mPoints;
    pts << mTempPoint << mPoints.first();

    mRubberBand->reset(Qgis::GeometryType::Polygon);

    for (const auto& p : std::as_const(pts))
    {
        mRubberBand->addPoint(p);
    }

    QgsPolygonXY poly;
    poly.append(pts);

    QgsGeometry geom = QgsGeometry::fromPolygonXY(poly);
    if (!geom.isEmpty())
    {
        emit areaChanged(createDistanceArea().measureArea(geom));
    }
}

void MeasurementTool::finalizeArea()
{
    QgsPolygonXY poly;
    poly.append(mPoints);

    QgsGeometry geom = QgsGeometry::fromPolygonXY(poly);
    emit areaChanged(createDistanceArea().measureArea(geom));
}

void MeasurementTool::updateAnglePreview()
{
    if (!mRubberBand || mPoints.size() < 1)
    {
        return;
    }

    mRubberBand->reset(Qgis::GeometryType::Line);

    if (mPoints.size() == 1)
    {
        mRubberBand->addPoint(mPoints[0]);
        mRubberBand->addPoint(mTempPoint);
        return;
    }

    const QgsPointXY& A = mPoints[0];
    const QgsPointXY& B = mPoints[1];
    const QgsPointXY& C = mTempPoint;

    mRubberBand->addPoint(B);
    mRubberBand->addPoint(A);
    mRubberBand->addPoint(B);
    mRubberBand->addPoint(C);

    emit angleChanged(calculateAngleDeg(A, B, C));
}

void MeasurementTool::finalizeAngle()
{
    if (mPoints.size() < 3)
    {
        return;
    }

    emit angleChanged(calculateAngleDeg(mPoints[0], mPoints[1], mPoints[2]));
}

QgsDistanceArea MeasurementTool::createDistanceArea() const
{
    QgsDistanceArea da;
    // Устанавливаем систему координат из настроек карты
    da.setSourceCrs(mCanvas->mapSettings().destinationCrs(), QgsProject::instance()->transformContext());
    // Устанавливаем эллипсоид для точных расчетов
    da.setEllipsoid(QgsProject::instance()->ellipsoid());
    return da;
}

// Расчет угла между тремя точками A-B-C в градусах
double MeasurementTool::calculateAngleDeg(const QgsPointXY& A, const QgsPointXY& B, const QgsPointXY& C)
{
    // Векторы BA и BC
    double v1x = A.x() - B.x();
    double v1y = A.y() - B.y();
    double v2x = C.x() - B.x();
    double v2y = C.y() - B.y();

    // Длины векторов
    double len1 = std::hypot(v1x, v1y);
    double len2 = std::hypot(v2x, v2y);

    // Проверка на нулевые векторы
    if (len1 == 0.0 || len2 == 0.0)
    {
        return 0.0;
    }

    // Косинус угла через скалярное произведение
    double cosA = (v1x * v2x + v1y * v2y) / (len1 * len2);
    // Ограничиваем значение для избежания ошибок из-за погрешностей вычислений
    cosA = std::clamp(cosA, -1.0, 1.0);

    // Переводим радианы в градусы
    return std::acos(cosA) * 180.0 / M_PI;
}
