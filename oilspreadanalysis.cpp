#include "oilspreadanalysis.h"

// Конструктор класса анализа распространения нефти
OilSpreadAnalysis::OilSpreadAnalysis(const OilSpreadAnalysisConfig &config, QObject *parent)
    : QObject(parent), m_config(config)
{
    // Проверяем валидность конфигурации
    if (!m_config.isValid())
    {
        m_errorMessage = m_config.errorMessage();
        return;
    }
    
    // Инициализация контекста обработки QGIS
    m_processingContext = std::make_unique<QgsProcessingContext>();
    m_processingContext->setProject(QgsProject::instance());
    m_processingContext->setInvalidGeometryCheck(QgsFeatureRequest::GeometryNoCheck);

    // Инициализация объекта обратной связи
    m_processingFeedback = std::make_unique<QgsProcessingFeedback>();

    // Проверка существования файла разлива
    if (!QFileInfo::exists(m_config.spillGeoJsonPath))
    {
        m_errorMessage = "Файл разлива не найден";
        return;
    }
    
    // Загрузка необходимых слоев и проверка пересечений
    if (!loadSpillLayer() || !loadLandLayer() || !checkNoIntersectionWithLand())
    {
        return;
    }

    // Создание пространственного индекса для суши
    m_landSpatialIndex = std::make_unique<QgsSpatialIndex>();
    QgsFeature f;
    QgsFeatureIterator it = m_landLayer->getFeatures();
    while (it.nextFeature(f))
    {
        m_landSpatialIndex->addFeature(f);
    }

    // Генерация кэша погодных данных
    if (!generateWeatherCache(m_config.weatherLatStep, m_config.weatherLonStep))
    {
        return;
    }

    m_valid = true;

    // Инициализация трансформатора координат
    QgsCoordinateReferenceSystem srcCRS("EPSG:3857");
    QgsCoordinateReferenceSystem destCRS("EPSG:4326");
    m_coordTransform = std::make_unique<QgsCoordinateTransform>(srcCRS, destCRS, QgsProject::instance());
}

// Проверка валидности объекта
bool OilSpreadAnalysis::isValid() const { return m_valid; }

// Получение сообщения об ошибке
QString OilSpreadAnalysis::errorMessage() const { return m_errorMessage; }

// Загрузка слоя с разливом нефти
bool OilSpreadAnalysis::loadSpillLayer()
{
    m_spillLayer = std::make_unique<QgsVectorLayer>(m_config.spillGeoJsonPath, "spill", "ogr");
    if (!m_spillLayer->isValid())
    {
        m_errorMessage = "Ошибка загрузки слоя разлива";
        return false;
    }
    return true;
}

// Загрузка слоя береговой линии
bool OilSpreadAnalysis::loadLandLayer()
{
    QString landPath;
    if (m_config.highResLand)
    {
        landPath = QStringLiteral(SRCDIR) + "Data/layers/Land_10m.gpkg";

    }
    else
    {
        landPath = QStringLiteral(SRCDIR) + "Data/layers/Land_50m.gpkg";

    }
    m_landLayer = std::make_unique<QgsVectorLayer>(landPath, "land", "ogr");
    if (!m_landLayer->isValid())
    {
        m_errorMessage = "Ошибка загрузки слоя суши";
        return false;
    }
    return true;
}

// Проверка пересечения разлива с сушей
bool OilSpreadAnalysis::checkNoIntersectionWithLand()
{
    QgsGeometry spillUnion;
    QgsFeature f;
    QgsFeatureIterator it = m_spillLayer->getFeatures();
    while (it.nextFeature(f))
    {
        if (spillUnion.isEmpty())
        {
            spillUnion = f.geometry();
        }
        else
        {
            spillUnion = spillUnion.combine(f.geometry());
        }
    }

    QgsFeature lf;
    QgsFeatureIterator lit = m_landLayer->getFeatures();
    while (lit.nextFeature(lf))
    {
        if (spillUnion.intersects(lf.geometry()))
        {
            m_errorMessage = "Начальный разлив пересекает сушу";
            return false;
        }
    }
    return true;
}

// Генерация начальных точек внутри полигона разлива
QList<QPointF> OilSpreadAnalysis::generateInitialPoints(int density)
{
    QList<QPointF> pts;
    QgsRectangle ext = m_spillLayer->extent();
    double step = 1000.0 / qSqrt(density);

    QgsFeature f;
    QgsFeatureIterator it = m_spillLayer->getFeatures();
    QList<QgsGeometry> geoms;
    while (it.nextFeature(f))
    {
        geoms.append(f.geometry());
    }

    for (double x = ext.xMinimum(); x <= ext.xMaximum(); x += step)
    {
        for (double y = ext.yMinimum(); y <= ext.yMaximum(); y += step)
        {
            QgsPointXY p(x, y);
            for (const auto &g : geoms)
            {
                if (g.contains(QgsGeometry::fromPointXY(p)))
                {
                    pts.append(QPointF(x, y));
                    break;
                }
            }
        }
    }
    return pts;
}

// Генерация кэша погодных данных
bool OilSpreadAnalysis::generateWeatherCache(double latStep, double lonStep)
{
    QgsRectangle ext = m_spillLayer->extent();
    ext.setXMinimum(ext.xMinimum() - m_config.bufferMeters);
    ext.setYMinimum(ext.yMinimum() - m_config.bufferMeters);
    ext.setXMaximum(ext.xMaximum() + m_config.bufferMeters);
    ext.setYMaximum(ext.yMaximum() + m_config.bufferMeters);


    QgsCoordinateReferenceSystem srcCRS("EPSG:3857");
    QgsCoordinateReferenceSystem destCRS("EPSG:4326");
    QgsCoordinateTransform transform(srcCRS, destCRS, QgsProject::instance());

    QgsPointXY minPt(ext.xMinimum(), ext.yMinimum());
    QgsPointXY wgsMin = transform.transform(minPt);
    double latOrigin = wgsMin.y();
    double lonOrigin = wgsMin.x();

    QgsPointXY maxPt(ext.xMaximum(), ext.yMaximum());
    QgsPointXY wgsMax = transform.transform(maxPt);

    int nLat = int((wgsMax.y() - latOrigin) / latStep) + 1;
    int nLon = int((wgsMax.x() - lonOrigin) / lonStep) + 1;

    QNetworkAccessManager mgr;
    QEventLoop loop;

    QDate startDate = m_config.startTime.date();
    QDate endDate = startDate.addDays(4); // 4 дня для API

    // Проверяем, нужно ли использовать архивный API (если дата старта более 3 месяцев назад)
    QDate currentDate = QDate::currentDate();
    bool useArchiveAPI = startDate.daysTo(currentDate) > 90; // Более 3 месяцев
    
    QString windApiBaseUrl;
    if (useArchiveAPI)
    {
        windApiBaseUrl = "https://archive-api.open-meteo.com/v1/archive";
    }
    else
    {
        windApiBaseUrl = "https://api.open-meteo.com/v1/forecast";
    }

    for (int i = 0; i < nLat; ++i)
    {
        for (int j = 0; j < nLon; ++j)
        {
            double lat = latOrigin + i * latStep;
            double lon = lonOrigin + j * lonStep;

            // Ветер
            QUrl windUrl(windApiBaseUrl);
            QUrlQuery windQuery;
            windQuery.addQueryItem("latitude", QString::number(lat));
            windQuery.addQueryItem("longitude", QString::number(lon));
            windQuery.addQueryItem("hourly", "wind_speed_10m,wind_direction_10m");
            windQuery.addQueryItem("timezone", "UTC");
            windQuery.addQueryItem("start_date", startDate.toString("yyyy-MM-dd"));
            windQuery.addQueryItem("end_date", endDate.toString("yyyy-MM-dd"));
            windUrl.setQuery(windQuery);

            QNetworkReply *windReply = mgr.get(QNetworkRequest(windUrl));
            QObject::connect(windReply, &QNetworkReply::finished, &loop, &QEventLoop::quit);
            loop.exec();
            if (windReply->error() != QNetworkReply::NoError)
            {
                m_errorMessage = "Ошибка запроса ветра: " + windReply->errorString();
                windReply->deleteLater();
                return false;
            }
            QJsonDocument windDoc = QJsonDocument::fromJson(windReply->readAll());
            windReply->deleteLater();
            if (!windDoc.isObject()) continue;
            QJsonObject windHourly = windDoc.object().value("hourly").toObject();
            QJsonArray wsArr = windHourly["wind_speed_10m"].toArray();
            QJsonArray wdArr = windHourly["wind_direction_10m"].toArray();
            int nHours = wsArr.size();

            // Течения
            QUrl curUrl("https://marine-api.open-meteo.com/v1/marine");
            QUrlQuery curQuery;
            curQuery.addQueryItem("latitude", QString::number(lat));
            curQuery.addQueryItem("longitude", QString::number(lon));
            curQuery.addQueryItem("hourly", "ocean_current_velocity,ocean_current_direction");
            curQuery.addQueryItem("timezone", "UTC");
            curQuery.addQueryItem("cell_selection", "sea");
            curQuery.addQueryItem("start_date", startDate.toString("yyyy-MM-dd"));
            curQuery.addQueryItem("end_date", endDate.toString("yyyy-MM-dd"));
            curUrl.setQuery(curQuery);

            QNetworkReply *curReply = mgr.get(QNetworkRequest(curUrl));
            QObject::connect(curReply, &QNetworkReply::finished, &loop, &QEventLoop::quit);
            loop.exec();
            if (curReply->error() != QNetworkReply::NoError)
            {
                m_errorMessage = "Ошибка запроса течений: " + curReply->errorString();
                curReply->deleteLater();
                return false;
            }
            QJsonDocument curDoc = QJsonDocument::fromJson(curReply->readAll());
            curReply->deleteLater();
            if (!curDoc.isObject()) continue;
            QJsonObject curHourly = curDoc.object().value("hourly").toObject();
            QJsonArray curVel = curHourly["ocean_current_velocity"].toArray();
            QJsonArray curDir = curHourly["ocean_current_direction"].toArray();
            if (curVel.size() != nHours || curDir.size() != nHours) continue;

            // Записываем в кэш первые 72 часа
            QList<WeatherHour> hourlyList;
            int maxHours = qMin(nHours, SIMULATION_HOURS);
            for (int h = 0; h < maxHours; ++h)
            {
                WeatherHour w;
                w.windSpeed = wsArr[h].toDouble();
                w.windDirection = wdArr[h].toDouble();
                w.currentSpeed = curVel[h].toDouble();
                w.currentDirection = curDir[h].toDouble();
                hourlyList.append(w);
            }

            // Округляем координаты до шага
            double keyLat = qRound(lat / latStep) * latStep;
            double keyLon = qRound(lon / lonStep) * lonStep;
            m_weatherCache[{keyLat, keyLon}] = hourlyList;
        }
    }

    qDebug() << "Сгенерирован кеш данных погоды";
    return true;
}

// Получение погодных данных для указанных координат и времени
WeatherHour OilSpreadAnalysis::weatherAt(double x_epsg3857, double y_epsg3857, int hour) const
{
    QgsPointXY ptWgs = m_coordTransform->transform(QgsPointXY(x_epsg3857, y_epsg3857));

    double lat = ptWgs.y();
    double lon = ptWgs.x();

    // Округление до сетки
    double keyLat = qRound(lat / m_config.weatherLatStep) * m_config.weatherLatStep;
    double keyLon = qRound(lon / m_config.weatherLonStep) * m_config.weatherLonStep;

    auto it = m_weatherCache.find({keyLat, keyLon});
    if (it != m_weatherCache.end() && hour < it.value().size())
    {
        return it.value()[hour];
    }

    return WeatherHour();
}

// Симуляция движения одной частицы
void OilSpreadAnalysis::simulateParticle(const QPointF &startPt, QList<ParticleState> &trajectory)
{
    QPointF pos = startPt;
    bool onShore = false;
    trajectory.append({pos, 0, false});

    // Цикл симуляции на 72 часа
    for (int hour = 1; hour <= SIMULATION_HOURS; ++hour)
    {
        if (onShore)
        {
            trajectory.append({pos, hour, true});
            continue;
        }

        // Получение погодных условий
        WeatherHour w = weatherAt(pos.x(), pos.y(), hour-1);

        // Расчет векторов движения
        double windRad = qDegreesToRadians(90.0 - w.windDirection);
        double curRad  = qDegreesToRadians(90.0 - w.currentDirection);

        double vx = 0.03 * w.windSpeed * qCos(windRad) + w.currentSpeed * qCos(curRad);
        double vy = 0.03 * w.windSpeed * qSin(windRad) + w.currentSpeed * qSin(curRad);

        // Новое положение частицы
        QPointF newPos(pos.x() + vx * 3600.0, pos.y() + vy * 3600.0);

        // Проверка пересечения с береговой линией (точность 50м)
        QgsRectangle searchRect(newPos.x() - 50.0, newPos.y() - 50.0, newPos.x() + 50.0, newPos.y() + 50.0);

        bool hitShore = false;
        const auto ids = m_landSpatialIndex->intersects(searchRect);
        for (QgsFeatureId fid : ids)
        {
            QgsFeature landF = m_landLayer->getFeature(fid);
            if (landF.geometry().contains(QgsGeometry::fromPointXY(QgsPointXY(newPos))))
            {
                hitShore = true;
                break;
            }
        }

        if (hitShore)
        {
            onShore = true;
            trajectory.append({newPos, hour, true});
        }
        else
        {
            pos = newPos;
            trajectory.append({pos, hour, false});
        }
    }
}

// Запуск симуляции распространения нефти
void OilSpreadAnalysis::run(const QString &outputPath, int density)
{
    bool ok = runSimulation(outputPath, density);
    emit finished(ok, ok ? QString() : m_errorMessage);
}

bool OilSpreadAnalysis::runSimulation(const QString &outputPath, int density)
{
    QList<QPointF> points = generateInitialPoints(density);
    QList<QFuture<QList<ParticleState>>> futures;

    for (const QPointF &p : std::as_const(points))
    {
        futures.append(QtConcurrent::run([this, p]()
        {
            QElapsedTimer timer;
            timer.start();
            QList<ParticleState> trajectory;
            simulateParticle(p, trajectory);
            return trajectory;
        }));
    }

    // Сбор результатов
    QList<QList<ParticleState>> all;
    all.reserve(futures.size());

    QElapsedTimer totalTimer;
    totalTimer.start();

    for (auto &f : futures)
    {
        f.waitForFinished();
        all.append(f.result());
    }

    saveTrajectoriesToGeoJSON(all, outputPath);
    return true;
}
// Сохранение траекторий в файл GeoJSON
void OilSpreadAnalysis::saveTrajectoriesToGeoJSON(const QList<QList<ParticleState>> &all, const QString &path)
{
    // Создание временного слоя для результатов
    QgsVectorLayer layer("Point?crs=EPSG:3857", "trajectories", "memory");

    // Настройка полей атрибутов
    QgsFields fields;
    fields.append(QgsField("hour", QVariant::Int));
    fields.append(QgsField("status", QVariant::String));
    layer.dataProvider()->addAttributes(fields.toList());
    layer.updateFields();

    // Создание объектов для каждой точки траектории
    QgsFeatureList feats;
    for (const auto &t : all)
    {
        for (const auto &s : t)
        {
            QgsFeature f(layer.fields());
            f.setGeometry(QgsGeometry::fromPointXY(QgsPointXY(s.position)));
            f.setAttribute("hour", s.hour);
            f.setAttribute("status", s.onShore ? "on_shore" : "in_water");
            feats.append(f);
        }
    }

    // Добавление объектов в слой и сохранение в файл
    layer.dataProvider()->addFeatures(feats);
    QgsVectorFileWriter::writeAsVectorFormat(&layer, path, "UTF-8", layer.crs(), "GeoJSON");
}

// Реализация методов OilSpreadAnalysisConfig
bool OilSpreadAnalysisConfig::isValid() const
{
    if (spillGeoJsonPath.isEmpty())
    {
        return false;
    }
    
    if (!QFileInfo::exists(spillGeoJsonPath))
    {
        return false;
    }
    
    if (!startTime.isValid())
    {
        return false;
    }
    
    if (bufferMeters <= 0)
    {
        return false;
    }
    
    if (weatherLatStep <= 0 || weatherLonStep <= 0)
    {
        return false;
    }
    
    return true;
}

QString OilSpreadAnalysisConfig::errorMessage() const
{
    if (spillGeoJsonPath.isEmpty())
    {
        return "Путь к файлу разлива не указан";
    }
    
    if (!QFileInfo::exists(spillGeoJsonPath))
    {
        return "Файл разлива не найден: " + spillGeoJsonPath;
    }
    
    if (!startTime.isValid())
    {
        return "Время начала не является корректным";
    }
    
    if (bufferMeters <= 0)
    {
        return "Размер буферной зоны должен быть положительным";
    }
    
    if (weatherLatStep <= 0 || weatherLonStep <= 0)
    {
        return "Шаги метеоданных должны быть положительными";
    }
    
    return QString();
}
