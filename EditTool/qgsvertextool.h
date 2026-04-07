#ifndef QGSVERTEXTOOL_H
#define QGSVERTEXTOOL_H

// Библиотеки Qt
#include <QMenu>
#include <QRubberBand>
#include <QTimer>
#include <QPointer>
#include <memory>

// Библиотеки QGIS API
#include "qgsadvanceddigitizingdockwidget.h"
#include "qgscurve.h"
#include "qgslinestring.h"
#include "qgscircularstring.h"
#include "qgscurvepolygon.h"
#include "qgsgeometryutils.h"
#include "qgsgeometryvalidator.h"
#include "qgsguiutils.h"
#include "qgslogger.h"
#include "qgsmapcanvas.h"
#include "qgsmulticurve.h"
#include "qgsmultipoint.h"
#include "qgspointlocator.h"
#include "qgsproject.h"
#include "qgsrubberband.h"
#include "qgssettingsregistrycore.h"
#include "qgssnapindicator.h"
#include "qgssnappingutils.h"
#include "qgsvectorlayer.h"
#include "qgsvertexmarker.h"
#include "qgslockedfeature.h"
#include "qgsvertexeditor.h"
#include "qgsmapmouseevent.h"
#include "qgsexpressioncontextutils.h"
#include "qgsmessagebar.h"
#include "qgsgeometryengine.h"
#include "qgisinterface.h"
#include "qgssettingsentry.h"
#include "qgssettingsentryimpl.h"
#include "qgsmaptooladvanceddigitizing.h"
#include "qgsgeometry.h"
#include "qgspointlocator.h"
#include "qobjectuniqueptr.h"

class QRubberBand;

class QgsGeometryValidator;
class QgsVertexEditor;
class QgsLockedFeature;
class QgsSnapIndicator;
class QgsVertexMarker;

/**
 * @struct Vertex
 * @brief Структура для хранения информации о вершине
 *
 * Данная структура представляет вершину объекта.
 * Хранит ссылку на слой, идентификатор объекта и номер вершины.
 */
struct Vertex
{
    Vertex(QgsVectorLayer *layer, QgsFeatureId fid, int vertexId)
        : layer(layer)
        , fid(fid)
        , vertexId(vertexId)
    {}

    bool operator==(const Vertex &other) const
    {
        return layer == other.layer && fid == other.fid && vertexId == other.vertexId;
    }
    bool operator!=(const Vertex &other) const
    {
        return !operator==(other);
    }

    QgsVectorLayer *layer = nullptr;
    QgsFeatureId fid;
    int vertexId;
};

uint qHash( const Vertex &v );

/**
 * @class QgsVertexTool
 * @brief Инструмент редактирования вершин
 *
 * Данный класс реализует инструмент для редактирования вершин объектов на карте.
 * Поддерживает перемещение, добавление и удаление вершин, а также редактирование граней объектов.
 */
class /*APP_EXPORT*/ QgsVertexTool : public QgsMapToolAdvancedDigitizing
{
    Q_OBJECT
public:

    /**
     * @brief Режим работы инструмента вершин
     *
     * Определяет, на каких слоях доступно редактирование вершин.
     */
    enum VertexToolMode
    {
        ActiveLayer, // Только активный слой
        AllLayers    // Все редактируемые слои
    };
    Q_ENUM(VertexToolMode)

    /**
     * @brief Конструктор
     *
     * Создает новый инструмент редактирования вершин.
     *
     * @param canvas указатель на карту
     * @param cadDock указатель на виджет оцифровки
     * @param mode режим работы инструмента
     */
    QgsVertexTool(QgsMapCanvas *canvas, QgsAdvancedDigitizingDockWidget *cadDock, VertexToolMode mode = QgsVertexTool::AllLayers);

    // Очистка созданных элементов на карте
    ~QgsVertexTool() override;

    void cadCanvasPressEvent(QgsMapMouseEvent *e) override;

    void cadCanvasReleaseEvent(QgsMapMouseEvent *e) override;

    void cadCanvasMoveEvent(QgsMapMouseEvent *e) override;

    // Функция добавления новой вершины по двойному щелчку
    void canvasDoubleClickEvent(QgsMapMouseEvent *e) override;

    void activate() override;

    void deactivate() override;

    // Очистка карты
    void clean() override;

    void keyPressEvent(QKeyEvent *e) override;

    QgsGeometry cachedGeometry(const QgsVectorLayer *layer, QgsFeatureId fid);

    /**
     * @brief Переключает видимость редактора вершин
     *
     * Открывает или закрывает панель редактора вершин.
     */
    void showVertexEditor();

    /**
     * @brief Обновляет редактор вершин для указанного объекта
     *
     * Привязывает редактор вершин к объекту из указанного слоя.
     *
     * @param layer указатель на векторный слой
     * @param fid идентификатор объекта
     */
    void updateVertexEditor(QgsVectorLayer *layer, QgsFeatureId fid);

private slots:
    /**
     * @brief Обновляет геометрию кэшированного объекта
     *
     * Вызывается при изменении геометрии объекта в слое.
     *
     * @param fid идентификатор объекта
     * @param geom новая геометрия объекта
     */
    void onCachedGeometryChanged(QgsFeatureId fid, const QgsGeometry &geom);

    /**
     * @brief Обрабатывает удаление кэшированной геометрии
     *
     * Вызывается при удалении объекта из слоя.
     *
     * @param fid идентификатор удаленного объекта
     */
    void onCachedGeometryDeleted(QgsFeatureId fid);

    // Функция очистки кэша геометрий
    void clearGeometryCache();

    // Функция удаления выбранных вершин через редактор
    void deleteVertexEditorSelection();

    /**
     * @brief Обрабатывает найденную ошибку валидации
     *
     * Вызывается при обнаружении ошибки в геометрии.
     *
     * @param e ошибка геометрии
     */
    void validationErrorFound(const QgsGeometry::Error &e);

    // Завершает процесс валидации геометрии
    void validationFinished();

    // Функция начинает выбор вершин по диапазону
    void startRangeVertexSelection();

    /**
     * @brief Очищает редактор для указанного объекта
     *
     * @param id идентификатор объекта
     */
    void cleanEditor(QgsFeatureId id);

    // Обрабатывает изменение выбора в заблокированном объекте
    void lockedFeatureSelectionChanged();

    /**
     * @brief Обрабатывает смену текущего слоя
     *
     * @param layer указатель на новый текущий слой
     */
    void currentLayerChanged(QgsMapLayer *layer);

private:

    /**
     * @brief Строит временные оверлей слой перетаскивания для вершин
     *
     * Создает визуальные индикаторы для перемещаемых вершин.
     *
     * @param movingVertices набор перемещаемых вершин
     * @param dragVertexMapPoint координаты точки перетаскивания
     */
    void buildDragBandsForVertices(const QSet<Vertex> &movingVertices, const QgsPointXY &dragVertexMapPoint);

    /**
     * @brief Добавляет оверлей слой перетаскивания
     *
     * @param v1 первая точка
     * @param v2 вторая точка
     */
    void addDragBand(const QgsPointXY &v1, const QgsPointXY &v2);

    /**
     * @brief Добавляет прямой оверлей слой перетаскивания
     *
     * @param layer указатель на векторный слой
     * @param v0 первая точка
     * @param v1 вторая точка
     * @param moving0 флаг перемещения первой точки
     * @param moving1 флаг перемещения второй точки
     * @param mapPoint координаты на карте
     */
    void addDragStraightBand(QgsVectorLayer *layer, QgsPointXY v0, QgsPointXY v1, bool moving0, bool moving1, const QgsPointXY &mapPoint);

    /**
     * @brief Добавляет кривую оверлей слой перетаскивания
     *
     * @param layer указатель на векторный слой
     * @param v0 первая точка
     * @param v1 вторая точка
     * @param v2 третья точка
     * @param moving0 флаг перемещения первой точки
     * @param moving1 флаг перемещения второй точки
     * @param moving2 флаг перемещения третьей точки
     * @param mapPoint координаты на карте
     */
    void addDragCircularBand(QgsVectorLayer *layer, QgsPointXY v0, QgsPointXY v1, QgsPointXY v2, bool moving0, bool moving1, bool moving2, const QgsPointXY &mapPoint);

    /**
     * @brief Перемещает временные оверлей слои
     *
     * @param mapPoint координаты на карте
     */
    void moveDragBands(const QgsPointXY &mapPoint);

    // Очищает временные оверлей слои
    void clearDragBands();

    /**
     * @brief Обрабатывает перемещение мыши при перетаскивании вершины
     *
     * @param e событие мыши
     */
    void mouseMoveDraggingVertex(QgsMapMouseEvent *e);

    /**
     * @brief Обрабатывает перемещение мыши при перетаскивании грани
     *
     * @param e событие мыши
     */
    void mouseMoveDraggingEdge(QgsMapMouseEvent *e);

    // Удаляет временные оверлей слои
    void removeTemporaryRubberBands();

    // Очищает редактор вершин
    void cleanupVertexEditor();

    // Очищает заблокированный объект
    void cleanupLockedFeature();

    /**
     * @brief Выполняет поиск совпадения с редактируемым слоем
     *
     * Временно переопределяет конфигурацию "прилипания" (snapping) для поиска совпадений с вершинами
     * и гранями любого редактируемого векторного слоя.
     *
     * @param e событие мыши
     * @return результат совпадения
     */
    QgsPointLocator::Match snapToEditableLayer(QgsMapMouseEvent *e);

    /**
     * @brief Ищет совпадение внутри полигонов
     *
     * @param e событие мыши
     * @return результат совпадения
     */
    QgsPointLocator::Match snapToPolygonInterior(QgsMapMouseEvent *e);

    /**
     * @brief Возвращает список всех совпадений в указанной точке
     *
     * Объединяет совпадения с вершинами, гранями и областями.
     *
     * @param mapPoint точка на карте
     * @param layer векторный слой
     * @return список совпадений
     */
    QList<QgsPointLocator::Match> findEditableLayerMatches(const QgsPointXY &mapPoint, QgsVectorLayer *layer);

    /**
     * @brief Находит все редактируемые объекты в точке
     *
     * @param mapPoint точка на карте
     * @return набор пар (слой, идентификатор объекта)
     */
    QSet<QPair<QgsVectorLayer *, QgsFeatureId> > findAllEditableFeatures(const QgsPointXY &mapPoint);

    /**
     * @brief Пытаемся выбрать объект для редактирования
     *
     * Функция реализует поведение для щелчка правкой кнопкой мыши.
     * Повторные щелчки переключают между объектами.
     *
     * @param e событие мыши
     */
    void tryToSelectFeature(QgsMapMouseEvent *e);

    // Проверяет, находится ли точка рядом с маркером конечной точки
    bool isNearEndpointMarker(const QgsPointXY &mapPoint);

    // Проверяет, является ли совпадение конечной точкой
    bool isMatchAtEndpoint(const QgsPointLocator::Match &match);

    // Возвращает позицию маркера конечной точки
    QgsPointXY positionForEndpointMarker(const QgsPointLocator::Match &match);

    /**
     * @brief Обрабатывает перемещение мыши без перетаскивания
     *
     * @param e событие мыши
     */
    void mouseMoveNotDragging(QgsMapMouseEvent *e);

    QgsGeometry cachedGeometryForVertex(const Vertex &vertex);

    /**
     * @brief Начинает перетаскивание
     *
     * @param e событие мыши
     */
    void startDragging(QgsMapMouseEvent *e);

    /**
     * @brief Начинает перетаскивание вершины
     *
     * @param m результат поиска совпадения
     */
    void startDraggingMoveVertex(const QgsPointLocator::Match &m);

    /**
     * @brief Возвращает список совпадений всех вершин слоя с точкой
     *
     * @param layer векторный слой
     * @param mapPoint точка на карте
     * @return список совпадений
     */
    QList<QgsPointLocator::Match> layerVerticesSnappedToPoint(QgsVectorLayer *layer, const QgsPointXY &mapPoint);

    /**
     * @brief Возвращает список совпадений всех сегментов слоя с заданным сегментом
     *
     * @param layer векторный слой
     * @param mapPoint1 первая точка сегмента
     * @param mapPoint2 вторая точка сегмента
     * @return список совпадений
     */
    QList<QgsPointLocator::Match> layerSegmentsSnappedToSegment(QgsVectorLayer *layer, const QgsPointXY &mapPoint1, const QgsPointXY &mapPoint2);

    /**
     * @brief Функция начала перетаскивания при добавлении вершины
     *
     * @param m результат совпадения
     */
    void startDraggingAddVertex(const QgsPointLocator::Match &m);

    /**
     * @brief Функция начала перетаскивания при добавлении вершины в конечную точку
     *
     * @param mapPoint точка на карте
     */
    void startDraggingAddVertexAtEndpoint(const QgsPointXY &mapPoint);

    /**
     * @brief Функция начала перетаскивания при добавлении грани
     *
     * @param m результат совпадения
     * @param mapPoint точка на карте
     */
    void startDraggingEdge(const QgsPointLocator::Match &m, const QgsPointXY &mapPoint);

    // Останавливает перетаскивание
    void stopDragging();

    /**
     * @brief Преобразует совпадение в точку слоя
     *
     * @param destLayer целевой слой
     * @param mapPoint точка на карте
     * @param match результат совпадения
     * @return точка в координатах слоя
     */
    QgsPoint matchToLayerPoint(const QgsVectorLayer *destLayer, const QgsPointXY &mapPoint, const QgsPointLocator::Match *match);

    /**
     * @brief Функция перемещения грани
     *
     * @param mapPoint точка на карте
     */
    void moveEdge(const QgsPointXY &mapPoint);

    /**
     * @brief Функция перемещения вершины
     *
     * @param mapPoint точка на карте
     * @param mapPointMatch результат совпадения точки
     */
    void moveVertex(const QgsPointXY &mapPoint, const QgsPointLocator::Match *mapPointMatch);

    // Функция удаления вершины
    void deleteVertex();

    // Функция переключения типа вершины на кривой
    void toggleVertexCurve();

    typedef QHash<QgsVectorLayer *, QHash<QgsFeatureId, QgsGeometry>> VertexEdits;

    /**
     * @brief Функция добавления дополнительных вершин
     *
     * @param edits правки
     * @param mapPoint точка на карте
     * @param dragLayer слой перетаскивания
     * @param layerPoint точка в координатах слоя
     */
    void addExtraVerticesToEdits(VertexEdits &edits, const QgsPointXY &mapPoint, QgsVectorLayer *dragLayer = nullptr, const QgsPoint &layerPoint = QgsPoint());

    /**
     * @brief Функция добавления дополнительных сегментов
     *
     * @param edits правки
     * @param mapPoint точка на карте
     * @param dragLayer слой перетаскивания
     * @param layerPoint точка в координатах слоя
     */
    void addExtraSegmentsToEdits(QgsVertexTool::VertexEdits &edits, const QgsPointXY &mapPoint, QgsVectorLayer *dragLayer, const QgsPoint &layerPoint);

    /**
     * @brief Функция применения изменений к слоям
     *
     * @param edits изменения для применения
     */
    void applyEditsToLayers(VertexEdits &edits);

    /**
     * @brief Функция нахождения совпадающих вершин
     *
     * Ищет вершины, совпадающие по координатам с данным набором.
     * Используется для топологического редактирования.
     *
     * @param vertices набор вершин для поиска
     * @return набор найденных совпадающих вершин
     */
    QSet<Vertex> findCoincidentVertices(const QSet<Vertex> &vertices);

    /**
     * @brief Строит дополнительные вершины
     *
     * Подготавливает массивы дополнительных вершин и их смещений.
     *
     * @param vertices набор вершин
     * @param anchorPoint опорная точка
     * @param anchorLayer опорный слой
     */
    void buildExtraVertices(const QSet<Vertex> &vertices, const QgsPointXY &anchorPoint, QgsVectorLayer *anchorLayer);

    // Возвращает список редактируемых векторных слоев
    QList<QgsVectorLayer *> editableVectorLayers();

    /**
     * @brief Режим подсветки вершин
     *
     * Определяет способ обработки выделения вершин.
     */
    enum HighlightMode
    {
        ModeReset,      // Сбросить текущее выделение
        ModeAdd,        // Добавить к текущему выделению
        ModeSubtract    // Удалить из текущего выделения
    };

    /**
     * @brief Устанавливает подсветку вершин
     *
     * @param listVertices список вершин для подсветки
     * @param mode режим подсветки
     */
    void setHighlightedVertices(const QList<Vertex> &listVertices, HighlightMode mode = ModeReset);

    /**
     * @brief Устанавливает видимость подсветки вершин
     *
     * @param visible флаг видимости
     */
    void setHighlightedVerticesVisible(bool visible);

    // Подсвечивает соседнюю вершину
    void highlightAdjacentVertex(double offset);

    // Инициализирует временный оверлей слой для выбора прямоугольником
    void initSelectionRubberBand();

    // Обновляет временный оверлей слой для выбора
    void updateSelectionRubberBand(QgsMapMouseEvent *e);

    // Останавливает выбор временного оверлей слоя
    void stopSelectionRubberBand();

    /**
     * @brief Функция проверки близости к центру грани
     *
     * Определяет, находится ли точка рядом с центром грани.
     *
     * @param m результат совпадения
     * @param mapPoint точка на карте
     * @param edgeCenterPtr указатель для возврата центра грани
     * @return true если точка близка к центру
     */
    bool matchEdgeCenterTest(const QgsPointLocator::Match &m, const QgsPointXY &mapPoint, QgsPointXY *edgeCenterPtr = nullptr);

    /**
     * @brief Запускает валидацию геометрии
     *
     * @param layer векторный слой
     * @param featureId идентификатор объекта
     */
    void validateGeometry(QgsVectorLayer *layer, QgsFeatureId featureId);

    // Масштабирует к вершине
    void zoomToVertex(const Vertex &vertex);

    // Возвращает список вершин в диапазоне
    QList<Vertex> verticesInRange(QgsVectorLayer *layer, QgsFeatureId fid, int vertexId0, int vertexId1, bool longWay);

    // Обновляет временный оверлей слой объекта
    void updateFeatureBand(const QgsPointLocator::Match &m);

    // Обновляет временный оверлей слой вершины
    void updateVertexBand(const QgsPointLocator::Match &m);

    // Обрабатывает нажатие мыши в режиме выбора диапазона
    void rangeMethodPressEvent(QgsMapMouseEvent *e);
    
    // Обрабатывает отпускание мыши в режиме выбора диапазона
    void rangeMethodReleaseEvent(QgsMapMouseEvent *e);
    
    // Обрабатывает перемещение мыши в режиме выбора диапазона
    void rangeMethodMoveEvent(QgsMapMouseEvent *e);

    // Останавливает выбор вершин по диапазону
    void stopRangeVertexSelection();

    // Обновляет подсветку вершин заблокированного объекта
    void updateLockedFeatureVertices();

private:
    std::unique_ptr<QgsSnapIndicator> mSnapIndicator;   // Индикатор совпадения при перетаскивании вершины
    QgsVertexMarker *mEdgeCenterMarker = nullptr;       // Временный оверлей слой для подсветки всего объекта при наведении
    QgsRubberBand *mFeatureBand = nullptr;              // Временный оверлей слой для подсветки всех вершин объекта при наведении, также используется для вершин заблокированного объекта
    QgsRubberBand *mFeatureBandMarkers = nullptr;       // Временный оверлей слой для подсветки всех вершин объекта при наведении курсора мыши без перетаскивания, а также для подсветки заблокированных вершин объекта.
    const QgsVectorLayer *mFeatureBandLayer = nullptr;  // Исходный слой для mFeatureBand
    QgsFeatureId mFeatureBandFid = 0;                   // Исходный идентификатор объекта для mFeatureBand
    QgsRubberBand *mVertexBand = nullptr;               // Подсветка вершины при наведении без перетаскивания
    QgsRubberBand *mEdgeBand = nullptr;                 // Подсветка грани при наведении без перетаскивания
    QList<QgsVertexMarker *> mLockedFeatureVerticesMarkers; // Подсветка вершин заблокированного объекта

    /**
     * @brief Тип перетаскивания вершины
     *
     * Определяет текущую операцию перетаскивания.
     */
    enum DraggingVertexType
    {
        NotDragging,    // Нет перетаскивания
        MovingVertex,   // Перемещение вершины
        AddingVertex,   // Добавление вершины
        AddingEndpoint, // Добавление конечной точки
    };

    QList<QgsVertexMarker *> mDragPointMarkers; // Маркеры точек для перемещения точечной геометрии
    QList<QgsVector> mDragPointMarkersOffset; // Сопутствующий массив к mDragPointMarkers: хранит смещение (в единицах карты) от позиции основной вершины

    /**
     * @brief Структура для хранения информации об оверлей слое перетаскивания
     *
     * Хранит данные о временном оверлей слое для перетаскивания прямолинейного сегмента.
     */
    struct StraightBand
    {
        QgsRubberBand *band = nullptr;  // Указатель на временный оверлей слой
        QgsPointXY p0, p1;              // Исходные позиции точек (в единицах карты)
        bool moving0, moving1;          // Флаги, показывающие какие точки перемещаются с курсором
        QgsVector offset0, offset1;     // Смещение точки от курсора
    };

    /**
     * @brief Структура для хранения информации о кривом оверлей слоев перетаскивания
     *
     * Хранит данные о временном оверлей слоев для перетаскивания кругового сегмента.
     */
    struct CircularBand
    {
        QgsRubberBand *band = nullptr;  // Указатель на временный оверлей слой
        QgsPointXY p0, p1, p2;          // Исходные позиции точек (в единицах карты)
        bool moving0, moving1, moving2; // Флаги, показывающие какие точки перемещаются с курсором
        QgsVector offset0, offset1, offset2; // Смещение точки от курсора

        // Обновляет геометрию временного оверлей слоя на текущей позиции курсора (в единицах карты)
        void updateRubberBand(const QgsPointXY &mapPoint);
    };

    QList<StraightBand> mDragStraightBands;     // Список активных прямых временных оверлей слоев
    QList<CircularBand> mDragCircularBands;     // Список активных кривых временных оверлей слоев
    std::unique_ptr<Vertex> mDraggingVertex;    // Перемещаемая вершина или nullptr
    DraggingVertexType mDraggingVertexType = NotDragging; // Тип перетаскивания (перемещение или добавление)
    bool mDraggingEdge = false;                 // Флаг перетаскивания грани

    QList<Vertex> mDraggingExtraVertices; // Список дополнительных вершин, перетаскиваемых вместе с основной (mDraggingVertex). Либо топологически связанные точки, либо выделенные вершины.
    QList<QgsVector> mDraggingExtraVerticesOffset; // Сопутствующий массив к mDraggingExtraVertices: хранит смещение (в единицах слоя) каждой вершины от позиции основной вершины (mDraggingVertex)

    QList<Vertex> mDraggingExtraSegments; // Список вершин, идентифицирующих сегменты. Используется для топологического редактирования при добавлении вершины к существующему сегменту
    QList<Vertex> mSelectedVertices; // Список выделенных вершин
    QList<QgsVertexMarker*> mSelectedVerticesMarkers; // Список маркеров вершин

    std::unique_ptr<QgsRubberBand> mSelectionRubberBand; // Временный оверлей слой для визуализации выбора прямоугольником/полигоном
    std::unique_ptr<QPoint> mSelectionRubberBandStartPos; // Начальная точка при выборе прямоугольником/полигоном

    std::unique_ptr<Vertex> mMouseAtEndpoint; // Вершина в конечной точке или nullptr
    std::unique_ptr<QgsPointXY> mEndpointMarkerCenter; // Центр маркера конечной точки или nullptr

    QgsVertexMarker *mEndpointMarker = nullptr; //  Маркер, отображаемый рядом с концом кривой, указывающий, что пользователь может добавить вершину в конце
    std::unique_ptr<QgsPointLocator::Match> mLastSnap; // Хранит информацию о предыдущем использованном совпадении, чтобы при следующем совпадении оставаться на том же объекте
    std::unique_ptr<QgsPointLocator::Match> mNewVertexFromDoubleClick; // При двойном щелчке для добавления вершины, хранит совпадение из события "press" для использования в "release"

    QHash<const QgsVectorLayer*, QHash<QgsFeatureId, QgsGeometry>> mCache; // Кэш геометрий для быстрого доступа (координаты в CRS слоя)
    QObjectUniquePtr<QgsLockedFeature> mLockedFeature; // Заблокированный объект для редактора вершин
    QPointer<QgsVertexEditor> mVertexEditor; // Виджет редактора вершин

    /**
     * @brief Структура для хранения альтернативных объектов
     *
     * Хранит альтернативные объекты для текущего выбранного (заблокированного).
     * Используется при многократном правом щелчке в одной точке
     * для переключения между объектами.
     */
    struct LockedFeatureAlternatives
    {
        QPoint screenPoint; // Экранные координаты точки
        QList< QPair<QgsVectorLayer*, QgsFeatureId>> alternatives; // Список альтернатив
        int index = -1;     // Текущий индекс
    };

    std::unique_ptr<LockedFeatureAlternatives> mLockedFeatureAlternatives;  // Хранит информацию о возможных объектах для выбора правым щелчком.

    /**
     * @brief Структура для валидации геометрии
     *
     * Хранит данные для проверки корректности геометрии объекта.
     */
    struct GeometryValidation
    {
        QgsVertexTool *tool = nullptr;              // Указатель на родительский инструмент (для связей/карты)
        QgsVectorLayer *layer = nullptr;            // Указатель на слой проверяемой геометрии
        QgsGeometryValidator *validator = nullptr;  // Объект валидатора. Non-null если активен
        QList<QgsVertexMarker*> errorMarkers;      // Маркеры ошибок от валидации
        QString errors;                             // Полный текст ошибок валидации

        void start(QgsGeometry &geom, QgsVertexTool *tool, QgsVectorLayer *l);  // Запустить валидацию
        void addError(QgsGeometry::Error e);  // Добавить ошибку к валидации
        void cleanup(); // Очистить все
    };

    // Хранилище данных валидации
    QHash< QPair<QgsVectorLayer *, QgsFeatureId>, GeometryValidation> mValidations;

    /**
     * @brief Перечисление методов выбора вершин
     *
     * Определяет режим выбора вершин.
     */
    enum VertexSelectionMethod
    {
        SelectionNormal,   // Обычный выбор: щелчок перемещает вершину, ctrl+щелчок выбирает, перетаскивание прямоугольником выбирает несколько
        SelectionRange,    // Выбор диапазона: первый щелчок выбирает начало, второй конец, вершины в диапазоне выделяются
        SelectionPolygon,  // Выбор полигоном: alt+щелчок начинает оцифровку, правый щелчок выбирает вершины внутри
    };

    VertexSelectionMethod mSelectionMethod = SelectionNormal; // Текущий метод выбора вершин
    std::unique_ptr<Vertex> mRangeSelectionFirstVertex; // Начальная вершина при выборе диапазоном
    VertexToolMode mMode = AllLayers;
};

#endif // QGSVERTEXTOOL_H
