#ifndef MAINWINDOW_H
#define MAINWINDOW_H

// Библиотеки Qt
#include <QMainWindow>
#include <QWidget>
#include <QApplication>
#include <QStackedWidget>
#include <QInputDialog>

// Библиотеки QGIS API
#include <qgsmapoverviewcanvas.h>
#include <qgsmaprendererparalleljob.h>
#include <qgsmaptoolpan.h>
#include <qgslayertree.h>
#include <qgslayertreemodel.h>
#include <qgslayertreemapcanvasbridge.h>
#include <qgslinesymbol.h>
#include <qgsfillsymbollayer.h>
#include <qgsscalecombobox.h>
#include <qgsprocessingregistry.h>
#include <EditTool/qgsvertextool.h>

// GDAL
#include <gdal_priv.h>

// Пользовательские классы
#include "measurementtool.h"
#include "drawpointtool.h"
#include "drawlinestringtool.h"
#include "drawpolygontool.h"
#include "selectbyrectangletool.h"
#include "maplayermanager.h"
#include "detectionoildialog.h"
#include "screenshotdialog.h"
#include "oilspreadanalysisdialog.h"
#include "copernicuslogindialog.h"
#include "reportoptionsdialog.h"
#include "sentineldownloaddialog.h"
#include "oilspreadanalysis.h"
#include "reportgenerator.h"
#include "SarProcess.h"
#include "maskedmenu.h"
#include "advancedslider.h"

QT_BEGIN_NAMESPACE
namespace Ui { class MainWindow; }
QT_END_NAMESPACE

/**
 * @class MainWindow
 * @brief Главное окно приложения OilSpillMonitor
 *
 * Класс главного окна предоставляет основной интерфейс для работы с приложением
 * обнаружения и анализа нефтяных пятен. Включает в себя картографические функции,
 * инструменты обработки спутниковых данных, моделирование распространения и генерацию отчетов.
 */
class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    /**
     * @brief Конструктор главного окна
     * @param parent Родительский виджет (по умолчанию nullptr)
     */
    MainWindow(QWidget *parent = nullptr);

    /**
     * @brief Деструктор главного окна
     */
    ~MainWindow();

    /**
     * @brief Обработчик событий для перехвата пользовательских действий
     *
     * Переопределённый метод QObject::eventFilter для фильтрации событий,
     * таких как нажатия клавиш или движения мыши, в контексте виджетов приложения.
     *
     * @param obj Указатель на объект, который отправил событие
     * @param event Указатель на событие
     * @return true, если событие обработано и не должно передаваться дальше;
     *         false в противном случае
     */
    bool eventFilter(QObject* obj, QEvent* event);

    // РАБОТА СО СЛОЯМИ КАРТЫ

    /**
     * @brief Контейнер для хранения всех загруженных слоёв карты
     *
     * Список указателей на QgsMapLayer, включающий как векторные, так и растровые слои
     */
    QList<QgsMapLayer*> layers;

    /**
     * @brief Создаёт общее представление (overview map) для навигации
     *
     * Инициализирует мини-карту, отображающую текущую область просмотра в контексте всей карты
     */
    void createOverview();

    /**
     * @brief Инициализирует дерево слоёв (QgsLayerTreeView)
     *
     * Настраивает отображение иерархии слоёв
     */
    void initLayerTreeView();

    /**
     * @brief Добавляет один или несколько векторных слоёв на карту
     *
     * Загружает векторные данные (например, GeoJSON, Shapefile) и добавляет их в проект
     */
    void addVectorLayers();

    /**
     * @brief Добавляет один или несколько растровых слоёв на карту
     */
    void addRasterLayers();

    /**
     * @brief Добавляет базовые слои (береговая линия, батиметрия)
     */
    void addBasicLayers();

    /**
     * @brief Удаляет выбранные слои из проекта и очищает ресурсы
     */
    void deleteLayers();

    /**
     * @brief Извлекает дату и время из имени файла
     * @param fileName Имя файла для анализа
     * @return Дата и время извлечённые из имени файла
     */
    QDateTime extractDateTimeFromFileName(const QString& fileName);

    // ВЗАИМОДЕЙСТВИЕ С ДЕРЕВОМ СЛОЁВ (QgsLayerTreeView)

    /**
     * @brief Обрабатывает двойной клик по элементу в дереве слоёв
     *
     * Приближает к охвату слоя
     *
     * @param index Индекс модели, соответствующий выбранному элементу
     */
    void onDoubleClicked(const QModelIndex &index);

    /**
     * @brief Обрабатывает одиночный клик по элементу в дереве слоёв
     *
     * Приближает к охвату всех слоев
     *
     * @param index Индекс модели, соответствующий выбранному элементу
     */
    void onClicked(const QModelIndex &index);

    // ВЗАИМОДЕЙСТВИЕ С КАРТОЙ (QgsMapCanvas)

    /**
     * @brief Обрабатывает движение мыши над картой и возвращает текущие координаты
     *
     * @param point Точка в системе координат карты
     * @return Строка с координатами
     */
    QString handleMouseMoved(const QgsPointXY& point);

    /**
     * @brief Обрабатывает щелчок мыши по карте
     *
     * @param point Координаты точки, по которой произошёл клик
     */
    void onMapClicked(const QgsPointXY& point);

    /**
     * @brief Слот для обработки движения мыши по карте
     *
     * Обновляет отображение координат
     *
     * @param point Текущая позиция курсора в координатах карты
     */
    void onMouseMoved(const QgsPointXY &point);

    // ИЗМЕРЕНИЯ НА КАРТЕ

    /**
     * @brief Активирует режим измерения расстояний на карте
     *
     * Позволяет пользователю кликать по карте для измерения линейного расстояния
     */
    void measurment();

    /**
     * @brief Активирует режим измерения площади
     *
     * Позволяет пользователю выделять полигон для расчета площади
     */
    void measureArea();

    /**
     * @brief Активирует режим измерения угла
     *
     * Позволяет пользователю указать три точки для определения угла между ними
     */
    void measureAngle();

    /**
     * @brief Обновляет отображение длины при изменении измеряемого расстояния
     *
     * @param totalDistance Общая длина линии (в метрах)
     * @param distance Длина последнего сегмента
     */
    void onDistanceChanged(double totalDistance, double distance);

    /**
     * @brief Обновляет отображение площади при её изменении
     *
     * @param area Площадь в квадратных метрах
     */
    void onDistanceChangedArea(double area);

    /**
     * @brief Обновляет отображение угла при его изменении
     *
     * @param angle Угол в градусах
     */
    void onAngleChanged(double angle);

    /**
     * @brief Извлекает дату и время из имени файла Sentinel
     * @param fileName Имя файла Sentinel для анализа
     * @return Дата и время извлечённые из имени файла
     */
    QDateTime extractSentinelDateTimeFromFileName(const QString &fileName);

    /**
     * @brief Очищает лейбл измерений при сбросе
     */
    void onMeasurementReset();

    // РЕЖИМЫ РАБОТЫ С КАРТОЙ

    /**
     * @brief Активирует режим выбора объектов на карте
     *
     * Позволяет пользователю выделять векторные объекты
     */
    void selectionMode();

    /**
     * @brief Очищает выделение со всех слоёв
     *
     * Слот, вызываемый при нажатии кнопки "Очистить выделение"
     */
    void onClearSelectionClicked();

    /**
     * @brief Начинает редактирование выбранного векторного слоя
     *
     * Переводит слой в режим редактирования (добавление/удаление/изменение объектов)
     */
    void onStartEditClicked();

    /**
     * @brief Завершает редактирование текущего слоя
     *
     * Сохраняет изменения и выходит из режима редактирования
     */
    void onStopEditClicked();

    /**
     * @brief Добавляет новый объект (геометрию) в активный векторный слой
     */
    void addNewObjectToLayer();

    /**
     * @brief Создаёт новый полигон для оцифровки нефтяного пятна
     */
    void onCreatePolygonClicked();

    // ПОЛУЧЕНИЕ ВНЕШНИХ ДАННЫХ

    /**
     * @brief Получает метеорологические данные по заданной точке
     *
     * @param point Географическая точка для запроса погодных данных
     */
    void getWeatherData(const QgsPointXY& point);

    /**
     * @brief Обрабатывает ответ от сервера погоды
     *
     * Парсит JSON-ответ и отображает данные пользователю
     *
     * @param reply Указатель на сетевой ответ (QNetworkReply)
     */
    void processWeatherResponse(QNetworkReply* reply);

    // РАБОТА С WMS / WFS / WMTS

    /**
     * @brief Инициирует процесс добавления WMS-слоя
     */
    void processWMS();

    /**
     * @brief Добавляет WMS-слой по заданному URL и имени слоя
     *
     * @param wmsBaseUrl Базовый URL сервиса WMS
     * @param wmsLayerName Имя слоя, предоставляемого сервисом
     */
    void addWMSLayer(const QString &wmsBaseUrl, const QString &wmsLayerName);

    /**
     * @brief Инициирует процесс добавления WFS-слоя
     */
    void processWFS();

    /**
     * @brief Добавляет WFS-слой по заданному URL и имени слоя
     *
     * @param wfsBaseUrl Базовый URL сервиса WFS
     * @param wfsLayerName Имя векторного слоя
     */
    void addWFSLayer(const QString &wfsBaseUrl, const QString &wfsLayerName);

    /**
     * @brief Инициирует процесс добавления WMTS-слоя
     */
    void processWMTS();

    /**
     * @brief Добавляет WMTS-слой по заданному URL и имени слоя
     *
     * @param wmtsBaseUrl Базовый URL сервиса WMTS
     * @param wmtsLayerName Имя тайлового слоя
     */
    void addWMTSLayer(const QString &wmtsBaseUrl, const QString &wmtsLayerName);

    // ОБРАБОТКА SAR И СКАЧИВАНИЕ СПУТНИКОВЫХ ДАННЫХ

    /**
     * @brief Запускает поиск нефтяного пятна на основе SAR-данных
     *
     * Слот, вызываемый при нажатии соответствующей кнопки
     */
    void onFindOilSpillClicked();

    /**
     * @brief Запускает скачивание спутниковых снимков Sentinel
     */
    void onDownloadSentinelClicked();

    /**
     * @brief Устанавливает пароль для доступа к Copernicus Open Access Hub
     */
    void setCopernicusPassword();

    // ПРОГНОЗИРОВАНИЕ И МОДЕЛИРОВАНИЕ

    /**
     * @brief Запускает процесс прогнозирования распространения пятна
     */
    void onStartAnalysisClicked();

    /**
     * @brief Настраивает док-виджет для моделирования распространения
     *
     * @param parent Родительский виджет (обычно главное окно)
     */
    void setupSpreadSimulationDock(QWidget* parent);

    /**
     * @brief Добавляет слой с результатами анализа распространения пятна
     */
    void addSpillAnalysis();

    /**
     * @brief Обрабатывает завершение анализа нефтяного пятна
     *
     * @param ok Флаг успешности анализа
     * @param error Текст ошибки при неудаче
     */
    void onOilAnalysisFinished(bool ok, const QString &error);

    // ЭКСПОРТ И ПОСТОБРАБОТКА

    /**
     * @brief Создаёт скриншот текущего представления карты
     *
     * Сохраняет изображение в файл или буфер
     */
    void makeScreenshot();

    /**
     * @brief Создаёт отчёт об обнаруженных нефтяных пятнах
     */
    void createReport();

    /**
     * @brief Рассчитывает геометрические параметры для объединенной геометрии слоя
     *
     * @param layer Векторный слой для анализа
     * @return Структура SpotDetails с рассчитанными параметрами (площадь, периметр, координаты)
     */
    SpotDetails calculateGeometryMetrics(QgsVectorLayer* layer);

    /**
     * @brief Генерирует кадры симуляции распространения пятна
     *
     * @param spillLayer Слой с нефтяным пятном
     * @param outputDir Каталог для сохранения кадров
     * @param hours Список часов для симуляции
     * @param mapCanvas Холст карты для отрисовки
     */
    void renderSpillFrames(QgsVectorLayer* spillLayer, const QString& outputDir, const QList<int>& hours, QgsMapCanvas* mapCanvas);

    // ИНТЕГРАЦИЯ С ROBOFLOW

    /**
     * @brief Устанавливает API-ключ для доступа к Roboflow
     */
    void setRoboflowApiKey();

    // УТИЛИТЫ И ВСПОМОГАТЕЛЬНЫЕ ФУНКЦИИ

    /**
     * @brief Включает отслеживание координат мыши в реальном времени
     */
    void setCoordinatesTracking();

    /**
     * @brief Отключает отслеживание координат мыши
     */
    void clearCoordinatesTracking();

    /**
     * @brief Настраивает путь к приложению SNAP с инструментами для обработки радиолокационных снимков
     */
    void setPathSnap();

    /**
     * @brief Отображает диалоговое окно с поставщиками данных (WMS|WFS|WMTS)
     *
     * @param parent Виджет родительского класса
     * @param predefinedProviders Предопределенные провайдеры/поставщики данных
     * @return Выбранный провайдер данных
     */
    QString showProviderSelectionDialog(QWidget* parent, const QStringList& predefinedProviders);

    /**
     * @brief Блокирует холст карты для предотвращения изменений
     */
    void lockMapCanvas();

    /**
     * @brief Разблокирует холст карты
     */
    void unlockMapCanvas();

    // ФУНКЦИИ КОНСТРУКТОРА ДЛЯ ИНИЦИАЛИЗАЦИИ ОБЪЕКТОВ

    /**
     * @brief Настраивает параметры главного окна
     */
    void setupWindow();

    /**
     * @brief Создаёт центральный слой размещения виджетов
     */
    void createCentralLayout();

    /**
     * @brief Создаёт холст карты
     */
    void createMapCanvas();

    /**
     * @brief Создаёт инструменты измерения
     */
    void createMeasurementTools();

    /**
     * @brief Создаёт отображение координат
     */
    void createCoordinateDisplay();

    /**
     * @brief Создаёт дерево слоёв
     */
    void createLayerTreeView();

    /**
     * @brief Создаёт инструменты карты
     */
    void createMapTools();

    /**
     * @brief Создаёт панель основных функций
     *
     * @param parent Родительский виджет
     */
    void createMainFunctionsPanel(QWidget *parent);

    /**
     * @brief Создаёт панель управления данными
     *
     * @param parent Родительский виджет
     */
    void createControlPanel(QWidget *parent);

    /**
     * @brief Устанавливает соединения между сигналами и слотами
     */
    void setupConnections();

    /**
     * @brief Инициализирует настройки карты
     */
    void initializeMapSettings();

    /**
     * @brief Инициализирует рабочий объект для обработки SAR
     */
    void initOilWorker();

    /**
     * @brief Обрабатывает завершение работы SAR обработчика
     *
     * @param result Результаты обработки SAR данных
     */
    void onWorkerFinished(const SARProcessingResult &result);


signals:
    /**
     * @brief Сигнал для запуска анализа нефтяного пятна
     *
     * @param outputPath Путь к выходным данным
     * @param density Плотность нефти
     */
    void startOilAnalysis(const QString& outputPath, int density);

private:
    Ui::MainWindow *ui;

    QString copernicusLogin;                        // Логин Copernicus
    QString copernicusPassword;                     // Пароль Copernicus

    QgsMapCanvas *mMapCanvas = nullptr;             // Основной холст для отображения карты
    QgsMapOverviewCanvas *mOverviewCanvas = nullptr;    // Класс обзорной карты
    QgsCoordinateReferenceSystem mSourceCrs;        // Система координат

    QgsLayerTreeView *mLayerTreeView = nullptr;     // Класс дерева слоёв
    QgsLayerTreeMapCanvasBridge *mLayerTreeCanvasBridge = nullptr;  // Связывает карту и дерево слоёв
    QgsLayerTree *root = nullptr;                   // Корневой узел дерева слоёв
    QgsLayerTreeViewDefaultActions *actions = nullptr;  // Стандартные действия для дерева слоёв

    QgsDockWidget *mLayerTreeDock = nullptr;        // Панель для дерева слоёв
    QgsDockWidget *mOverviewDock = nullptr;         // Панель для обзорной карты
    QgsMapToolEmitPoint *mClickPoint = nullptr;     // Инструмент получения координат под курсором
    QgsMapTool *mpPanTool = nullptr;                // Инструмент перемещения по карте
    QgsVertexTool* vertexTool = nullptr;            // Инструмент редактирования вершин векторного слоя
    QgsAdvancedDigitizingDockWidget* digitizingDock = nullptr;  // Панель продвинутой векторизации
    QgsVectorLayer* editedLayer = nullptr;          // Текущий редактируемый векторный слой
    QStackedWidget *mCentralContainer = nullptr;    // Контейнер для виджетов
    QHBoxLayout* layout = nullptr;                  // Основной слой размещения

    QLabel* mLabelMeasurment = nullptr;             // Лейбл для отображения измеренного расстояния
    QWidget* bottomRightOverlay = nullptr;          // Накладываемый виджет в правом нижнем углу
    QLabel* coordinatesLabel = nullptr;             // Лейбл для координат
    QgsScaleComboBox *scaleCombo = nullptr;         // Комбобокс масштаба
    QPushButton* pbSelect = nullptr;                // Кнопка выбора
    QPushButton* pbStopEdit = nullptr;              // Кнопка завершения редактирования
    QPushButton *pbClearSelection = nullptr;        // Кнопка очистки выделения
    QPushButton *pbFindOilSpill = nullptr;          // Кнопка обнаружения нефтяных пятен на снимке
    QPushButton *pbStartAnalysis = nullptr;         // Кнопка начала анализа распространения
    QPushButton *pbStartEdit = nullptr;             // Кнопка начала редактирования объектов
    QPushButton *pbCreateReport = nullptr;          // Кнопка создания отчета
    QPushButton *pbAddSpillAnalysis = nullptr;      // Кнопка добавления слоя с анализом распространения
    QPushButton *pbDownloadSentinel = nullptr;      // Кнопка скачивания спутникового снимка Sentinel
    QPushButton *pbCreatePolygon = nullptr;         // Кнопка создания нового полигонального слоя
    QPushButton *pbAddNewObject = nullptr;          // Кнопка добавления объекта в существующий слой
    QToolButton *pbMeasurment = nullptr;            // Кнопка измерения
    QAction *actDistance = nullptr;                 // Действие измерения расстояния
    QAction *actArea = nullptr;                     // Действие измерения площади
    QAction *actAngle = nullptr;                    // Действие измерения угла
    QPushButton* pbStartSimulation = nullptr;       // Кнопка старта симуляции
    QPushButton* pbStopSimulation = nullptr;        // Кнопка остановки симуляции

    advancedSlider* simulationSpeedSlider = nullptr;    // Слайдер скорости симуляции

    double mDistance = 0.0;                         // Текущее расстояние, измеренное линейкой
    bool isMeasurment = false;                      // Флаг активности режима измерения
    MeasurementTool *measurementTool = nullptr;     // Инструмент измерения
    MapLayerManager* m_dataHandling = nullptr;      // Менеджер обработки слоёв

    QString SnapPath;                               // Путь к инструментам SNAP
    QString outputPath_vv;                          // Путь для вывода обработанных данных SAR
    QDateTime currentDataTime;                      // Текущее время для анализа
    QDateTime imageDataTime;                        // Время данных изображения
    QDateTime predictionDateTime;                   // Время прогноза (currentDataTime + количество часов для предсказания распространения)
    QDateTime baseTime = QDateTime::currentDateTime();  // Базовое время для симуляции

    bool predictionMode = false;                    // Режим предсказания
    bool coordinateTracking = false;                // Отслеживание координат
    bool m_processing = false;                      // Флаг обработки SAR данных
    QNetworkAccessManager *networkManager = nullptr;    // Менеджер сетевых запросов для AI
    QNetworkAccessManager *weatherNetworkManager = nullptr; // Менеджер сетевых запросов для погоды

    QgsPointXY lastWeatherPoint;                    // Последняя точка запроса погоды
    QgsPointXY pendingWeatherPoint;                 // Точка ожидания погодных данных
    QString currentWindData;                        // Данные о ветре (скорость и направление)
    QTimer *weatherUpdateTimer = nullptr;           // Таймер обновления погодных данных

    QString roboflowApiKey;                         // API ключ Roboflow
    QLabel *imageLabel = nullptr;                   // Лейбл для отображения изображения с обнаружением
    QSlider *confidenceSlider = nullptr;            // Ползунок порога достоверности
    QComboBox *labelModeCombo = nullptr;            // Комбобокс режима меток
    QTextEdit *jsonView = nullptr;                  // Просмотр JSON результатов
    QPushButton *copyButton = nullptr;              // Кнопка копирования
    double confidenceThreshold = 0.5;               // Порог достоверности для детекции
    QString lastDetectionOverlayImagePath;          // Путь к сохранённому изображению с детекцией (для отчёта)
    QVector<double> m_geotiffGeoTransform;          // Географическая трансформация GeoTIFF
    QSize m_geotiffSize;                            // Размер файла
    QgsRectangle m_lockedExtent;                    // Заблокированная область карты

    QThread* m_analysisThread = nullptr;            // Поток для анализа распространения
    OilSpreadAnalysis* m_analysisWorker = nullptr;  // Рабочий объект для анализа распространения
    QFutureWatcher<SARProcessingResult>* m_sarWatcher = nullptr;    // Наблюдатель за результатами обработки SAR
    QThread *m_workerThread = nullptr;              // Поток для обработки SAR данных
    SarProcess *m_oilWorker = nullptr;              // Рабочий объект для обработки SAR данных

};
#endif // MAINWINDOW_H
