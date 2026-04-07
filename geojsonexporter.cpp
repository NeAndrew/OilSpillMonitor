#include "geojsonexporter.h"

GeoJsonExporter::GeoJsonExporter()
{
}

bool GeoJsonExporter::exportToGeoJson(const QString &filePath,  const QJsonArray &predictions, const GeoTransform &geoTransform, const QString &targetClass) const
{
    if (!geoTransform.isValid())
    {
        return false;
    }

    QFile file(filePath);
    if (!file.open(QIODevice::WriteOnly | QIODevice::Text))
    {
        return false;
    }

    QTextStream out(&file);
    
    // Заголовок GeoJSON
    out << "{\n  \"type\": \"FeatureCollection\",\n  \"crs\": {\n";
    out << "    \"type\": \"name\",\n    \"properties\": { \"name\": \"urn:ogc:def:crs:EPSG::3857\" }\n  },\n";
    out << "  \"features\": [\n";

    bool firstFeature = true;
    
    for (const QJsonValue &val : predictions)
    {
        QJsonObject pred = val.toObject();
        
        // Фильтрация по классу
        if (pred["class"].toString() != targetClass)
        {
            continue;
        }

        if (!firstFeature)
        {
            out << ",\n";
        }
        firstFeature = false;

        out << generateFeature(pred, geoTransform);
    }

    out << "\n  ]\n}\n";
    file.close();

    return true;
}

QPointF GeoJsonExporter::pixelToGeo(const QPointF &pixel, const GeoTransform &geoTransform) const
{
    double px = pixel.x();
    double py = pixel.y();
    
    // Учитываем масштабирование если размер растра отличается от размера изображения
    // Предполагаем, что pixel уже в координатах растра
    double geoX = geoTransform.coefficients[0] + px * geoTransform.coefficients[1] + py * geoTransform.coefficients[2];
    double geoY = geoTransform.coefficients[3] + px * geoTransform.coefficients[4] + py * geoTransform.coefficients[5];
    
    return QPointF(geoX, geoY);
}

QString GeoJsonExporter::generateFeature(const QJsonObject &prediction, const GeoTransform &geoTransform) const
{
    QString feature;
    QTextStream stream(&feature);
    
    stream << "    {\n      \"type\": \"Feature\",\n      \"properties\": {\n";
    stream << "        \"class\": \"" << prediction["class"].toString() << "\",\n";
    stream << "        \"confidence\": " << formatCoordinate(prediction["confidence"].toDouble()) << "\n";
    stream << "      },\n      \"geometry\": {\n        \"type\": \"Polygon\",\n        \"coordinates\": [\n          [\n";

    QJsonArray points = prediction["points"].toArray();
    bool firstPoint = true;
    
    // Записываем все точки полигона
    for (const QJsonValue &ptVal : std::as_const(points))
    {
        QJsonObject pt = ptVal.toObject();
        QPointF pixelPt(pt["x"].toDouble(), pt["y"].toDouble());
        QPointF geoPt = pixelToGeo(pixelPt, geoTransform);
        
        if (!firstPoint)
        {
            stream << ",\n";
        }
        stream << "            [" << formatCoordinate(geoPt.x()) << ", " << formatCoordinate(geoPt.y()) << "]";
        firstPoint = false;
    }
    
    // Замыкаем полигон первой точкой
    if (!points.isEmpty())
    {
        QJsonObject firstPt = points[0].toObject();
        QPointF pixelPt(firstPt["x"].toDouble(), firstPt["y"].toDouble());
        QPointF geoPt = pixelToGeo(pixelPt, geoTransform);
        stream << ",\n            [" << formatCoordinate(geoPt.x()) << ", " << formatCoordinate(geoPt.y()) << "]";
    }

    stream << "\n          ]\n        ]\n      }\n    }";
    
    return feature;
}

QString GeoJsonExporter::formatCoordinate(double value) const
{
    return QString::number(value, 'g', 10);
}
