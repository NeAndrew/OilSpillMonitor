#ifndef MAPLAYERMANAGER_H
#define MAPLAYERMANAGER_H

// Библиотеки Qt
#include <QProcess>

// Библиотеки QGIS API
#include <qgsbrightnesscontrastfilter.h>
#include <qgsrasterlayer.h>
#include <qgsmarkersymbol.h>
#include <qgssinglebandgrayrenderer.h>
#include <qgssinglesymbolrenderer.h>
#include <qgsmarkersymbollayer.h>
#include <qgscategorizedsymbolrenderer.h>

// Инструменты рисования
#include "drawlinestringtool.h"
#include "drawpointtool.h"
#include "drawpolygontool.h"

/**
 * @brief Класс для работы с геоданными в QGIS
 *
 * Предоставляет функционал для добавления и обработки векторных и растровых слоев,
 * создания точечных, линейных и полигональных слоев.
 */
class MapLayerManager
{
public:
     // Конструктор класса MapLayerManager
    MapLayerManager();

    /**
     * @brief Добавляет новый точечный слой на карту
     *
     * Создает векторный слой типа Point с системой координат карты,
     * добавляет стандартные поля (id, name), настраивает стиль маркера
     * и активирует инструмент рисования точек.
     *
     * @param mMapCanvas ссылка на объект карты QGIS
     */
    void addPointLayer(QgsMapCanvas &mMapCanvas);

    /**
     * @brief Добавляет новый линейный слой на карту
     *
     * Создает векторный слой типа LineString с системой координат карты,
     * добавляет стандартные поля (id, name), настраивает стиль линии (синий цвет по умолчанию)
     * и активирует инструмент рисования линий.
     *
     * @param mMapCanvas ссылка на объект карты QGIS
     */
    void addLineLayer(QgsMapCanvas &mMapCanvas);

    /**
     * @brief Добавляет новый полигональный слой на карту
     *
     * Создает векторный слой типа Polygon с системой координат карты,
     * добавляет стандартные поля (id, name), настраивает стиль полигона
     * и активирует инструмент для рисования полигонов.
     * @param mMapCanvas ссылка на объект карты QGIS
     * @param layerName имя создаваемого слоя (по умолчанию "Polygon")
     */
    void addPolyLayer(QgsMapCanvas &mMapCanvas, const QString &layerName = "Polygon");
    
    /**
     * @brief Загружает векторный слой из файла
     *
     * Открывает векторный файл с использованием GDAL/OGR драйвера,
     * добавляет слой в проект QGIS
     *
     * @param filename путь к файлу векторного слоя
     * @param mSourceCrs система координат источника
     */
    void addVectorLayers(QString filename, QgsCoordinateReferenceSystem &mSourceCrs);

    /**
     * @brief Загружает растровый слой из файла
     *
     * Открывает растровый файл с использованием GDAL драйвера, выполняет перепроецирование
     * при необходимости через gdalwarp, применяет стили рендеринга в зависимости от флага
     * и добавляет слой в проект QGIS.
     *
     * @param filename путь к файлу растрового слоя
     * @param mSourceCrs система координат источника
     * @param styleFlag флаг стиля рендеринга:
     *                  - false (по умолчанию): стандартный рендеринг
     *                  - true: рендеринг с контрастным усилением и гамма-коррекцией
     */
    void addRasterLayers(QString filename, QgsCoordinateReferenceSystem &mSourceCrs, bool styleFlag = false);
    
    /**
     * @brief Множество поддерживаемых типов геометрий OGR
     *
     * Содержит типы геометрий, которые поддерживаются классом
     */

    /**
     * @brief Активирует инструмент рисования для добавления объектов в полигональный слой
     *
     * Создает и активирует инструмент DrawPolygonTool для рисования новых полигонов
     * в существующем слое.
     *
     * @param mMapCanvas ссылка на объект карты QGIS
     * @param layer ссылка на векторный полигональный слой
     */
    void addObjectToPolyLayer(QgsMapCanvas &mMapCanvas, QgsVectorLayer &layer);

    /**
     * @brief Активирует инструмент рисования для добавления объектов в точечный слой
     *
     * Создает и активирует инструмент DrawPointTool для рисования новых точек
     * в существующем слое.
     *
     * @param mMapCanvas ссылка на объект карты QGIS
     * @param layer ссылка на векторный точечный слой
     */
    void addObjectToPointLayer(QgsMapCanvas &mMapCanvas, QgsVectorLayer &layer);

    /**
     * @brief Активирует инструмент рисования для добавления объектов в линейный слой
     *
     * Создает и активирует инструмент DrawLineStringTool для рисования новых линий
     * в существующем слое.
     *
     * @param mMapCanvas ссылка на объект карты QGIS
     * @param layer ссылка на векторный линейный слой
     */
    void addObjectToLineLayer(QgsMapCanvas &mMapCanvas, QgsVectorLayer &layer);

    /**
     * @brief Перепроецирует векторный слой в целевую систему координат
     *
     * Создает новый векторный слой в памяти с трансформированными координатами.
     * Копирует все поля и объекты из исходного слоя, выполняя трансформацию геометрий.
     *
     * @param inputLayer указатель на исходный векторный слой
     * @param targetCrs целевая система координат
     * @return указатель на перепроецированный слой или nullptr в случае ошибки
     */
    QgsVectorLayer* reprojectLayer(QgsVectorLayer* inputLayer, const QgsCoordinateReferenceSystem& targetCrs);

    /**
     * @brief Добавляет стилизованный векторный слой на карту
     *
     * Загружает векторный слой из файла, применяет стилизацию и добавляет его на карту.
     * Функция автоматически определяет тип геометрии и применяет соответствующий стиль.
     *
     * @param filename путь к файлу векторного слоя
     * @param mSourceCrs исходная система координат слоя
     * @param field имя поля для отображения подписей объектов (опционально)
     */
    void addStyledVectorLayers(const QString &filename, QgsCoordinateReferenceSystem &mSourceCrs, QString field);
};

#endif // MAPLAYERMANAGER_H
