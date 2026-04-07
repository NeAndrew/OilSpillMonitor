#include "qgsvertextool.h"

/**
 * @brief Хэш-функция для вершины. Комбинирует хэши слоя, ID объекта и ID вершины
 * @param v Вершина для которой вычисляется хэш
 * @return Хэш-значение вершины
 */
uint qHash(const Vertex &v)
{
    return qHash(v.layer) ^ qHash(v.fid) ^ qHash(v.vertexId);
}

/**
 * @brief Проверяет, является ли вершина конечной точкой линии
 * @param geom Геометрия для проверки
 * @param vertexIndex Индекс вершины
 * @return true если вершина является конечной точкой, иначе false
 */
static bool isEndpointAtVertexIndex(const QgsGeometry &geom, int vertexIndex)
{
    const QgsAbstractGeometry *g = geom.constGet();
    if (const QgsCurve *curve = qgsgeometry_cast< const QgsCurve *>(g))
    {
        return vertexIndex == 0 || vertexIndex == curve->numPoints() - 1;
    }
    else if (const QgsMultiCurve *multiCurve = qgsgeometry_cast<const QgsMultiCurve *>(g))
    {
        for (int i = 0; i < multiCurve->numGeometries(); ++i)
        {
            const QgsCurve *part = multiCurve->curveN(i);
            Q_ASSERT(part);
            if (vertexIndex < part->numPoints())
            {
                return vertexIndex == 0 || vertexIndex == part->numPoints() - 1;
            }
            // Смещаем индекс для следующей части мультилинии
            vertexIndex -= part->numPoints();
        }
        Q_ASSERT(false);
        return false;
    }
    else
    {
        return false;
    }
}

/**
 * @brief Возвращает индекс соседней вершины для конечной точки линии
 * @param geom Геометрия линии
 * @param vertexIndex Индекс конечной вершины
 * @return Индекс соседней вершины или -1 если не найдена
 */
int adjacentVertexIndexToEndpoint(const QgsGeometry &geom, int vertexIndex)
{
    const QgsAbstractGeometry *g = geom.constGet();
    if (const QgsCurve *curve = qgsgeometry_cast<const QgsCurve *>(g))
    {
        // Возвращает индекс соседней вершины для конечной точки линии
        return vertexIndex == 0 ? 1 : curve->numPoints() - 2;
    }
    else if (const QgsMultiCurve *multiCurve = qgsgeometry_cast<const QgsMultiCurve *>(g))
    {
        int offset = 0;
        for (int i = 0; i < multiCurve->numGeometries(); ++i)
        {
            const QgsCurve *part = multiCurve->curveN(i);
            Q_ASSERT(part);
            if (vertexIndex < part->numPoints())
            {
                return vertexIndex == 0 ? offset + 1 : offset + part->numPoints() - 2;
            }
            vertexIndex -= part->numPoints();
            // Смещение для учета вершин в предыдущих частях мультилинии
            offset += part->numPoints();
        }
    }
    return -1;
}

/**
 * @brief Проверяет, является ли вершина частью дуги (криволинейного сегмента)
 * @param geom Геометрия для проверки
 * @param vertexIndex Индекс вершины
 * @return true если вершина является частью дуги, иначе false
 */
static bool isCircularVertex(const QgsGeometry &geom, int vertexIndex)
{
    QgsVertexId vid;
    return geom.vertexIdFromVertexNr(vertexIndex, vid) && vid.type == Qgis::VertexType::Curve;
}

/**
 * @brief Преобразует все вершины геометрии в мультиточку
 * @param geom Исходная геометрия
 * @return Мультиточка содержащая все вершины исходной геометрии
 */
static QgsGeometry geometryToMultiPoint(const QgsGeometry &geom)
{
    QgsMultiPoint *multiPoint = new QgsMultiPoint();
    QgsGeometry outputGeom(multiPoint);
    for (auto pointIt = geom.vertices_begin(); pointIt != geom.vertices_end(); ++pointIt)
    {
        multiPoint->addGeometry((*pointIt).clone());
    }
    return outputGeom;
}

/**
 * @brief Фильтр для поиска совпадений только одного конкретного объекта
 *
 * Класс используется для ограничения поиска вершин только указанным объектом.
 */
class OneFeatureFilter : public QgsPointLocator::MatchFilter
{
public:
    OneFeatureFilter(const QgsVectorLayer *layer, QgsFeatureId fid)
        : layer(layer)
        , fid(fid)
    {}

    bool acceptMatch(const QgsPointLocator::Match &match) override
    {
        return match.layer() == layer && match.featureId() == fid;
    }

private:
    const QgsVectorLayer *layer = nullptr;
    QgsFeatureId fid;
};

/**
 * @brief Фильтр для сбора всех совпадений в одной точке
 *
 * Собирает все совпадения вершин, которые находятся в одной точке.
 * Обрабатывает полигоны для избежания дублирования совпадений
 * первой и последней вершин кольца.
 */
class MatchCollectingFilter : public QgsPointLocator::MatchFilter
{
public:
    QList<QgsPointLocator::Match> matches;
    QgsVertexTool *vertextool = nullptr;

    MatchCollectingFilter(QgsVertexTool *vertextool)
        : vertextool(vertextool) {}

    bool acceptMatch(const QgsPointLocator::Match &match) override
    {
        if (match.distance() > 0)
        {
            return false;
        }

        QgsGeometry matchGeom = vertextool->cachedGeometry(match.layer(), match.featureId());
        bool isPolygon = matchGeom.wkbType() == Qgis::WkbType::Polygon;
        QgsVertexId polygonRingVid;
        QgsVertexId vid;
        QgsPoint pt;
        while (matchGeom.constGet()->nextVertex(vid, pt))
        {
            int vindex = matchGeom.vertexNrFromVertexId(vid);
            if (pt.x() == match.point().x() && pt.y() == match.point().y())
            {
                if (isPolygon)
                {
                    if (vid.vertex == 0)
                    {
                        polygonRingVid = vid;
                    }
                    else if (vid.ringEqual(polygonRingVid) && vid.vertex == matchGeom.constGet()->vertexCount(vid.part, vid.ring) - 1)
                    {
                        continue;
                    }
                }

                QgsPointLocator::Match extra_match(match.type(), match.layer(), match.featureId(), 0, match.point(), vindex);
                matches.append(extra_match);
            }
        }
        return true;
    }
};

/**
 * @brief Фильтр для поиска лучшего совпадения среди выбранных объектов
 *
 * Поддерживает два режима работы:
 * 1. С "заблокированным" объектом - ищет совпадения только для этого объекта
 * 2. С выбранными объектами слоя - ищет совпадения для всех выбранных объектов
 *
 * Сохраняет лучшее найденное совпадение для последующего использования.
 */
class SelectedMatchFilter : public QgsPointLocator::MatchFilter
{
public:
    explicit SelectedMatchFilter(double tol, QgsLockedFeature *selectedFeature)
        : mTolerance(tol)
        , mLockedFeature(selectedFeature) {}

    bool acceptMatch(const QgsPointLocator::Match &match) override
    {
        if (match.distance() <= mTolerance && match.layer())
        {
            if ((mLockedFeature && mLockedFeature->layer() == match.layer() && mLockedFeature->featureId() == match.featureId())
                    || ( !mLockedFeature && match.layer()->selectedFeatureIds().contains(match.featureId())))
            {
                if (!mBestSelectedMatch.isValid() || match.distance() < mBestSelectedMatch.distance())
                {
                    mBestSelectedMatch = match;
                }
            }
        }
        return true;
    }

    bool hasSelectedMatch() const { return mBestSelectedMatch.isValid(); }
    QgsPointLocator::Match bestSelectedMatch() const { return mBestSelectedMatch; }

private:
    double mTolerance;
    QgsLockedFeature *mLockedFeature;
    QgsPointLocator::Match mBestSelectedMatch;
};

QgsVertexTool::QgsVertexTool(QgsMapCanvas *canvas, QgsAdvancedDigitizingDockWidget *cadDock, VertexToolMode mode)
    : QgsMapToolAdvancedDigitizing(canvas, cadDock)
    , mMode(mode)
{
    // Запрещаем расширенную оцифровку для инструмента вершин
    setAdvancedDigitizingAllowed(false);

    mSnapIndicator.reset(new QgsSnapIndicator(canvas));

    // Маркер центра ребра для отображения 'средних' точек линий
    mEdgeCenterMarker = new QgsVertexMarker(canvas);
    mEdgeCenterMarker->setIconType(QgsVertexMarker::ICON_CROSS);
    mEdgeCenterMarker->setColor(Qt::red);
    mEdgeCenterMarker->setIconSize(QgsGuiUtils::scaleIconSize(10));
    mEdgeCenterMarker->setPenWidth(QgsGuiUtils::scaleIconSize(3));
    mEdgeCenterMarker->setVisible(false);

    // Оверлей слой для выделения объекта
    mFeatureBand = createRubberBand(Qgis::GeometryType::Line);
    mFeatureBand->setVisible(false);

    QColor color = digitizingStrokeColor();
    // Маркеры вершин объекта для отображения всех вершин
    mFeatureBandMarkers = new QgsRubberBand(canvas);
    mFeatureBandMarkers->setIcon(QgsRubberBand::ICON_CIRCLE);
    mFeatureBandMarkers->setColor(color);
    mFeatureBandMarkers->setWidth(QgsGuiUtils::scaleIconSize(2));
    mFeatureBandMarkers->setBrushStyle(Qt::NoBrush);
    mFeatureBandMarkers->setIconSize(QgsGuiUtils::scaleIconSize(6));
    mFeatureBandMarkers->setVisible(false);

    // Оверлей слой для выделения отдельных вершин
    mVertexBand = new QgsRubberBand(canvas);
    mVertexBand->setIcon(QgsRubberBand::ICON_CIRCLE);
    mVertexBand->setColor(color);
    mVertexBand->setWidth(QgsGuiUtils::scaleIconSize(2));
    mVertexBand->setBrushStyle(Qt::NoBrush);
    mVertexBand->setIconSize(QgsGuiUtils::scaleIconSize(15));
    mVertexBand->setVisible(false);

    // Создаем полупрозрачный цвет для ребер
    QColor color2(color);
    color2.setAlpha(color2.alpha()/3);
    mEdgeBand = new QgsRubberBand(canvas);
    mEdgeBand->setColor(color2);
    mEdgeBand->setWidth(QgsGuiUtils::scaleIconSize(10));
    mEdgeBand->setVisible(false);

    // Маркер конечных точек линий
    mEndpointMarker = new QgsVertexMarker(canvas);
    mEndpointMarker->setIconType(QgsVertexMarker::ICON_CROSS);
    mEndpointMarker->setColor(Qt::red);
    mEndpointMarker->setIconSize(QgsGuiUtils::scaleIconSize(10));
    mEndpointMarker->setPenWidth(QgsGuiUtils::scaleIconSize(3));
    mEndpointMarker->setVisible(false);
}

QgsVertexTool::~QgsVertexTool()
{
    // Освобождаем память от всех визуальных элементов
    delete mEdgeCenterMarker;
    delete mFeatureBand;
    delete mFeatureBandMarkers;
    delete mVertexBand;
    delete mEdgeBand;
    delete mEndpointMarker;
    delete mVertexEditor;
}

void QgsVertexTool::activate()
{
    // Показываем редактор вершин при активации инструмента
    showVertexEditor();
    // Подключаемся к изменению текущего слоя для обновления состояния
    connect(mCanvas, &QgsMapCanvas::currentLayerChanged, this, &QgsVertexTool::currentLayerChanged);

    QgsMapToolAdvancedDigitizing::activate();
}

void QgsVertexTool::deactivate()
{
    setHighlightedVertices(QList<Vertex>());
    removeTemporaryRubberBands();
    cleanupVertexEditor();

    mSnapIndicator->setMatch(QgsPointLocator::Match());

    QHash< QPair<QgsVectorLayer *, QgsFeatureId>, GeometryValidation>::iterator it = mValidations.begin();
    for (; it != mValidations.end(); ++it)
    {
        it->cleanup();
    }
    mValidations.clear();

    // Отключаемся от сигналов изменения слоя
    disconnect(mCanvas, &QgsMapCanvas::currentLayerChanged, this, &QgsVertexTool::currentLayerChanged);

    QgsMapToolAdvancedDigitizing::deactivate();
}

void QgsVertexTool::currentLayerChanged(QgsMapLayer *layer)
{
    if (mMode == QgsVertexTool::ActiveLayer)
    {
        if (mLockedFeature && mLockedFeature->layer() != layer)
        {
            cleanupLockedFeature();
        }
    }
}

void QgsVertexTool::addDragBand(const QgsPointXY &v1, const QgsPointXY &v2)
{
    addDragStraightBand(nullptr, v1, v2, false, true, v2);
}

void QgsVertexTool::addDragStraightBand(QgsVectorLayer *layer, QgsPointXY v0, QgsPointXY v1, bool moving0, bool moving1, const QgsPointXY &mapPoint)
{
    // Преобразуем координаты из CRS слоя в картографические CRS если необходимо
    if (layer)
    {
        v0 = toMapCoordinates(layer, v0);
        v1 = toMapCoordinates(layer, v1);
    }

    StraightBand b;
    b.band = createRubberBand(Qgis::GeometryType::Line, true);
    b.p0 = v0;
    b.p1 = v1;
    b.moving0 = moving0;
    b.moving1 = moving1;
    // Вычисляем смещение относительно точки перетаскивания
    b.offset0 = v0 - mapPoint;
    b.offset1 = v1 - mapPoint;

    b.band->addPoint(v0);
    b.band->addPoint(v1);

    mDragStraightBands << b;
}

void QgsVertexTool::addDragCircularBand(QgsVectorLayer *layer, QgsPointXY v0, QgsPointXY v1, QgsPointXY v2, bool moving0, bool moving1, bool moving2, const QgsPointXY &mapPoint)
{
    // Преобразуем координаты из CRS слоя в картографические CRS если необходимо
    if (layer)
    {
        v0 = toMapCoordinates(layer, v0);
        v1 = toMapCoordinates(layer, v1);
        v2 = toMapCoordinates(layer, v2);
    }

    CircularBand b;
    b.band = createRubberBand(Qgis::GeometryType::Line, true);
    b.p0 = v0;
    b.p1 = v1;
    b.p2 = v2;
    b.moving0 = moving0;
    b.moving1 = moving1;
    b.moving2 = moving2;
    // Вычисляем смещения для всех трех точек дуги
    b.offset0 = v0 - mapPoint;
    b.offset1 = v1 - mapPoint;
    b.offset2 = v2 - mapPoint;
    // Обновляем оверлей слой для отображения дуги
    b.updateRubberBand(mapPoint);

    mDragCircularBands << b;
}

void QgsVertexTool::clearDragBands()
{
    // Удаляем все маркеры точек перетаскивания
    qDeleteAll(mDragPointMarkers);
    mDragPointMarkers.clear();
    mDragPointMarkersOffset.clear();

    // Удаляем все оверлей слои
    for (const StraightBand &b : std::as_const(mDragStraightBands))
    {
        delete b.band;
    }
    mDragStraightBands.clear();

    // Удаляем все дугообразные оверлей слои
    for (const CircularBand &b : std::as_const(mDragCircularBands))
    {
        delete b.band;
    }
    mDragCircularBands.clear();
}

void QgsVertexTool::cadCanvasPressEvent(QgsMapMouseEvent *e)
{
    if (mSelectionMethod == SelectionRange)
    {
        rangeMethodPressEvent(e);
        return;
    }

    // Проверяем, нужно ли сбросить выделение при клике вне выделенных вершин
    if (!mDraggingVertex && !mSelectedVertices.isEmpty() && !(e->modifiers() & Qt::ShiftModifier) && !(e->modifiers() & Qt::ControlModifier))
    {
        // Проверяем, кликнул ли пользователь по одной из выделенных вершин
        bool clickedOnHighlightedVertex = false;
        QgsPointLocator::Match m = snapToEditableLayer(e);
        if (m.hasVertex())
        {
            for (const Vertex &selectedVertex : std::as_const(mSelectedVertices))
            {
                if (selectedVertex.layer == m.layer() && selectedVertex.fid == m.featureId() && selectedVertex.vertexId == m.vertexIndex())
                {
                    clickedOnHighlightedVertex = true;
                    break;
                }
            }
        }

        // Сбрасываем выделение если кликнули вне выделенных вершин
        if (!clickedOnHighlightedVertex && e->button() == Qt::LeftButton)
        {
            setHighlightedVertices(QList<Vertex>()); // сброс выделения
            updateLockedFeatureVertices();
        }
    }

    if (e->button() == Qt::LeftButton)
    {
        // Shift или Ctrl клик для выделения вершин без входа в режим редактирования
        if (e->modifiers() & Qt::ControlModifier || e->modifiers() & Qt::ShiftModifier)
        {
            QgsPointLocator::Match m = snapToEditableLayer(e);
            if (m.hasVertex())
            {
                Vertex vertex(m.layer(), m.featureId(), m.vertexIndex());

                HighlightMode mode = ModeReset;
                if (e->modifiers() & Qt::ShiftModifier)
                {
                    // Shift+Click для добавления вершины к выделению
                    mode = ModeAdd;
                }
                else if (e->modifiers() & Qt::ControlModifier)
                {
                    // Ctrl+Click для удаления вершины из выделения
                    mode = ModeSubtract;
                }

                setHighlightedVertices(QList<Vertex>() << vertex, mode);
                return;
            }
        }

        if (!mDraggingVertex && !mDraggingEdge)
        {
            mSelectionRubberBandStartPos.reset(new QPoint(e->pos()));
        }
    }
}

void QgsVertexTool::cadCanvasReleaseEvent(QgsMapMouseEvent *e)
{
    if (mSelectionMethod == SelectionRange)
    {
        rangeMethodReleaseEvent(e);
        return;
    }

    if (mNewVertexFromDoubleClick)
    {
        QgsPointLocator::Match m(*mNewVertexFromDoubleClick);
        if (mLockedFeature && (mLockedFeature->featureId() != m.featureId() || mLockedFeature->layer() != m.layer()))
        {
            return;
        }

        mNewVertexFromDoubleClick.reset();

        stopDragging();
        startDraggingAddVertex(m);

        if (e->modifiers() & Qt::ShiftModifier)
        {
            moveVertex(m.point(), &m);
            // Принудительно обновляем оверлей слои
            mouseMoveNotDragging(e);
        }
    }
    else if (mSelectionRubberBand &&
             (mSelectionMethod == SelectionNormal  ||
              (mSelectionMethod == SelectionPolygon && e->button() == Qt::RightButton)))
    {
        // Обрабатываем только перетаскивание прямоугольника выделения
        QList<Vertex> vertices;
        QList<Vertex> selectedVertices;
        bool showInvisibleFeatureWarning = false;

        QgsGeometry rubberBandGeometry = mSelectionRubberBand->asGeometry();

        // Логика выбора вершин с помощью прямоугольника:
        // - если есть заблокированный объект, разрешаем выбор только его вершин
        // - если нет заблокированного объекта, выбираем вершины из любого объекта,
        //   но если есть вершины из выбранных объектов, отдаем им приоритет

        const auto editableLayers = editableVectorLayers();
        for (QgsVectorLayer *vlayer : editableLayers)
        {
            if (mMode == ActiveLayer && vlayer != currentVectorLayer())
            {
                continue;
            }
            if (mLockedFeature && mLockedFeature->layer() != vlayer)
            {
                continue;
            }
            QgsGeometry layerRubberBandGeometry = rubberBandGeometry;
            // Преобразуем геометрию оверлей слоя в CRS слоя
            try
            {
                QgsCoordinateTransform ct = mCanvas->mapSettings().layerTransform(vlayer);
                if (ct.isValid())
                {
                    layerRubberBandGeometry.transform(ct, Qgis::TransformDirection::Reverse);
                }
            }
            catch (QgsCsException &)
            {
                continue;
            }

            QgsRenderContext context = QgsRenderContext::fromMapSettings(mCanvas->mapSettings());
            context.setExpressionContext(mCanvas->createExpressionContext());
            context.expressionContext() << QgsExpressionContextUtils::layerScope(vlayer);
            std::unique_ptr<QgsFeatureRenderer> r;
            if (vlayer->renderer())
            {
                r.reset(vlayer->renderer()->clone());
                r->startRender(context, vlayer->fields());
            }

            QgsRectangle layerRect = layerRubberBandGeometry.boundingBox();
            // Создаем движок геометрии для проверки попадания точек
            std::unique_ptr<QgsGeometryEngine> layerRubberBandEngine(QgsGeometry::createGeometryEngine(layerRubberBandGeometry.constGet()));
            layerRubberBandEngine->prepareGeometry();

            QgsFeatureRequest request;
            request.setFilterRect(layerRect);
            request.setFlags(QgsFeatureRequest::ExactIntersect);
            if (r)
            {
                request.setSubsetOfAttributes(r->usedAttributes(context), vlayer->fields());
            }
            else
            {
                request.setNoAttributes();
            }
            request.setExpressionContext(context.expressionContext());

            QgsFeature f;
            QgsFeatureIterator fi = vlayer->getFeatures(request);
            while (fi.nextFeature(f))
            {
                if (mLockedFeature && mLockedFeature->featureId() != f.id())
                {
                    continue;
                }
                context.expressionContext().setFeature(f);
                bool isFeatureInvisible = (r && !r->willRenderFeature(f, context));

                if (isFeatureInvisible && showInvisibleFeatureWarning)
                {
                    continue;
                }
                bool isFeatureSelected = vlayer->selectedFeatureIds().contains(f.id());
                QgsGeometry g = f.geometry();
                int i = 0;
                // Проверяем каждую вершину объекта на попадание в область выделения
                for (auto it = g.constGet()->vertices_begin(); it != g.constGet()->vertices_end(); ++it)
                {
                    QgsPoint pt = *it;
                    if (layerRubberBandEngine->contains(&pt))
                    {
                        if (isFeatureInvisible)
                        {
                            showInvisibleFeatureWarning = true;
                            break;
                        }
                        vertices << Vertex(vlayer, f.id(), i);

                        if (isFeatureSelected)
                        {
                            selectedVertices << Vertex(vlayer, f.id(), i);
                        }
                    }
                    i++;
                }
            }
            if (r)
            {
                r->stopRender(context);
            }
        }

        // Здесь мы отдаем приоритет вершинам выбранных объектов, если нет заблокированного объекта
        if (!mLockedFeature && !selectedVertices.isEmpty())
        {
            vertices = selectedVertices;
        }

        HighlightMode mode = ModeReset;
        if (e->modifiers() & Qt::ShiftModifier)
        {
            mode = ModeAdd;
        }
        else if (e->modifiers() & Qt::ControlModifier )
        {
            mode = ModeSubtract;
        }

        setHighlightedVertices(vertices, mode);

        stopSelectionRubberBand();
    }
    else if (e->button() == Qt::LeftButton && e->modifiers() & Qt::AltModifier)
    {
        mSelectionMethod = SelectionPolygon;
        initSelectionRubberBand();
        mSelectionRubberBand->addPoint(toMapCoordinates(e->pos()));
        return;
    }
    else if (mSelectionRubberBand && mSelectionMethod == SelectionPolygon)
    {
        mSelectionRubberBand->addPoint(toMapCoordinates(e->pos()));
        return;
    }
    else
    {
        if (e->button() == Qt::LeftButton && !(e->modifiers() & Qt::ShiftModifier) && !(e->modifiers() & Qt::ControlModifier))
        {
            if (mDraggingVertex)
            {
                QgsPointLocator::Match match = e->mapPointMatch();
                moveVertex(e->mapPoint(), &match);
            }
            else if (mDraggingEdge)
            {
                moveEdge(toMapCoordinates(e->pos()));
            }
            else
            {
                startDragging(e);
            }
        }
        else if (e->button() == Qt::RightButton)
        {
            if (mDraggingVertex || mDraggingEdge)
            {
                stopDragging();
            }
            else if (!mSelectionRubberBand)
            {
                // Правый клик для выбора/отмены выбора объекта для редактирования
                // Если несколько объектов в одном месте, переключаемся между ними последующими правыми кликами
                tryToSelectFeature(e);
            }
        }
    }

    mSelectionRubberBandStartPos.reset();
}

void QgsVertexTool::cadCanvasMoveEvent(QgsMapMouseEvent *e)
{
    if (mLockedFeatureAlternatives && (e->pos() - mLockedFeatureAlternatives->screenPoint).manhattanLength() >= QApplication::startDragDistance())
    {
        mLockedFeatureAlternatives.reset();
    }

    if (mSelectionMethod == SelectionRange)
    {
        rangeMethodMoveEvent(e);
        return;
    }

    if (mDraggingVertex)
    {
        mouseMoveDraggingVertex(e);
    }
    else if (mDraggingEdge)
    {
        mouseMoveDraggingEdge(e);
    }
    else if (mSelectionRubberBandStartPos)
    {
        // Пользователь может перетаскивать прямоугольник для выбора вершин
        if (!mSelectionRubberBand && (e->pos() - *mSelectionRubberBandStartPos).manhattanLength() >= 10)
        {
            mSelectionMethod = SelectionNormal;
            initSelectionRubberBand();
        }
        if (mSelectionRubberBand)
        {
            updateSelectionRubberBand(e);
        }
    }
    else
    {
        mouseMoveNotDragging(e);
    }
}

void QgsVertexTool::mouseMoveDraggingVertex(QgsMapMouseEvent *e)
{
    mSnapIndicator->setMatch(e->mapPointMatch());
    mEdgeCenterMarker->setVisible(false);

    moveDragBands(e->mapPoint());
}

void QgsVertexTool::moveDragBands(const QgsPointXY &mapPoint)
{
    for (int i = 0; i < mDragStraightBands.count(); ++i)
    {
        StraightBand &b = mDragStraightBands[i];
        if (b.moving0)
        {
            b.band->movePoint(0, mapPoint + b.offset0);
        }
        if (b.moving1)
        {
            b.band->movePoint(1, mapPoint + b.offset1);
        }
    }

    for (int i = 0; i < mDragCircularBands.count(); ++i)
    {
        CircularBand &b = mDragCircularBands[i];
        b.updateRubberBand(mapPoint);
    }

    for (int i = 0; i < mDragPointMarkers.count(); ++i)
    {
        QgsVertexMarker *marker = mDragPointMarkers[i];
        QgsVector offset = mDragPointMarkersOffset[i];
        marker->setCenter(mapPoint + offset);
    }

    // Убеждаемся, что временный оверлей слой не виден
    removeTemporaryRubberBands();
}

void QgsVertexTool::mouseMoveDraggingEdge(QgsMapMouseEvent *e)
{
    mSnapIndicator->setMatch(QgsPointLocator::Match());
    mEdgeCenterMarker->setVisible(false);

    QgsPointXY mapPoint = toMapCoordinates(e->pos());

    moveDragBands(mapPoint);
}

void QgsVertexTool::canvasDoubleClickEvent(QgsMapMouseEvent *e)
{
    if (e->button() != Qt::LeftButton)
    {
        return;
    }

    QgsPointLocator::Match m = snapToEditableLayer(e);
    if (!m.hasEdge())
    {
        return;
    }

    mNewVertexFromDoubleClick.reset(new QgsPointLocator::Match(m));
}

void QgsVertexTool::removeTemporaryRubberBands()
{
    mFeatureBand->setVisible(false);
    mFeatureBandMarkers->setVisible(false);
    mFeatureBandLayer = nullptr;
    mFeatureBandFid = QgsFeatureId();
    mVertexBand->setVisible(false);
    mEdgeBand->setVisible(false);
    mEndpointMarkerCenter.reset();
    mEndpointMarker->setVisible(false);
}

QgsPointLocator::Match QgsVertexTool::snapToEditableLayer(QgsMapMouseEvent *e)
{
    QgsSnappingUtils *snapUtils = canvas()->snappingUtils();
    QgsSnappingConfig oldConfig = snapUtils->config();
    QgsPointLocator::Match m;

    QgsPointXY mapPoint = toMapCoordinates(e->pos());
    double tol = QgsTolerance::vertexSearchRadius(canvas()->mapSettings());

    QgsSnappingConfig config = oldConfig;
    config.setEnabled(true);
    config.setMode(Qgis::SnappingMode::AdvancedConfiguration);
    config.setIntersectionSnapping(false);
    config.clearIndividualLayerSettings();

    typedef QHash<QgsVectorLayer *, QgsSnappingConfig::IndividualLayerSettings> SettingsHashMap;
    SettingsHashMap oldLayerSettings = oldConfig.individualLayerSettings();

    if (QgsVectorLayer *currentVlayer = currentVectorLayer())
    {
        if (currentVlayer->isEditable())
        {
            const auto layers = canvas()->layers(true);
            for (QgsMapLayer *layer : layers)
            {
                QgsVectorLayer *vlayer = qobject_cast<QgsVectorLayer*>(layer);
                if (!vlayer)
                {
                    continue;
                }

                QgsSnappingConfig::IndividualLayerSettings layerSettings;
                SettingsHashMap::const_iterator existingSettings = oldLayerSettings.constFind(vlayer);
                if (existingSettings != oldLayerSettings.constEnd())
                {
                    layerSettings = existingSettings.value();
                    layerSettings.setEnabled(vlayer == currentVlayer);
                    layerSettings.setTolerance(tol);
                    layerSettings.setTypeFlag(Qgis::SnappingType::Vertex | Qgis::SnappingType::Segment);
                    layerSettings.setUnits(Qgis::MapToolUnit::Project);
                }
                else
                {
                    layerSettings = QgsSnappingConfig::IndividualLayerSettings(vlayer == currentVlayer, (Qgis::SnappingType::Vertex | Qgis::SnappingType::Segment), tol, Qgis::MapToolUnit::Project, 0.0, 0.0);
                }

                config.setIndividualLayerSettings(vlayer, layerSettings);
            }

            snapUtils->setConfig(config);
            SelectedMatchFilter filter(tol, mLockedFeature.get());
            m = snapUtils->snapToMap(mapPoint, &filter, true);

            if (filter.hasSelectedMatch())
            {
                m = filter.bestSelectedMatch();
                mLastSnap.reset();
            }
        }
    }

    if (!m.isValid() && mMode == AllLayers)
    {
        const auto layers = canvas()->layers(true);
        for (QgsMapLayer *layer : layers)
        {
            QgsVectorLayer *vlayer = qobject_cast<QgsVectorLayer*>(layer);
            if (!vlayer)
            {
                continue;
            }
            QgsSnappingConfig::IndividualLayerSettings layerSettings;
            SettingsHashMap::const_iterator existingSettings = oldLayerSettings.constFind(vlayer);
            if (existingSettings != oldLayerSettings.constEnd())
            {
                layerSettings = existingSettings.value();
                layerSettings.setEnabled(vlayer->isEditable());
                layerSettings.setTolerance(tol);
                layerSettings.setTypeFlag(Qgis::SnappingType::Vertex | Qgis::SnappingType::Segment);
                layerSettings.setUnits(Qgis::MapToolUnit::Project);
            }
            else
            {
                layerSettings = QgsSnappingConfig::IndividualLayerSettings(vlayer->isEditable(), (Qgis::SnappingType::Vertex | Qgis::SnappingType::Segment), tol, Qgis::MapToolUnit::Project, 0.0, 0.0);
            }
            config.setIndividualLayerSettings(vlayer, layerSettings);
        }

        snapUtils->setConfig(config);
        SelectedMatchFilter filter(tol, mLockedFeature.get());
        m = snapUtils->snapToMap(mapPoint, &filter, true);

        if (filter.hasSelectedMatch())
        {
            m = filter.bestSelectedMatch();
            mLastSnap.reset();
        }
    }

    // Пытаемся остаться привязанным к ранее использованному объекту
    // чтобы выделение не "прыгало" на вершинах где соединяются объекты
    if (mLastSnap)
    {
        OneFeatureFilter filterLast(mLastSnap->layer(), mLastSnap->featureId());
        QgsPointLocator::Match lastMatch = snapUtils->snapToMap(mapPoint, &filterLast, true);

        bool matchHasVertexLastHasEdge = m.hasVertex() && lastMatch.hasEdge();
        if (lastMatch.isValid() && lastMatch.distance() <= m.distance() && !matchHasVertexLastHasEdge)
        {
            m = lastMatch;
        }
    }

    snapUtils->setConfig(oldConfig);

    mLastSnap.reset(new QgsPointLocator::Match(m));

    return m;
}

QgsPointLocator::Match QgsVertexTool::snapToPolygonInterior(QgsMapMouseEvent *e)
{
    QgsSnappingUtils *snapUtils = canvas()->snappingUtils();
    QgsPointLocator::Match m;

    QgsPointXY mapPoint = toMapCoordinates(e->pos());

    if (QgsVectorLayer *currentVlayer = currentVectorLayer())
    {
        if (currentVlayer->isEditable() && currentVlayer->geometryType() == Qgis::GeometryType::Polygon)
        {
            QgsPointLocator::MatchList matchList = snapUtils->locatorForLayer(currentVlayer)->pointInPolygon(mapPoint, true);
            if (!matchList.isEmpty())
            {
                m = matchList.first();
            }
        }
    }

    // Если нет совпадения из текущего слоя, пробуем использовать любой редактируемый векторный слой
    if (!m.isValid() && mMode == AllLayers)
    {
        const auto layers = canvas()->layers(true);
        for (QgsMapLayer *layer : layers)
        {
            QgsVectorLayer *vlayer = qobject_cast<QgsVectorLayer *>(layer);
            if (!vlayer)
            {
                continue;
            }

            if (vlayer->isEditable() && vlayer->geometryType() == Qgis::GeometryType::Polygon)
            {
                QgsPointLocator::MatchList matchList = snapUtils->locatorForLayer(vlayer)->pointInPolygon(mapPoint, true);
                if (!matchList.isEmpty())
                {
                    m = matchList.first();
                    break;
                }
            }
        }
    }

    if (!mLastSnap && m.isValid())
    {
        mLastSnap.reset(new QgsPointLocator::Match(m));
    }

    return m;
}

QList<QgsPointLocator::Match> QgsVertexTool::findEditableLayerMatches(const QgsPointXY &mapPoint, QgsVectorLayer *layer)
{
    QgsPointLocator::MatchList matchList;

    if (!layer->isEditable())
    {
        return matchList;
    }

    QgsSnappingUtils *snapUtils = canvas()->snappingUtils();
    QgsPointLocator *locator = snapUtils->locatorForLayer(layer);

    if (layer->geometryType() == Qgis::GeometryType::Polygon)
    {
        matchList << locator->pointInPolygon(mapPoint, true);
    }

    double tolerance = QgsTolerance::vertexSearchRadius(canvas()->mapSettings());
    matchList << locator->edgesInRect(mapPoint, tolerance);
    matchList << locator->verticesInRect(mapPoint, tolerance);

    return matchList;
}

QSet<QPair<QgsVectorLayer*, QgsFeatureId>> QgsVertexTool::findAllEditableFeatures(const QgsPointXY &mapPoint)
{
    QSet<QPair<QgsVectorLayer*, QgsFeatureId>> alternatives;

    if (QgsVectorLayer *currentVlayer = currentVectorLayer())
    {
        const auto matches = findEditableLayerMatches(mapPoint, currentVlayer);
        for (const QgsPointLocator::Match &m : matches)
        {
            alternatives.insert(qMakePair(m.layer(), m.featureId()));
        }
    }

    if (mMode == AllLayers)
    {
        const auto layers = canvas()->layers(true);
        for (QgsMapLayer *layer : layers)
        {
            QgsVectorLayer *vlayer = qobject_cast<QgsVectorLayer *>(layer);
            if (!vlayer)
            {
                continue;
            }

            const auto matches = findEditableLayerMatches(mapPoint, vlayer);
            for (const QgsPointLocator::Match &m : matches)
            {
                alternatives.insert(qMakePair(m.layer(), m.featureId()));
            }

        }
    }

    return alternatives;
}

void QgsVertexTool::tryToSelectFeature(QgsMapMouseEvent *e)
{
    if (!mLockedFeatureAlternatives)
    {
        QSet<QPair<QgsVectorLayer*, QgsFeatureId>> alternatives = findAllEditableFeatures(toMapCoordinates(e->pos()));
        if (!alternatives.isEmpty())
        {
            QgsPointLocator::Match m = snapToEditableLayer(e);
            if (!m.isValid())
            {
                m = snapToPolygonInterior(e);
            }

            mLockedFeatureAlternatives.reset(new LockedFeatureAlternatives);
            mLockedFeatureAlternatives->screenPoint = e->pos();
            mLockedFeatureAlternatives->index = -1;
            if (m.isValid())
            {
                QPair<QgsVectorLayer*, QgsFeatureId> firstChoice(m.layer(), m.featureId());
                mLockedFeatureAlternatives->alternatives.append(firstChoice);
                alternatives.remove(firstChoice);
            }
            mLockedFeatureAlternatives->alternatives.append(qgis::setToList(alternatives));

            if (mLockedFeature)
            {
                // Если уже есть заблокированный объект, продолжаем цикл с него
                QPair<QgsVectorLayer *, QgsFeatureId> currentSelection(mLockedFeature->layer(), mLockedFeature->featureId());
                int currentIndex = mLockedFeatureAlternatives->alternatives.indexOf(currentSelection);
                if (currentIndex != -1)
                {
                    mLockedFeatureAlternatives->index = currentIndex;
                }
            }
        }
    }

    if (mLockedFeatureAlternatives)
    {
        if (mLockedFeatureAlternatives->index < mLockedFeatureAlternatives->alternatives.count() - 1)
        {
            ++mLockedFeatureAlternatives->index;
        }
        else
        {
            mLockedFeatureAlternatives->index = -1;
        }
    }

    if (mLockedFeatureAlternatives && mLockedFeatureAlternatives->index != -1)
    {
        // У нас есть объект для выбора
        QPair<QgsVectorLayer*, QgsFeatureId> alternative = mLockedFeatureAlternatives->alternatives.at(mLockedFeatureAlternatives->index);

        QList<Vertex> vertices;
        for (const Vertex &v : std::as_const(mSelectedVertices))
        {
            if (v.layer == alternative.first && v.fid == alternative.second)
            {
                vertices << v;
            }
        }
        setHighlightedVertices(vertices, ModeReset);
        updateVertexEditor(alternative.first, alternative.second);
    }
    else
    {
        // Под курсором действительно ничего нет или при циклическом просмотре списка доступных объектов
        // мы дошли до конца списка - отменяем выбор любого объекта, который могли выбрать
        setHighlightedVertices(QList<Vertex>(), ModeReset);
        cleanupLockedFeature();
    }

    updateFeatureBand(QgsPointLocator::Match());
}

bool QgsVertexTool::isNearEndpointMarker(const QgsPointXY &mapPoint)
{
    if (!mEndpointMarkerCenter)
    {
        return false;
    }

    double distMarker = std::sqrt(mEndpointMarkerCenter->sqrDist(mapPoint));
    double tol = QgsTolerance::vertexSearchRadius(canvas()->mapSettings());

    QgsGeometry geom = cachedGeometryForVertex(*mMouseAtEndpoint);
    QgsPointXY vertexPointV2 = geom.vertexAt(mMouseAtEndpoint->vertexId);
    QgsPointXY vertexPoint = QgsPointXY(vertexPointV2.x(), vertexPointV2.y());
    double distVertex = std::sqrt(vertexPoint.sqrDist(mapPoint));

    return distMarker < tol && distMarker < distVertex;
}

bool QgsVertexTool::isMatchAtEndpoint(const QgsPointLocator::Match &match)
{
    QgsGeometry geom = cachedGeometry(match.layer(), match.featureId());

    if (geom.type() != Qgis::GeometryType::Line)
    {
        return false;
    }

    return isEndpointAtVertexIndex(geom, match.vertexIndex());
}

QgsPointXY QgsVertexTool::positionForEndpointMarker(const QgsPointLocator::Match &match)
{
    QgsGeometry geom = cachedGeometry(match.layer(), match.featureId());

    QgsPointXY pt0 = geom.vertexAt(adjacentVertexIndexToEndpoint(geom, match.vertexIndex()));
    QgsPointXY pt1 = geom.vertexAt(match.vertexIndex());

    pt0 = toMapCoordinates(match.layer(), pt0);
    pt1 = toMapCoordinates(match.layer(), pt1);

    double dx = pt1.x() - pt0.x();
    double dy = pt1.y() - pt0.y();
    double dist = 15 * canvas()->mapSettings().mapUnitsPerPixel();
    double angle = std::atan2(dy, dx);
    double x = pt1.x() + std::cos(angle) * dist;
    double y = pt1.y() + std::sin(angle) * dist;
    return QgsPointXY(x, y);
}

void QgsVertexTool::mouseMoveNotDragging(QgsMapMouseEvent *e)
{
    if (mMouseAtEndpoint)
    {
        QgsPointXY mapPoint = toMapCoordinates(e->pos());
        if (isNearEndpointMarker(mapPoint))
        {
            mEndpointMarker->setColor(Qt::red);
            mEndpointMarker->update();
            mVertexBand->setVisible(false);
            return;
        }
    }

    // Не используем привязку из события мыши, используем свою с любым редактируемым слоем
    QgsPointLocator::Match m = snapToEditableLayer(e);
    bool targetIsAllowed = (!mLockedFeature || ( mLockedFeature->featureId() == m.featureId() && mLockedFeature->layer() == m.layer()));

    if (m.type() == QgsPointLocator::Vertex && targetIsAllowed)
    {
        updateVertexBand(m);

        // Если мы у конечной точки, покажем также индикатор конечной точки
        // чтобы пользователь мог добавить новую вершину в конце
        if (isMatchAtEndpoint(m))
        {
            mMouseAtEndpoint.reset(new Vertex(m.layer(), m.featureId(), m.vertexIndex()));
            mEndpointMarkerCenter.reset(new QgsPointXY(positionForEndpointMarker(m)));
            mEndpointMarker->setCenter(*mEndpointMarkerCenter);
            mEndpointMarker->setColor(Qt::gray);
            mEndpointMarker->setVisible(true);
            mEndpointMarker->update();
        }
        else
        {
            mMouseAtEndpoint.reset();
            mEndpointMarkerCenter.reset();
            mEndpointMarker->setVisible(false);
        }
    }
    else
    {
        mVertexBand->setVisible(false);
        mMouseAtEndpoint.reset();
        mEndpointMarkerCenter.reset();
        mEndpointMarker->setVisible(false);
    }

    // Возможность создать новую вершину здесь или переместить ребро
    if (m.type() == QgsPointLocator::Edge && targetIsAllowed)
    {
        QgsPointXY mapPoint = toMapCoordinates(e->pos());
        bool isCircularEdge = false;

        QgsPointXY p0, p1;
        m.edgePoints(p0, p1);

        QgsGeometry geom = cachedGeometry(m.layer(), m.featureId());
        if (isCircularVertex(geom, m.vertexIndex()))
        {
            // Дуговое ребро у первой вершины
            isCircularEdge = true;
            QgsPointXY pX = toMapCoordinates(m.layer(), geom.vertexAt(m.vertexIndex() - 1));
            QgsPointSequence points;
            QgsGeometryUtils::segmentizeArc(QgsPoint(pX), QgsPoint(p0), QgsPoint(p1), points);
            mEdgeBand->reset();
            for (const QgsPoint &pt : std::as_const(points))
            {
                mEdgeBand->addPoint(pt);
            }
        }
        else if (isCircularVertex(geom, m.vertexIndex() + 1))
        {
            // Дуговое ребро у второй вершины
            isCircularEdge = true;
            QgsPointXY pX = toMapCoordinates(m.layer(), geom.vertexAt(m.vertexIndex() + 2));
            QgsPointSequence points;
            QgsGeometryUtils::segmentizeArc(QgsPoint(p0), QgsPoint(p1), QgsPoint(pX), points);
            mEdgeBand->reset();
            for (const QgsPoint &pt : std::as_const(points))
            {
                mEdgeBand->addPoint(pt);
            }
        }
        else
        {
            // Прямое ребро
            QgsPolylineXY points;
            points << p0 << p1;
            mEdgeBand->setToGeometry(QgsGeometry::fromPolylineXY(points), nullptr);
        }

        QgsPointXY edgeCenter;
        bool isNearCenter = matchEdgeCenterTest(m, mapPoint, &edgeCenter);
        mEdgeCenterMarker->setCenter(edgeCenter);
        mEdgeCenterMarker->setColor(isNearCenter ? Qt::red : Qt::gray);
        mEdgeCenterMarker->setVisible(!isCircularEdge);
        mEdgeCenterMarker->update();

        mEdgeBand->setVisible(!isNearCenter);
    }
    else
    {
        mEdgeCenterMarker->setVisible(false);
        mEdgeBand->setVisible(false);
    }

    if (!m.isValid())
    {
        m = snapToPolygonInterior(e);
    }

    updateFeatureBand(mLockedFeature ? QgsPointLocator::Match() : m);
}

void QgsVertexTool::updateVertexBand( const QgsPointLocator::Match &m )
{
    if (m.hasVertex() && m.layer())
    {
        mVertexBand->setToGeometry( QgsGeometry::fromPointXY(m.point() ), nullptr);
        mVertexBand->setVisible(true);
        bool isCircular = false;
        if (m.layer())
        {
            isCircular = isCircularVertex(cachedGeometry(m.layer(), m.featureId()), m.vertexIndex());
        }

        mVertexBand->setIcon(isCircular ? QgsRubberBand::ICON_FULL_DIAMOND : QgsRubberBand::ICON_CIRCLE);
    }
    else
    {
        mVertexBand->setVisible(false);
    }
}

void QgsVertexTool::updateFeatureBand(const QgsPointLocator::Match &m)
{
    // Выделяем объект
    if (m.isValid() && m.layer())
    {
        if (mFeatureBandLayer == m.layer() && mFeatureBandFid == m.featureId())
        {
            return;  // Пропускаем регенерацию оверлей слоя, если не нужно
        }

        QgsGeometry geom = cachedGeometry(m.layer(), m.featureId());
        mFeatureBandMarkers->setToGeometry(geometryToMultiPoint(geom), m.layer());
        mFeatureBandMarkers->setVisible(true);
        if (QgsWkbTypes::isCurvedType(geom.wkbType()))
        {
            geom = QgsGeometry(geom.constGet()->segmentize());
        }
        mFeatureBand->setToGeometry(geom, m.layer());
        mFeatureBand->setVisible(true);
        mFeatureBandLayer = m.layer();
        mFeatureBandFid = m.featureId();
    }
    else
    {
        mFeatureBand->setVisible(false);
        mFeatureBandMarkers->setVisible(false);
        mFeatureBandLayer = nullptr;
        mFeatureBandFid = QgsFeatureId();
    }
}

void QgsVertexTool::keyPressEvent(QKeyEvent *e)
{
    switch (e->key())
    {
    case Qt::Key_Escape:
    {
        if (mSelectionMethod == SelectionRange)
        {
            stopRangeVertexSelection();
        }
        if (mSelectionMethod == SelectionPolygon)
        {
            stopSelectionRubberBand();
        }
        if (mDraggingVertex || mDraggingEdge)
        {
            stopDragging();
        }
        break;
    }
    case Qt::Key_Delete:
    case Qt::Key_Backspace:
    {
        if (mSelectionMethod == SelectionPolygon)
        {
            e->ignore();
            mSelectionRubberBand->removeLastPoint();
            if (mSelectionRubberBand->numberOfVertices() < 2)
            {
                stopSelectionRubberBand();
            }
        }
        if (mDraggingVertex || (!mDraggingEdge && !mSelectedVertices.isEmpty()))
        {
            e->ignore();
            deleteVertex();
        }
        break;
    }
    case Qt::Key_O:
    {
        if (mDraggingVertex || (!mDraggingEdge && !mSelectedVertices.isEmpty()))
        {
            e->ignore();
            toggleVertexCurve();
        }
        break;
    }
    case Qt::Key_R:
    {
        if (e->modifiers() & Qt::ShiftModifier && !mDraggingVertex && !mDraggingEdge)
        {
            startRangeVertexSelection();
        }
        break;
    }
    case Qt::Key_Less:
    case Qt::Key_Comma:
    {
        if (!mDraggingVertex && !mDraggingEdge)
        {
            highlightAdjacentVertex(-1);
        }
        break;
    }
    case Qt::Key_Greater:
    case Qt::Key_Period:
    {
        if (!mDraggingVertex && !mDraggingEdge)
        {
            highlightAdjacentVertex(+1);
        }
        break;
    }
    default:
    {
        return;
    }
    }
    return;
}

QgsGeometry QgsVertexTool::cachedGeometry(const QgsVectorLayer *layer, QgsFeatureId fid)
{
    const bool layerWasNotInCache = !mCache.contains(layer);

    QHash<QgsFeatureId, QgsGeometry> &layerCache = mCache[layer];
    if (layerWasNotInCache)
    {
        connect(layer, &QgsVectorLayer::geometryChanged, this, &QgsVertexTool::onCachedGeometryChanged);
        connect(layer, &QgsVectorLayer::featureDeleted, this, &QgsVertexTool::onCachedGeometryDeleted);
        connect(layer, &QgsVectorLayer::willBeDeleted, this, &QgsVertexTool::clearGeometryCache);
        connect(layer, &QgsVectorLayer::dataChanged, this, &QgsVertexTool::clearGeometryCache);
    }

    if (!layerCache.contains(fid))
    {
        QgsFeature f;
        layer->getFeatures(QgsFeatureRequest(fid).setNoAttributes()).nextFeature(f);
        layerCache[fid] = f.geometry();
    }

    return layerCache[fid];
}

QgsGeometry QgsVertexTool::cachedGeometryForVertex(const Vertex &vertex)
{
    return cachedGeometry(vertex.layer, vertex.fid);
}

void QgsVertexTool::clearGeometryCache()
{
    const QgsVectorLayer *layer = qobject_cast<const QgsVectorLayer*>(sender());
    mCache.remove(layer);
    disconnect(layer, &QgsVectorLayer::geometryChanged, this, &QgsVertexTool::onCachedGeometryChanged);
    disconnect(layer, &QgsVectorLayer::featureDeleted, this, &QgsVertexTool::onCachedGeometryDeleted);
    disconnect(layer, &QgsVectorLayer::willBeDeleted, this, &QgsVertexTool::clearGeometryCache);
    disconnect(layer, &QgsVectorLayer::dataChanged, this, &QgsVertexTool::clearGeometryCache);
}

void QgsVertexTool::onCachedGeometryChanged(QgsFeatureId fid, const QgsGeometry &geom)
{
    QgsVectorLayer *layer = qobject_cast<QgsVectorLayer*>(sender());
    Q_ASSERT(mCache.contains(layer));
    QHash<QgsFeatureId, QgsGeometry> &layerCache = mCache[layer];
    if (layerCache.contains(fid))
    {
        layerCache[fid] = geom;
    }

    setHighlightedVertices(mSelectedVertices);

    validateGeometry(layer, fid);

    if (mLockedFeature && mLockedFeature->featureId() == fid && mLockedFeature->layer() == layer)
    {
        mLockedFeature->geometryChanged(fid, geom);
        if (mVertexEditor)
        {
            mVertexEditor->updateEditor(mLockedFeature.get());
        }
        updateLockedFeatureVertices();
    }
}

void QgsVertexTool::onCachedGeometryDeleted(QgsFeatureId fid)
{
    QgsVectorLayer *layer = qobject_cast<QgsVectorLayer*>(sender());
    Q_ASSERT(mCache.contains(layer));
    QHash<QgsFeatureId, QgsGeometry> &layerCache = mCache[layer];
    if (layerCache.contains(fid))
    {
        layerCache.remove(fid);
    }

    setHighlightedVertices(mSelectedVertices);

    if (mLockedFeature && mLockedFeature->featureId() == fid && mLockedFeature->layer() == layer)
    {
        mLockedFeature->featureDeleted(fid);
        updateLockedFeatureVertices();
    }
}

void QgsVertexTool::updateVertexEditor(QgsVectorLayer *layer, QgsFeatureId fid)
{
    if (layer)
    {
        if (mLockedFeature && mLockedFeature->featureId() == fid && mLockedFeature->layer() == layer)
        {
            // Если вызывается для объекта, который уже привязан к редактору вершин, отключаем его
            cleanupLockedFeature();
            return;
        }

        mLockedFeature.reset(new QgsLockedFeature(fid, layer, mCanvas));
        connect(mLockedFeature->layer(), &QgsVectorLayer::featureDeleted, this, &QgsVertexTool::cleanEditor);
        for (int i = 0; i < mSelectedVertices.length(); ++i)
        {
            if (mSelectedVertices.at(i).layer == layer && mSelectedVertices.at(i).fid == fid)
            {
                mLockedFeature->selectVertex(mSelectedVertices.at(i).vertexId);
            }
        }
        connect(mLockedFeature.get(), &QgsLockedFeature::selectionChanged, this, &QgsVertexTool::lockedFeatureSelectionChanged);
    }

    updateLockedFeatureVertices();

    showVertexEditor();

    mVertexEditor->updateEditor(mLockedFeature.get());
}

void QgsVertexTool::updateLockedFeatureVertices()
{
    qDeleteAll(mLockedFeatureVerticesMarkers);
    mLockedFeatureVerticesMarkers.clear();
    if (mVertexEditor && mLockedFeature)
    {
        const QList<QgsVertexEntry*> &vertexMap = mLockedFeature->vertexMap();
        for (const QgsVertexEntry *vertex : vertexMap)
        {
            if (!vertex->isSelected())
            {
                QgsVertexMarker *marker = new QgsVertexMarker(canvas());
                marker->setIconType(QgsVertexMarker::ICON_CIRCLE);
                marker->setIconSize(QgsGuiUtils::scaleIconSize(10));
                marker->setPenWidth(QgsGuiUtils::scaleIconSize(2));
                marker->setColor(Qt::red);
                marker->setCenter(toMapCoordinates(mLockedFeature->layer(), vertex->point()));
                mLockedFeatureVerticesMarkers.append(marker);
            }
        }
    }
}

void QgsVertexTool::showVertexEditor() 
{
    if (!mVertexEditor)
    {
        mVertexEditor = new QgsVertexEditor(mCanvas);

        connect(mVertexEditor, &QgsVertexEditor::deleteSelectedRequested, this, &QgsVertexTool::deleteVertexEditorSelection);
        connect(mVertexEditor, &QgsVertexEditor::editorClosed, this, &QgsVertexTool::cleanupVertexEditor);

        QTimer::singleShot(200, this, [=] { if (mVertexEditor) { mVertexEditor->show(); mVertexEditor->raise(); } } );
    }
    else
    {
        mVertexEditor->show();
        mVertexEditor->raise();
    }
}

void QgsVertexTool::cleanupVertexEditor()
{
    if (mVertexEditor)
    {
        cleanupLockedFeature();
        // Не удаляем немедленно, так как редактор вершин
        // может все еще использоваться в цикле событий qt
        mVertexEditor->deleteLater();
    }
}

void QgsVertexTool::cleanupLockedFeature()
{
    mLockedFeature.reset();
    if (mVertexEditor)
    {
        mVertexEditor->updateEditor(nullptr);
    }
    updateLockedFeatureVertices();
}

void QgsVertexTool::lockedFeatureSelectionChanged()
{
    Q_ASSERT(mLockedFeature);
    QList<QgsVertexEntry*> &vertexMap = mLockedFeature->vertexMap();
    QList<Vertex> vertices;
    for (int i = 0, n = vertexMap.size(); i < n; ++i)
    {
        if (vertexMap[i]->isSelected())
        {
            vertices << Vertex(mLockedFeature->layer(), mLockedFeature->featureId(), i);
        }
    }

    setHighlightedVertices(vertices, ModeReset);

    updateLockedFeatureVertices();
}

static int _firstSelectedVertex(QgsLockedFeature &selectedFeature)
{
    QList<QgsVertexEntry *> &vertexMap = selectedFeature.vertexMap();
    for (int i = 0, n = vertexMap.size(); i < n; ++i)
    {
        if (vertexMap[i]->isSelected())
        {
            return i;
        }
    }
    return -1;
}

static void _safeSelectVertex(QgsLockedFeature &selectedFeature, int vertexNr)
{
    int n = selectedFeature.vertexMap().size();
    selectedFeature.selectVertex((vertexNr + n) % n);
}

void QgsVertexTool::deleteVertexEditorSelection()
{
    if (!mLockedFeature)
    {
        return;
    }

    int firstSelectedIndex = _firstSelectedVertex(*mLockedFeature);
    if (firstSelectedIndex == -1)
    {
        return;
    }

    // Создаем список выбранных вершин
    QList<Vertex> vertices;
    QList<QgsVertexEntry*> &selFeatureVertices = mLockedFeature->vertexMap();
    QgsVectorLayer *layer = mLockedFeature->layer();
    QgsFeatureId fid = mLockedFeature->featureId();
    QgsGeometry geometry = cachedGeometry(layer, fid);
    for (QgsVertexEntry *vertex : std::as_const(selFeatureVertices))
    {
        if (vertex->isSelected())
        {
            int vertexIndex = geometry.vertexNrFromVertexId(vertex->vertexId());
            if (vertexIndex != -1)
            {
                vertices.append(Vertex(layer, fid, vertexIndex));
            }
        }
    }

    // Теперь выбираем вершины и удаляем их
    setHighlightedVertices(vertices);
    deleteVertex();

    if (!mLockedFeature->geometry()->isNull())
    {
        int nextVertexToSelect = firstSelectedIndex;
        if (mLockedFeature->geometry()->type() == Qgis::GeometryType::Line)
        {
            nextVertexToSelect = std::min(nextVertexToSelect, mLockedFeature->geometry()->constGet()->nCoordinates() - 1);
        }

        _safeSelectVertex(*mLockedFeature, nextVertexToSelect);
    }
    mLockedFeature->layer()->triggerRepaint();
}

void QgsVertexTool::startDragging(QgsMapMouseEvent *e)
{
    QgsPointXY mapPoint = toMapCoordinates(e->pos());
    if (isNearEndpointMarker(mapPoint))
    {
        startDraggingAddVertexAtEndpoint(mapPoint);
        return;
    }

    QgsPointLocator::Match m = snapToEditableLayer(e);
    if (!m.isValid())
    {
        return;
    }
    if (mLockedFeature && (mLockedFeature->featureId() != m.featureId() || mLockedFeature->layer() != m.layer()))
    {
        return;
    }

    setAdvancedDigitizingAllowed(true);

    // Добавление новой вершины вместо перемещения вершины
    if (m.hasEdge())
    {
        // Начинаем перетаскивание только если мы возле центра ребра
        mapPoint = toMapCoordinates(e->pos());
        bool isNearCenter = matchEdgeCenterTest(m, mapPoint);
        if (isNearCenter)
        {
            startDraggingAddVertex(m);
        }
        else
        {
            startDraggingEdge(m, mapPoint);
        }
    }
    else
    {
        startDraggingMoveVertex(m);
    }
}

QList<QgsVectorLayer *> QgsVertexTool::editableVectorLayers()
{
    QList<QgsVectorLayer *> editableLayers;
    const auto layers = canvas()->layers(true);
    for (QgsMapLayer *layer : layers)
    {
        QgsVectorLayer *vlayer = qobject_cast<QgsVectorLayer *>(layer);
        if (vlayer && vlayer->isEditable() && vlayer->isSpatial())
        {
            editableLayers << vlayer;
        }
    }
    return editableLayers;
}

QSet<Vertex> QgsVertexTool::findCoincidentVertices(const QSet<Vertex> &vertices)
{
    QSet<Vertex> topoVertices;
    const auto editableLayers = editableVectorLayers();

    for (const Vertex &v : std::as_const(vertices))
    {
        QgsPointXY origPointV = cachedGeometryForVertex(v).vertexAt(v.vertexId);
        QgsPointXY mapPointV = toMapCoordinates(v.layer, origPointV);
        for (QgsVectorLayer *vlayer : editableLayers)
        {
            const auto snappedVertices = layerVerticesSnappedToPoint(vlayer, mapPointV);
            for (const QgsPointLocator::Match &otherMatch : snappedVertices)
            {
                Vertex otherVertex( otherMatch.layer(), otherMatch.featureId(), otherMatch.vertexIndex() );
                if (!vertices.contains(otherVertex))
                {
                    topoVertices << otherVertex;
                }
            }
        }
    }
    return topoVertices;
}

void QgsVertexTool::buildExtraVertices(const QSet<Vertex> &vertices, const QgsPointXY &anchorPoint, QgsVectorLayer *anchorLayer)
{
    for (const Vertex &v : std::as_const(vertices))
    {
        if (mDraggingVertex && v == *mDraggingVertex)
        {
            continue;
        }

        QgsPointXY pointV = cachedGeometryForVertex(v).vertexAt(v.vertexId);

        QgsPointXY anchorPointLayer;
        if (v.layer->crs() == anchorLayer->crs())
        {
            anchorPointLayer = anchorPoint;
        }
        else
        {
            anchorPointLayer = toLayerCoordinates(v.layer, toMapCoordinates(anchorLayer, anchorPoint));
        }
        QgsVector offset = pointV - anchorPointLayer;

        mDraggingExtraVertices << v;
        mDraggingExtraVerticesOffset << offset;
    }
}

void QgsVertexTool::startDraggingMoveVertex(const QgsPointLocator::Match &m)
{
    Q_ASSERT(m.hasVertex());

    QgsGeometry geom = cachedGeometry(m.layer(), m.featureId());

    mDraggingVertex.reset(new Vertex(m.layer(), m.featureId(), m.vertexIndex()));
    mDraggingVertexType = MovingVertex;
    mDraggingExtraVertices.clear();
    mDraggingExtraVerticesOffset.clear();

    // Скрываем любое дополнительное выделение вершин до завершения перемещения
    QSet<Vertex> movingVertices;
    movingVertices << *mDraggingVertex;
    for (const Vertex &v : std::as_const(mSelectedVertices))
    {
        movingVertices << v;
    }

    if (QgsProject::instance()->topologicalEditing())
    {
        movingVertices.unite(findCoincidentVertices(movingVertices));
    }

    buildExtraVertices(movingVertices, geom.vertexAt(m.vertexIndex()), m.layer());

    cadDockWidget()->setPoints(QList<QgsPointXY>() << m.point() << m.point());

    QgsPointXY dragVertexMapPoint = m.point();

    buildDragBandsForVertices(movingVertices, dragVertexMapPoint);
}

void QgsVertexTool::buildDragBandsForVertices(const QSet<Vertex> &movingVertices, const QgsPointXY &dragVertexMapPoint)
{
    QSet<Vertex> verticesInStraightBands;  // Вершина с меньшим индексом

    QSet<Vertex> verticesInCircularBands;

    for (const Vertex &v : std::as_const(movingVertices))
    {
        int v0idx, v1idx;
        QgsGeometry geom = cachedGeometry(v.layer, v.fid);
        QgsPointXY pt = geom.vertexAt(v.vertexId);

        geom.adjacentVertices(v.vertexId, v0idx, v1idx);

        if (v0idx != -1 && v1idx != -1 && isCircularVertex(geom, v.vertexId))
        {
            if (!verticesInCircularBands.contains(v))
            {
                addDragCircularBand(v.layer,
                                    geom.vertexAt(v0idx),
                                    pt,
                                    geom.vertexAt(v1idx),
                                    movingVertices.contains(Vertex(v.layer, v.fid, v0idx)),
                                    true,
                                    movingVertices.contains(Vertex(v.layer, v.fid, v1idx)),
                                    dragVertexMapPoint);
                verticesInCircularBands << v;
            }

            continue;
        }

        if (v0idx != -1)
        {
            Vertex v0(v.layer, v.fid, v0idx);
            if (isCircularVertex(geom, v0idx))
            {
                if (!verticesInCircularBands.contains(v0))
                {
                    addDragCircularBand(v.layer,
                                        geom.vertexAt(v0idx - 1),
                                        geom.vertexAt(v0idx),
                                        pt,
                                        movingVertices.contains(Vertex(v.layer, v.fid, v0idx - 1)),
                                        movingVertices.contains(Vertex(v.layer, v.fid, v0idx)),
                                        true,
                                        dragVertexMapPoint);
                    verticesInCircularBands << v0;
                }
            }
            else
            {
                if (!verticesInStraightBands.contains(v0))
                {
                    addDragStraightBand(v.layer,
                                        geom.vertexAt(v0idx),
                                        pt,
                                        movingVertices.contains(v0),
                                        true,
                                        dragVertexMapPoint);
                    verticesInStraightBands << v0;
                }
            }
        }

        if (v1idx != -1)
        {
            Vertex v1(v.layer, v.fid, v1idx);
            if ( isCircularVertex(geom, v1idx))
            {
                if (!verticesInCircularBands.contains(v1))
                {
                    addDragCircularBand(v.layer,
                                        pt,
                                        geom.vertexAt(v1idx),
                                        geom.vertexAt(v1idx + 1),
                                        true,
                                        movingVertices.contains(v1),
                                        movingVertices.contains(Vertex(v.layer, v.fid, v1idx + 1)),
                                        dragVertexMapPoint);
                    verticesInCircularBands << v1;
                }
            }
            else
            {
                if (!verticesInStraightBands.contains(v))
                {
                    addDragStraightBand(v.layer,
                                        pt,
                                        geom.vertexAt(v1idx),
                                        true,
                                        movingVertices.contains(v1),
                                        dragVertexMapPoint);
                    verticesInStraightBands << v;
                }
            }
        }

        if (v0idx == -1 && v1idx == -1)
        {
            // Это отдельная точка, нужно использовать маркер для нее,
            // чтобы визуализировать для пользователя

            QgsPointXY ptMapPoint = toMapCoordinates(v.layer, pt);
            QgsVertexMarker *marker = new QgsVertexMarker(mCanvas);
            marker->setIconType(QgsVertexMarker::ICON_X);
            marker->setColor(Qt::red);
            marker->setIconSize(QgsGuiUtils::scaleIconSize(10));
            marker->setPenWidth(QgsGuiUtils::scaleIconSize(3));
            marker->setVisible(true);
            marker->setCenter(ptMapPoint);
            mDragPointMarkers << marker;
            mDragPointMarkersOffset << (ptMapPoint - dragVertexMapPoint);
        }
    }
}

QList<QgsPointLocator::Match> QgsVertexTool::layerVerticesSnappedToPoint(QgsVectorLayer *layer, const QgsPointXY &mapPoint)
{
    MatchCollectingFilter myfilter(this);
    QgsPointLocator *loc = canvas()->snappingUtils()->locatorForLayer(layer);
    loc->nearestVertex(mapPoint, 0, &myfilter, true);
    return myfilter.matches;
}

QList<QgsPointLocator::Match> QgsVertexTool::layerSegmentsSnappedToSegment(QgsVectorLayer *layer, const QgsPointXY &mapPoint1, const QgsPointXY &mapPoint2)
{
    QList<QgsPointLocator::Match> finalMatches;
    // Мы хотим совпадения ребер, которые имеют точно такие же вершины, как данный сегмент (mapPoint1, mapPoint2)
    // Поэтому вместо поиска ближайшего ребра, которое может вернуть любой сегмент в пределах допуска,
    // мы сначала находим совпадения для одной конечной точки, а затем смотрим, есть ли совпадающая другая конечная точка.
    const QList<QgsPointLocator::Match> matches1 = layerVerticesSnappedToPoint(layer, mapPoint1);
    for (const QgsPointLocator::Match &m : matches1)
    {
        QgsGeometry g = cachedGeometry(layer, m.featureId());
        int v0, v1;
        g.adjacentVertices(m.vertexIndex(), v0, v1);
        if (v0 != -1 && QgsPointXY(g.vertexAt(v0)) == mapPoint2)
        {
            finalMatches << QgsPointLocator::Match( QgsPointLocator::Edge, layer, m.featureId(), 0, m.point(), v0);
        }
        else if (v1 != -1 && QgsPointXY(g.vertexAt(v1)) == mapPoint2)
        {
            finalMatches << QgsPointLocator::Match(QgsPointLocator::Edge, layer, m.featureId(), 0, m.point(), m.vertexIndex());
        }
    }
    return finalMatches;
}

void QgsVertexTool::startDraggingAddVertex(const QgsPointLocator::Match &m)
{
    Q_ASSERT(m.hasEdge());

    setAdvancedDigitizingAllowed(true);

    mDraggingVertex.reset(new Vertex(m.layer(), m.featureId(), m.vertexIndex() + 1));
    mDraggingVertexType = AddingVertex;
    mDraggingExtraVertices.clear();
    mDraggingExtraVerticesOffset.clear();
    mDraggingExtraSegments.clear();

    QgsGeometry geom = cachedGeometry(m.layer(), m.featureId());

    QgsPointXY v0 = geom.vertexAt(m.vertexIndex());
    QgsPointXY v1 = geom.vertexAt(m.vertexIndex() + 1);

    QgsPointXY map_v0 = toMapCoordinates(m.layer(), v0);
    QgsPointXY map_v1 = toMapCoordinates(m.layer(), v1);

    if (v0.x() != 0 || v0.y() != 0)
    {
        addDragBand( map_v0, m.point() );
    }
    if (v1.x() != 0 || v1.y() != 0)
    {
        addDragBand(map_v1, m.point());
    }

    if (QgsProject::instance()->topologicalEditing())
    {
        const auto editableLayers = editableVectorLayers();
        for (QgsVectorLayer *vlayer : editableLayers)
        {
            if (vlayer->geometryType() != Qgis::GeometryType::Line && vlayer->geometryType() != Qgis::GeometryType::Polygon)
            {
                continue;
            }

            QgsPointXY pt1, pt2;
            m.edgePoints(pt1, pt2);
            const auto snappedSegments = layerSegmentsSnappedToSegment(vlayer, pt1, pt2);
            for (const QgsPointLocator::Match &otherMatch : snappedSegments)
            {
                if (otherMatch.layer() == m.layer() && otherMatch.featureId() == m.featureId() && otherMatch.vertexIndex() == m.vertexIndex())
                {
                    continue;
                }

                mDraggingExtraSegments << Vertex(otherMatch.layer(), otherMatch.featureId(), otherMatch.vertexIndex());
            }
        }
    }

    cadDockWidget()->setPoints(QList<QgsPointXY>() << m.point() << m.point());
}

void QgsVertexTool::startDraggingAddVertexAtEndpoint(const QgsPointXY &mapPoint)
{
    Q_ASSERT(mMouseAtEndpoint);

    setAdvancedDigitizingAllowed(true);

    mDraggingVertex.reset(new Vertex(mMouseAtEndpoint->layer, mMouseAtEndpoint->fid, mMouseAtEndpoint->vertexId));
    mDraggingVertexType = AddingEndpoint;
    mDraggingExtraVertices.clear();
    mDraggingExtraVerticesOffset.clear();

    QgsGeometry geom = cachedGeometry(mMouseAtEndpoint->layer, mMouseAtEndpoint->fid);
    QgsPointXY v0 = geom.vertexAt(mMouseAtEndpoint->vertexId);
    QgsPointXY map_v0 = toMapCoordinates(mMouseAtEndpoint->layer, v0);

    addDragBand(map_v0, mapPoint);

    QgsPointXY pt0 = geom.vertexAt(adjacentVertexIndexToEndpoint(geom, mMouseAtEndpoint->vertexId));
    QgsPointXY pt1 = geom.vertexAt(mMouseAtEndpoint->vertexId);

    cadDockWidget()->setPoints(QList<QgsPointXY>() << pt0 << pt1 << pt1);
}

void QgsVertexTool::startDraggingEdge(const QgsPointLocator::Match &m, const QgsPointXY &mapPoint)
{
    Q_ASSERT(m.hasEdge());

    setAdvancedDigitizingAllowed(true);

    mDraggingEdge = true;
    mDraggingExtraVertices.clear();
    mDraggingExtraVerticesOffset.clear();

    QgsGeometry geom = cachedGeometry(m.layer(), m.featureId());

    QSet<Vertex> movingVertices;
    movingVertices << Vertex(m.layer(), m.featureId(), m.vertexIndex());
    movingVertices << Vertex(m.layer(), m.featureId(), m.vertexIndex() + 1);

    if (isCircularVertex(geom, m.vertexIndex()))
    {
        movingVertices << Vertex(m.layer(), m.featureId(), m.vertexIndex() - 1);
    }
    else if (isCircularVertex(geom, m.vertexIndex() + 1))
    {
        movingVertices << Vertex(m.layer(), m.featureId(), m.vertexIndex() + 2);
    }

    buildDragBandsForVertices(movingVertices, mapPoint);

    if (QgsProject::instance()->topologicalEditing())
    {
        movingVertices.unite(findCoincidentVertices(movingVertices));
    }

    QgsPointXY layerPoint = toLayerCoordinates(m.layer(), mapPoint);
    buildExtraVertices(movingVertices, layerPoint, m.layer());

    cadDockWidget()->setPoints(QList<QgsPointXY>() << m.point() << m.point());
}

void QgsVertexTool::stopDragging()
{
    setAdvancedDigitizingAllowed(false);
    cadDockWidget()->clear();

    mDraggingVertex.reset();
    mDraggingVertexType = NotDragging;
    mDraggingEdge = false;
    clearDragBands();

    setHighlightedVerticesVisible(true);

    mSnapIndicator->setMatch(QgsPointLocator::Match());
}

QgsPoint QgsVertexTool::matchToLayerPoint(const QgsVectorLayer *destLayer, const QgsPointXY &mapPoint, const QgsPointLocator::Match *match)
{
    if (match->layer())
    {
        switch (match->type())
        {
        case QgsPointLocator::Vertex:
        case QgsPointLocator::LineEndpoint:
        case QgsPointLocator::All:
        {
            QgsFeature f;
            QgsFeatureIterator fi = match->layer()->getFeatures(QgsFeatureRequest(match->featureId()).setNoAttributes());
            if (fi.nextFeature(f))
            {
                QgsPoint layerPoint = f.geometry().vertexAt(match->vertexIndex());
                if (match->layer()->crs() == destLayer->crs())
                {
                    return layerPoint;
                }
                else
                {
                    QgsCoordinateTransform transform(match->layer()->crs(), destLayer->crs(), mCanvas->mapSettings().transformContext());
                    if (transform.isValid())
                    {
                        try
                        {
                            layerPoint.transform(transform);
                            return layerPoint;
                        }
                        catch (QgsCsException &)
                        {

                        }
                    }
                    return layerPoint;
                }
            }
        }
            break;
        case QgsPointLocator::Edge:
        case QgsPointLocator::MiddleOfSegment:
            return toLayerCoordinates(destLayer, match->interpolatedPoint(mCanvas->mapSettings().destinationCrs()));
            break;
        case QgsPointLocator::Invalid:
        case QgsPointLocator::Area:
        case QgsPointLocator::Centroid:
            break;
        }
    }

    return QgsPoint(toLayerCoordinates(destLayer, mapPoint));
}

void QgsVertexTool::moveEdge(const QgsPointXY &mapPoint)
{
    stopDragging();

    VertexEdits edits;
    addExtraVerticesToEdits(edits, mapPoint);

    applyEditsToLayers(edits);
}

void QgsVertexTool::moveVertex(const QgsPointXY &mapPoint, const QgsPointLocator::Match *mapPointMatch)
{
    setAdvancedDigitizingAllowed(false);

    QgsVectorLayer *dragLayer = mDraggingVertex->layer;
    QgsFeatureId dragFid = mDraggingVertex->fid;

    int dragVertexId = mDraggingVertex->vertexId;
    bool addingVertex = mDraggingVertexType == AddingVertex || mDraggingVertexType == AddingEndpoint;
    bool addingAtEndpoint = mDraggingVertexType == AddingEndpoint;
    QgsGeometry geom = cachedGeometryForVertex(*mDraggingVertex);
    stopDragging();

    QgsPoint layerPoint = matchToLayerPoint(dragLayer, mapPoint, mapPointMatch);

    QgsVertexId vid;
    if (!geom.vertexIdFromVertexNr(dragVertexId, vid))
    {
        return;
    }

    QgsAbstractGeometry *geomTmp = geom.constGet()->clone();

    // Добавляем/перемещаем вершину
    if (addingVertex)
    {
        if (addingAtEndpoint && vid.vertex != 0)
        {
            vid.vertex++;
        }

        QgsPoint pt(layerPoint);
        if (QgsWkbTypes::hasZ(dragLayer->wkbType()) && !pt.is3D())
            pt.addZValue(defaultZValue());

        if (!geomTmp->insertVertex(vid, pt))
        {
            return;
        }
    }
    else
    {
        if (!geomTmp->moveVertex(vid, layerPoint))
        {
            return;
        }
    }

    geom.set(geomTmp);

    VertexEdits edits;
    edits[dragLayer][dragFid] = geom;

    addExtraVerticesToEdits(edits, mapPoint, dragLayer, layerPoint);

    if (addingVertex && !addingAtEndpoint && QgsProject::instance()->topologicalEditing())
    {
        addExtraSegmentsToEdits(edits, mapPoint, dragLayer, layerPoint);
    }

    applyEditsToLayers(edits);

    if (QgsProject::instance()->topologicalEditing())
    {
        const auto editKeys = edits.keys();
        for (QgsVectorLayer *layer : editKeys)
        {
            const auto editGeom = edits[layer].values();
            for (const QgsGeometry &g : editGeom)
            {
                QgsGeometry p = QgsGeometry::fromPointXY(QgsPointXY(layerPoint.x(), layerPoint.y()));
                if (((mapPointMatch->hasEdge() || mapPointMatch->hasMiddleSegment() ) && mapPointMatch->layer() && layer->crs() == mapPointMatch->layer()->crs())
                        || (mapPointMatch->hasVertex() && !mapPointMatch->layer() && layer->crs() == mCanvas->mapSettings().destinationCrs()))
                {
                    if (g.convertToType(Qgis::GeometryType::Point, true).contains(p))
                    {
                        if (!layerPoint.is3D())
                        {
                            layerPoint.addZValue(defaultZValue());
                        }
                        layer->addTopologicalPoints(layerPoint);
                        if (mapPointMatch->layer())
                        {
                            mapPointMatch->layer()->addTopologicalPoints(layerPoint);
                        }
                    }
                }
                if (QgsProject::instance()->avoidIntersectionsMode() != Qgis::AvoidIntersectionsMode::AllowIntersections)
                {
                    for (QgsAbstractGeometry::vertex_iterator it = g.vertices_begin() ; it != g.vertices_end() ; it++)
                    {
                        layer->addTopologicalPoints(*it);
                    }
                }
            }
        }
    }

    updateLockedFeatureVertices();
    if (mVertexEditor)
    {
        mVertexEditor->updateEditor(mLockedFeature.get());
    }

    // Обновляем позиции существующих выделенных вершин
    setHighlightedVertices(mSelectedVertices);
    setHighlightedVerticesVisible(true);  // Показываем выделенные вершины

    // Перезапускаем startDraggingAddVertexAtEndpoint сразу после его завершения
    if (addingAtEndpoint)
    {
        if (mMouseAtEndpoint->vertexId != 0)
        {
            // Если мы добавляли в конец объекта, нужно обновить индекс
            mMouseAtEndpoint.reset(new Vertex( mMouseAtEndpoint->layer, mMouseAtEndpoint->fid, mMouseAtEndpoint->vertexId + 1));
        }
        // И затем перезапускаем перетаскивание
        startDraggingAddVertexAtEndpoint(mapPoint);
    }
}

void QgsVertexTool::addExtraVerticesToEdits(QgsVertexTool::VertexEdits &edits, const QgsPointXY &mapPoint, QgsVectorLayer *dragLayer, const QgsPoint &layerPoint)
{
    Q_ASSERT(mDraggingExtraVertices.count() == mDraggingExtraVerticesOffset.count());
    // Добавляем перемещенные вершины из других слоев
    for (int i = 0; i < mDraggingExtraVertices.count(); ++i)
    {
        const Vertex &topo = mDraggingExtraVertices[i];
        const QgsVector &offset = mDraggingExtraVerticesOffset[i];

        QHash<QgsFeatureId, QgsGeometry> &layerEdits = edits[topo.layer];
        QgsGeometry topoGeom;
        if (layerEdits.contains(topo.fid))
        {
            topoGeom = QgsGeometry(edits[topo.layer][topo.fid]);
        }
        else
        {
            topoGeom = QgsGeometry(cachedGeometryForVertex(topo));
        }
        QgsPoint point;
        if (dragLayer && topo.layer->crs() == dragLayer->crs())
        {
            point = layerPoint;
        }
        else
        {
            point = QgsPoint(toLayerCoordinates(topo.layer, mapPoint));
        }

        if (offset.x() || offset.y())
        {
            point += offset;
        }

        if (!topoGeom.moveVertex(point.x(), point.y(), topo.vertexId))
        {
            continue;
        }
        edits[topo.layer][topo.fid] = topoGeom;
    }
}

void QgsVertexTool::addExtraSegmentsToEdits(QgsVertexTool::VertexEdits &edits, const QgsPointXY &mapPoint, QgsVectorLayer *dragLayer, const QgsPoint &layerPoint)
{
    // Вставляем новую вершину также в другие геометрии/слои
    for (int i = 0; i < mDraggingExtraSegments.count(); ++i)
    {
        const Vertex &topo = mDraggingExtraSegments[i];

        QHash<QgsFeatureId, QgsGeometry> &layerEdits = edits[topo.layer];
        QgsGeometry topoGeom;
        if (layerEdits.contains(topo.fid))
        {
            topoGeom = QgsGeometry(edits[topo.layer][topo.fid]);
        }
        else
        {
            topoGeom = QgsGeometry(cachedGeometryForVertex(topo));
        }

        QgsPointXY point;
        if (dragLayer && topo.layer->crs() == dragLayer->crs())
        {
            point = layerPoint;
        }
        else
        {
            point = QgsPoint(toLayerCoordinates(topo.layer, mapPoint));
        }

        QgsPoint pt(point);
        if (QgsWkbTypes::hasZ(topo.layer->wkbType()))
            pt.addZValue(defaultZValue());

        if (!topoGeom.insertVertex(pt, topo.vertexId + 1))
        {
            continue;
        }
        edits[topo.layer][topo.fid] = topoGeom;
    }
}

void QgsVertexTool::applyEditsToLayers(QgsVertexTool::VertexEdits &edits)
{
    QHash<QgsVectorLayer *, QHash<QgsFeatureId, QgsGeometry> >::iterator it = edits.begin();
    for (; it != edits.end(); ++it)
    {
        QgsVectorLayer *layer = it.key();
        QHash<QgsFeatureId, QgsGeometry> &layerEdits = it.value();
        layer->beginEditCommand(tr("Перемещение вершины"));
        QHash<QgsFeatureId, QgsGeometry>::iterator it2 = layerEdits.begin();
        for (; it2 != layerEdits.end(); ++it2)
        {
            QgsGeometry featGeom = it2.value();
            layer->changeGeometry(it2.key(), featGeom);
            edits[layer][it2.key()] = featGeom;
        }

        if (mVertexEditor)
        {
            mVertexEditor->updateEditor(mLockedFeature.get());
        }
    }

    for (it = edits.begin() ; it != edits.end(); ++it)
    {
        QgsVectorLayer *layer = it.key();
        QHash<QgsFeatureId, QgsGeometry> &layerEdits = it.value();
        QHash<QgsFeatureId, QgsGeometry>::iterator it2 = layerEdits.begin();
        for (; it2 != layerEdits.end(); ++it2)
        {
            QList<QgsVectorLayer *>  avoidIntersectionsLayers;
            switch (QgsProject::instance()->avoidIntersectionsMode())
            {
            case Qgis::AvoidIntersectionsMode::AvoidIntersectionsCurrentLayer:
                avoidIntersectionsLayers.append(layer);
                break;
            case Qgis::AvoidIntersectionsMode::AvoidIntersectionsLayers:
                avoidIntersectionsLayers = QgsProject::instance()->avoidIntersectionsLayers();
                break;
            case Qgis::AvoidIntersectionsMode::AllowIntersections:
                break;
            }
            QgsGeometry featGeom = it2.value();
            layer->changeGeometry(it2.key(), featGeom);
            if (avoidIntersectionsLayers.size() > 0)
            {
                QHash<QgsVectorLayer *, QSet<QgsFeatureId> > ignoreFeatures;
                QSet<QgsFeatureId> id;
                id.insert(it2.key());
                ignoreFeatures.insert(layer, id);
                int avoidIntersectionsReturn = featGeom.avoidIntersections(avoidIntersectionsLayers, ignoreFeatures);
                switch (avoidIntersectionsReturn)
                {
                case 2:
                    emit messageEmitted(tr("Операция изменит тип геометрии."), Qgis::MessageLevel::Warning);
                    break;

                case 3:
                    emit messageEmitted(tr("По крайней мере, одна из пересекающихся геометрий является недопустимой. Ошибки нужно исправить вручную."), Qgis::MessageLevel::Warning);
                    break;
                default:
                    break;
                }
            }
            layer->changeGeometry(it2.key(), featGeom);
            edits[layer][it2.key()] = featGeom;
        }
        layer->endEditCommand();
        layer->triggerRepaint();

        if (mVertexEditor)
        {
            mVertexEditor->updateEditor(mLockedFeature.get());
        }
    }

}

void QgsVertexTool::deleteVertex()
{
    QSet<Vertex> toDelete;
    if (!mSelectedVertices.isEmpty())
    {
        toDelete = qgis::listToSet(mSelectedVertices);
    }
    else
    {
        bool addingVertex = mDraggingVertexType == AddingVertex || mDraggingVertexType == AddingEndpoint;
        toDelete << *mDraggingVertex;
        toDelete += qgis::listToSet(mDraggingExtraVertices);

        if (addingVertex)
        {
            stopDragging();
            return;
        }
    }

    stopDragging();
    setHighlightedVertices(QList<Vertex>());
    if (QgsProject::instance()->topologicalEditing())
    {
        QSet<Vertex> topoVerticesToDelete;
        for (const Vertex &vertexToDelete : std::as_const(toDelete))
        {
            QgsPointXY layerPt = cachedGeometryForVertex(vertexToDelete).vertexAt(vertexToDelete.vertexId);
            QgsPointXY mapPt = toMapCoordinates(vertexToDelete.layer, layerPt);
            const auto snappedVertices = layerVerticesSnappedToPoint(vertexToDelete.layer, mapPt);
            for (const QgsPointLocator::Match &otherMatch : snappedVertices)
            {
                Vertex otherVertex(otherMatch.layer(), otherMatch.featureId(), otherMatch.vertexIndex());
                if (toDelete.contains(otherVertex) || topoVerticesToDelete.contains(otherVertex))
                {
                    continue;
                }

                topoVerticesToDelete.insert(otherVertex);
            }
        }

        toDelete.unite(topoVerticesToDelete);
    }

    QHash<QgsVectorLayer*, QHash<QgsFeatureId, QList<int>>> toDeleteGrouped;
    for (const Vertex &vertex : std::as_const(toDelete))
    {
        toDeleteGrouped[vertex.layer][vertex.fid].append(vertex.vertexId);
    }

    QHash<QgsVectorLayer*, QHash<QgsFeatureId, QList<int>>>::iterator lIt = toDeleteGrouped.begin();
    for (; lIt != toDeleteGrouped.end(); ++lIt)
    {
        QgsVectorLayer *layer = lIt.key();
        QHash<QgsFeatureId, QList<int>> &featuresDict = lIt.value();

        QHash<QgsFeatureId, QList<int>>::iterator fIt = featuresDict.begin();
        for (; fIt != featuresDict.end(); ++fIt)
        {
            QgsFeatureId fid = fIt.key();
            QList<int> &vertexIds = fIt.value();
            if (vertexIds.count() >= 2 && (layer->geometryType() == Qgis::GeometryType::Polygon ||
                                           layer->geometryType() == Qgis::GeometryType::Line))
            {
                std::sort(vertexIds.begin(), vertexIds.end(), std::greater<int>());
                const QgsGeometry geom = cachedGeometry(layer, fid);
                const QgsAbstractGeometry *ag = geom.constGet();
                QVector<QVector<int>> numberOfVertices;
                for (int p = 0 ; p < ag->partCount() ; ++p)
                {
                    numberOfVertices.append(QVector<int>());
                    for (int r = 0 ; r < ag->ringCount(p) ; ++r)
                    {
                        numberOfVertices[p].append(ag->vertexCount(p, r));
                    }
                }
                // Полигональные кольца с менее чем 4 вершинами удаляются автоматически
                // Линейные части с менее чем 2 вершинами удаляются автоматически
                const int minAllowedVertices = geom.type() == Qgis::GeometryType::Polygon ? 4 : 2;
                for (int i = vertexIds.count() - 1; i >= 0 ; --i)
                {
                    QgsVertexId vid;
                    if (geom.vertexIdFromVertexNr(vertexIds[i], vid))
                    {
                        // Также не пытаемся удалить первую вершину кольца, так как мы уже удалили последнюю
                        if (numberOfVertices.at(vid.part).at(vid.ring) < minAllowedVertices ||
                                (0 == vid.vertex && geom.type() == Qgis::GeometryType::Polygon))
                        {
                            vertexIds.removeOne(vertexIds.at(i));
                        }
                        else
                        {
                            --numberOfVertices[vid.part][vid.ring];
                        }
                    }
                }
            }
        }
    }

    // Основной цикл для удаления всех выбранных вершин
    QHash<QgsVectorLayer*, QHash<QgsFeatureId, QList<int>>>::iterator it = toDeleteGrouped.begin();
    for (; it != toDeleteGrouped.end(); ++it)
    {
        QgsVectorLayer *layer = it.key();
        QHash<QgsFeatureId, QList<int> > &featuresDict = it.value();

        layer->beginEditCommand(tr("Удаление вершины"));
        bool success = true;

        QHash<QgsFeatureId, QList<int>>::iterator it2 = featuresDict.begin();
        for (; it2 != featuresDict.end(); ++it2)
        {
            QgsFeatureId fid = it2.key();
            QList<int> &vertexIds = it2.value();

            Qgis::VectorEditResult res = Qgis::VectorEditResult::Success;
            std::sort(vertexIds.begin(), vertexIds.end(), std::greater<int>());
            for (int vertexId : vertexIds)
            {
                if (res != Qgis::VectorEditResult::EmptyGeometry)
                {
                    res = layer->deleteVertex( fid, vertexId );
                }
                if (res != Qgis::VectorEditResult::EmptyGeometry && res != Qgis::VectorEditResult::Success)
                {
                    success = false;
                }
            }

            if (res == Qgis::VectorEditResult::EmptyGeometry )
            {
                emit messageEmitted(tr("Геометрия была очищена. Ипользуйте инструмент, чтобы задать геометрию заново."));
            }
        }

        if (success)
        {
            layer->endEditCommand();
            layer->triggerRepaint();
        }
        else
        {
            layer->destroyEditCommand();
        }
    }

    // Убедимся, что временный оверлей объекта не виден
    removeTemporaryRubberBands();

    // Предварительно выбираем следующую вершину для удаления, если удаляем только одну вершину
    if (toDelete.count() == 1)
    {
        const Vertex &vertex = *toDelete.constBegin();
        QgsGeometry geom( cachedGeometryForVertex(vertex));
        int vertexId = vertex.vertexId;

        // Если следующая вершина недоступна, используем предыдущую
        if (geom.vertexAt(vertexId) == QgsPoint())
        {
            vertexId -= 1;
        }
        if (geom.vertexAt(vertexId) != QgsPoint())
        {
            QList<Vertex> vertices_new;
            vertices_new << Vertex(vertex.layer, vertex.fid, vertexId);
            setHighlightedVertices(vertices_new);
        }
    }

    if (mVertexEditor && mLockedFeature)
    {
        mVertexEditor->updateEditor(mLockedFeature.get());
    }
}

void QgsVertexTool::toggleVertexCurve()
{
    Vertex toConvert = Vertex(nullptr, -1, -1);
    if (mSelectedVertices.size() == 1)
    {
        toConvert = mSelectedVertices.first();
    }
    else if (mDraggingVertexType == AddingVertex || mDraggingVertexType == MovingVertex)
    {
        toConvert = *mDraggingVertex;
    }
    else
    {
        return;
    }

    if (mDraggingVertex)
    {
        if (mDraggingVertexType == AddingVertex || mDraggingVertexType == AddingEndpoint)
        {
            return;
        }
        stopDragging();
    }

    QgsVectorLayer *layer = toConvert.layer;

    if (!QgsWkbTypes::isCurvedType(layer->wkbType()))
    {
        return;
    }

    layer->beginEditCommand(tr("Переключение вершины с кривой на кривую"));

    QgsGeometry geom = layer->getFeature(toConvert.fid).geometry();

    bool success = geom.toggleCircularAtVertex(toConvert.vertexId);

    if (success)
    {
        layer->changeGeometry(toConvert.fid, geom);
        layer->endEditCommand();
        layer->triggerRepaint();
    }
    else
    {
        layer->destroyEditCommand();
    }

    if (mVertexEditor && mLockedFeature)
    {
        mVertexEditor->updateEditor(mLockedFeature.get());
    }
}

void QgsVertexTool::setHighlightedVertices(const QList<Vertex> &listVertices, HighlightMode mode)
{
    // Нужно сделать локальную копию вершин
    QList<Vertex> listVerticesLocal(listVertices);

    if (mode == ModeReset)
    {
        qDeleteAll(mSelectedVerticesMarkers);
        mSelectedVerticesMarkers.clear();
        mSelectedVertices.clear();
    }
    else if (mode == ModeSubtract)
    {
        // Нужно очистить маркеры вершин и перестроить позже.
        qDeleteAll(mSelectedVerticesMarkers);
        mSelectedVerticesMarkers.clear();
    }

    auto createMarkerForVertex = [=](const Vertex & vertex)->bool
    {
        QgsGeometry geom = cachedGeometryForVertex(vertex);
        QgsVertexId vid;
        if (!geom.vertexIdFromVertexNr(vertex.vertexId, vid))
        {
            return false;  // вершина может больше не существовать
        }

        QgsVertexMarker *marker = new QgsVertexMarker(canvas());
        marker->setIconType(QgsVertexMarker::ICON_CIRCLE);
        marker->setIconSize(QgsGuiUtils::scaleIconSize(10));
        marker->setPenWidth(QgsGuiUtils::scaleIconSize(2));
        marker->setColor(Qt::blue);
        marker->setCenter(toMapCoordinates(vertex.layer, geom.vertexAt(vertex.vertexId)));
        mSelectedVerticesMarkers.append(marker);
        return true;
    };

    for (const Vertex &vertex : listVerticesLocal)
    {
        if (mode == ModeAdd && mSelectedVertices.contains(vertex))
        {
            continue;
        }
        else if (mode == ModeSubtract)
        {
            mSelectedVertices.removeAll(vertex);
            continue;
        }

        if (!createMarkerForVertex(vertex))
        {
            continue;
        }
        mSelectedVertices.append(vertex);
    }

    if (mode == ModeSubtract)
    {
        // Перестраиваем маркеры для оставшегося выделения
        for (const Vertex &vertex : std::as_const(mSelectedVertices))
        {
            createMarkerForVertex(vertex);
        }
    }

    if (mLockedFeature)
    {
        disconnect(mLockedFeature.get(), &QgsLockedFeature::selectionChanged, this, &QgsVertexTool::lockedFeatureSelectionChanged);

        mLockedFeature->deselectAllVertices();
        for (const Vertex &vertex : std::as_const(mSelectedVertices))
        {
            Q_ASSERT(mLockedFeature->featureId() == vertex.fid && mLockedFeature->layer() == vertex.layer);
            mLockedFeature->selectVertex(vertex.vertexId);
        }

        connect(mLockedFeature.get(), &QgsLockedFeature::selectionChanged, this, &QgsVertexTool::lockedFeatureSelectionChanged);
    }
}

void QgsVertexTool::setHighlightedVerticesVisible(bool visible)
{
    for (QgsVertexMarker *marker : std::as_const(mSelectedVerticesMarkers))
    {
        marker->setVisible(visible);
    }
}

void QgsVertexTool::highlightAdjacentVertex(double offset)
{
    if (mSelectedVertices.isEmpty())
    {
        return;
    }

    Vertex vertex = mSelectedVertices[0];

    QgsGeometry geom = cachedGeometryForVertex(vertex);

    // Пытаемся обойти полигональные кольца
    int newVertexId, v0idx, v1idx;
    geom.adjacentVertices(vertex.vertexId, v0idx, v1idx);
    if (offset == -1 && v0idx != -1)
    {
        newVertexId = v0idx;
    }
    else if (offset == 1 && v1idx != -1)
    {
        newVertexId = v1idx;
    }
    else
    {
        newVertexId = vertex.vertexId + offset;
    }

    QgsPointXY pt = geom.vertexAt(newVertexId);
    if (pt != QgsPointXY())
    {
        vertex = Vertex(vertex.layer, vertex.fid, newVertexId);
    }
    setHighlightedVertices(QList<Vertex>() << vertex);
    zoomToVertex(vertex);
}

void QgsVertexTool::initSelectionRubberBand()
{
    if (!mSelectionRubberBand)
    {
        mSelectionRubberBand = std::make_unique<QgsRubberBand>(mCanvas, Qgis::GeometryType::Polygon);
        QColor fillColor = QColor(0, 120, 215, 63);
        QColor strokeColor = QColor(0, 102, 204, 100);
        mSelectionRubberBand->setFillColor(fillColor);
        mSelectionRubberBand->setStrokeColor(strokeColor);
    }
}

void QgsVertexTool::updateSelectionRubberBand(QgsMapMouseEvent *e)
{
    switch (mSelectionMethod)
    {
    case SelectionNormal:
    {
        QRect rect = QRect(e->pos(), *mSelectionRubberBandStartPos);
        if (mSelectionRubberBand)
        {
            mSelectionRubberBand->setToCanvasRectangle(rect);
        }
        break;
    }
    case SelectionPolygon:
    {
        mSelectionRubberBand->movePoint(toMapCoordinates(e->pos()));
        break;
    }
    default:
        break;
    }
}

void QgsVertexTool::stopSelectionRubberBand()
{
    mSelectionRubberBand.reset();
    mSelectionRubberBandStartPos.reset();
    mSelectionMethod = SelectionNormal;
}

bool QgsVertexTool::matchEdgeCenterTest(const QgsPointLocator::Match &m, const QgsPointXY &mapPoint, QgsPointXY *edgeCenterPtr)
{
    QgsPointXY p0, p1;
    m.edgePoints(p0, p1);

    QgsGeometry geom = cachedGeometry(m.layer(), m.featureId());
    if (isCircularVertex(geom, m.vertexIndex() ) || isCircularVertex(geom, m.vertexIndex() + 1))
    {
        return false;
    }

    QgsRectangle visible_extent = canvas()->mapSettings().visibleExtent();
    // Проверяем, находится ли одна точка внутри видимой области, а другая снаружи, чтобы переместить средний маркер
    // Если обе внутри или снаружи, такой необходимости нет
    if (visible_extent.contains(p0) != visible_extent.contains(p1))
    {
        // Обрезаем сегмент линии до области, чтобы средний маркер всегда был виден
        QgsGeometry extentGeom = QgsGeometry::fromRect(visible_extent);
        QgsGeometry lineGeom = QgsGeometry::fromPolylineXY(QgsPolylineXY() << p0 << p1);
        lineGeom = extentGeom.intersection(lineGeom);
        QgsPolylineXY polyline = lineGeom.asPolyline();
        Q_ASSERT_X(polyline.count() == 2, "QgsVertexTool::matchEdgeCenterTest", QgsLineString(polyline).asWkt().toUtf8().constData());
        p0 = polyline[0];
        p1 = polyline[1];
    }

    QgsPointXY edgeCenter((p0.x() + p1.x()) / 2, (p0.y() + p1.y()) / 2);
    if (edgeCenterPtr)
    {
        *edgeCenterPtr = edgeCenter;
    }

    double distFromEdgeCenter = std::sqrt(mapPoint.sqrDist(edgeCenter));
    double tol = QgsTolerance::vertexSearchRadius(canvas()->mapSettings());
    bool isNearCenter = distFromEdgeCenter < tol;
    return isNearCenter;
}

void QgsVertexTool::CircularBand::updateRubberBand(const QgsPointXY &mapPoint)
{
    QgsPointSequence points;
    QgsPointXY v0 = moving0 ? mapPoint + offset0 : p0;
    QgsPointXY v1 = moving1 ? mapPoint + offset1 : p1;
    QgsPointXY v2 = moving2 ? mapPoint + offset2 : p2;
    QgsGeometryUtils::segmentizeArc(QgsPoint(v0), QgsPoint(v1), QgsPoint(v2), points);
    band->reset();
    for (const QgsPoint &p : std::as_const(points))
    {
        band->addPoint(p);
    }
}

void QgsVertexTool::validationErrorFound(const QgsGeometry::Error &e)
{
    QgsGeometryValidator *validator = qobject_cast<QgsGeometryValidator *>(sender());
    if (!validator)
    {
        return;
    }

    QHash< QPair<QgsVectorLayer*, QgsFeatureId>, GeometryValidation>::iterator it = mValidations.begin();
    for (; it != mValidations.end(); ++it)
    {
        GeometryValidation &validation = *it;
        if (validation.validator == validator)
        {
            validation.addError(e);
            break;
        }
    }
}

void QgsVertexTool::validationFinished()
{
    QgsGeometryValidator *validator = qobject_cast<QgsGeometryValidator *>(sender());
    if (!validator)
    {
        return;
    }

    QHash< QPair<QgsVectorLayer*, QgsFeatureId>, GeometryValidation>::iterator it = mValidations.begin();
    for (; it != mValidations.end(); ++it)
    {
        GeometryValidation &validation = *it;
        if (validation.validator == validator)
        {
            if (validation.errorMarkers.isEmpty())
            {
                // Больше не нужно (нет маркеров для отображения)
                validation.cleanup();
                mValidations.remove(it.key());
            }
            break;
        }
    }
}

void QgsVertexTool::GeometryValidation::start(QgsGeometry &geom, QgsVertexTool *t, QgsVectorLayer *l)
{
    tool = t;
    layer = l;
    Qgis::GeometryValidationEngine method = Qgis::GeometryValidationEngine::QgisInternal;
    if (QgsSettingsRegistryCore::settingsDigitizingValidateGeometries->value() == 2)
    {
        method = Qgis::GeometryValidationEngine::Geos;
    }

    validator = new QgsGeometryValidator(geom, nullptr, method);
    connect(validator, &QgsGeometryValidator::errorFound, tool, &QgsVertexTool::validationErrorFound);
    connect(validator, &QThread::finished, tool, &QgsVertexTool::validationFinished);
    validator->start();
}

void QgsVertexTool::GeometryValidation::addError(QgsGeometry::Error e)
{
    if (!errors.isEmpty())
    {
        errors += '\n';
    }
    errors += e.what();

    if (e.hasWhere())
    {
        QgsVertexMarker *marker = new QgsVertexMarker(tool->canvas());
        marker->setCenter(tool->canvas()->mapSettings().layerToMapCoordinates(layer, e.where()));
        marker->setIconType(QgsVertexMarker::ICON_X);
        marker->setColor(Qt::green);
        marker->setZValue(marker->zValue() + 1);
        marker->setIconSize(QgsGuiUtils::scaleIconSize(10));
        marker->setPenWidth(QgsGuiUtils::scaleIconSize(2));
        marker->setToolTip(e.what());
        errorMarkers << marker;
    }
}

void QgsVertexTool::GeometryValidation::cleanup()
{
    if (validator)
    {
        validator->stop();
        validator->wait();
        validator->deleteLater();
        validator = nullptr;
    }

    qDeleteAll(errorMarkers);
    errorMarkers.clear();
}

void QgsVertexTool::validateGeometry(QgsVectorLayer *layer, QgsFeatureId featureId)
{
    if (QgsSettingsRegistryCore::settingsDigitizingValidateGeometries->value() == 0)
    {
        return;
    }

    QPair<QgsVectorLayer*, QgsFeatureId> id(layer, featureId);
    if (mValidations.contains(id))
    {
        mValidations[id].cleanup();
        mValidations.remove(id);
    }

    GeometryValidation validation;
    QgsGeometry geom = cachedGeometry(layer, featureId);
    validation.start(geom, this, layer);
    mValidations.insert(id, validation);
}

void QgsVertexTool::zoomToVertex(const Vertex &vertex)
{
    QgsPointXY newCenter = cachedGeometryForVertex(vertex).vertexAt(vertex.vertexId);
    QgsPointXY mapPoint = mCanvas->mapSettings().layerToMapCoordinates(vertex.layer, newCenter);
    QPolygonF ext = mCanvas->mapSettings().visiblePolygon();
    if (!ext.containsPoint(mapPoint.toQPointF(), Qt::OddEvenFill))
    {
        mCanvas->setCenter(mapPoint);
        mCanvas->refresh();
    }
}

QList<Vertex> QgsVertexTool::verticesInRange(QgsVectorLayer *layer, QgsFeatureId fid, int vertexId0, int vertexId1, bool longWay)
{
    QgsGeometry geom = cachedGeometry(layer, fid);

    if (vertexId0 > vertexId1)
    {
        std::swap(vertexId0, vertexId1);
    }

    QgsVertexId vid0, vid1;
    geom.vertexIdFromVertexNr(vertexId0, vid0);
    geom.vertexIdFromVertexNr(vertexId1, vid1);
    if (vid0.part != vid1.part || vid0.ring != vid1.ring)
    {
        return QList<Vertex>();
    }

    int vertexIdTmp = vertexId0 - 1;
    QgsVertexId vidTmp;
    while (geom.vertexIdFromVertexNr(vertexIdTmp, vidTmp) &&
            vidTmp.part == vid0.part && vidTmp.ring == vid0.ring)
    {
        --vertexIdTmp;
    }
    int startVertexIndex = vertexIdTmp + 1;

    vertexIdTmp = vertexId1 + 1;
    while (geom.vertexIdFromVertexNr(vertexIdTmp, vidTmp) &&
            vidTmp.part == vid0.part && vidTmp.ring == vid0.ring)
    {
        ++vertexIdTmp;
    }
    int endVertexIndex = vertexIdTmp - 1;

    QList<Vertex> lst;

    if (geom.vertexAt(startVertexIndex) == geom.vertexAt(endVertexIndex))
    {
        double lengthTotal = 0, length0to1 = 0;
        QgsPoint ptOld = geom.vertexAt(startVertexIndex);
        for (int i = startVertexIndex + 1; i <= endVertexIndex; ++i)
        {
            QgsPoint pt(geom.vertexAt(i));
            double len = ptOld.distance(pt);
            lengthTotal += len;
            if (i > vertexId0 && i <= vertexId1)
            {
                length0to1 += len;
            }
            ptOld = pt;
        }

        bool use0to1 = length0to1 < lengthTotal / 2;
        if (longWay)
        {
            use0to1 = !use0to1;
        }
        for (int i = startVertexIndex; i <= endVertexIndex; ++i)
        {
            bool isPickedVertex = i == vertexId0 || i == vertexId1;
            bool is0to1 = i > vertexId0 && i < vertexId1;
            if (isPickedVertex || is0to1 == use0to1)
            {
                lst.append(Vertex(layer, fid, i));
            }
        }
    }
    else
    {
        for (int i = vertexId0; i <= vertexId1; ++i)
        {
            lst.append(Vertex(layer, fid, i));
        }
    }
    return lst;
}

void QgsVertexTool::rangeMethodPressEvent(QgsMapMouseEvent *e)
{
    Q_UNUSED(e)
}

void QgsVertexTool::rangeMethodReleaseEvent(QgsMapMouseEvent *e)
{
    if (e->button() == Qt::RightButton)
    {
        stopRangeVertexSelection();
        return;
    }
    else if (e->button() == Qt::LeftButton)
    {
        if (mRangeSelectionFirstVertex)
        {
            QgsPointLocator::Match m = snapToEditableLayer(e);
            if (m.hasVertex())
            {
                if (m.layer() == mRangeSelectionFirstVertex->layer && m.featureId() == mRangeSelectionFirstVertex->fid)
                {
                    QList<Vertex> lst = verticesInRange(m.layer(), m.featureId(), mRangeSelectionFirstVertex->vertexId, m.vertexIndex(), e->modifiers() & Qt::ControlModifier);
                    setHighlightedVertices(lst);

                    mSelectionMethod = SelectionNormal;
                }
            }
        }
        else
        {
            QgsPointLocator::Match m = snapToEditableLayer(e);
            if (m.hasVertex())
            {
                mRangeSelectionFirstVertex.reset(new Vertex(m.layer(), m.featureId(), m.vertexIndex()));
                setHighlightedVertices(QList<Vertex>() << *mRangeSelectionFirstVertex);
            }
        }
    }
}

void QgsVertexTool::rangeMethodMoveEvent(QgsMapMouseEvent *e)
{
    if (e->buttons())
    {
        return;
    }

    QgsPointLocator::Match m = snapToEditableLayer(e);

    updateFeatureBand(m);
    updateVertexBand(m);

    if (!m.hasVertex())
    {
        QList<Vertex> lst;
        if (mRangeSelectionFirstVertex)
        {
            lst << *mRangeSelectionFirstVertex;
        }
        setHighlightedVertices(lst);
        return;
    }

    if (mRangeSelectionFirstVertex)
    {
        if (m.layer() == mRangeSelectionFirstVertex->layer && m.featureId() == mRangeSelectionFirstVertex->fid)
        {
            QList<Vertex> lst = verticesInRange(m.layer(), m.featureId(), mRangeSelectionFirstVertex->vertexId, m.vertexIndex(), e->modifiers() & Qt::ControlModifier);
            setHighlightedVertices(lst);
        }
    }
}

void QgsVertexTool::startRangeVertexSelection()
{
    mSelectionMethod = SelectionRange;
    setHighlightedVertices(QList<Vertex>());
    mRangeSelectionFirstVertex.reset();
}

void QgsVertexTool::stopRangeVertexSelection()
{
    mSelectionMethod = SelectionNormal;
    setHighlightedVertices(QList<Vertex>());
}

void QgsVertexTool::cleanEditor(QgsFeatureId id)
{
    if (mLockedFeature && mLockedFeature->featureId() == id)
    {
        cleanupVertexEditor();
    };
}

void QgsVertexTool::clean()
{
    if (mDraggingVertex || mDraggingEdge)
    {
        stopDragging();
    }

    if (mSelectionRubberBand)
    {
        stopSelectionRubberBand();
        mSelectionRubberBandStartPos.reset();
    }
}
