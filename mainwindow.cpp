/**********************************************************************************
 * OilSpillMonitor - Приложение для обнаружения и анализа распространения нефтяных пятен на основе спутниковых данных
 *
 * Разработчик: Андрей Литвинов
 * Email: andrewlitvinov47@gmail.com
 * Институт: РТУ МИРЭА
 *
 * Назначение приложения:
 * Обнаружение разливов нефти на водной поверхности с использованием данных
 * спутниковой радиолокационной съёмки (SAR) и методов компьютерного зрения,
 * а также анализ их последующего распространения, основываясь на метеорологических данных.
 *
 * Основные функции:
 * - Загрузка и обработка спутниковых изображений Sentinel-1 SAR
 * - Автоматическое обнаружение разливов нефти с использованием CV алгоритмов
 * - Визуализация результатов обнаружения на интерактивной карте
 * - Инструменты для ручной разметки и анализа областей разливов
 * - Моделирование распространения нефтяных пятен (Используя метерологические данные Wearher API)
 * - Генерация отчетов об обнаруженных разливах
 * - Экспорт результатов в GeoJSON формат
 * - Интеграция с Copernicus Open Access Hub для загрузки данных
 *
 * Используемые технологии:
 * - Qt Framework (GUI, работа с данными)
 * - QGIS (картографические функции, работа с геоданными)
 * - CV (компьютерное зрение, обработка изображений)
 * - GDAL/OGR (работа с геопространственными данными)
 * - Алгоритмы машинного обучения для обнаружения разливов
 * - SAR-обработка спутниковых данных
 *
 * Версия: 1.0
 * Годы разработки: 2025/2026
 *
 * --------------------------------------------------------------------------------
 *
 * OilSpillMonitor - Application for Detecting and Analyzing Oil Spill Spread Using Satellite Imagery
 *
 * Developer: Andrei Litvinov
 * Email: andrewlitvinov47@gmail.com
 * Institute: RTU MIREA
 *
 * Application Purpose:
 * Detection of oil spills on water surfaces using satellite radar (SAR) data
 * and computer vision methods, as well as analysis of their subsequent spread
 * based on meteorological data.
 *
 * Key Features:
 * - Loading and processing Sentinel-1 SAR satellite imagery
 * - Automatic oil spill detection using CV algorithms
 * - Visualization of detection results on interactive map
 * - Tools for manual annotation and analysis of spill areas
 * - Oil spill spread modeling (Using meteorological data from Weather API)
 * - Report generation for detected spills
 * - Export results to GeoJSON format
 * - Integration with Copernicus Open Access Hub for data downloading
 *
 * Technologies Used:
 * - Qt Framework (GUI, data processing)
 * - QGIS (mapping functions, geodata processing)
 * - CV (computer vision, image processing)
 * - GDAL/OGR (geospatial data processing)
 * - Machine learning algorithms for spill detection
 * - SAR satellite data processing
 *
 * Version: 1.0
 * Development Years: 2025/2026
 **********************************************************************************/

#include "mainwindow.h"
#include "ui_mainwindow.h"

// Конструктор
MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
{
    ui->setupUi(this);
    setupWindow();
    createCentralLayout();
    createMapCanvas();
    createMeasurementTools();
    createCoordinateDisplay();
    createLayerTreeView();
    createMapTools();
    setupSpreadSimulationDock(parent);
    createMainFunctionsPanel(parent);
    createControlPanel(parent);
    setupConnections();
    initializeMapSettings();
    addBasicLayers();
    setPathSnap();
    initOilWorker();
}

// Деструктор
MainWindow::~MainWindow()
{   
    // Ожидаем завершения потока обработки SAR данных
    if (m_workerThread)
    {
        m_workerThread->quit();
        m_workerThread->wait(3000);
        delete m_workerThread;
    }

    if (m_oilWorker)
    {
        delete m_oilWorker;
    }

    delete ui;
    delete vertexTool;
    delete digitizingDock;
}

/// ФУНКЦИИ КОНСТРУКТОРА

// Функция установки внешнего вида главного окна
void MainWindow::setupWindow()
{
    setWindowTitle("OilSpillMonitor"); // заголовок
    setWindowIcon(QIcon(":/icon/res/application-icon.png")); // иконка
    showMaximized(); // полноэкранный режим
}

// Функция создания центрального лейаута
void MainWindow::createCentralLayout()
{
    QWidget *centralWidget = this->centralWidget();
    QGridLayout *centralLayout = new QGridLayout(centralWidget);
    centralWidget->setLayout(centralLayout);
    centralLayout->setContentsMargins(0, 0, 0, 0);

    mCentralContainer = new QStackedWidget;
    centralLayout->addWidget(mCentralContainer, 0, 0, 2, 1);
    mCentralContainer->setCurrentIndex(0);
}

// Функция создания холста
void MainWindow::createMapCanvas()
{
    QWidget *centralWidget = this->centralWidget();
    mMapCanvas = new QgsMapCanvas(centralWidget);
    
    // Настраиваем основные параметры холста
    mMapCanvas->setVisible(true);           // Делаем холст видимым
    mMapCanvas->enableAntiAliasing(true);   // Включаем сглаживание для лучшего качества отображения
    mMapCanvas->setMouseTracking(true);     // Включаем отслеживание движения мыши для координат
    
    mClickPoint = new QgsMapToolEmitPoint(mMapCanvas); // Создаем инструмент для определения координат клика мыши
    mCentralContainer->insertWidget(0, mMapCanvas);
    mMapCanvas->installEventFilter(this); // Устанавливаем фильтр событий для обработки событий холста
}

// Функция создания кнопки измерения
void MainWindow::createMeasurementTools()
{
    QWidget* overlayWidget = new QWidget(mMapCanvas);
    overlayWidget->setAttribute(Qt::WA_NoSystemBackground);

    pbMeasurment = new QToolButton(overlayWidget);
    pbMeasurment->setText("Линейка");
    pbMeasurment->setIcon(QIcon(":/res/area.png"));
    pbMeasurment->setIconSize(QSize(24,24));
    pbMeasurment->setPopupMode(QToolButton::InstantPopup);
    
    pbMeasurment->setStyleSheet("QToolButton::menu-indicator { image: none; }"); // Убираем стрелку вниз

    QMenu* menu = new MaskedMenu(pbMeasurment);
    actDistance = new QAction("Измерить расстояние", menu);
    actArea = new QAction("Измерить площадь", menu);
    actAngle = new QAction("Измерить угол", menu);
    menu->addAction(actDistance);
    menu->addAction(actArea);
    menu->addAction(actAngle);
    pbMeasurment->setMenu(menu);

    QVBoxLayout* overlayLayout = new QVBoxLayout(overlayWidget);
    overlayLayout->setContentsMargins(5,5,5,5);
    overlayLayout->addWidget(pbMeasurment);
    overlayLayout->addStretch();
    overlayWidget->setLayout(overlayLayout);
    overlayWidget->move(0,0);
}

// Функция создания лейбла с отображением координат поверх холста
void MainWindow::createCoordinateDisplay()
{
    bottomRightOverlay = new QWidget(mMapCanvas);
    bottomRightOverlay->setAttribute(Qt::WA_NoSystemBackground);

    coordinatesLabel = new QLabel("Координаты: ", bottomRightOverlay);
    coordinatesLabel->setAutoFillBackground(true);
    coordinatesLabel->setAttribute(Qt::WA_StyledBackground, true);
    
    // Задаем стиль
    coordinatesLabel->setStyleSheet(
                "QLabel {"
                "    color: white;"
                "    background-color: #9ACEEB;"
                "    border-radius: 14px;"
                "    padding: 8px 12px;"
                "}"
                );
    coordinatesLabel->setMinimumWidth(300);
    coordinatesLabel->setMinimumHeight(35);
    coordinatesLabel->setAlignment(Qt::AlignHCenter);

    QHBoxLayout* hLayout = new QHBoxLayout(bottomRightOverlay);
    hLayout->setContentsMargins(5, 5, 5, 5);
    hLayout->setSpacing(20);
    hLayout->addWidget(coordinatesLabel);
    bottomRightOverlay->setLayout(hLayout);
    bottomRightOverlay->setMinimumWidth(450);

    bottomRightOverlay->adjustSize();
    bottomRightOverlay->move(mMapCanvas->width() - bottomRightOverlay->width(), mMapCanvas->height() - bottomRightOverlay->height());
}

// Функция создания дерева слоев и виджета обзора карты
void MainWindow::createLayerTreeView()
{
    mLayerTreeView = new QgsLayerTreeView(this);
    mLayerTreeView->setObjectName(QStringLiteral("theLayerTreeView"));

    initLayerTreeView();
    createOverview();
}

// Функция создания инструментов холста
void MainWindow::createMapTools()
{
    mpPanTool = new QgsMapToolPan(mMapCanvas);
    mMapCanvas->setMapTool(mpPanTool);
}

// Функция создания панели симуляции распространения нефтяного пятна
void MainWindow::setupSpreadSimulationDock(QWidget* parent)
{
    // Создание док виджета
    QDockWidget* spreadSimulationPanel = new QDockWidget("Симуляция нефтяного пятна", parent);
    spreadSimulationPanel->setAllowedAreas(Qt::BottomDockWidgetArea | Qt::TopDockWidgetArea);
    spreadSimulationPanel->setMinimumWidth(300);
    spreadSimulationPanel->setMaximumHeight(150);

    QWidget* spreadSimulationWidget = new QWidget(spreadSimulationPanel);

    // Кнопки
    pbStartSimulation = new QPushButton("", spreadSimulationWidget);
    pbStopSimulation = new QPushButton("", spreadSimulationWidget);

    pbStartSimulation->setEnabled(false);
    pbStopSimulation->setEnabled(false);

    // Иконки
    QIcon playIcon(":/res/play-button.png");
    QIcon pauseIcon(":/res/pause-button.png");

    auto makeGrayIcon = [](const QPixmap& pix)
    {
        QPixmap gray = pix;
        QPainter p(&gray);
        p.setCompositionMode(QPainter::CompositionMode_SourceIn);
        p.fillRect(gray.rect(), Qt::gray);
        p.end();
        return gray;
    };

    playIcon.addPixmap(makeGrayIcon(QPixmap(":/res/play-button.png")), QIcon::Disabled);
    pauseIcon.addPixmap(makeGrayIcon(QPixmap(":/res/pause-button.png")), QIcon::Disabled);

    pbStartSimulation->setIcon(playIcon);
    pbStopSimulation->setIcon(pauseIcon);

    pbStartSimulation->setIconSize(QSize(24,24));
    pbStopSimulation->setIconSize(QSize(24,24));

    QString buttonStyle =
            "QPushButton { background-color: transparent; border: none; outline: none; }"
            "QPushButton:hover { background-color: transparent; }"
            "QPushButton:pressed { background-color: transparent; }";

    pbStartSimulation->setStyleSheet(buttonStyle);
    pbStopSimulation->setStyleSheet(buttonStyle);
    pbStartSimulation->setFocusPolicy(Qt::NoFocus);
    pbStopSimulation->setFocusPolicy(Qt::NoFocus);

    // Метки времени
    QLabel* dateTimeLabel = new QLabel("Дата и время:", spreadSimulationWidget);
    QLabel* currentTimeLabel = new QLabel("--.--.---- --:--:--", spreadSimulationWidget);
    currentTimeLabel->setTextInteractionFlags(Qt::TextSelectableByMouse);
    currentTimeLabel->setAlignment(Qt::AlignCenter);

    // Слайдер
    simulationSpeedSlider = new advancedSlider(Qt::Horizontal, spreadSimulationWidget);
    simulationSpeedSlider->setRange(0,72);
    simulationSpeedSlider->setValue(0);
    simulationSpeedSlider->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
    simulationSpeedSlider->setMinimumHeight(100);
    simulationSpeedSlider->setFocusPolicy(Qt::NoFocus);
    simulationSpeedSlider->setEnabled(false);

    // Компоновка
    QHBoxLayout* buttonLayout = new QHBoxLayout();
    buttonLayout->addWidget(pbStartSimulation);
    buttonLayout->addWidget(pbStopSimulation);

    dateTimeLabel->setAlignment(Qt::AlignHCenter);

    QVBoxLayout* leftLayout = new QVBoxLayout();
    leftLayout->addLayout(buttonLayout);
    leftLayout->addWidget(dateTimeLabel);
    leftLayout->addWidget(currentTimeLabel);
    leftLayout->addStretch();

    QVBoxLayout* sliderWrapper = new QVBoxLayout();
    sliderWrapper->addStretch();
    sliderWrapper->addWidget(simulationSpeedSlider);
    sliderWrapper->addStretch();

    QHBoxLayout* mainLayout = new QHBoxLayout(spreadSimulationWidget);
    mainLayout->addLayout(leftLayout);
    mainLayout->addLayout(sliderWrapper);

    spreadSimulationWidget->setLayout(mainLayout);
    spreadSimulationPanel->setWidget(spreadSimulationWidget);
    addDockWidget(Qt::BottomDockWidgetArea, spreadSimulationPanel);

    QTimer* simulationTimer = new QTimer(this);

    auto filterByHour = [this](int hour)
    {
        QList<QgsLayerTreeNode*> selectedNodes = mLayerTreeView->selectedNodes();
        if (selectedNodes.isEmpty())
        {
            return;
        }

        QgsLayerTreeLayer* treeLayer = dynamic_cast<QgsLayerTreeLayer*>(selectedNodes.first());
        if (!treeLayer)
        {
            return;
        }

        QgsVectorLayer* activeLayer = qobject_cast<QgsVectorLayer*>(treeLayer->layer());
        if (activeLayer && activeLayer->geometryType() == Qgis::GeometryType::Point)
        {
            activeLayer->setSubsetString(QString("hour = %1").arg(hour));
            activeLayer->triggerRepaint();
        }
    };

    // Обновление метки времени при движении слайдера
    connect(simulationSpeedSlider->slider(), &QSlider::valueChanged, this, [=](int value)
    {
        QDateTime newTime = baseTime.addSecs(value * 3600);
        currentTimeLabel->setText(newTime.toString("dd.MM.yyyy HH:mm:ss"));

        // Теперь слой фильтруется всегда, когда меняется значение слайдера
        filterByHour(value);
    });

    // Старт
    connect(pbStartSimulation, &QPushButton::clicked, this, [=]() mutable
    {
        // Получаем активный слой только при старте
        QList<QgsLayerTreeNode*> selectedNodes = mLayerTreeView->selectedNodes();
        if(selectedNodes.isEmpty())
        {
            QMessageBox::warning(this, tr("Ошибка"), tr("Выберите точечный слой в дереве слоев для начала симуляции"));
            return;
        }

        QgsLayerTreeLayer* treeLayer = dynamic_cast<QgsLayerTreeLayer*>(selectedNodes.first());
        if(!treeLayer) return;

        QgsVectorLayer* activeLayer = qobject_cast<QgsVectorLayer*>(treeLayer->layer());
        if(!activeLayer || activeLayer->geometryType() != Qgis::GeometryType::Point)
        {
            return;
        }

        // Таймер увеличивает слайдер и фильтрует слой
        connect(simulationTimer, &QTimer::timeout, [=]() mutable
        {
            int currentValue = simulationSpeedSlider->value();
            int maxValue = simulationSpeedSlider->slider()->maximum();

            if(currentValue < maxValue)
            {
                currentValue++;
                simulationSpeedSlider->setValue(currentValue);
                filterByHour(currentValue);
            }
            else
            {
                simulationTimer->stop();
                pbStartSimulation->setDisabled(false);
            }
        });

        simulationTimer->start(1000); // 1 секунда = +1 час
        pbStartSimulation->setDisabled(true);
    });

    // Стоп
    connect(pbStopSimulation, &QPushButton::clicked, this, [=]()
    {
        simulationTimer->stop();
        pbStartSimulation->setDisabled(false);
    });
}

// Функция создания панели с основными кнопками программы
void MainWindow::createMainFunctionsPanel(QWidget *parent)
{
    QDockWidget* mainFunctionsPanel = new QDockWidget("Панель обработки", parent);
    mainFunctionsPanel->setAllowedAreas(Qt::LeftDockWidgetArea | Qt::RightDockWidgetArea);
    QWidget* mainFunctionsWidget = new QWidget();

    pbDownloadSentinel = new QPushButton("Скачать спутниковый снимок", mainFunctionsWidget);
    pbStartAnalysis = new QPushButton("Прогноз распространения", mainFunctionsWidget);
    pbFindOilSpill = new QPushButton("Найти нефтяное пятно на снимке", mainFunctionsWidget);
    pbCreatePolygon = new QPushButton("Оцифровать нефтяное пятно вручную", mainFunctionsWidget);
    pbCreateReport = new QPushButton("Создать отчет", mainFunctionsWidget);
    pbAddSpillAnalysis = new QPushButton("Добавить слой распространения пятна", mainFunctionsWidget);

    QFrame* labelsFrame = new QFrame(mainFunctionsWidget);
    labelsFrame->setFrameShape(QFrame::StyledPanel);
    labelsFrame->setFrameShadow(QFrame::Sunken);
    labelsFrame->setObjectName("infoFrame");
    labelsFrame->setStyleSheet(
                "QFrame#infoFrame {"
                "  border: 1px solid #9ACEEB;"
                "  border-radius: 4px;"
                "  background-color: #ffffff;"
                "}"
                "QLabel {"
                "  border: none;"
                "  background-color: transparent;"
                "}"
                );    QVBoxLayout* frameLayout = new QVBoxLayout(labelsFrame);
    frameLayout->setContentsMargins(5, 5, 5, 5);

    QLabel* coordinateSystemLabel = new QLabel("Система координат EPSG:3857\nТекущий масштаб:", mainFunctionsWidget);
    coordinateSystemLabel->setAlignment(Qt::AlignCenter);
    mLabelMeasurment = new QLabel("Поле вывода измерений");
    mLabelMeasurment->setAlignment(Qt::AlignCenter);
    mLabelMeasurment->setTextInteractionFlags(Qt::TextSelectableByMouse);

    scaleCombo = new QgsScaleComboBox(bottomRightOverlay);
    scaleCombo->setScale(mMapCanvas->scale());
    scaleCombo->setMinimumWidth(10);
    scaleCombo->setStyleSheet(
                "QComboBox {"
                "  background-color: white;"
                "}"
                "QComboBox QAbstractItemView {"
                "  border-radius: 6px;"
                "  background-color: white;"
                "  selection-background-color: #C8C8C8;"
                "}"
                );

    frameLayout->addWidget(coordinateSystemLabel);
    frameLayout->addWidget(scaleCombo);
    frameLayout->addStretch();  // Добавляем отступ сверху
    frameLayout->addWidget(mLabelMeasurment);
    frameLayout->addStretch();  // Добавляем отступ снизу


    QVBoxLayout* vLayout = new QVBoxLayout(mainFunctionsWidget);
    vLayout->addWidget(labelsFrame);

    vLayout->addWidget(pbDownloadSentinel);
    vLayout->addWidget(pbCreatePolygon);
    vLayout->addWidget(pbFindOilSpill);
    vLayout->addWidget(pbStartAnalysis);
    vLayout->addWidget(pbAddSpillAnalysis);
    vLayout->addWidget(pbCreateReport);
    vLayout->addStretch();

    mainFunctionsPanel->setWidget(mainFunctionsWidget);
    addDockWidget(Qt::LeftDockWidgetArea, mainFunctionsPanel);
}

// Функция создания панели с кнопка управления данными
void MainWindow::createControlPanel(QWidget *parent)
{
    QDockWidget* myPanel = new QDockWidget("Панель управления данными", parent);
    QWidget* content = new QWidget();
    QVBoxLayout* vLayout = new QVBoxLayout(content);

    QHBoxLayout* hLayout = new QHBoxLayout();
    pbSelect = new QPushButton("Выбрать объекты", content);
    pbClearSelection = new QPushButton("Очистить выделение", content);
    pbAddNewObject = new QPushButton("Добавить объект в слой", content);
    pbStartEdit = new QPushButton("Начать редактирование", content);
    pbStopEdit = new QPushButton("Завершить редактирование", content);

    hLayout->addWidget(pbSelect);
    hLayout->addWidget(pbClearSelection);
    hLayout->addWidget(pbAddNewObject);
    hLayout->addWidget(pbStartEdit);
    hLayout->addWidget(pbStopEdit);

    vLayout->addLayout(hLayout);
    content->setLayout(vLayout);
    myPanel->setWidget(content);
    addDockWidget(Qt::TopDockWidgetArea, myPanel);
}

// Создание коннектов между функциями и кнопками
void MainWindow::setupConnections()
{    
    // Коннекты дерева слоев
    connect(mLayerTreeView, &QgsLayerTreeView::clicked, this, &MainWindow::onClicked);
    connect(mLayerTreeView, &QgsLayerTreeView::doubleClicked, this, &MainWindow::onDoubleClicked);

    // Коннекты холста
    connect(mMapCanvas, &QgsMapCanvas::xyCoordinates, this, &MainWindow::handleMouseMoved);
    connect(mMapCanvas, &QgsMapCanvas::xyCoordinates, this, &MainWindow::onMouseMoved);

    // Коннекты кнопок
    connect(ui->action_addVectorLayer, &QAction::triggered, this, &MainWindow::addVectorLayers);
    connect(ui->action_addRasterLayer, &QAction::triggered, this, &MainWindow::addRasterLayers);
    connect(ui->action_WMS, &QAction::triggered, this, &MainWindow::processWMS);
    connect(ui->action_WFS, &QAction::triggered, this, &MainWindow::processWFS);
    connect(ui->action_WMTS, &QAction::triggered, this, &MainWindow::processWMTS);
    connect(ui->action_deleteSelectedLayers, &QAction::triggered, this, &MainWindow::deleteLayers);
    connect(ui->action_makeScreenshot, &QAction::triggered, this, &MainWindow::makeScreenshot);
    connect(ui->action_setPathSnap, &QAction::triggered, this, &MainWindow::setPathSnap);
    connect(ui->action_setRoboflowAPI, &QAction::triggered, this, &MainWindow::setRoboflowApiKey);
    connect(ui->action_setCopernicusPassword, &QAction::triggered, this, &MainWindow::setCopernicusPassword);

    connect(pbAddSpillAnalysis, &QPushButton::clicked, this, &MainWindow::addSpillAnalysis);
    connect(pbCreateReport, &QPushButton::clicked, this, &MainWindow::createReport);
    connect(pbDownloadSentinel, &QPushButton::clicked, this, &MainWindow::onDownloadSentinelClicked);
    connect(pbClearSelection, &QPushButton::clicked, this, &MainWindow::onClearSelectionClicked);
    connect(pbFindOilSpill, &QPushButton::clicked, this, &MainWindow::onFindOilSpillClicked);
    connect(pbStartAnalysis, &QPushButton::clicked, this, &MainWindow::onStartAnalysisClicked);
    connect(pbStopEdit, &QPushButton::clicked, this, &MainWindow::onStopEditClicked);
    connect(pbStartEdit, &QPushButton::clicked, this, &MainWindow::onStartEditClicked);
    connect(pbSelect, &QPushButton::clicked, this, &MainWindow::selectionMode);
    connect(pbAddNewObject, &QPushButton::clicked, this, &MainWindow::addNewObjectToLayer);
    connect(pbCreatePolygon, &QPushButton::clicked, this, &MainWindow::onCreatePolygonClicked);

    // Коннекты инструментов измерения
    connect(actDistance, &QAction::triggered, this, &MainWindow::measurment);
    connect(actArea, &QAction::triggered, this, &MainWindow::measureArea);
    connect(actAngle, &QAction::triggered, this, &MainWindow::measureAngle);
    connect(pbMeasurment, &QToolButton::clicked, this, &MainWindow::measurment);

    // Коннекты обновления комбобокса с масштабом
    if (scaleCombo)
    {
        connect(scaleCombo, &QgsScaleComboBox::scaleChanged, mMapCanvas, [this](double scale){ mMapCanvas->zoomScale(scale); });
        connect(mMapCanvas, &QgsMapCanvas::extentsChanged, scaleCombo, [this](){ scaleCombo->setScale(mMapCanvas->scale()); });
    }
}

// Функция инициализации настроек карты (СК и обновление погоды на карте)
void MainWindow::initializeMapSettings()
{
    mSourceCrs = QgsCoordinateReferenceSystem("EPSG:3857");
    mMapCanvas->setDestinationCrs(mSourceCrs);

    networkManager = new QNetworkAccessManager(this);
    weatherNetworkManager = new QNetworkAccessManager(this);
    weatherUpdateTimer = new QTimer(this);
    weatherUpdateTimer->setSingleShot(true);
    connect(weatherUpdateTimer, &QTimer::timeout, this, [this](){ getWeatherData(pendingWeatherPoint); });
}

// Функция добавления базовых слоев на карту
void MainWindow::addBasicLayers()
{
    QString dataPath = QStringLiteral(SRCDIR) + "Data/layers/";

    // Создаем группу для слоев
    QgsLayerTreeGroup *basicLayersGroup = new QgsLayerTreeGroup("Базовые слои");
    QgsLayerTree *root = QgsProject::instance()->layerTreeRoot();
    root->addChildNode(basicLayersGroup);

    // Создаем и добавляем векторный слой береговой линии
    QgsVectorLayer* vectorLayer = new QgsVectorLayer(dataPath + "Coastline.gpkg", "Береговая линия", "ogr");
    if (vectorLayer->isValid())
    {
        vectorLayer->setCrs(mSourceCrs);
        QgsProject::instance()->addMapLayer(vectorLayer, false);
        basicLayersGroup->addLayer(vectorLayer);

        // Создаём символ по умолчанию для линий
        QgsSymbol* symbol = QgsSymbol::defaultSymbol(Qgis::GeometryType::Line);
        if (symbol)
        {
            QgsLineSymbol* lineSymbol = dynamic_cast<QgsLineSymbol*>(symbol);
            if (lineSymbol)
            {
                lineSymbol->setColor(QColor(0x00BFFF));
                lineSymbol->setWidth(1.0); // Устанавливаем толщину линии
            }

            vectorLayer->setRenderer(new QgsSingleSymbolRenderer(symbol));
        }
    }
    else
    {
        qDebug() << "Не удалось загрузить слой:" << dataPath + "Coastline.gpkg";
        delete vectorLayer;
    }
    // Создаем и добавляем векторный слой суши
    QgsVectorLayer* vectorLayerLand = new QgsVectorLayer(dataPath + "Land.gpkg", "Суша", "ogr");
    if (vectorLayerLand->isValid())
    {
        vectorLayerLand->setCrs(mSourceCrs);
        QgsProject::instance()->addMapLayer(vectorLayerLand, false);
        basicLayersGroup->addLayer(vectorLayerLand);

        // Создаём явно простой символ заливки
        QgsSimpleFillSymbolLayer* simpleFill = new QgsSimpleFillSymbolLayer();
        simpleFill->setColor(Qt::white);
        simpleFill->setStrokeColor(QColor(0x00BFFF));      // цвет границы
        simpleFill->setStrokeWidth(1.0);                   // толщина границы

        QgsSymbol* symbol = QgsSymbol::defaultSymbol(Qgis::GeometryType::Polygon);
        symbol->changeSymbolLayer(0, simpleFill);

        vectorLayerLand->setRenderer(new QgsSingleSymbolRenderer(symbol));
    }
    else
    {
        qDebug() << "Не удалось загрузить слой:" << dataPath + "Land.gpkg";
        delete vectorLayerLand;
    }

    // Создаем и добавляем растровый слой
    QgsRasterLayer* rasterLayer = new QgsRasterLayer(dataPath + "gebco_2021_cog.tif", "Батиметрия", "gdal");
    if (rasterLayer->isValid()) {
        rasterLayer->setCrs(mSourceCrs);
        QgsProject::instance()->addMapLayer(rasterLayer, false);
        basicLayersGroup->addLayer(rasterLayer);
    }
    else
    {
        qDebug() << "Не удалось загрузить слой:" << dataPath + "gebco_2021_cog.tif";
        delete rasterLayer;
    }
}

// Функция нажатия кнопки "Задать путь к SNAP"
void MainWindow::setPathSnap()
{
    // Проверяем, если путь к SNAP уже установлен автоматически
    if (!SnapPath.isEmpty())
    {
        // Проверяем, что путь все еще действителен
        QString gptExecutable = SnapPath + "/gpt";
#ifdef Q_OS_WIN
        gptExecutable += ".exe";
#endif
        
        QFileInfo gptFile(gptExecutable);
        if (gptFile.exists() && gptFile.isExecutable())
        {
            QMessageBox::information(this, tr("Путь к SNAP"), tr("Путь к SNAP уже найден автоматически:\n%1\n\nИнструменты для обработки SAR-снимков будут использованы из найденной директории.").arg(SnapPath));
            return;
        }
    }

    // Пытаемся найти путь к инструментам программы SNAP для обработки радиолокационных снимков
    QString defaultPath = QDir::homePath() + "/esa-snap/bin";
    QString gptExecutable = defaultPath + "/gpt";

    // Если пользователь работает под Windows
#ifdef Q_OS_WIN
    gptExecutable += ".exe";
#endif

    QFileInfo gptFile(gptExecutable);
    if (gptFile.exists() && gptFile.isExecutable())
    {
        // Автоматически нашли SNAP — сохраняем и выходим
        SnapPath = defaultPath;
        qDebug() << "SNAP обнаружен автоматически:" << SnapPath;
        return;
    }

    // Не найден - запрашиваем у пользователя
    bool ok = false;
    QString userInput = QInputDialog::getText(this, tr("Путь к SNAP"), tr("Введите путь к папке с приложением SNAP (например, ~/esa-snap/bin):"), QLineEdit::Normal, SnapPath, &ok);

    if (!ok || userInput.trimmed().isEmpty())
    {
        qDebug() << "Путь к SNAP не задан.";
        SnapPath.clear();
        return;
    }

    QString candidatePath = QDir::cleanPath(userInput.trimmed());
    QString candidateGpt = candidatePath + "/gpt";

#ifdef Q_OS_WIN
    candidateGpt += ".exe";
#endif

    QFileInfo candidateGptFile(candidateGpt);
    if (candidateGptFile.exists() && candidateGptFile.isExecutable())
    {
        SnapPath = candidatePath;
        qDebug() << "SNAP путь задан вручную и подтверждён:" << SnapPath;
    }
    else
    {
        QMessageBox::warning(this, tr("Ошибка"), tr("Файл gpt не найден по указанному пути:\n%1").arg(candidateGpt));
        SnapPath.clear();
    }
}

void MainWindow::initOilWorker()
{
    m_workerThread = new QThread(this);
    m_oilWorker = new SarProcess();
    m_oilWorker->moveToThread(m_workerThread);

    // Настройки
    m_oilWorker->setSnapPath(SnapPath);
    m_oilWorker->setRoboflowApiKey(roboflowApiKey);
    m_oilWorker->setSourceDir(QStringLiteral(SRCDIR));
    m_oilWorker->setTargetCrs("EPSG:3857");

    // Соединение результата работы класса обработчка SAR в отдельном потоке и mainwindow
    connect(m_oilWorker, &SarProcess::finished, this, &MainWindow::onWorkerFinished, Qt::QueuedConnection);

    m_workerThread->start();
}

// Инициализация дерева со слоями
void MainWindow::initLayerTreeView()
{
    mLayerTreeDock = new QgsDockWidget(tr("Слои"), this);
    mLayerTreeDock->setObjectName(QStringLiteral("Слои"));
    mLayerTreeDock->setAllowedAreas(Qt::LeftDockWidgetArea | Qt::RightDockWidgetArea);

    QgsLayerTreeModel *model = new QgsLayerTreeModel(QgsProject::instance()->layerTreeRoot(), this);

    model->setFlag(QgsLayerTreeModel::AllowNodeReorder);
    model->setFlag(QgsLayerTreeModel::AllowNodeRename);
    model->setFlag(QgsLayerTreeModel::AllowNodeChangeVisibility);
    model->setFlag(QgsLayerTreeModel::ShowLegendAsTree);

    model->setAutoCollapseLegendNodes(10);

    mLayerTreeView->setModel(model);

    QVBoxLayout *vboxLayout = new QVBoxLayout;
    vboxLayout->setMargin(0);
    vboxLayout->setContentsMargins(0, 0, 0, 0);
    vboxLayout->setSpacing(0);
    vboxLayout->addWidget(mLayerTreeView);

    QWidget *w = new QWidget;
    w->setLayout(vboxLayout);
    mLayerTreeDock->setWidget(w);
    addDockWidget(Qt::RightDockWidgetArea, mLayerTreeDock);

    mLayerTreeCanvasBridge = new QgsLayerTreeMapCanvasBridge(QgsProject::instance()->layerTreeRoot(), mMapCanvas, this);
}

// Инициализация и добавление карты
void MainWindow::createOverview()
{
    mOverviewCanvas = new QgsMapOverviewCanvas(nullptr, mMapCanvas);
    mOverviewCanvas->setBackgroundColor(QColor(Qt::white));

    QCursor mOverviewMapCursor(Qt::OpenHandCursor);
    mOverviewCanvas->setCursor(mOverviewMapCursor);
    mOverviewDock = new QgsDockWidget(tr("Обзор"), this);

    mOverviewDock->setObjectName(QStringLiteral("Обзор"));
    mOverviewDock->setAllowedAreas(Qt::LeftDockWidgetArea | Qt::RightDockWidgetArea);
    mOverviewDock->setWidget(mOverviewCanvas);
    addDockWidget(Qt::RightDockWidgetArea, mOverviewDock);

    mLayerTreeCanvasBridge->setOverviewCanvas( mOverviewCanvas );

    mOverviewDock->hide();
}

/// ------------------------------

/// ФУНКЦИИ ВЗАИМОДЕЙСТВИЯ С КАРТОЙ (MAPCANVAS)

void MainWindow::lockMapCanvas()
{
    if (!mMapCanvas)
    {
        return;
    }

    // Запоминаем текущий экстент
    m_lockedExtent = mMapCanvas->extent();

    // Отключаем обновление и ввод
    mMapCanvas->setRenderFlag(false);
    mMapCanvas->setEnabled(false);
}

void MainWindow::unlockMapCanvas()
{
    if (!mMapCanvas)
    {
        return;
    }

    mMapCanvas->setEnabled(true);
    mMapCanvas->setRenderFlag(true);

    // Восстанавливаем прежний вид
    mMapCanvas->setExtent(m_lockedExtent);
    mMapCanvas->refresh();
}

// Функция обработки событий (для перемещения лейбла с координатами в правый нижний угол текущего положения карты)
bool MainWindow::eventFilter(QObject* obj, QEvent* event)
{
    if(obj == mMapCanvas && event->type() == QEvent::Resize)
    {
        bottomRightOverlay->move(mMapCanvas->width() - bottomRightOverlay->width(), mMapCanvas->height() - bottomRightOverlay->height());
    }

    return QMainWindow::eventFilter(obj, event);
}

// Функция обработки движения мыши по карте
void MainWindow::onMouseMoved(const QgsPointXY &point)
{
    QString text = handleMouseMoved(point);
    coordinatesLabel->setText(text);
}

// Функция сброса флага отслеживания координат
void MainWindow::clearCoordinatesTracking()
{
    coordinateTracking = 0;
}

// Функция установки флага отслеживания координат
void MainWindow::setCoordinatesTracking()
{
    coordinateTracking = !coordinateTracking;

    QgsMapToolEmitPoint* clickTool = new QgsMapToolEmitPoint(mMapCanvas);
    connect(clickTool, &QgsMapToolEmitPoint::canvasClicked, this, &MainWindow::onMapClicked);
    mMapCanvas->setMapTool(clickTool);
}

// Функция обработки нажатия на карту
void MainWindow::onMapClicked(const QgsPointXY& point)
{
    if(coordinateTracking)
    {
        QString result = handleMouseMoved(point);
        qDebug() << "Клик на карте:" << result << " , " << point.x() << point.y();
    }
}

// Обработка движения мыши
QString MainWindow::handleMouseMoved(const QgsPointXY& point)
{
    QString strCoord;
    int x = point.x();
    int y = point.y();

    strCoord = "Координаты: " + QString::number(x) + "м & " + QString::number(y) + "м";

    if (!mSourceCrs.isValid())
    {
        strCoord.append(" (СК не задана)");
        return strCoord;
    }

    // Добавляем данные о ветре, если они уже загружены
    if (!currentWindData.isEmpty())
    {
        strCoord += currentWindData;
    }

    // Запрашиваем данные о погоде с задержкой (чтобы не делать запрос при каждом движении мыши)
    // Останавливаем предыдущий таймер, если он был запущен
    if (weatherUpdateTimer->isActive())
    {
        weatherUpdateTimer->stop();
    }

    // Сохраняем текущую точку для использования в слоте таймера
    pendingWeatherPoint = point;

    // Запускаем таймер на 500 мс - запрос будет выполнен только если мышь не двигалась
    weatherUpdateTimer->setInterval(500);
    weatherUpdateTimer->start();

    return strCoord;

}

// Функция получения информации о погоде в определенной точке карты
void MainWindow::getWeatherData(const QgsPointXY& point)
{
    // Проверяем, нужно ли делать новый запрос (если точка изменилась значительно)
    // Используем расстояние в метрах для определения необходимости нового запроса
    if (lastWeatherPoint.x() != 0 || lastWeatherPoint.y() != 0)
    {
        double distance = std::sqrt(std::pow(point.x() - lastWeatherPoint.x(), 2) + std::pow(point.y() - lastWeatherPoint.y(), 2));
        
        // Если точка изменилась менее чем на 1000 метров, используем кэшированные данные
        if (distance < 1000.0 && !currentWindData.isEmpty())
        {
            return;
        }
    }

    // Преобразуем координаты из проекции карты в WGS84 (широта/долгота)
    if (!mSourceCrs.isValid())
    {
        return;
    }

    QgsCoordinateReferenceSystem wgs84 = QgsCoordinateReferenceSystem::fromEpsgId(4326);
    QgsCoordinateTransform transform(mSourceCrs, wgs84, QgsProject::instance());

    QgsPointXY wgs84Point;
    try
    {
        wgs84Point = transform.transform(point);
    }
    catch (const QgsCsException &e)
    {
        qDebug() << "Ошибка преобразования координат:" << e.what();
        return;
    }

    double latitude = wgs84Point.y();
    double longitude = wgs84Point.x();

    // Сохраняем текущую точку
    lastWeatherPoint = point;

    // Формируем URL для запроса данных о ветре
    QString url = QString("https://api.open-meteo.com/v1/forecast?latitude=%1&longitude=%2&current=wind_speed_10m,wind_direction_10m&timezone=auto").arg(latitude, 0, 'f', 6).arg(longitude, 0, 'f', 6);

    QNetworkRequest request((QUrl(url)));
    request.setRawHeader("User-Agent", "OilSpillMonitor/1.0");

    // Отправляем GET-запрос
    QNetworkReply* reply = weatherNetworkManager->get(request);

    // Обрабатываем ответ
    connect(reply, &QNetworkReply::finished, this, [this, reply]()
    {
        processWeatherResponse(reply);
        reply->deleteLater();
    });
}

/// ------------------------------

/// СОЗДАНИЕ ОТЧЕТА

// Извлечение даты/времени из имени файла спутникового снимка Sentinel
QDateTime MainWindow::extractSentinelDateTimeFromFileName(const QString &fileName)
{
    // Формат 1: S1A_IW_GRDH_1SDV_20250820T112323_20250820T112352_060618_078A7E_124D
    // Формат 2: Любая строка содержащая YYYYMMDDTHHMMSS
    static const QRegularExpression regex(R"((\d{8})T(\d{6}))");
    QRegularExpressionMatch match = regex.match(fileName);

    if (match.hasMatch())
    {
        QString dateStr = match.captured(1); // 20250820
        QString timeStr = match.captured(2); // 112323

        QDateTime dateTime = QDateTime::fromString(dateStr + timeStr, "yyyyMMddHHmmss");
        dateTime.setTimeSpec(Qt::UTC); // Устанавливаем UTC

        qDebug() << "Извлечено время из файла:" << dateTime.toString(Qt::ISODate);
        qDebug() << "Исходное имя файла:" << fileName;
        return dateTime;
    }

    qDebug() << "Не удалось извлечь время из файла:" << fileName;
    return QDateTime(); // Пустое время если не найдено
}

// Извлечение ID снимка из имени файла
static QString extractSentinelIdFromPath(const QString &path)
{
    QFileInfo fi(path);
    QString baseName = fi.completeBaseName();

    if (baseName.startsWith("S1"))
    {
        QStringList parts = baseName.split('_');
        // Для Sentinel-1 стандартный ID — это первые 9 частей
        if (baseName.startsWith("S1") && parts.size() >= 9)
        {
            return QStringList(parts.mid(0, 9)).join('_');
        }
        return baseName;
    }

    return QString();
}

// Функция создания отчета
void MainWindow::createReport()
{
    QgsVectorLayer *polyLayer = nullptr;     // Полигональный слой нефтяного пятна
    QString layerSourcePath;                 // Путь к файлу слоя
    bool ownsLayer = false;

    // Пытаемся получить выбранный слой из дерева слоев
    QList<QgsLayerTreeNode *> selectedNodes = mLayerTreeView ? mLayerTreeView->selectedNodes() : QList<QgsLayerTreeNode *>();
    if (!selectedNodes.isEmpty())
    {
        QgsLayerTreeLayer *treeLayer = dynamic_cast<QgsLayerTreeLayer *>(selectedNodes.first());
        if (treeLayer)
        {
            QgsMapLayer *layer = treeLayer->layer();
            polyLayer = qobject_cast<QgsVectorLayer *>(layer);
            // Проверяем, что слой векторный и полигональный
            if (polyLayer && polyLayer->geometryType() == Qgis::GeometryType::Polygon)
            {
                layerSourcePath = polyLayer->source().split("|").first(); // Получаем путь к источнику данных
            }
        }
    }

    // Если подходящий слой не найден в дереве слоев, предлагаем выбрать файл
    if (!polyLayer)
    {
        // Диалог выбора файла GeoJSON
        QString filename = QFileDialog::getOpenFileName(this, tr("Выбрать полигональный слой"), QDir::homePath(), tr("GeoJSON (*.geojson);;Все файлы (*)"));
        if (filename.isEmpty())
        {
            return; // Пользователь отменил выбор файла
        }

        // Создаем новый векторный слой из файла
        polyLayer = new QgsVectorLayer(filename, QFileInfo(filename).baseName(), "ogr");
        ownsLayer = true; // Мы создали слой, поэтому должны удалить его после использования
        
        // Проверяем валидность слоя
        if (!polyLayer->isValid())
        {
            QMessageBox::warning(this, tr("Ошибка"), tr("Не удалось загрузить слой: %1").arg(filename));
            delete polyLayer;
            return;
        }
        
        // Проверяем, что слой полигональный
        if (polyLayer->geometryType() != Qgis::GeometryType::Polygon)
        {
            QMessageBox::warning(this, tr("Ошибка"), tr("Выбранный слой должен быть полигональным."));
            delete polyLayer;
            return;
        }
        layerSourcePath = filename;
    }

    // Расчет геометрических параметров слоя (площадь, периметр, координаты)
    SpotDetails geometry = calculateGeometryMetrics(polyLayer);
    double areaVal = geometry.area;           // Площадь в квадратных метрах
    double perimeterVal = geometry.perimeter;  // Периметр в метрах
    QString extentStr = geometry.coordinates;  // Координаты границ
    QString centerStr = geometry.centerCoordinates; // Координаты центра

    // Извлечение ID спутникового снимка из пути к файлу
    QString imageId = extractSentinelIdFromPath(layerSourcePath);
    // Попытка получить временную метку из имени файла
    QDateTime timestamp = extractSentinelDateTimeFromFileName(layerSourcePath);
    // Если не удалось получить из ID, используем время данных изображения
    if (!timestamp.isValid() && imageDataTime.isValid())
    {
        timestamp = imageDataTime;
    }
    // Если и это не сработало, используем текущее время
    if (!timestamp.isValid())
    {
        timestamp = QDateTime::currentDateTime();
    }

    // Поиск изображений в директории слоя
    QDir layerDir(QFileInfo(layerSourcePath).absolutePath());
    
    // Поиск спутникового изображения (VV канал)
    QStringList vvFiles = layerDir.entryList(QStringList() << "*vv*.png" << "*vv*.jpg" << "*VV*.png", QDir::Files);
    // Поиск RGB композитного изображения
    QStringList rgbFiles = layerDir.entryList(QStringList() << "*rgb*.png" << "*RGB*.png", QDir::Files);
    
    // Поиск изображений прогноза распространения разлива (spill_0, spill_24, spill_48, spill_72)
    QStringList spillFiles;
    QStringList spillPatterns = {"spill_0.png", "spill_24.png", "spill_48.png", "spill_72.png"};
    for (const QString& pattern : spillPatterns)
    {
        QStringList found = layerDir.entryList(QStringList() << pattern, QDir::Files);
        if (!found.isEmpty())
        {
            spillFiles << layerDir.absoluteFilePath(found.first());
        }
        else
        {
            spillFiles << QString(); // Пустая строка, если файл не найден
        }
    }

    // Создание и настройка диалога опций отчета
    ReportOptionsDialog dialog(this);
    dialog.setImageId(imageId);                                    // ID снимка
    dialog.setCoordinates(extentStr, centerStr);                    // Координаты границ и центра
    dialog.setPerimeter(perimeterVal);                             // Периметр
    dialog.setArea(areaVal);                                       // Площадь
    dialog.setTimestamp(timestamp);                                 // Временная метка
    
    // Установка путей к изображениям, если они найдены
    if (!vvFiles.isEmpty())
    {
        dialog.setSentinelImageOriginalPath(layerDir.absoluteFilePath(vvFiles.first()));
    }
    if (!rgbFiles.isEmpty())
    {
        dialog.setSentinelImagePolyPath(layerDir.absoluteFilePath(rgbFiles.first()));
    }
    if (!lastDetectionOverlayImagePath.isEmpty())
    {
        dialog.setDetectionOverlayPath(lastDetectionOverlayImagePath);
    }
    
    // Установка изображений прогноза разлива
    dialog.setSpillForecastImages(spillFiles);

    // Освобождение памяти, если слой был создан внутри функции
    if (ownsLayer)
    {
        delete polyLayer;
    }

    // Показываем диалоговое окно и ждем действия пользователя
    if (dialog.exec() != QDialog::Accepted)
    {
        return; // Пользователь отменил создание отчета
    }

    // Получаем путь для сохранения PDF файла
    QString outPath = dialog.outputFilePath();
    if (outPath.isEmpty())
    {
        // Если путь не задан, показываем диалог сохранения файла
        outPath = QFileDialog::getSaveFileName(this, tr("Сохранить отчёт"), QDir::homePath(), tr("PDF (*.pdf)"));
        if (outPath.isEmpty())
        {
            return; // Пользователь отменил сохранение
        }
    }

    // Создание структуры данных для генерации отчета
    ReportData reportData;
    reportData.outputFilePath = outPath;                          // Путь к выходному файлу
    reportData.appName = "OilSpillMonitor";                       // Название приложения
    reportData.logoImagePath = ":/res/application-icon.png";      // Путь к логотипу

    // Настройка флагов включения различных секций отчета
    reportData.includeFlags.includeImageId = dialog.isImageIdEnabled();
    reportData.includeFlags.includeCoordinates = dialog.isCoordinatesEnabled();
    reportData.includeFlags.includePerimeter = dialog.isPerimeterEnabled();
    reportData.includeFlags.includeArea = dialog.isAreaEnabled();
    reportData.includeFlags.includeTimestamp = dialog.isTimestampEnabled();
    reportData.includeFlags.includePhotos = dialog.isPhotosEnabled();
    reportData.includeFlags.includeSpillScreenshots = dialog.isSpillForecastEnabled();
    reportData.includeFlags.includeDetectionOverlay = dialog.isDetectionOverlayEnabled();

    // Установка путей к изображениям
    reportData.sentinelImageOriginalPath = dialog.sentinelImageOriginalPath();
    reportData.sentinelImagePolyPath = dialog.sentinelImagePolyPath();
    reportData.detectionOverlayImagePath = dialog.detectionOverlayPath();
    reportData.spillForecastImages = dialog.spillForecastImages();

    // Заполнение детальной информации о пятне
    reportData.details.imageId = dialog.imageId();
    reportData.details.coordinates = dialog.extentText();
    // Добавление координат центра, если они включены в отчет
    if (dialog.isCoordinatesEnabled() && !dialog.centerText().isEmpty())
    {
        reportData.details.coordinates += "; " + tr("Центр: ") + dialog.centerText();
    }
    reportData.details.perimeter = dialog.perimeter();
    reportData.details.area = dialog.area();
    reportData.details.timestamp = dialog.timestamp();

    // Создание генератора отчета и его запуск
    ReportGenerator generator(reportData);
    if (generator.generate())
    {
        // Успешное создание отчета
        QMessageBox::information(this, tr("Успешно"), tr("Отчет успешно создан: %1").arg(reportData.outputFilePath));
        qDebug() << "Отчет успешно создан:" << reportData.outputFilePath;
    }
    else
    {
        // Ошибка при создании отчета
        QMessageBox::warning(this, tr("Предупреждение"), tr("Не удалось создать отчет"));
        qDebug() << "Ошибка создания отчета.";
    }
}

// Расчет геометрических параметров слоя
SpotDetails MainWindow::calculateGeometryMetrics(QgsVectorLayer* layer)
{
    SpotDetails details;
    
    if (!layer || !layer->isValid())
    {
        return details;
    }
    QgsGeometry unionGeom;
    QgsFeature feat;
    QgsFeatureIterator it = layer->getFeatures();
    
    while (it.nextFeature(feat))
    {
        QgsGeometry geom = feat.geometry();
        if (unionGeom.isNull())
        {
            unionGeom = geom;
        }
        else
        {
            unionGeom = unionGeom.combine(geom);
        }
    }

    if (unionGeom.isNull() || unionGeom.isEmpty())
    {
        return details;
    }


    QgsDistanceArea da;
    da.setSourceCrs(layer->crs(), QgsProject::instance()->transformContext());
    da.setEllipsoid(QgsProject::instance()->ellipsoid());

    details.area = da.measureArea(unionGeom);

    QgsGeometry boundary;
    const QgsAbstractGeometry *absGeom = unionGeom.constGet();

    if (absGeom)
    {
        QgsAbstractGeometry *boundaryAbs = absGeom->boundary();
        if (boundaryAbs)
        {
            boundary = QgsGeometry(boundaryAbs);
        }
    }

    if (!boundary.isEmpty())
    {
        details.perimeter = da.measureLength(boundary);
    }

    QgsRectangle ext = unionGeom.boundingBox();
    QgsPointXY centerPt = unionGeom.centroid().asPoint();

    QgsCoordinateReferenceSystem wgs84("EPSG:4326");
    QgsCoordinateTransform ct(layer->crs(), wgs84, QgsProject::instance()->transformContext());
    QgsRectangle extWgs = ct.transformBoundingBox(ext);
    QgsPointXY centerWgs = ct.transform(centerPt);

    QString extentStr = QString("%1°N, %2°E : %3°N, %4°E")
            .arg(extWgs.yMinimum(), 0, 'f', 4)
            .arg(extWgs.xMinimum(), 0, 'f', 4)
            .arg(extWgs.yMaximum(), 0, 'f', 4)
            .arg(extWgs.xMaximum(), 0, 'f', 4);
    
    QString centerStr = QString("%1°N, %2°E")
            .arg(centerWgs.y(), 0, 'f', 4)
            .arg(centerWgs.x(), 0, 'f', 4);

    details.coordinates = extentStr;
    details.centerCoordinates = centerStr;
    details.timestamp = QDateTime::currentDateTime();
    
    return details;
}

/// ------------------------------

/// СКАЧИВАНИЕ СНИМКОВ SENTINEL-1

// Функция обработки нажатия кнопки "Скачать снимки Sentinel-1"
void MainWindow::onDownloadSentinelClicked()
{
    // Блокируем кнопку скачивания спутникового снимка
    pbDownloadSentinel->setEnabled(false);
    
    if (copernicusPassword.isEmpty())
    {
        setCopernicusPassword();
        if (copernicusPassword.isEmpty())
        {
            QMessageBox::warning(this, "Внимание", "Для скачивания спутниковых данных необходимо авторизироваться на сайте Copernicus");
            // Включаем кнопку обратно, так как авторизация отменена
            pbDownloadSentinel->setEnabled(true);
            return;
        }
    }
    SentinelDownloadDialog *dialog = new SentinelDownloadDialog(this, copernicusLogin, copernicusPassword, mMapCanvas);
    dialog->exec();
    
    // Включаем кнопку скачивания спутникового снимка обратно после закрытия диалога
    pbDownloadSentinel->setEnabled(true);
    
    delete dialog;
}

void MainWindow::setCopernicusPassword()
{
    // Создаем диалог
    CopernicusLoginDialog dialog(this);

    // Если уже есть сохраненный логин, подставим его
    if (!(copernicusLogin.isEmpty()))
    {
        dialog.setLogin(copernicusLogin);
    }

    // Запускаем модальное окно
    if (dialog.exec() == QDialog::Accepted)
    {
        QString login = dialog.getLogin();
        QString password = dialog.getPassword();

        if (login.isEmpty() || password.isEmpty())
        {
            QMessageBox::warning(this, "Ошибка", "Логин и пароль не могут быть пустыми.");
            return;
        }

        // Сохраняем
        copernicusLogin = login;
        copernicusPassword = password;
        QMessageBox::information(this, "Успешно", "Данные Copernicus сохранены.");
    }
}

/// ------------------------------

/// ФУНКЦИЯ СОЗДАНИЯ СКРИНШОТА

// Функция обработки нажания кнопки "Сделать скриншот"
void MainWindow::makeScreenshot()
{
    if (!mMapCanvas)
    {
        return;
    }

    // Вызываем диалоговое окно для выбора параметров скиншота
    ScreenshotDialog dialog(this);
    if (dialog.exec() != QDialog::Accepted)
    {
        return;
    }

    ScreenshotOptions opts = dialog.getOptions();
    if (opts.filePath.isEmpty())
    {
        return;
    }

    // Настройка рендера
    QgsMapSettings mapSettings = mMapCanvas->mapSettings();
    const qreal scaleFactor = static_cast<qreal>(opts.dpi) / 96.0;
    mapSettings.setOutputSize(QSize(static_cast<int>(mMapCanvas->width() * scaleFactor), static_cast<int>(mMapCanvas->height() * scaleFactor)));
    mapSettings.setOutputDpi(opts.dpi);
    mapSettings.setFlag(Qgis::MapSettingsFlag::Antialiasing, true);

    QgsMapRendererParallelJob job(mapSettings);
    job.start();
    job.waitForFinished();
    QImage image = job.renderedImage();

    if (image.isNull())
    {
        QMessageBox::warning(this, "Ошибка", "Не удалось создать изображение карты.");
        return;
    }

    // Добавление масштаба при необходимости
    if (opts.addScale)
    {
        QPainter painter(&image);
        painter.setRenderHint(QPainter::Antialiasing);
        painter.setRenderHint(QPainter::TextAntialiasing);

        // Настройка шрифта
        QFont font = painter.font();
        font.setPointSize(qMax(6, static_cast<int>(2 * scaleFactor)));
        font.setBold(true);
        painter.setFont(font);

        const QString scaleText = QString("1 : %1").arg(static_cast<int>(mMapCanvas->scale()));
        const int margin = static_cast<int>(10 * scaleFactor);
        const int x = margin;
        const int y = image.height() - margin;
        const int outlineThickness = qMax(1, static_cast<int>(1 * scaleFactor));

        // Обводка (тень)
        painter.setPen(QColor(0, 0, 0, 180));
        for (int dx = -outlineThickness; dx <= outlineThickness; ++dx)
        {
            for (int dy = -outlineThickness; dy <= outlineThickness; ++dy)
            {
                if (dx * dx + dy * dy <= outlineThickness * outlineThickness && (dx != 0 || dy != 0)) {
                    painter.drawText(x + dx, y + dy, scaleText);
                }
            }
        }

        // Основной белый текст
        painter.setPen(Qt::white);
        painter.drawText(x, y, scaleText);
        painter.end();
    }

    //  Сохранение изображения
    bool saveOk = image.save(opts.filePath);
    if (!saveOk)
    {
        QMessageBox::critical(this, tr("Ошибка"), tr("Не удалось сохранить изображение в указанный файл."));
    }
    else
    {
        QMessageBox::information(this, tr("Выполнено"), tr("Скриншот карты успешно сохранен."));
    }
}

/// ------------------------------

/// WMS | WFS | WMTS

// Функция вызова диалогового окна с поставщиками данных (WMS|WFS|WMTS)
QString MainWindow::showProviderSelectionDialog(QWidget* parent, const QStringList& predefinedProviders)
{
    QDialog dialog(parent);
    dialog.setWindowTitle("Выберите сервис поставщика данных");

    QVBoxLayout* layout = new QVBoxLayout(&dialog);

    QLabel* label = new QLabel("Выберите сервис или введите URL:", &dialog);
    layout->addWidget(label);

    QComboBox* comboBox = new QComboBox(&dialog);
    comboBox->setEditable(true);  // Разрешаем редактирование
    comboBox->addItems(predefinedProviders);

    QHBoxLayout* buttonLayout = new QHBoxLayout();

    QPushButton* okButton = new QPushButton("OK", &dialog);
    QPushButton* cancelButton = new QPushButton("Отмена", &dialog);

    buttonLayout->addStretch();
    buttonLayout->addWidget(okButton);
    buttonLayout->addWidget(cancelButton);

    layout->addWidget(comboBox);
    layout->addLayout(buttonLayout);

    QObject::connect(okButton, &QPushButton::clicked, &dialog, &QDialog::accept);
    QObject::connect(cancelButton, &QPushButton::clicked, &dialog, &QDialog::reject);

    // Также закрываем диалог по Enter при редактировании
    QObject::connect(comboBox->lineEdit(), &QLineEdit::returnPressed, &dialog, &QDialog::accept);

    if (dialog.exec() == QDialog::Accepted)
    {
        return comboBox->currentText().trimmed();  // Возвращаем текущий текст (выбранный или введенный)
    }

    return QString();  // Пустая строка если отменено
}

// Функция создания диалогового окна для добавления слоя с GeoServer через WTS
void MainWindow::processWFS()
{
    // Список предварительно отобранных сервисов поставщиков данных
    QStringList providerServices =
    {
        "https://drive.emodnet-geology.eu/geoserver/bgs/wms",
    };

    QString selectedService = showProviderSelectionDialog(this, providerServices);

    if (selectedService.isEmpty())
    {
        qDebug() << "Выбор сервиса отменен";
        return;
    }

    QString wfsBaseUrl = selectedService;

    QString getCapabilitiesUrl = wfsBaseUrl + "?service=WFS&request=GetCapabilities";

    QNetworkAccessManager *manager = new QNetworkAccessManager(this);
    QNetworkReply *reply = manager->get(QNetworkRequest(QUrl(getCapabilitiesUrl)));

    // Получаем список слоев на сервере
    connect(reply, &QNetworkReply::finished, this, [this, reply, wfsBaseUrl]()
    {
        if (reply->error() != QNetworkReply::NoError)
        {
            qDebug() << "Ошибка запроса GetCapabilities:" << reply->errorString();
            reply->deleteLater();
            return;
        }

        // Парсим XML
        QDomDocument doc;
        if (!doc.setContent(reply->readAll()))
        {
            qDebug() << "Ошибка парсинга GetCapabilities";
            reply->deleteLater();
            return;
        }

        reply->deleteLater();

        // Получаем список слоев
        QDomNodeList layerNodes = doc.elementsByTagName("FeatureType");
        QStringList layerNames;
        for (int i = 0; i < layerNodes.count(); ++i)
        {
            QDomElement layerElem = layerNodes.at(i).toElement();
            if (!layerElem.isNull())
            {
                // Получаем имена слоев
                QDomElement nameElem = layerElem.firstChildElement("Name");
                if (!nameElem.isNull())
                {
                    layerNames << nameElem.text();
                }
            }
        }

        if (layerNames.isEmpty())
        {
            qDebug() << "Слои не найдены";
            return;
        }

        // Показываем диалог для выбора слоя
        bool ok;
        QString selectedLayer = QInputDialog::getItem(this, "Выбор WFS слоя", "Слои с сервера:", layerNames, 0, false, &ok);
        
        if (ok && !selectedLayer.isEmpty())
        {
            // Добавляем слой на карту
            addWFSLayer(wfsBaseUrl, selectedLayer);
        }
    });
}

// Функция добавляет WFS-слой по заданному URL и имени слоя
void MainWindow::addWFSLayer(const QString &wfsBaseUrl, const QString &layerName)
{
    QString uri = QString("%1?service=WFS&version=1.1.0&request=GetFeature&typeName=%2&outputFormat=application/json").arg(wfsBaseUrl, layerName);

    QgsVectorLayer *vectorLayer = new QgsVectorLayer(uri, layerName, "ogr"); // "ogr" поддерживает WFS

    if (vectorLayer->isValid())
    {
        QgsProject::instance()->addMapLayer(vectorLayer);
        qDebug() << "WFS слой успешно добавлен:" << layerName;
    }
    else
    {
        qDebug() << "Ошибка при добавлении WFS слоя";
        delete vectorLayer;
    }
}

// Функция создания диалогового окна для добавления слоя с GeoServer через WMS
void MainWindow::processWMS()
{
    // Список предварительно отобранных сервисов поставщиков данных
    QStringList providerServices =
    {
        // Исходные серверы
        "https://wms.gebco.net/mapserv",
        "https://ahocevar.com/geoserver/wms",
        "https://gibs.earthdata.nasa.gov/wms/epsg3857/best/wms.cgi",
    };

    QString selectedService = showProviderSelectionDialog(this, providerServices);

    if (selectedService.isEmpty())
    {
        qDebug() << "Выбор сервиса отменен";
        return;
    }

    QString wmsBaseUrl = selectedService.trimmed(); // Убираем лишние пробелы

    QString getCapabilitiesUrl = wmsBaseUrl + "?service=WMS&request=GetCapabilities";

    QNetworkAccessManager *manager = new QNetworkAccessManager(this);
    QNetworkReply *reply = manager->get(QNetworkRequest(QUrl(getCapabilitiesUrl)));

    // Получаем список слоев на сервере
    connect(reply, &QNetworkReply::finished, this, [this, reply, wmsBaseUrl]()
    {
        if (reply->error() != QNetworkReply::NoError)
        {
            qDebug() << "Ошибка запроса GetCapabilities:" << reply->errorString();
            reply->deleteLater();
            return;
        }

        // Парсим XML
        QDomDocument doc;
        if (!doc.setContent(reply->readAll()))
        {
            qDebug() << "Ошибка парсинга GetCapabilities";
            reply->deleteLater();
            return;
        }

        reply->deleteLater();

        // Получаем список слоев
        QDomNodeList layerNodes = doc.elementsByTagName("Layer");
        QStringList layerNames;
        for (int i = 0; i < layerNodes.count(); ++i)
        {
            QDomElement layerElem = layerNodes.at(i).toElement();
            if (!layerElem.isNull())
            {
                // Получаем имена слоев
                QDomElement nameElem = layerElem.firstChildElement("Name");
                if (!nameElem.isNull())
                {
                    layerNames << nameElem.text();
                }
            }
        }

        if (layerNames.isEmpty())
        {
            qDebug() << "Слои не найдены";
            return;
        }

        // Показываем диалог для выбора слоя
        bool ok;
        QString selectedLayer = QInputDialog::getItem(this, "Выбор WMS слоя", "Слои с сервера:", layerNames, 0, false, &ok);
        
        if (ok && !selectedLayer.isEmpty())
        {
            // Добавляем слой на карту
            addWMSLayer(wmsBaseUrl, selectedLayer);
        }
    });
}

// Функция добавляет WMS-слой по заданному URL и имени слоя
void MainWindow::addWMSLayer(const QString &wmsBaseUrl, const QString &wmsLayerName)
{
    QString wmsCrs = "EPSG:3857";
    QString uri = QString("url=%1&layers=%2&styles=&format=image/png&crs=%3&dpiMode=7&featureCount=10&request=GetMap&version=1.1.0").arg(wmsBaseUrl, wmsLayerName, wmsCrs);

    QgsRasterLayer *wmsLayer = new QgsRasterLayer(uri, wmsLayerName, "wms");

    if (wmsLayer->isValid())
    {
        QgsProject::instance()->addMapLayer(wmsLayer);
        qDebug() << "WMS слой успешно добавлен:" << wmsLayerName;
    }
    else
    {
        qDebug() << "Ошибка при добавлении WMS слоя:" << wmsLayer->error().message();
        delete wmsLayer;
    }
}

// Функция создания диалогового окна для добавления слоя с GeoServer через WMTS
void MainWindow::processWMTS()
{
    // Список предварительно отобранных сервисов поставщиков данных
    QStringList providerServices =
    {
        "https://tiles.emodnet-bathymetry.eu/wmts/1.0.0/WMTSCapabilities.xml",
    };

    QString selectedService = showProviderSelectionDialog(this, providerServices);

    if (selectedService.isEmpty())
    {
        qDebug() << "Выбор сервиса отменен";
        return;
    }

    QString url = selectedService.trimmed();
    QNetworkAccessManager *manager = new QNetworkAccessManager(this);
    QNetworkRequest request((QUrl(url)));
    QNetworkReply *reply = manager->get(request);

    connect(reply, &QNetworkReply::finished, this, [this, reply, url]()
    {
        if (reply->error() != QNetworkReply::NoError)
        {
            qDebug() << "Ошибка загрузки WMTSCapabilities:" << reply->errorString();
            reply->deleteLater();
            return;
        }

        QByteArray data = reply->readAll();
        reply->deleteLater();

        QDomDocument doc;
        if (!doc.setContent(data))
        {
            qDebug() << "Ошибка парсинга WMTSCapabilities XML";
            return;
        }

        // Извлекаем все слои <Layer>
        QDomNodeList layerNodes = doc.elementsByTagName("Layer");
        QStringList layerIds;
        QMap<QString, QString> layerTitles; // для отображения в диалоге

        for (int i = 0; i < layerNodes.count(); ++i)
        {
            QDomElement layerElem = layerNodes.at(i).toElement();
            if (layerElem.isNull()) continue;

            QDomElement idElem = layerElem.firstChildElement("ows:Identifier");
            if (idElem.isNull()) idElem = layerElem.firstChildElement("Identifier");
            if (idElem.isNull()) continue;

            QString layerId = idElem.text();

            // Получаем заголовок (для удобства)
            QDomElement titleElem = layerElem.firstChildElement("ows:Title");
            if (titleElem.isNull()) titleElem = layerElem.firstChildElement("Title");
            QString title = titleElem.isNull() ? layerId : titleElem.text();

            layerIds << layerId;
            layerTitles[layerId] = title;
        }

        if (layerIds.isEmpty())
        {
            qDebug() << "Слои не найдены в WMTS Capabilities";
            return;
        }

        // Готовим отображаемые строки: "название (id)"
        QStringList displayNames;
        for (const QString &id : std::as_const(layerIds))
        {
            displayNames << QString("%1 (%2)").arg(layerTitles.value(id), id);
        }

        bool ok;
        QString selectedDisplay = QInputDialog::getItem(this,  tr("Выбор WMTS слоя"), tr("Доступные слои:"), displayNames, 0, false, &ok);

        if (ok && !selectedDisplay.isEmpty())
        {
            // Извлекаем ID из строки "название (id)"
            static const QRegularExpression re("\\(([^)]+)\\)$");
            QRegularExpressionMatch match = re.match(selectedDisplay);
            QString layerId = match.hasMatch() ? match.captured(1) : selectedDisplay;

            // Добавляем слой с учётом поддерживаемых параметров (например, EPSG:3857 или EPSG:4326)
            addWMTSLayer(url, layerId);
        }
    });
}

// Функция добавляет WMTS-слой по заданному URL и имени слоя
void MainWindow::addWMTSLayer(const QString &capabilitiesUrl, const QString &layerId)
{
    QString crs = "EPSG:3857";
    QString tileMatrixSet = "web_mercator";

    QString uri = QString(
                "contextualWMSLegend=0"
                "&crs=%1"
                "&dpiMode=7"
                "&format=image/png"
                "&layers=%2"
                "&styles=default"
                "&tileMatrixSet=%3"
                "&url=%4")
            .arg(crs, layerId, tileMatrixSet, capabilitiesUrl);

    QgsRasterLayer *layer = new QgsRasterLayer(uri, layerId, "wms");

    if (layer->isValid())
    {
        QgsProject::instance()->addMapLayer(layer);
        qDebug() << "WMTS слой добавлен:" << layerId;
    }
    else
    {
        qDebug() << "Ошибка при добавлении WMTS:" << layer->error().message();
        delete layer;
    }
}

/// РАСПОЗНАВАНИЕ НЕФТЯНОГО ПЯТНА НА СПУТНИКОВОМ СНИМКЕ

// Функция обработки нажатия кнопки "Задать API ключ Roboflow"
void MainWindow::setRoboflowApiKey()
{
    bool ok;
    QString apiKey = QInputDialog::getText( this, tr("Roboflow API ключ"), tr("Введите ваш Roboflow API ключ:"), QLineEdit::Normal, roboflowApiKey, &ok);

    if (ok && !apiKey.trimmed().isEmpty())
    {
        roboflowApiKey = apiKey.trimmed();
        QMessageBox::information(this, tr("Успешно"), tr("API ключ сохранен."));
    }
    else if (ok)
    {
        QMessageBox::warning(this, tr("Ошибка"), tr("API ключ не может быть пустым."));
    }
    // Если пользователь нажал Cancel — ничего не делаем
}

// Функция обработки нажатия кнопки "Найти нефтяное пятно"

void MainWindow::onFindOilSpillClicked()
{
    if (m_processing)
    {
        return;
    }

    QString inputPath = QFileDialog::getOpenFileName(this, tr("Выберите SAR-файл"), QDir::homePath(), tr("Sentinel-1 SAFE ZIP (*.SAFE.zip);;Sentinel-1 VV TIFF (*.tiff *.tif)"));

    if (inputPath.isEmpty())
    {
        return;
    }

    QFileInfo fileInfo(inputPath);
    QString ext = fileInfo.suffix().toLower();
    bool isPreprocessed = (ext == "tiff" || ext == "tif");

    // Валидация настроек (как в оригинале)
    if (!isPreprocessed && SnapPath.isEmpty())
    {
        setPathSnap();
        if (SnapPath.isEmpty())
        {
            QMessageBox::warning(this, tr("Ошибка"), tr("Путь к SNAP не задан."));
            return;
        }
        m_oilWorker->setSnapPath(SnapPath);
    }
    if (roboflowApiKey.isEmpty())
    {
        setRoboflowApiKey();
        if (roboflowApiKey.isEmpty())
        {
            QMessageBox::warning(this, tr("Предупреждение"), tr("API ключ Roboflow не задан."));
            return;
        }
        m_oilWorker->setRoboflowApiKey(roboflowApiKey);
    }

    // Блокировка UI
    m_processing = true;
    pbFindOilSpill->setEnabled(false);

    // Запуск в потоке
    SarProcess::Mode mode = isPreprocessed ? SarProcess::ModePreprocessed : SarProcess::ModeRawSAFE;


    QMetaObject::invokeMethod(m_oilWorker, [this, inputPath, mode]()
    {
        m_oilWorker->process(inputPath, mode);
    }, Qt::QueuedConnection);
}

void MainWindow::onWorkerFinished(const SARProcessingResult &result)
{
    // Разблокировка UI
    m_processing = false;
    pbFindOilSpill->setEnabled(true);

    if (!result.success)
    {
        QMessageBox::critical(this, tr("Ошибка обработки"), result.error);
        return;
    }

    // Сохранение метаданных
    imageDataTime = result.imageDataTime;
    m_geotiffGeoTransform = result.geotransform;
    m_geotiffSize = result.geotiffSize;

    // Добавление слоёв в QGIS
    if (!result.finalOut.isEmpty())
    {
        m_dataHandling->addRasterLayers(result.finalOut, mSourceCrs);
    }
    m_dataHandling->addRasterLayers(result.vvOut, mSourceCrs, true);

    if (!QFile::exists(result.outputPathVv))
    {
        QMessageBox::critical(this, "Ошибка", "Не удалось загрузить изображение для анализа");
        return;
    }

    auto* dialog = new DetectionOilDialog(result.outputPathVv, roboflowApiKey, m_geotiffGeoTransform, m_geotiffSize, this);

    connect(dialog, &DetectionOilDialog::detectionImageSaved, this, [this](const QString &path)
    {
        lastDetectionOverlayImagePath = path;
    });

    connect(dialog, &DetectionOilDialog::layerSaved, this, [this, geojsonPath = QString()](const QString &path) mutable
    {
        QFileInfo fileInfo(path);

        QString tempPath = QStringLiteral(SRCDIR) + "Data/temp/" + fileInfo.baseName() + "_fixed.geojson";
        QgsVectorLayer* inputLayer = new QgsVectorLayer(path, "input", "ogr");
        if (!inputLayer->isValid())
        {
            return;
        }
        if (inputLayer->featureCount() <= 0)
        {
            QMessageBox::information(this, "Внимание", "Не найдено нефтяных пятен на снимке. Слой пустой.");
            return;
        }

        const QgsProcessingAlgorithm* alg = QgsApplication::processingRegistry()->algorithmById("native:fixgeometries");
        if (!alg) return;

        QVariantMap params{ {"INPUT", QVariant::fromValue(inputLayer)}, {"OUTPUT", tempPath} };
        QgsProcessingContext context;
        QgsProcessingFeedback feedback;
        QVariantMap result = alg->run(params, context, &feedback);

        if (feedback.isCanceled())
        {
            return;
        }


        QgsVectorLayer* inputLayer2 = new QgsVectorLayer(tempPath, fileInfo.baseName(), "ogr");
        QgsProject::instance()->addMapLayer(inputLayer2);

        QFile::remove(path);

    });

    dialog->show();
    dialog->setAttribute(Qt::WA_DeleteOnClose);
}

/// ------------------------------

/// ИЗМЕРЕНИЕ РАССТОЯНИЯ/ПЛОЩАДИ/УГЛА

// Функцяи обработки нажатия кнопки "Измерить площадь"
void MainWindow::measureArea()
{
    if(!mMapCanvas || mMapCanvas->layerCount() == 0)
    {
        qDebug() << "Ошибка, нет слоев";
        return;
    }

    if(!mSourceCrs.isValid())
    {
        qDebug() << "Не задана система координат";
        return;
    }

    isMeasurment = !(isMeasurment);
    measurementTool = new MeasurementTool(mMapCanvas, MeasurementTool::Area);
    connect(measurementTool, &MeasurementTool::areaChanged, this, &MainWindow::onDistanceChangedArea);
    connect(measurementTool, &MeasurementTool::measurementReset, this, &MainWindow::onMeasurementReset);
    mMapCanvas->setMapTool(measurementTool);
}

// Функцяи обработки нажатия кнопки "Измерить угол"
void MainWindow::measureAngle()
{
    if(!mMapCanvas || mMapCanvas->layerCount() == 0)
    {
        qDebug() << "Ошибка, нет слоев";
        return;
    }

    if(!mSourceCrs.isValid())
    {
        qDebug() << "Не задана система координат";
        return;
    }

    isMeasurment = !(isMeasurment);
    measurementTool = new MeasurementTool(mMapCanvas, MeasurementTool::Angle);
    connect(measurementTool, &MeasurementTool::angleChanged, this, &MainWindow::onAngleChanged);
    connect(measurementTool, &MeasurementTool::measurementReset, this, &MainWindow::onMeasurementReset);
    mMapCanvas->setMapTool(measurementTool);

}

// Функция смены режима на выбор объектов
void MainWindow::selectionMode()
{
    // Устанавливаем инструмент выделения
    SelectByRectangleTool* selectTool = new SelectByRectangleTool(mMapCanvas);
    mMapCanvas->setMapTool(selectTool);

}

// Функция 'линейки'
void MainWindow::measurment()
{
    if(!mMapCanvas || mMapCanvas->layerCount() == 0)
    {
        qDebug() << "Ошибка, нет слоев";
        return;
    }

    if(!mSourceCrs.isValid())
    {
        qDebug() << "Не задана система координат";
        return;
    }

    isMeasurment = !(isMeasurment);
    measurementTool = new MeasurementTool(mMapCanvas, MeasurementTool::Distance);
    connect(measurementTool, &MeasurementTool::distanceChanged, this, &MainWindow::onDistanceChanged);
    connect(measurementTool, &MeasurementTool::measurementReset, this, &MainWindow::onMeasurementReset);
    mMapCanvas->setMapTool(measurementTool);
}

// Функция формирования вывода значений для 'линейки'
static QString formatNumber(double value, int precision = 2)
{
    return QLocale().toString(value, 'f', precision);
}

// Обработка сигнала от 'линейки'
void MainWindow::onDistanceChanged(double totalDistanceKm, double lastSegmentKm)
{
    QString totalStr;
    QString segmentStr;

    if (totalDistanceKm >= 1.0)
    {
        totalStr = formatNumber(totalDistanceKm, 2) + " км";
    }
    else
    {
        totalStr = formatNumber(totalDistanceKm * 1000.0, 1) + " м";
    }

    if (lastSegmentKm >= 1.0)
    {
        segmentStr = formatNumber(lastSegmentKm, 2) + " км";
    }
    else
    {
        segmentStr = formatNumber(lastSegmentKm * 1000.0, 1) + " м";
    }

    mLabelMeasurment->setText("Общее расстояние: " + totalStr + "\nПоследний сегмент: " + segmentStr);
}

// Функция изменения вычисляемой площади
void MainWindow::onDistanceChangedArea(double areaSqMeters)
{
    QString areaStr;

    if (areaSqMeters >= 1e6)
    {
        areaStr = formatNumber(areaSqMeters / 1e6, 3) + " км²";
    }
    else
    {
        areaStr = formatNumber(areaSqMeters, 1) + " м²";
    }

    mLabelMeasurment->setText("Площадь: " + areaStr);
}

// Функция изменения измерений угла
void MainWindow::onAngleChanged(double angle)
{
    mLabelMeasurment->setText("Угол: " + formatNumber(angle, 2) + "°");
}

// Функция сброса измерений
void MainWindow::onMeasurementReset()
{
    mLabelMeasurment->setText("Поле вывода измерений");
}

/// ------------------------------

/// ФУНКЦИОНАЛ ВЗАИМОДЕЙСТВИЯ СО СЛОЯМИ

// Функция обработки нажатия кнопки "Очистить выделение"
void MainWindow::onClearSelectionClicked()
{
    const auto mapLayers = QgsProject::instance()->mapLayers();
    for (auto* layer : mapLayers)
    {
        QgsVectorLayer* vlayer = qobject_cast<QgsVectorLayer*>(layer);
        if (vlayer && vlayer->isSpatial())
        {
            vlayer->removeSelection();
        }
    }
}

// Функция нажатия кнопки "Завершить редактирование"
void MainWindow::onStopEditClicked()
{
    pbStopEdit->setStyleSheet(
                "QPushButton {"
                "   background-color: white;"
                "     color: #2b2b2b;"
                "   border: 2px solid #9ACEEB;"
                " border-radius: 16px;"
                "   padding: 6px 14px;"
                "}"
                "QPushButton:hover {"
                "   background-color: #9ACEEB;"
                "    color: white;"
                "}"
                " QPushButton:pressed {"
                "  background-color: #d0d0d0;"
                " border-color: #9ACEEB;"
                " color: #2b2b2b;"
                "}"

                "QPushButton:disabled {"
                "background-color: #f2f2f2;"
                "border-color: #c0c0c0;"
                "color: #9a9a9a;"
                "}"
                );

    pbSelect->setEnabled(true);
    if (!mMapCanvas)
    {
        return;
    }

    QgsMapTool* currentTool = mMapCanvas->mapTool();
    DrawPointTool* drawToolPoint = dynamic_cast<DrawPointTool*>(currentTool);
    DrawLineStringTool* drawToolLineString = dynamic_cast<DrawLineStringTool*>(currentTool);
    DrawPolygonTool* drawToolPoly = dynamic_cast<DrawPolygonTool*>(currentTool);

    // Проверяем какой инструмент создания был выбран
    QgsVectorLayer* layer = nullptr;
    if (drawToolPoint && drawToolPoint->mLayer)
    {
        layer = drawToolPoint->mLayer;
    }
    else if (drawToolLineString && drawToolLineString->mLayer)
    {
        layer = drawToolLineString->mLayer;
    }
    else if (drawToolPoly && drawToolPoly->mLayer)
    {
        layer = drawToolPoly->mLayer;
    }
    else if (editedLayer)
    {
        layer = editedLayer;
    }
    else
    {
        mMapCanvas->setMapTool(mpPanTool);
        return;
    }

    layer->commitChanges();
    if (layer->isEditable())
    {
        if (!layer->commitChanges())
        {
            qDebug() << "Не удалось сохранить изменения слоя:" << layer->name();
        }
        else
        {
            qDebug() << "Редактирование слоя завершено:" << layer->name();
        }
    }

    QString filePath = QFileDialog::getSaveFileName(this, "Сохранить слой как", QDir::homePath(), "Shapefile (*.shp);;GeoPackage (*.gpkg);;GeoJSON (*.geojson)");

    if (!filePath.isEmpty())
    {
        // Определяем драйвер обработки сохранения файла
        QString driver;
        if (filePath.endsWith(".shp"))
        {
            driver = "ESRI Shapefile";
        }
        else if (filePath.endsWith(".gpkg"))
        {
            driver = "GPKG";
        }
        else if (filePath.endsWith(".geojson"))
        {
            driver = "GeoJSON";
        }
        else
        {
            driver = "ESRI Shapefile";
        }

        // Сохраняем векторный файл
        QgsVectorFileWriter::writeAsVectorFormat(layer, filePath, "UTF-8", layer->crs(), driver);
        QMessageBox::information(this, "Успешно","Файл успешно сохранен");

    }

    mMapCanvas->setMapTool(mpPanTool);

    // Включаем кнопку оцифровки обратно в любом случае
    pbCreatePolygon->setEnabled(true);

}

// Функция нажатия кнопки "Начать редактирование"
void MainWindow::onStartEditClicked()
{
    // Проверяем, что выбран слой для редактирования
    QList<QgsLayerTreeNode*> selectedNodes = mLayerTreeView->selectedNodes();
    if (selectedNodes.isEmpty())
    {
        qDebug() << "Нет выбранных слоев для редактирования.";
        return;
    }

    // Выбираем только первый выбранный слой
    QgsLayerTreeLayer* layerNode = dynamic_cast<QgsLayerTreeLayer*>(selectedNodes.first());
    if (!layerNode)
    {
        return;
    }

    editedLayer = qobject_cast<QgsVectorLayer*>(layerNode->layer());
    if (!editedLayer)
    {
        qDebug() << "Выбранный слой не является векторным.";
        return;
    }

    // Проверяем, можно ли редактировать данный слой
    if (!editedLayer->isEditable())
    {
        editedLayer->startEditing();
        qDebug() << "Редактирование включено для слоя:" << editedLayer->name();

        if (!digitizingDock)
        {
            digitizingDock = new QgsAdvancedDigitizingDockWidget(mMapCanvas);
            digitizingDock->setWindowTitle("Редактирование слоя");
            digitizingDock->setAllowedAreas(Qt::LeftDockWidgetArea | Qt::RightDockWidgetArea);
            addDockWidget(Qt::RightDockWidgetArea, digitizingDock); // Добавляем панель для редактирования слоя
        }

        vertexTool = new QgsVertexTool(mMapCanvas, digitizingDock);
        mMapCanvas->setMapTool(vertexTool);

        // После mMapCanvas->setMapTool(vertexTool);
        QList<QDockWidget*> docks = findChildren<QDockWidget*>();
        for (QDockWidget* dock : std::as_const(docks))
        {

            if (dock->windowTitle() == "Vertex Editor")
            {

                dock->setWindowTitle(tr("Редактор вершин"));
                break;
            }
        }
    }
    else
    {
        qDebug() << "Слой уже находится в режиме редактирования:" << editedLayer->name();
    }

    pbStopEdit->setStyleSheet(
                "QPushButton {"
                "   background-color: #CCFFCC;"
                "     color: #2b2b2b;"
                "   border: 2px solid #9ACEEB;"
                " border-radius: 16px;"
                "   padding: 6px 14px;"
                "}"

                );

    pbSelect->setEnabled(false);
}

// Функция добавляет новый объект (геометрию) в активный векторный слой.
void MainWindow::addNewObjectToLayer()
{
    QList<QgsLayerTreeNode*> selectedNodes = mLayerTreeView->selectedNodes();

    if (selectedNodes.isEmpty())
    {
        qDebug() << "Нет выбранных слоев для редактирования.";
        return;
    }

    // Берем первый выбранный слой
    QgsLayerTreeLayer* layerNode = dynamic_cast<QgsLayerTreeLayer*>(selectedNodes.first());
    if (!layerNode)
    {
        return;
    }

    editedLayer = qobject_cast<QgsVectorLayer*>(layerNode->layer());

    if (!editedLayer)
    {
        qDebug() << "Выбранный слой не является векторным.";
        return;
    }

    Qgis::WkbType layerWKBType = editedLayer->wkbType();
    if(layerWKBType == Qgis::WkbType::Point || layerWKBType == Qgis::WkbType::MultiPoint)
    {
        qDebug() << "Точка";
        m_dataHandling->addObjectToPointLayer(*mMapCanvas, *editedLayer);

    }
    else if (layerWKBType == Qgis::WkbType::LineString || layerWKBType == Qgis::WkbType::MultiLineString)
    {
        qDebug() << "Линия";
        m_dataHandling->addObjectToLineLayer(*mMapCanvas, *editedLayer);

    }
    else if (layerWKBType == Qgis::WkbType::Polygon || layerWKBType == Qgis::WkbType::MultiPolygon)
    {
        qDebug() << "Полигон";
        m_dataHandling->addObjectToPolyLayer(*mMapCanvas, *editedLayer);
    }
    qDebug() << "Добавление объекта в слой";

    pbStopEdit->setStyleSheet(
                "QPushButton {"
                "   background-color: #CCFFCC;"
                "     color: #2b2b2b;"
                "   border: 2px solid #9ACEEB;"
                " border-radius: 16px;"
                "   padding: 6px 14px;"
                "}"

                );
}

// Обработчик нажатия кнопки создания полигона
void MainWindow::onCreatePolygonClicked()
{
    // Блокируем кнопку оцифровки
    pbCreatePolygon->setEnabled(false);

    // Если пользователь отменил ввод, используем имя по умолчанию
    m_dataHandling->addPolyLayer(*mMapCanvas, "Oil Spill");

    pbStopEdit->setStyleSheet(
                "QPushButton {"
                "   background-color: #CCFFCC;"
                "     color: #2b2b2b;"
                "   border: 2px solid #9ACEEB;"
                " border-radius: 16px;"
                "   padding: 6px 14px;"
                "}"
                );
}

// Функция обработки двойного клика на слой в дереве слоев
void MainWindow::onDoubleClicked(const QModelIndex &index)
{
    // Получаем узел дерева
    QgsLayerTreeNode* node = mLayerTreeView->index2node(index);

    if (node && QgsLayerTree::isLayer(node))
    {
        QgsMapLayer* layer = QgsLayerTree::toLayer(node)->layer();

        if (layer)
        {
            // Берем экстент конкретного слоя
            QgsRectangle layerExtent = layer->extent();

            if (!layerExtent.isEmpty())
            {
                mMapCanvas->setExtent(layerExtent);
                mMapCanvas->refresh();
            }
        }
    }
}

// Функция обработки клика на слой в дереве слоев
void MainWindow::onClicked(const QModelIndex &index)
{
    // Получаем узел дерева слоев по индексу
    QgsLayerTreeNode* node = mLayerTreeView->index2node(index);
    if (!node)
    {
        return;
    }

    // Вычисляем экстент всех слоев проекта
    QgsRectangle projectExtent;
    const auto layers = QgsProject::instance()->mapLayers().values();
    for (QgsMapLayer* layer : layers)
    {
        if (!layer)
        {
            continue;
        }


        QgsVectorLayer* vLayer = qobject_cast<QgsVectorLayer*>(layer);
        if (vLayer)
        {
            vLayer->updateExtents();
        }

        if (projectExtent.isEmpty())
        {
            projectExtent = layer->extent();
        }
        else
        {
            projectExtent.combineExtentWith(layer->extent());
        }
    }

    // Применяем экстент к карте, если он не пустой
    if (!projectExtent.isEmpty())
    {
        mMapCanvas->setExtent(projectExtent);
        mMapCanvas->refresh();
    }
}

// Функция удаления слоев (из QgsLayerTreeView)
void MainWindow::deleteLayers()
{
    if (mLayerTreeView->selectedLayers().isEmpty())
    {
        QMessageBox::warning(this, "Внимание", "Отсутствуют выбранные слои для удаления");
    }
    else
    {
        QgsProject::instance()->removeMapLayers( mLayerTreeView->selectedLayers());
        mMapCanvas->setMapTool(mpPanTool);
        mMapCanvas->refresh();
    }
}

// Функция добавления векторного слоя
void MainWindow::addVectorLayers()
{
    QString filename = QFileDialog::getOpenFileName(
                this,
                tr("Открыть векторный файл"),
                QStringLiteral(SRCDIR) + "Data/temp",
                "ESRI Shapefile (*.shp);;GeoPackage (*.gpkg);;GeoJSON (*.geojson);;ALL (*.*)"
                );

    if (filename.isEmpty())
    {
        return;
    }

    m_dataHandling->addVectorLayers(filename, mSourceCrs);
}

// Функция добавления растрового слоя
void MainWindow::addRasterLayers()
{
    QString filename = QFileDialog::getOpenFileName(
                this,
                tr("Открыть растровый файл File"),
                QStringLiteral(SRCDIR) + "Data/temp",
                "GeoTIFF(*.tif *.tiff);;GeoPackage (*.gpkg);;All(*.*)");
    m_dataHandling->addRasterLayers(filename, mSourceCrs);
}

/// ------------------------------

/// ФУНКЦИОНАЛ АНАЛИЗА РАСПРОСТРАНЕНИЯ

void MainWindow::addSpillAnalysis()
{
    QString filename = QFileDialog::getOpenFileName(
                this,
                tr("Открыть файл"),
                QDir::homePath(),
                "GeoJSON (*.geojson)"
                );

    if (filename.isEmpty())
        return;

    // Добавляем слой spill
    m_dataHandling->addStyledVectorLayers(filename, mSourceCrs, "hour");

    // Получаем последний добавленный слой
    QgsVectorLayer* spillLayer = qobject_cast<QgsVectorLayer*>(QgsProject::instance()->mapLayers().values().last());

    if (!spillLayer)
    {
        return;
    }

    // Подготовка директории
    QString outputDir = QStringLiteral(SRCDIR) + "Data/temp/";
    QDir().mkpath(outputDir);

    // Выносим рендеринг в отдельную функцию
    QList<int> hours = {0, 24, 48, 72};
    renderSpillFrames(spillLayer, outputDir, hours, mMapCanvas);

    // Восстановление состояния UI
    pbStartSimulation->setEnabled(true);
    pbStopSimulation->setEnabled(true);
    simulationSpeedSlider->setEnabled(true);
}

void MainWindow::renderSpillFrames(QgsVectorLayer* spillLayer, const QString& outputDir, const QList<int>& hours, QgsMapCanvas* mapCanvas)
{
    if (!spillLayer || !mapCanvas)
    {
        return;
    }

    for (int hour : hours)
    {
        // Фильтр по часу
        spillLayer->setSubsetString(QString("hour = %1").arg(hour));

        // Принудительное обновление данных
        spillLayer->dataProvider()->forceReload();
        spillLayer->updateExtents();
        spillLayer->triggerRepaint();

        QCoreApplication::processEvents(QEventLoop::AllEvents, 300);

        QgsRectangle extent = spillLayer->extent();
        if (extent.isEmpty())
        {
            continue;
        }

        // Настройки рендера
        QgsMapSettings settings;
        QList<QgsMapLayer*> layers = mapCanvas->layers();

        if (!layers.contains(spillLayer))
        {
            layers.append(spillLayer);
        }

        settings.setLayers(layers);
        settings.setBackgroundColor(Qt::white);
        settings.setExtent(extent);
        settings.setOutputSize(QSize(3840, 2160));
        settings.setOutputDpi(300);
        settings.setFlag(Qgis::MapSettingsFlag::Antialiasing, true);
        settings.setFlag(Qgis::MapSettingsFlag::UseAdvancedEffects, true);
        settings.setDestinationCrs(mapCanvas->mapSettings().destinationCrs());

        // Параллельный рендеринг
        QgsMapRendererParallelJob job(settings);
        job.start();
        job.waitForFinished();

        QImage img = job.renderedImage();
        QString outputPath = outputDir + QString("spill_%1.png").arg(hour);
        img.save(outputPath, "PNG");
    }

    // Сброс фильтра после цикла
    spillLayer->setSubsetString(QString());
}

// Извлечение времени из имени файла спутникового снимка
QDateTime MainWindow::extractDateTimeFromFileName(const QString& fileName)
{
    return extractSentinelDateTimeFromFileName(fileName);
}

void MainWindow::onOilAnalysisFinished(bool ok, const QString& error)
{
    unlockMapCanvas();
    pbStartAnalysis->setEnabled(1);

    predictionMode = false;

    if (!ok)
    {
        QMessageBox::critical(this, "Ошибка моделирования", error);
        return;
    }

    // Путь к результату моделирования
    QString outputPath = QStringLiteral(SRCDIR) + "Data/temp/prediction_result.geojson";

    // Добавляем слой с результатом
    m_dataHandling->addStyledVectorLayers(outputPath, mSourceCrs, "hour");

    // Получаем последний добавленный слой
    QgsVectorLayer* predictionLayer = qobject_cast<QgsVectorLayer*>(QgsProject::instance()->mapLayers().values().last());

    if (predictionLayer)
    {
        // Подготовка директории для экспорта
        QString exportDir = QStringLiteral(SRCDIR) + "Data/temp/";
        QDir().mkpath(exportDir);

        // Часовые срезы для рендеринга
        QList<int> hours = {0, 24, 48, 72};

        // Вызываем универсальную функцию рендеринга
        renderSpillFrames(predictionLayer, exportDir, hours, mMapCanvas);
    }

    // Восстановление состояния UI
    pbStartSimulation->setEnabled(true);
    pbStopSimulation->setEnabled(true);
    simulationSpeedSlider->setEnabled(true);
}

void MainWindow::onStartAnalysisClicked()
{
    if (predictionMode)
    {
        predictionMode = false;
        return;
    }

    predictionMode = true;

    QList<QgsLayerTreeNode*> selectedNodes = mLayerTreeView->selectedNodes();
    if (selectedNodes.isEmpty())
    {
        predictionMode = false;
        QMessageBox::warning(this, "Предупреждение!","Нужно выбрать необходимый полигональный слой в дереве слоев");
        return;
    }

    auto* layerNode = dynamic_cast<QgsLayerTreeLayer*>(selectedNodes.first());
    if (!layerNode)
    {
        predictionMode = false;
        return;
    }

    auto* vectorLayer = qobject_cast<QgsVectorLayer*>(layerNode->layer());
    if (!vectorLayer)
    {
        predictionMode = false;
        return;
    }
    if (vectorLayer->featureCount() <= 0)
    {
        QMessageBox::information(this, "Внимание", "В файле нет объектов.");
        return;
    }

    if (vectorLayer->geometryType() != Qgis::GeometryType::Polygon)
    {
        QMessageBox::warning(this, "Ошибка", "Слой для анализа должен быть полигональным");
        predictionMode = false;
        return;
    }

    QString layerPath = vectorLayer->dataProvider()->dataSourceUri();

    // Извлечение времени из имени файла
    QDateTime extractedTime = extractDateTimeFromFileName(layerPath);
    if (!extractedTime.isValid())
    {
        // Если не удалось извлечь время, используем текущее
        extractedTime = QDateTime::currentDateTime();
        QMessageBox::information(this, "Время анализа","Не удалось извлечь время из имени файла.\n" "Будет использовано текущее время.");
    }

    // Диалог параметров
    OilSpreadAnalysisDialog dialog(this);
    dialog.setSpillGeoJsonPath(layerPath);
    dialog.setStartTime(extractedTime);

    if (dialog.exec() != QDialog::Accepted)
    {
        predictionMode = false;
        return;
    }

    pbStartAnalysis->setEnabled(0);

    //Считываем данные из диалогового окна
    OilSpreadAnalysisConfig config;
    config.spillGeoJsonPath = dialog.getSpillGeoJsonPath();
    config.startTime = dialog.getStartTime();
    config.bufferMeters = dialog.getByfferMeters();
    config.highResLand = dialog.getResLand();
    config.weatherLatStep = dialog.getWeatherLatStep();
    config.weatherLonStep = dialog.getWeatherLonStep();

    baseTime = config.startTime;

    const int pointsPerKm2 = dialog.getPointsPerKm2();
    const QString outputPath = QStringLiteral(SRCDIR) + "Data/temp/prediction_result.geojson";

    m_analysisThread = new QThread(this);

    m_analysisWorker = new OilSpreadAnalysis(config);

    if (!m_analysisWorker->isValid())
    {
        qWarning() << m_analysisWorker->errorMessage();
        delete m_analysisWorker;
        delete m_analysisThread;
        m_analysisWorker = nullptr;
        m_analysisThread = nullptr;
        predictionMode = false;
        return;
    }

    m_analysisWorker->moveToThread(m_analysisThread);

    // Запуск вычислений
    connect(m_analysisThread, &QThread::started, this, [this, outputPath, pointsPerKm2]()
    {
        emit startOilAnalysis(outputPath, pointsPerKm2);
    });


    connect(this, &MainWindow::startOilAnalysis, m_analysisWorker, &OilSpreadAnalysis::run);

    // Завершение
    connect(m_analysisWorker, &OilSpreadAnalysis::finished, this, &MainWindow::onOilAnalysisFinished);

    connect(m_analysisWorker, &OilSpreadAnalysis::finished, m_analysisThread, &QThread::quit);

    connect(m_analysisThread, &QThread::finished, m_analysisWorker, &QObject::deleteLater);

    connect(m_analysisThread, &QThread::finished, m_analysisThread, &QObject::deleteLater);

    lockMapCanvas();

    m_analysisThread->start();
}

// Функция обработки ответа с сервера с информацией о погоде
void MainWindow::processWeatherResponse(QNetworkReply* reply)
{
    if (reply->error() != QNetworkReply::NoError)
    {
        qDebug() << "Ошибка запроса погоды:" << reply->errorString();
        currentWindData = "";
        // Обновляем отображение координат без данных о ветре
        if (coordinatesLabel)
        {
            QString text = coordinatesLabel->text();
            // Удаляем старые данные о ветре, если они есть
            int windIndex = text.indexOf(" | Ветер:");
            if (windIndex != -1)
            {
                text = text.left(windIndex);
            }
            coordinatesLabel->setText(text);
        }
        return;
    }

    QByteArray responseData = reply->readAll();
    QJsonDocument jsonDoc = QJsonDocument::fromJson(responseData);

    if (!jsonDoc.isObject())
    {
        qDebug() << "Ошибка: ответ не является JSON объектом";
        currentWindData = "";
        return;
    }

    QJsonObject obj = jsonDoc.object();
    if (!obj.contains("current"))
    {
        qDebug() << "Ошибка: в ответе нет данных current";
        currentWindData = "";
        return;
    }

    QJsonObject current = obj["current"].toObject();

    double windSpeed = current.value("wind_speed_10m").toDouble(-999);
    double windDirection = current.value("wind_direction_10m").toDouble(-999);

    if (windSpeed < 0 || windDirection < 0)
    {
        currentWindData = "";
        return;
    }

    // Формируем строку с данными о ветре
    // Направление ветра: 0° = север, 90° = восток, 180° = юг, 270° = запад
    QString directionStr;
    if (windDirection >= 337.5 || windDirection < 22.5)
    {
        directionStr = "С";
    }
    else if (windDirection >= 22.5 && windDirection < 67.5)
    {
        directionStr = "СВ";
    }
    else if (windDirection >= 67.5 && windDirection < 112.5)
    {
        directionStr = "В";
    }
    else if (windDirection >= 112.5 && windDirection < 157.5)
    {
        directionStr = "ЮВ";
    }
    else if (windDirection >= 157.5 && windDirection < 202.5)
    {
        directionStr = "Ю";
    }
    else if (windDirection >= 202.5 && windDirection < 247.5)
    {
        directionStr = "ЮЗ";
    }
    else if (windDirection >= 247.5 && windDirection < 292.5)
    {
        directionStr = "З";
    }
    else
    {
        directionStr = "СЗ";
    }

    currentWindData = QString(" | Ветер: %1 м/с, %2 (%3°)")
            .arg(windSpeed, 0, 'f', 1)
            .arg(directionStr)
            .arg(static_cast<int>(windDirection));

    // Обновляем отображение координат с данными о ветре
    if (coordinatesLabel)
    {
        QString text = coordinatesLabel->text();
        // Удаляем старые данные о ветре, если они есть
        int windIndex = text.indexOf(" | Ветер:");
        if (windIndex != -1)
        {
            text = text.left(windIndex);
        }
        text += currentWindData;
        coordinatesLabel->setText(text);
    }
}

/// ------------------------------
