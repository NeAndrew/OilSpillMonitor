#include "qgslockedfeature.h"

QgsLockedFeature::QgsLockedFeature(QgsFeatureId featureId,
                                   QgsVectorLayer *layer,
                                   QgsMapCanvas *canvas)
    : mFeatureId(featureId)
    , mLayer(layer)
    , mCanvas(canvas)
{
    replaceVertexMap(); // Пересоздание контейнера вершин
}

QgsLockedFeature::~QgsLockedFeature()
{
    deleteVertexMap(); // Удаление контейнера вершин

    while (!mGeomErrorMarkers.isEmpty())
    {
        delete mGeomErrorMarkers.takeFirst(); // Удаление маркеров ошибок геометрии
    }

    if (mValidator)
    {
        mValidator->stop();
        mValidator->wait();
        mValidator->deleteLater();
        mValidator = nullptr;
    }

    delete mGeometry;
}

void QgsLockedFeature::updateGeometry(const QgsGeometry *geom)
{
    delete mGeometry; // Удаление старой геометрии

    if (!geom)
    {
        QgsFeature f;
        mLayer->getFeatures(QgsFeatureRequest().setFilterFid(mFeatureId)).nextFeature(f); // Получение объекта по ID
        if (f.hasGeometry())
        {
            mGeometry = new QgsGeometry(f.geometry()); // Создание копии геометрии
        }
        else
        {
            mGeometry = new QgsGeometry();
        }
    }
    else
    {
        mGeometry = new QgsGeometry(*geom); // Копирование геометрии
    }
}

void QgsLockedFeature::beforeRollBack()
{
    disconnect(mLayer, &QgsVectorLayer::geometryChanged, this, &QgsLockedFeature::geometryChanged); // Отключение сигнала
    deleteVertexMap(); // Удаление контейнера вершин перед откатом
}

void QgsLockedFeature::beginGeometryChange()
{
    Q_ASSERT(!mChangingGeometry);
    mChangingGeometry = true; // Флаг начала изменения геометрии

    disconnect(mLayer, &QgsVectorLayer::geometryChanged, this, &QgsLockedFeature::geometryChanged); // Отключение сигнала
}

void QgsLockedFeature::endGeometryChange()
{
    Q_ASSERT(mChangingGeometry);
    mChangingGeometry = false; // Флаг окончания изменения геометрии

    connect(mLayer, &QgsVectorLayer::geometryChanged, this, &QgsLockedFeature::geometryChanged); // Подключение сигнала
}

void QgsLockedFeature::featureDeleted(QgsFeatureId fid)
{
    if (fid == mFeatureId)
    {
        deleteLater(); // Удаление объекта если удален связанный признак
    }
}

void QgsLockedFeature::geometryChanged(QgsFeatureId fid, const QgsGeometry &geom)
{
    if (!mLayer || fid != mFeatureId)
    {
        return; // Выход если не тот слой или ID
    }

    updateGeometry(&geom); // Обновление геометрии

    replaceVertexMap(); // Пересоздание контейнера вершин
}

void QgsLockedFeature::validateGeometry(QgsGeometry *g)
{
    if (QgsSettingsRegistryCore::settingsDigitizingValidateGeometries->value() == 0)
    {
        return; // Выходим если валидация отключена
    }

    if (!g)
    {
        g = mGeometry; // Использование текущей геометрии если не была передана
    }

    mTip.clear();

    if (mValidator)
    {
        mValidator->stop(); // Остановка текущего валидатора
        mValidator->wait();
        mValidator->deleteLater();
        mValidator = nullptr;
    }

    mGeomErrors.clear();

    while (!mGeomErrorMarkers.isEmpty())
    {
        QgsVertexMarker *vm = mGeomErrorMarkers.takeFirst();
        delete vm;
    }

    Qgis::GeometryValidationEngine method = Qgis::GeometryValidationEngine::QgisInternal;

    if (QgsSettingsRegistryCore::settingsDigitizingValidateGeometries->value() == 2)
    {
        method = Qgis::GeometryValidationEngine::Geos; // Выбор метода валидации
    }

    mValidator = new QgsGeometryValidator(*g, nullptr, method);
    connect(mValidator, &QgsGeometryValidator::errorFound, this, &QgsLockedFeature::addError);
    connect(mValidator, &QThread::finished, this, &QgsLockedFeature::validationFinished);
    mValidator->start(); // Запуск валидации
}

void QgsLockedFeature::addError(QgsGeometry::Error e)
{
    mGeomErrors << e; // Добавление ошибки в список
    if (!mTip.isEmpty())
    {
        mTip += '\n';
    }
    mTip += e.what();

    if (e.hasWhere())
    {
        QgsVertexMarker *marker = new QgsVertexMarker(mCanvas); // Создание маркера ошибки
        marker->setCenter(mCanvas->mapSettings().layerToMapCoordinates(mLayer, e.where()));
        marker->setIconType(QgsVertexMarker::ICON_X);
        marker->setColor(Qt::green);
        marker->setZValue(marker->zValue() + 1);
        marker->setIconSize(QgsGuiUtils::scaleIconSize(10));
        marker->setPenWidth(QgsGuiUtils::scaleIconSize(2));
        marker->setToolTip(e.what()); // Установка подсказки с описанием ошибки
        mGeomErrorMarkers << marker;
    }
}

void QgsLockedFeature::validationFinished() {}

void QgsLockedFeature::replaceVertexMap()
{
    deleteVertexMap();

    createVertexMap();

    validateGeometry(); // Валидация новой геометрии

    emit vertexMapChanged(); // Сигнал об изменении карты вершин
}

void QgsLockedFeature::deleteVertexMap()
{
    const auto constMVertexMap = mVertexMap;
    for (QgsVertexEntry *entry : constMVertexMap)
    {
        delete entry;
    }

    mVertexMap.clear(); // Очистка списка
}

bool QgsLockedFeature::isSelected(int vertexNr)
{
    return mVertexMap.at(vertexNr)->isSelected(); // Проверка выбора вершины
}

QgsGeometry *QgsLockedFeature::geometry()
{
    Q_ASSERT(mGeometry);
    return mGeometry; // Возвращаем указатель на геометрию
}

void QgsLockedFeature::createVertexMap()
{

    if (!mGeometry)
    {
        updateGeometry(nullptr); // Обновление геометрии если отсутствует
    }

    if (!mGeometry)
    {
        return; // Выходим если геометрия не создана
    }

    const QgsAbstractGeometry *geom = mGeometry->constGet();
    if (!geom)
    {
        return; // Выходим если геометрия пуста
    }

    QgsVertexId vertexId;
    QgsPoint pt;
    while (geom->nextVertex(vertexId, pt))
    {
        mVertexMap.append(new QgsVertexEntry(pt, vertexId)); // Добавление вершины в карту
    }
}

void QgsLockedFeature::selectVertex(int vertexNr)
{
    if (vertexNr < 0 || vertexNr >= mVertexMap.size())
    {
        return; // Проверка границ индекса
    }

    QgsVertexEntry *entry = mVertexMap.at(vertexNr);
    entry->setSelected(true); // Выбор вершины

    emit selectionChanged(); // Сигнал об изменении выбора
}

void QgsLockedFeature::deselectVertex(int vertexNr)
{
    if (vertexNr < 0 || vertexNr >= mVertexMap.size())
    {
        return; // Проверка границ индекса
    }

    QgsVertexEntry *entry = mVertexMap.at(vertexNr);
    entry->setSelected(false); // Снятие выбора вершины

    emit selectionChanged(); // Сигнал об изменении выбора
}

void QgsLockedFeature::deselectAllVertices()
{
    for (int i = 0; i < mVertexMap.size(); i++)
    {
        mVertexMap.at(i)->setSelected(false); // Снятие выбора со всех вершин
    }
    emit selectionChanged(); // Сигнал об изменении выбора
}

void QgsLockedFeature::invertVertexSelection(int vertexNr)
{
    if (vertexNr < 0 || vertexNr >= mVertexMap.size())
    {
        return; // Проверка границ индекса
    }

    QgsVertexEntry *entry = mVertexMap.at(vertexNr);

    const bool selected = !entry->isSelected(); // Инверсия выбора

    entry->setSelected(selected);
    emit selectionChanged(); // Сигнал об изменении выбора
}

void QgsLockedFeature::invertVertexSelection(const QVector<int> &vertexIndices)
{
    const auto constVertexIndices = vertexIndices;
    for (const int index : constVertexIndices)
    {
        if (index < 0 || index >= mVertexMap.size())
        {
            continue; // Пропуск недопустимых индексов
        }

        QgsVertexEntry *entry = mVertexMap.at(index);
        entry->setSelected( !entry->isSelected()); // Инверсия выбора
    }
    emit selectionChanged(); // Сигнал об изменении выбора
}

QgsFeatureId QgsLockedFeature::featureId()
{
    return mFeatureId; // Возврат ID признака
}

QList<QgsVertexEntry *> &QgsLockedFeature::vertexMap()
{
    return mVertexMap; // Возврат ссылки на карту вершин
}

QgsVectorLayer *QgsLockedFeature::layer()
{
    return mLayer; // Возврат указателя на слой
}
