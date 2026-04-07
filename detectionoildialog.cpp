#include "detectionoildialog.h"

DetectionOilDialog::DetectionOilDialog(const QString &imagePath, const QString &userApiKey, const QVector<double> &geoTransform, const QSize &rasterSize, QWidget *parent)
    : QDialog(parent)
    , imagePath(imagePath)
    , mApiKey(userApiKey)
    , confidenceThreshold(0.5)
    , m_geoTransform(geoTransform, rasterSize)
{
    setWindowTitle("Результаты обнаружения нефтяного пятна");
    resize(1000, 700);

    initializeComponents();
    setupUI();
    connectSignals();

    setEnabled(false);
    sendRequestsToAllModels();

    QTimer::singleShot(0, this, &DetectionOilDialog::onRedraw);
}

void DetectionOilDialog::initializeComponents()
{
    m_modelManager = new ModelManager(mApiKey, this);
    m_renderer = new DetectionRenderer();
    m_exporter = new GeoJsonExporter();
}

void DetectionOilDialog::connectSignals()
{
    connect(m_modelManager, &ModelManager::modelResponseReceived, this, &DetectionOilDialog::onModelResponseReceived);
    connect(m_modelManager, &ModelManager::modelErrorOccurred, this, &DetectionOilDialog::onModelErrorOccurred);
    connect(m_modelManager, &ModelManager::allModelsCompleted, this, &DetectionOilDialog::onAllModelsCompleted);
}

void DetectionOilDialog::setupUI()
{
    imageLabel = new QLabel(this);
    imageLabel->setAlignment(Qt::AlignCenter);
    imageLabel->setMinimumSize(600, 600);
    imageLabel->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);

    confidenceSlider = new QSlider(Qt::Horizontal);
    confidenceSlider->setRange(0, 100);
    confidenceSlider->setValue(50);
    connect(confidenceSlider, &QSlider::valueChanged, this, &DetectionOilDialog::onConfidenceChanged);

    auto confidenceLabel = new QLabel("Порог уверенности: 50%");
    connect(confidenceSlider, &QSlider::valueChanged, confidenceLabel, [confidenceLabel](int v)
    {
        confidenceLabel->setText(QString("Порог уверенности: %1%").arg(v));
    });

    modelCombo = new QComboBox;
    modelCombo->addItems(m_modelManager->getModelNames());
    connect(modelCombo, QOverload<int>::of(&QComboBox::currentIndexChanged), this, &DetectionOilDialog::onSwitchModel);

    drawModeCombo = new QComboBox;
    drawModeCombo->addItems({"Рисовать подписи", "Рисовать формы"});
    connect(drawModeCombo, QOverload<int>::of(&QComboBox::currentIndexChanged), this, &DetectionOilDialog::onRedraw);

    jsonView = new QTextEdit;
    jsonView->setReadOnly(true);
    jsonView->setMaximumHeight(200);

    copyButton = new QPushButton("Копировать JSON");
    connect(copyButton, &QPushButton::clicked, this, [this]()
    {
        QApplication::clipboard()->setText(jsonView->toPlainText());
    });

    addLayerButton = new QPushButton("Добавить слой в проект");
    connect(addLayerButton, &QPushButton::clicked, this, &DetectionOilDialog::onAddLayerToProject);

    auto settingsBox = new QGroupBox("Настройки обнаружения");
    auto settingsLayout = new QVBoxLayout(settingsBox);
    settingsLayout->addWidget(confidenceLabel);
    settingsLayout->addWidget(confidenceSlider);
    settingsLayout->addWidget(new QLabel("Модель нейросети:"));
    settingsLayout->addWidget(modelCombo);
    settingsLayout->addWidget(new QLabel("Режим отображения:"));
    settingsLayout->addWidget(drawModeCombo);

    auto rightLayout = new QVBoxLayout;
    rightLayout->addWidget(settingsBox);
    rightLayout->addWidget(jsonView);
    rightLayout->addWidget(copyButton);
    rightLayout->addWidget(addLayerButton);
    rightLayout->addStretch();

    auto mainLayout = new QHBoxLayout(this);
    mainLayout->addWidget(imageLabel, 1);
    mainLayout->addLayout(rightLayout);

    setModal(true);
}

void DetectionOilDialog::sendRequestsToAllModels()
{
    QFile file(imagePath);
    if (!file.open(QIODevice::ReadOnly))
    {
        QMessageBox::critical(this, "Ошибка", "Не удалось открыть изображение.");
        return;
    }

    QByteArray imageData = file.readAll();
    m_modelManager->sendRequestsToAllModels(imageData);
}

void DetectionOilDialog::onModelResponseReceived(int modelIndex, const QJsonArray &modelPredictions)
{
    if (modelIndex == modelCombo->currentIndex())
    {
        predictions = modelPredictions;
        
        QJsonObject obj;
        obj["predictions"] = predictions;
        jsonView->setPlainText(QJsonDocument(obj).toJson(QJsonDocument::Indented));
        
        onRedraw();
    }
}

void DetectionOilDialog::onModelErrorOccurred(int modelIndex, const QString &error)
{
    QStringList modelNames = m_modelManager->getModelNames();
    QMessageBox::critical(this, "Ошибка Roboflow", QString("Ошибка модели %1: %2").arg(modelNames[modelIndex], error));
}

void DetectionOilDialog::onAllModelsCompleted()
{
    setEnabled(true);
}

void DetectionOilDialog::onSwitchModel(int index)
{
    predictions = m_modelManager->getModelResponse(index);
    
    QJsonObject obj;
    obj["predictions"] = predictions;
    jsonView->setPlainText(QJsonDocument(obj).toJson(QJsonDocument::Indented));
    
    onRedraw();
}

void DetectionOilDialog::onConfidenceChanged(int value)
{
    confidenceThreshold = value / 100.0;
    m_renderer->setConfidenceThreshold(confidenceThreshold);
    onRedraw();
}

/**
 * @brief Создаёт изображение с наложенными полигонами детекции
 */
QPixmap DetectionOilDialog::renderImageWithOverlay() const
{
    QPixmap original(imagePath);
    if (original.isNull())
    {
        return QPixmap();
    }

    // Устанавливаем режим отрисовки
    QString mode = drawModeCombo->currentText();
    if (mode == "Рисовать подписи")
    {
        m_renderer->setDrawMode(DetectionRenderer::DrawMode::Labels);
    }
    else
    {
        m_renderer->setDrawMode(DetectionRenderer::DrawMode::Shapes);
    }

    return m_renderer->renderImage(original, predictions);
}

void DetectionOilDialog::onRedraw()
{
    QPixmap result = renderImageWithOverlay();
    if (result.isNull())
    {
        QMessageBox::critical(this, "Ошибка", "Не удалось загрузить изображение.");
        return;
    }
    imageLabel->setPixmap(result.scaled(imageLabel->size(), Qt::KeepAspectRatio, Qt::SmoothTransformation));
}

void DetectionOilDialog::onAddLayerToProject()
{
    if (!m_geoTransform.isValid())
    {
        QMessageBox::warning(this, "Ошибка", "Геопривязка отсутствует.");
        return;
    }

    QFileInfo pngInfo(imagePath);
    QString baseName = pngInfo.baseName().section('_', 0, -2);
    QString defaultPath = pngInfo.path() + "/" + baseName + "_oil_spills.geojson";

    QString geojsonPath = QFileDialog::getSaveFileName(this, "Сохранить слой", defaultPath, "GeoJSON Files (*.geojson)");
    if (geojsonPath.isEmpty())
    {
        return;
    }

    if (m_exporter->exportToGeoJson(geojsonPath, predictions, m_geoTransform))
    {
        // Сохраняем картинку со слоем поверх для отчёта
        QString overlayPath = QFileInfo(geojsonPath).absolutePath() + "/" + QFileInfo(geojsonPath).completeBaseName() + "_overlay.png";
        QPixmap overlayPix = renderImageWithOverlay();
        
        if (!overlayPix.isNull() && overlayPix.save(overlayPath))
        {
            emit detectionImageSaved(overlayPath);
        }

        emit layerSaved(geojsonPath);

        QMessageBox::information(this, "Выполнено", "GeoJSON слой сохранен:\n" + geojsonPath);
    }
    else
    {
        QMessageBox::critical(this, "Ошибка", "Не удалось сохранить GeoJSON файл.");
    }
}
