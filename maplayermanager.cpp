#include "maplayermanager.h"

MapLayerManager::MapLayerManager() { }

void MapLayerManager::addPointLayer(QgsMapCanvas &mMapCanvas)
{
    // Создаем векторный слой типа Point в памяти
    QgsVectorLayer* vectorLayer = new QgsVectorLayer("Point?crs=EPSG:3857", "PointsLayer", "memory");
    
    if (!vectorLayer || !vectorLayer->isValid())
    {
        qDebug() << "Ошибка: не удалось создать точечный слой";
        return;
    }

    // Устанавливаем систему координат из карты
    vectorLayer->setCrs(mMapCanvas.mapSettings().destinationCrs());
    
    // Создаем и добавляем поля в слой
    QgsFields fields;
    fields.append(QgsField("id", QVariant::Int));
    fields.append(QgsField("name", QVariant::String));

    QList<QgsField> fieldList = fields.toList();
    vectorLayer->dataProvider()->addAttributes(fieldList);
    vectorLayer->updateFields();
    vectorLayer->commitChanges();
    
    // Добавляем слой в проект
    QgsProject::instance()->addMapLayer(vectorLayer);

    // Создаем и активируем инструмент рисования точек
    DrawPointTool* drawTool = new DrawPointTool(&mMapCanvas);
    if (drawTool)
    {
        drawTool->setLayer(vectorLayer);
        mMapCanvas.setMapTool(drawTool);
        mMapCanvas.refresh();
        mMapCanvas.setLayers({vectorLayer});
    }

    QgsSingleSymbolRenderer* renderer = dynamic_cast<QgsSingleSymbolRenderer*>(vectorLayer->renderer());
    if (renderer)
    {
        QgsSymbol* symbol = renderer->symbol();
        if (symbol && symbol->symbolLayerCount() > 0)
        {
            QgsSimpleMarkerSymbolLayer* markerLayer = dynamic_cast<QgsSimpleMarkerSymbolLayer*>(symbol->symbolLayer(0));
            if (markerLayer)
            {
                markerLayer->setShape(Qgis::MarkerShape::Cross2);
                markerLayer->setSize(5);
                vectorLayer->triggerRepaint();
            }
        }
    }
}

void MapLayerManager::addLineLayer(QgsMapCanvas &mMapCanvas)
{
    // Создаем векторный слой типа LineString в памяти
    QgsVectorLayer* vectorLayer = new QgsVectorLayer("LineString?crs=EPSG:3857", "LinesLayer", "memory");
    
    if (!vectorLayer || !vectorLayer->isValid())
    {
        qDebug() << "Ошибка: не удалось создать линейный слой";
        return;
    }
    
    // Устанавливаем систему координат из карты
    vectorLayer->setCrs(mMapCanvas.mapSettings().destinationCrs());
    
    // Создаем и добавляем поля в слой
    QgsFields fields;
    fields.append(QgsField("id", QVariant::Int));
    fields.append(QgsField("name", QVariant::String));

    QList<QgsField> fieldList = fields.toList();
    vectorLayer->dataProvider()->addAttributes(fieldList);
    vectorLayer->updateFields();
    vectorLayer->commitChanges();
    
    // Добавляем слой в проект
    QgsProject::instance()->addMapLayer(vectorLayer);

    // Настраиваем стиль линии (синий цвет)
    QgsSingleSymbolRenderer* renderer = dynamic_cast<QgsSingleSymbolRenderer*>(vectorLayer->renderer());
    if (renderer)
    {
        QgsSymbol* symbol = renderer->symbol();
        if (symbol && symbol->symbolLayerCount() > 0)
        {
            QgsLineSymbolLayer* lineLayer = dynamic_cast<QgsLineSymbolLayer*>(symbol->symbolLayer(0));
            if (lineLayer)
            {
                lineLayer->setColor(Qt::blue);
                vectorLayer->triggerRepaint();
            }
        }
    }

    // Создаем и активируем инструмент рисования линий
    DrawLineStringTool* drawTool = new DrawLineStringTool(&mMapCanvas);
    if (drawTool)
    {
        drawTool->setLayer(vectorLayer);
        mMapCanvas.setMapTool(drawTool);
        mMapCanvas.refresh();
        mMapCanvas.setLayers({vectorLayer});
    }
}

void MapLayerManager::addPolyLayer(QgsMapCanvas &mMapCanvas, const QString &layerName)
{
    // Создаем векторный слой типа Polygon в памяти
    QgsVectorLayer* vectorLayer = new QgsVectorLayer("Polygon?crs=EPSG:3857", layerName, "memory");
    
    if (!vectorLayer || !vectorLayer->isValid())
    {
        qDebug() << "Ошибка: не удалось создать полигональный слой";
        return;
    }
    
    // Устанавливаем систему координат из карты
    vectorLayer->setCrs(mMapCanvas.mapSettings().destinationCrs());

    // Создаем и добавляем поля в слой
    QgsFields fields;
    fields.append(QgsField("id", QVariant::Int));
    fields.append(QgsField("name", QVariant::String));

    QList<QgsField> fieldList = fields.toList();
    vectorLayer->dataProvider()->addAttributes(fieldList);
    vectorLayer->updateFields();
    vectorLayer->commitChanges();
    
    // Добавляем слой в проект
    QgsProject::instance()->addMapLayer(vectorLayer);

    // Настройка стиля заливки: зеленый цвет
    QgsSymbol* symbol = QgsSymbol::defaultSymbol(Qgis::GeometryType::Polygon);
    if (symbol)
    {
        symbol->setColor(QColor(100, 200, 100, 100));
        vectorLayer->setRenderer(new QgsSingleSymbolRenderer(symbol));
    }

    // Создаем и активируем инструмент рисования полигонов
    DrawPolygonTool* drawTool = new DrawPolygonTool(&mMapCanvas);
    if (drawTool)
    {
        drawTool->setLayer(vectorLayer);
        mMapCanvas.setMapTool(drawTool);
        mMapCanvas.refresh();
        mMapCanvas.setLayers({vectorLayer});
    }
}



void MapLayerManager::addObjectToPolyLayer(QgsMapCanvas &mMapCanvas, QgsVectorLayer &layer)
{
    if (!layer.isValid())
    {
        qDebug() << "Ошибка: переданный полигональный слой невалиден";
        return;
    }
    
    // Создаем и активируем инструмент рисования полигонов
    DrawPolygonTool* drawTool = new DrawPolygonTool(&mMapCanvas);
    if (drawTool)
    {
        drawTool->setLayer(&layer);
        mMapCanvas.setMapTool(drawTool);
        qDebug() << "Инструмент рисования полигонов активирован.";
    }
}

void MapLayerManager::addObjectToLineLayer(QgsMapCanvas &mMapCanvas, QgsVectorLayer &layer)
{
    if (!layer.isValid())
    {
        qDebug() << "Ошибка: переданный линейный слой невалиден";
        return;
    }
    
    // Создаем и активируем инструмент рисования линий
    DrawLineStringTool* drawTool = new DrawLineStringTool(&mMapCanvas);
    if (drawTool)
    {
        drawTool->setLayer(&layer);
        mMapCanvas.setMapTool(drawTool);
        qDebug() << "Инструмент рисования линий активирован.";
    }
}

void MapLayerManager::addObjectToPointLayer(QgsMapCanvas &mMapCanvas, QgsVectorLayer &layer)
{
    if (!layer.isValid())
    {
        qDebug() << "Ошибка: переданный точечный слой невалиден";
        return;
    }
    
    // Создаем и активируем инструмент рисования точек
    DrawPointTool* drawTool = new DrawPointTool(&mMapCanvas);
    if (drawTool)
    {
        drawTool->setLayer(&layer);
        mMapCanvas.setMapTool(drawTool);
        qDebug() << "Инструмент рисования точек активирован.";
    }
}

QgsVectorLayer* MapLayerManager::reprojectLayer(QgsVectorLayer* inputLayer, const QgsCoordinateReferenceSystem& targetCrs)
{
    if (!inputLayer || !inputLayer->isValid() || !inputLayer->crs().isValid())
    {
        qDebug() << "Ошибка: исходный слой невалиден";
        return nullptr;
    }

    // Если системы координат совпадают, возвращаем исходный слой
    if (inputLayer->crs() == targetCrs)
    {
        return inputLayer;
    }

    // Определяем тип геометрии для URI слоя в памяти
    QString geomTypeName;
    switch (inputLayer->wkbType())
    {
    case Qgis::WkbType::PointZ:
        geomTypeName = "Point25D";
        break;
    case Qgis::WkbType::LineStringZ:
        geomTypeName = "LineString25D";
        break;
    case Qgis::WkbType::PolygonZ:
        geomTypeName = "Polygon25D";
        break;
    case Qgis::WkbType::MultiPointZ:
        geomTypeName = "MultiPoint25D";
        break;
    case Qgis::WkbType::MultiLineStringZ:
        geomTypeName = "MultiLineString25D";
        break;
    case Qgis::WkbType::MultiPolygonZ:
        geomTypeName = "MultiPolygon25D";
        break;
    default:
        geomTypeName = "Unknown";
        break;
    }

    // Формируем URI для слоя в памяти
    QString memLayerUri = geomTypeName + "?crs=" + targetCrs.authid();
    qDebug() << "URI перепроецированного слоя:" << memLayerUri;

    // Создаем новый слой в памяти
    QgsVectorLayer* reprojectedLayer = new QgsVectorLayer(memLayerUri, inputLayer->name() + "_3857", "memory");

    if (!reprojectedLayer || !reprojectedLayer->isValid())
    {
        qDebug() << "Ошибка: не удалось создать перепроецированный слой";
        delete reprojectedLayer;
        return nullptr;
    }

    // Копируем поля из исходного слоя
    QgsFields fields = inputLayer->fields();
    QList<QgsField> fieldList;
    for (int i = 0; i < fields.count(); ++i)
    {
        fieldList.append(fields.at(i));
    }
    reprojectedLayer->dataProvider()->addAttributes(fieldList);
    reprojectedLayer->updateFields();

    // Создаем трансформатор координат
    QgsCoordinateTransform transform(inputLayer->crs(), targetCrs, QgsProject::instance());

    // Копируем и трансформируем объекты
    QgsFeatureList features;
    QgsFeature feature;
    QgsFeatureIterator it = inputLayer->getFeatures();
    
    while (it.nextFeature(feature))
    {
        QgsGeometry geom = feature.geometry();

        // Если геометрия пустая, добавляем объект без трансформации
        if (geom.isNull())
        {
            features.append(feature);
            continue;
        }

        // Выполняем трансформацию координат
        try
        {
            geom.transform(transform);
            feature.setGeometry(geom);
        }
        catch (const QgsCsException &e)
        {
            Q_UNUSED(e)
            qDebug() << "Предупреждение: не удалось трансформировать объект, пропускаем";
            continue;
        }

        features.append(feature);
    }

    // Добавляем трансформированные объекты в новый слой
    reprojectedLayer->dataProvider()->addFeatures(features);
    reprojectedLayer->setCrs(targetCrs);

    return reprojectedLayer;
}

void MapLayerManager::addVectorLayers(QString filename, QgsCoordinateReferenceSystem &mSourceCrs)
{
    // Проверка на пустое имя файла
    if (filename.isEmpty())
    {
        qDebug() << "Ошибка: имя файла пустое";
        return;
    }

    // Получаем информацию о файле
    QFileInfo fileInfo(filename);

    // Создаем векторный слой из файла через OGR драйвер
    QgsVectorLayer* vecLayer = new QgsVectorLayer(filename, fileInfo.baseName(), "ogr");

    // Проверяем валидность созданного слоя
    if (!vecLayer || !vecLayer->isValid())
    {
        qDebug() << "Ошибка: не удалось загрузить векторный слой из файла:" << filename;
        delete vecLayer;
        return;
    }

    // Добавляем слой в проект QGIS
    QgsProject::instance()->addMapLayer(vecLayer);

    // Обновляем систему координат источника, если она не была задана
    if (!mSourceCrs.isValid())
    {
        mSourceCrs = vecLayer->crs();
        qDebug() << "Система координат установлена из слоя:" << mSourceCrs.authid();
    }
}

void MapLayerManager::addRasterLayers(QString filename, QgsCoordinateReferenceSystem &mSourceCrs, bool styleFlag)
{
    // Проверка на пустое имя файла
    if (filename.isEmpty())
    {
        qDebug() << "Ошибка: имя файла пустое";
        return;
    }

    // Получаем информацию о файле
    QFileInfo fileInfo(filename);
    
    // Создаем растровый слой из файла через GDAL драйвер
    QgsRasterLayer* rasterLayer = new QgsRasterLayer(filename, fileInfo.baseName(), "gdal");
    
    if (!rasterLayer || !rasterLayer->isValid())
    {
        qDebug() << "Ошибка: не удалось загрузить растровый слой из файла:" << filename;
        delete rasterLayer;
        return;
    }

    // Обновляем систему координат источника, если она не была задана
    if (!mSourceCrs.isValid())
    {
        mSourceCrs = rasterLayer->crs();
        qDebug() << "Система координат установлена из слоя:" << mSourceCrs.authid();
    }

    // Перепроецируем растр, если система координат слоя отличается от требуемой
    if (mSourceCrs.authid() != rasterLayer->crs().authid())
    {
        qDebug() << "Выполняется перепроецирование растра из" << rasterLayer->crs().authid() << "в" << mSourceCrs.authid();
        
        QString inputPath = filename;
        QString outputPath = fileInfo.path() + "/" + fileInfo.baseName() + "_tmp.tif";

        // Формируем аргументы для gdalwarp
        QStringList warpArguments;
        warpArguments << "-s_srs" << rasterLayer->crs().authid()  // исходная СК
                      << "-t_srs" << mSourceCrs.authid()          // целевая СК
                      << "-of" << "GTiff"                         // формат выходного файла
                      << inputPath
                      << outputPath;

        // Запускаем процесс перепроецирования
        QProcess warpProcess;
        warpProcess.start("gdalwarp", warpArguments);
        
        if (!warpProcess.waitForFinished(30000))
        {
            qDebug() << "Ошибка: процесс gdalwarp завершился с ошибкой";
            delete rasterLayer;
            return;
        }

        // Удаляем старый слой и создаем новый из перепроецированного файла
        delete rasterLayer;
        rasterLayer = new QgsRasterLayer(outputPath, fileInfo.baseName() + "_tmp", "gdal");
        
        if (!rasterLayer || !rasterLayer->isValid())
        {
            qDebug() << "Ошибка: не удалось загрузить перепроецированный растр";
            delete rasterLayer;
            return;
        }
        rasterLayer->setCrs(mSourceCrs);
    }

    // Применение стиля рендеринга (срез 2% - 98%, контраст)
    if (styleFlag == true)
    {
        int band = 1;
        QgsRasterDataProvider* provider = rasterLayer->dataProvider();
        
        if (!provider)
        {
            qDebug() << "Ошибка: провайдер данных растра недоступен";
            rasterLayer->triggerRepaint();
            QgsProject::instance()->addMapLayer(rasterLayer);
            return;
        }

        // Определяем границы среза: 2% и 98% для исключения выбросов
        const double lowerPercent = 0.02;
        const double upperPercent = 0.98;

        double minVal, maxVal;
        // Вычисляем минимальное и максимальное значения с учетом процентных границ
        provider->cumulativeCut(
                    band,
                    lowerPercent,
                    upperPercent,
                    minVal,
                    maxVal,
                    rasterLayer->extent(),
                    0
                    );

        // Создаем контрастное усиление с линейным растяжением между найденными границами
        QgsContrastEnhancement* contrast = new QgsContrastEnhancement(provider->dataType(band));
        contrast->setMinimumValue(minVal);
        contrast->setMaximumValue(maxVal);
        contrast->setContrastEnhancementAlgorithm(QgsContrastEnhancement::StretchToMinimumMaximum, true);

        // Применяем серый рендерер с контрастным усилением
        QgsSingleBandGrayRenderer* renderer = new QgsSingleBandGrayRenderer(provider, band);
        renderer->setContrastEnhancement(contrast);
        rasterLayer->setRenderer(renderer);
        
        // Устанавливаем гамма-коррекцию для улучшения видимости
        if (rasterLayer->brightnessFilter())
        {
            rasterLayer->brightnessFilter()->setGamma(2.0);
        }
        
        qDebug() << "Применен стиль рендеринга: серый с контрастным усилением";
    }
    
    // Обновляем отображение слоя
    rasterLayer->triggerRepaint();
    
    // Добавляем слой в проект QGIS
    QgsProject::instance()->addMapLayer(rasterLayer);
}

void MapLayerManager::addStyledVectorLayers(const QString &filename, QgsCoordinateReferenceSystem &mSourceCrs, QString field)
{
    // Проверяем, что путь к файлу не пустой
    if (filename.isEmpty())
    {
        qDebug() << "Ошибка: имя файла пустое";
        return;
    }

    // Извлекаем информацию о файле для получения имени слоя
    QFileInfo fileInfo(filename);
    
    // Создаем векторный слой из файла с помощью OGR драйвера
    QgsVectorLayer* vecLayer = new QgsVectorLayer(filename, fileInfo.baseName(), "ogr");

    // Проверяем, что слой успешно создан и является валидным
    if (!vecLayer || !vecLayer->isValid())
    {
        qDebug() << "Ошибка: не удалось загрузить слой:" << filename;
        delete vecLayer;
        return;
    }

    // Проверяем, что слой содержит точечную геометрию
    if (QgsWkbTypes::geometryType(vecLayer->wkbType()) != Qgis::GeometryType::Point)
    {
        qDebug() << "Ошибка: слой не является точечным. Тип:" << QgsWkbTypes::displayString(vecLayer->wkbType());
        delete vecLayer;
        return;
    }

    // Добавляем слой в проект QGIS для отображения на карте
    QgsProject::instance()->addMapLayer(vecLayer);

    // Обновляем систему координат источника, если она еще не установлена
    if (!mSourceCrs.isValid())
    {
        mSourceCrs = vecLayer->crs();
        qDebug() << "CRS установлен из слоя:" << mSourceCrs.authid();
    }

    // Проверяем, что указано поле для стилизации
    if (field.isEmpty())
    {
        qDebug() << "Поле для стилизации не задано";
        return;
    }

    // Находим индекс поля в атрибутивной таблице слоя
    int fieldIndex = vecLayer->fields().lookupField(field);
    if (fieldIndex == -1)
    {
        qDebug() << "Поле не найдено:" << field;
        return;
    }

    // Получаем все уникальные значения из указанного поля
    QList<QVariant> uniqueValues = vecLayer->uniqueValues(fieldIndex).values();
    
    // Сортируем значения по возрастанию для корректного отображения градиента
    std::sort(uniqueValues.begin(), uniqueValues.end(), [](const QVariant &a, const QVariant &b)
    {
        return a.toDouble() < b.toDouble();
    });

    // Определяем количество категорий для стилизации
    int categoryCount = uniqueValues.size();
    QList<QColor> palette;

    // Создаем цветовую палитру с градиентом от светлого к темному синему
    for (int i = 0; i < categoryCount; ++i)
    {
        // Вычисляем яркость: от 230 (светлый) до 50 (темный)
        int lightness = 230 - (i * 180 / qMax(1, categoryCount - 1));
        palette << QColor::fromHsl(210, 200, lightness); // HSL: оттенок 210°, насыщенность 200, яркость переменная
    }

    // Создаем категории для классифицированного рендерера
    QList<QgsRendererCategory> categories;
    for (int i = 0; i < categoryCount; ++i)
    {
        const QVariant& value = uniqueValues[i];
        QColor color = palette[i];

        // Создаем символ маркера: круг с заданным цветом, черной обводкой и размером
        QgsMarkerSymbol* symbol = QgsMarkerSymbol::createSimple(
                    {
                        {"name", "circle"},
                        {"color", color.name()},
                        {"outline_color", "black"},
                        {"outline_width", "0.2"},
                        {"size", "2.5"}
                    });

        // Добавляем категорию: значение, символ, подпись
        categories.append(QgsRendererCategory(value, symbol, value.toString()));
    }

    // Создаем классифицированный рендерер на основе поля и категорий
    QgsCategorizedSymbolRenderer* renderer = new QgsCategorizedSymbolRenderer(field, categories);

    // Применяем рендерер к слою и обновляем отображение
    vecLayer->setRenderer(renderer);
    vecLayer->triggerRepaint();
}
