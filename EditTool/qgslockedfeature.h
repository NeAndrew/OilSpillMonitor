#ifndef QGSLOCKEDFEATURE_H
#define QGSLOCKEDFEATURE_H

#include <QObject>

#include "qgsgeometry.h"
#include "qgsfeatureid.h"
#include "qgsvertexeditor.h"
#include "qgsfeatureiterator.h"
#include "qgspoint.h"
#include "qgssettingsregistrycore.h"
#include "qgslogger.h"
#include "qgsvertexmarker.h"
#include "qgsgeometryvalidator.h"
#include "qgsguiutils.h"
#include "qgsvectorlayer.h"
#include "qgsrubberband.h"
#include "qgslayertreeview.h"
#include "qgsproject.h"
#include "qgsstatusbar.h"
#include "qgsmapcanvas.h"
#include "qgisinterface.h"
#include "qgssettingsentry.h"
#include "qgssettingsentryimpl.h"

class QgsMapCanvas;
class QgsVectorLayer;
class QgsMapLayer;
class QgsGeometryValidator;
class QgsVertexMarker;
class QgsVertexEntry;

/**
 * @class QgsLockedFeature
 * @brief Класс для хранения выбранного объекта инструмента редактирования вершин
 *
 * Данный класс управляет выбранным векторным объектом и его вершинами.
 * Предоставляет функционал для выбора, отмены выбора и манипуляции вершинами объекта на карте.
 */
class QgsLockedFeature: public QObject
{
    Q_OBJECT

public:

    /**
     * @brief Конструктор
     *
     * Создает новый экземпляр класса для работы с выбранным объектом.
     *
     * @param id идентификатор выбранного объекта
     * @param layer указатель на векторный слой, в котором находится объект
     * @param canvas указатель на карту
     */
    QgsLockedFeature(QgsFeatureId id, QgsVectorLayer *layer, QgsMapCanvas *canvas);

    /**
     * @brief Деструктор класса
     *
     * Освобождает ресурсы
     */
    ~QgsLockedFeature() override;

    /**
     * @brief Выбирает вершину с указанным номером
     *
     * Устанавливает вершину как выбранную для дальнейших операций.
     *
     * @param vertexNr номер вершины
     */
    void selectVertex(int vertexNr);

    /**
     * @brief Снимает выбор с вершины с указанным номером
     *
     * Убирает вершину из списка выбранных.
     *
     * @param vertexNr номер вершины
     */
    void deselectVertex(int vertexNr);

    /**
     * @brief Снимает выбор со всех вершин выбранного объекта
     *
     * Очищает список всех выбранных вершин.
     */
    void deselectAllVertices();

    /**
     * @brief Инвертирует выбор набора вершин
     *
     * Для каждой вершины из списка инвертируется её состояние выбора.
     *
     * @param vertexNr номер вершины
     */
    void invertVertexSelection(int vertexNr);

    /**
     * @brief Инвертирует выбор набора вершин
     *
     * Для каждой вершины из списка инвертируется её состояние выбора.
     *
     * @param vertexIndices список индексов вершин
     */
    void invertVertexSelection(const QVector<int> &vertexIndices);

    /**
     * @brief Проверяет, выбрана ли вершина
     *
     * @param vertexNr номер вершины
     * @return true если вершина выбрана, false в противном случае
     */
    bool isSelected(int vertexNr);

    /**
     * @brief Получает идентификатор выбранного объекта
     *
     * @return идентификатор выбранного объекта
     */
    QgsFeatureId featureId();

    /**
     * @brief Возвращает контейнер с вершинами объекта
     *
     * @return ссылка на текущий контейнер вершин
     */
    QList<QgsVertexEntry*> &vertexMap();

    /**
     * @brief Функция пересоздает контейнер вершин
     *
     * Пересоздает контейнер вершин на основе текущего состояния геометрии объекта.
     */
    void replaceVertexMap();

    /**
     * @brief Возвращает векторный слой выбранного объекта
     *
     * @return указатель на используемый векторный слой
     */
    QgsVectorLayer *layer();

    /**
     * @brief Возвращает геометрию текущего объекта
     *
     * @return указатель на геометрию текущего объекта
     */
    QgsGeometry *geometry();

    /**
     * @brief Начинает операцию изменения геометрии
     *
     * Устанавливает флаг начала изменения геометрии для отслеживания состояния редактирования.
     */
    void beginGeometryChange();
    
    /**
     * @brief Завершает операцию изменения геометрии
     *
     * Сбрасывает флаг изменения геометрии и сохраняет изменения.
     */
    void endGeometryChange();

signals:
    /**
     * @brief Сигнал об изменении выбора вершин
     *
     * Испускается при изменении состояния выбора любой вершины.
     */
    void selectionChanged();
    
    /**
     * @brief Сигнал об изменении структуры контейнера вершин
     *
     * Испускается при изменении структуры контейнера вершин.
     */
    void vertexMapChanged();

public slots:
    /**
     * @brief Слот для обработки ошибок валидации геометрии
     *
     * Вызывается, когда при проверке геометрии происходит ошибка.
     *
     * @param error ошибка, найденная при валидации
     */
    void addError(QgsGeometry::Error);

    /**
     * @brief Слот для завершения валидации геометрии
     *
     * Вызывается, когда процесс валидации геометрии завершен.
     */
    void validationFinished();

    /**
     * @brief Слот для обработки удаления объекта из слоя
     *
     * Вызывается, когда объект удаляется из слоя.
     *
     * @param fid идентификатор удаленного объекта
     */
    void featureDeleted(QgsFeatureId);

    /**
     * @brief Слот для обработки изменения геометрии объекта
     *
     * Вызывается, когда геометрия объекта в слое изменена
     *
     * @param fid идентификатор объекта с измененной геометрией
     * @param geometry новая геометрия объекта
     */
    void geometryChanged(QgsFeatureId, const QgsGeometry &);

    /**
     * @brief Слот для обработки отката изменений
     *
     * Вызывается перед откатом изменений. Прекращает мониторинг геометрии.
     */
    void beforeRollBack();

private:
    /**
     * @brief Удаляет весь контейнер вершин
     *
     * Освобождает память
     */
    void deleteVertexMap();

    /**
     * @brief Создает контейнер вершин
     *
     * Инициализирует и заполняет контейнер вершин на основе текущей геометрии.
     */
    void createVertexMap();

    /**
     * @brief Функция обновляет геометрию
     *
     * Обновляет геометрию до актуального состояния, загруженного из слоя или уже доступной геометрии.
     *
     * @param geom указатель на новую геометрию (если значение nullptr, то используется текущая геометрия)
     */
    void updateGeometry(const QgsGeometry *geom);

    /**
     * @brief Проверка геометрии объекта
     *
     * Запускает процесс валидации геометрии для проверки её корректности.
     *
     * @param g указатель на геометрию для валидации (если значение nullptr, то используется текущая геометрия)
     */
    void validateGeometry(QgsGeometry *g = nullptr);

    QgsFeatureId mFeatureId;            // ID объекта
    QgsGeometry *mGeometry = nullptr;   // Указатель на геометрию объекта
    QgsVectorLayer *mLayer = nullptr;   // Указатель на векторный слой
    QgsMapCanvas *mCanvas = nullptr;    // Указатель на карту
    
    QList<QgsVertexEntry*> mVertexMap;  // Контейнер вершин объекта
    bool mChangingGeometry = false;     // Флаг, указывающий на изменение геометрии
    
    QgsGeometryValidator *mValidator = nullptr; // Валидатор геометрии
    QString mTip;
    QList< QgsGeometry::Error> mGeomErrors;     // Список ошибок геометрии объекта
    QList< QgsVertexMarker*> mGeomErrorMarkers;
};

#endif
