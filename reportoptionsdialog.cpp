#include "reportoptionsdialog.h"

// Конструктор диалогового окна настроек отчёта
ReportOptionsDialog::ReportOptionsDialog(QWidget *parent)
    : QDialog(parent)
{
    setWindowTitle(tr("Параметры отчёта"));
    setMinimumWidth(500);
    setMinimumHeight(400);
    resize(550, 600);
    setupUI();
}

// Создание пользовательского интерфейса диалогового окна
void ReportOptionsDialog::setupUI()
{
    QVBoxLayout *mainLayout = new QVBoxLayout(this);

    // Создаем виджет с прокруткой для списка опций
    QScrollArea *scrollArea = new QScrollArea(this);
    scrollArea->setWidgetResizable(true);
    scrollArea->setHorizontalScrollBarPolicy(Qt::ScrollBarAsNeeded);
    scrollArea->setVerticalScrollBarPolicy(Qt::ScrollBarAsNeeded);

    // Создаем контейнер для всего содержимого
    QWidget *contentWidget = new QWidget();
    QVBoxLayout *contentLayout = new QVBoxLayout(contentWidget);

    // Группа файлов и сохранения
    QGroupBox *gbFiles = new QGroupBox(tr("Файлы и сохранение"));
    QFormLayout *flFiles = new QFormLayout(gbFiles);

    // Поле для выбора пути сохранения PDF файла
    leOutputPath = new QLineEdit;
    leOutputPath->setPlaceholderText(tr("Путь для сохранения PDF"));
    QPushButton *pbBrowseOutput = new QPushButton(tr("Обзор..."));
    
    // Обработчик кнопки выбора файла сохранения PDF
    connect(pbBrowseOutput, &QPushButton::clicked, this, [this]()
    {
        QString path = QFileDialog::getSaveFileName(this, tr("Сохранить отчёт"), QDir::homePath(), tr("PDF (*.pdf)"));
        if (!path.isEmpty())
        {
            leOutputPath->setText(path);
        }
    });

    QHBoxLayout *hlOutput = new QHBoxLayout;
    hlOutput->addWidget(leOutputPath);
    hlOutput->addWidget(pbBrowseOutput);
    flFiles->addRow(tr("Сохранить как:"), hlOutput);

    cbPhotos = new QCheckBox(tr("Включить спутниковые снимки"));
    cbPhotos->setChecked(true);
    flFiles->addRow("", cbPhotos);

    leVvPath = new QLineEdit;
    leVvPath->setPlaceholderText(tr("S1...vv.png — VV поляризация"));
    QPushButton *pbBrowseVv = new QPushButton(tr("Обзор..."));
    connect(pbBrowseVv, &QPushButton::clicked, this, [this]()
    {
        QString path = QFileDialog::getOpenFileName(this, tr("Выбрать VV снимок"), QDir::homePath(), tr("Изображения (*.png *.jpg *.tif *.tiff)"));
        if (!path.isEmpty())
        {
            leVvPath->setText(path);
        }
    });

    QHBoxLayout *hlVv = new QHBoxLayout;
    hlVv->addWidget(leVvPath);
    hlVv->addWidget(pbBrowseVv);
    flFiles->addRow(tr("VV снимок:"), hlVv);

    leRgbPath = new QLineEdit;
    leRgbPath->setPlaceholderText(tr("S1...rgb.png — RGB композит"));
    QPushButton *pbBrowseRgb = new QPushButton(tr("Обзор..."));
    connect(pbBrowseRgb, &QPushButton::clicked, this, [this]()
    {
        QString path = QFileDialog::getOpenFileName(this, tr("Выбрать RGB снимок"), QDir::homePath(), tr("Изображения (*.png *.jpg *.tif *.tiff)"));
        if (!path.isEmpty())
        {
            leRgbPath->setText(path);
        }
    });
    QHBoxLayout *hlRgb = new QHBoxLayout;
    hlRgb->addWidget(leRgbPath);
    hlRgb->addWidget(pbBrowseRgb);
    flFiles->addRow(tr("RGB снимок:"), hlRgb);

    leDetectionOverlayPath = new QLineEdit;
    leDetectionOverlayPath->setPlaceholderText(tr("Изображение с обнаруженным нефтяным пятном"));
    QPushButton *pbBrowseDetection = new QPushButton(tr("Обзор..."));
    connect(pbBrowseDetection, &QPushButton::clicked, this, [this]()
    {
        QString path = QFileDialog::getOpenFileName(this, tr("Выбрать изображение с обнаруженным пятном"), QDir::homePath(), tr("Изображения (*.png *.jpg *.tif *.tiff)"));
        if (!path.isEmpty())
        {
            leDetectionOverlayPath->setText(path);
        }
    });
    QHBoxLayout *hlDetection = new QHBoxLayout;
    hlDetection->addWidget(leDetectionOverlayPath);
    hlDetection->addWidget(pbBrowseDetection);
    flFiles->addRow(tr("Слой обнаружения:"), hlDetection);

    cbDetectionOverlay = new QCheckBox(tr("Включить обнаруженное пятно"));
    cbDetectionOverlay->setChecked(true);
    flFiles->addRow("", cbDetectionOverlay);

    contentLayout->addWidget(gbFiles);

    // Группа для прогноза распространения нефтяного пятна
    QGroupBox *gbSpill = new QGroupBox(tr("Прогноз распространения нефтяного пятна"));
    QVBoxLayout *vlSpill = new QVBoxLayout(gbSpill);

    // Чекбокс включения прогноза разлива
    cbSpillForecast = new QCheckBox(tr("Включить прогноз (spill_0, spill_24, spill_48, spill_72)"));
    cbSpillForecast->setChecked(true);
    vlSpill->addWidget(cbSpillForecast);

    // Создаем сетку 2x2 для компактного отображения изображений прогноза
    QGridLayout *gridSpill = new QGridLayout();
    int row = 0;
    
    leSpill0Path = new QLineEdit;
    leSpill0Path->setPlaceholderText(tr("spill_0.png — 0 часов"));
    QPushButton *pbBrowseSpill0 = new QPushButton(tr("Обзор..."));
    connect(pbBrowseSpill0, &QPushButton::clicked, this, [this]()
    {
        QString path = QFileDialog::getOpenFileName(this, tr("Выбрать spill_0.png"), QDir::homePath(), tr("Изображения (*.png *.jpg *.tif *.tiff)"));
        if (!path.isEmpty())
        {
            leSpill0Path->setText(path);
        }
    });
    QHBoxLayout *hlSpill0 = new QHBoxLayout;
    hlSpill0->addWidget(leSpill0Path);
    hlSpill0->addWidget(pbBrowseSpill0);
    gridSpill->addWidget(new QLabel(tr("0 ч:")), row, 0);
    gridSpill->addLayout(hlSpill0, row, 1);
    row++;
    
    leSpill24Path = new QLineEdit;
    leSpill24Path->setPlaceholderText(tr("spill_24.png — 24 часа"));
    QPushButton *pbBrowseSpill24 = new QPushButton(tr("Обзор..."));
    connect(pbBrowseSpill24, &QPushButton::clicked, this, [this]()
    {
        QString path = QFileDialog::getOpenFileName(this, tr("Выбрать spill_24.png"), QDir::homePath(), tr("Изображения (*.png *.jpg *.tif *.tiff)"));
        if (!path.isEmpty())
        {
            leSpill24Path->setText(path);
        }
    });
    QHBoxLayout *hlSpill24 = new QHBoxLayout;
    hlSpill24->addWidget(leSpill24Path);
    hlSpill24->addWidget(pbBrowseSpill24);
    gridSpill->addWidget(new QLabel(tr("24 ч:")), row, 0);
    gridSpill->addLayout(hlSpill24, row, 1);
    row++;

    leSpill48Path = new QLineEdit;
    leSpill48Path->setPlaceholderText(tr("spill_48.png — 48 часов"));
    QPushButton *pbBrowseSpill48 = new QPushButton(tr("Обзор..."));
    connect(pbBrowseSpill48, &QPushButton::clicked, this, [this]()
    {
        QString path = QFileDialog::getOpenFileName(this, tr("Выбрать spill_48.png"), QDir::homePath(), tr("Изображения (*.png *.jpg *.tif *.tiff)"));
        if (!path.isEmpty())
        {
            leSpill48Path->setText(path);
        }
    });
    QHBoxLayout *hlSpill48 = new QHBoxLayout;
    hlSpill48->addWidget(leSpill48Path);
    hlSpill48->addWidget(pbBrowseSpill48);
    gridSpill->addWidget(new QLabel(tr("48 ч:")), row, 0);
    gridSpill->addLayout(hlSpill48, row, 1);
    row++;

    leSpill72Path = new QLineEdit;
    leSpill72Path->setPlaceholderText(tr("spill_72.png — 72 часа"));
    QPushButton *pbBrowseSpill72 = new QPushButton(tr("Обзор..."));
    connect(pbBrowseSpill72, &QPushButton::clicked, this, [this]()
    {
        QString path = QFileDialog::getOpenFileName(this, tr("Выбрать spill_72.png"), QDir::homePath(), tr("Изображения (*.png *.jpg *.tif *.tiff)"));
        if (!path.isEmpty())
        {
            leSpill72Path->setText(path);
        }
    });
    QHBoxLayout *hlSpill72 = new QHBoxLayout;
    hlSpill72->addWidget(leSpill72Path);
    hlSpill72->addWidget(pbBrowseSpill72);
    gridSpill->addWidget(new QLabel(tr("72 ч:")), row, 0);
    gridSpill->addLayout(hlSpill72, row, 1);

    vlSpill->addLayout(gridSpill);
    contentLayout->addWidget(gbSpill);

    // Группа параметров отчёта с деталями пятна
    QGroupBox *gbParams = new QGroupBox(tr("Параметры отчёта"));
    QFormLayout *flParams = new QFormLayout(gbParams);

    // Опции включения различных полей в отчёт
    cbImageId = new QCheckBox(tr("Включить ID снимка"));
    cbImageId->setChecked(true);
    flParams->addRow("", cbImageId);

    leImageId = new QLineEdit;
    leImageId->setPlaceholderText(tr("S1A_IW_GRDH_1SDV_20250208T062731_..."));
    flParams->addRow(tr("ID снимка:"), leImageId);

    cbCoordinates = new QCheckBox(tr("Включить координаты"));
    cbCoordinates->setChecked(true);
    flParams->addRow("", cbCoordinates);

    lbExtent = new QLabel(tr("-"));
    lbExtent->setWordWrap(false);
    flParams->addRow(tr("Охват слоя:"), lbExtent);

    lbCenter = new QLabel(tr("—"));
    lbCenter->setWordWrap(true);
    flParams->addRow(tr("Центр полигона:"), lbCenter);

    cbPerimeter = new QCheckBox(tr("Включить периметр"));
    cbPerimeter->setChecked(true);
    flParams->addRow("", cbPerimeter);

    lbPerimeter = new QLabel(tr("0 м"));
    flParams->addRow(tr("Периметр:"), lbPerimeter);

    cbArea = new QCheckBox(tr("Включить площадь"));
    cbArea->setChecked(true);
    flParams->addRow("", cbArea);

    lbArea = new QLabel(tr("0 м²"));
    flParams->addRow(tr("Площадь:"), lbArea);

    cbTimestamp = new QCheckBox(tr("Включить время"));
    cbTimestamp->setChecked(true);
    flParams->addRow("", cbTimestamp);

    lbTimestamp = new QLabel(tr("—"));
    flParams->addRow(tr("Время:"), lbTimestamp);

    contentLayout->addWidget(gbParams);

    contentLayout->addStretch();

    // Устанавливаем виджет с содержимым в область прокрутки
    scrollArea->setWidget(contentWidget);
    mainLayout->addWidget(scrollArea);

    // Кнопки OK / Cancel
    QDialogButtonBox *buttonBox = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel);
    connect(buttonBox, &QDialogButtonBox::accepted, this, &QDialog::accept);
    connect(buttonBox, &QDialogButtonBox::rejected, this, &QDialog::reject);
    mainLayout->addWidget(buttonBox);
}

void ReportOptionsDialog::setImageId(const QString &id)
{
    if (leImageId)
    {
        leImageId->setText(id);
    }
}

void ReportOptionsDialog::setCoordinates(const QString &extent, const QString &center)
{
    m_extentText = extent;
    m_centerText = center;
    if (lbExtent)
    {
        lbExtent->setText(extent.isEmpty() ? tr("—") : extent);
    }
    if (lbCenter)
    {
        lbCenter->setText(center.isEmpty() ? tr("—") : center);
    }
}

void ReportOptionsDialog::setPerimeter(double meters)
{
    m_perimeterVal = meters;
    if (lbPerimeter)
    {
        lbPerimeter->setText(QString::number(meters, 'f', 2) + " " + tr("м"));
    }
}

void ReportOptionsDialog::setArea(double sqMeters)
{
    m_areaVal = sqMeters;
    if (lbArea)
    {
        if (sqMeters >= 1e6)
        {   
            lbArea->setText(QString::number(sqMeters / 1e6, 'f', 3) + " " + tr("км²"));
        }
        else
        {
            lbArea->setText(QString::number(sqMeters, 'f', 1) + " " + tr("м²"));
        }
    }
}

void ReportOptionsDialog::setTimestamp(const QDateTime &dt)
{
    m_timestamp = dt;
    if (lbTimestamp)
    {
        lbTimestamp->setText(dt.isValid() ? dt.toString("dd.MM.yyyy HH:mm:ss") : tr("—"));
    }
}

void ReportOptionsDialog::setSentinelImageOriginalPath(const QString &path)
{
    if (leVvPath)
    {
        leVvPath->setText(path);
    }
}

void ReportOptionsDialog::setSentinelImagePolyPath(const QString &path)
{
    if (leRgbPath)
    {
        leRgbPath->setText(path);
    }
}

void ReportOptionsDialog::setDetectionOverlayPath(const QString &path)
{
    if (leDetectionOverlayPath)
    {
        leDetectionOverlayPath->setText(path);
    }
}

bool ReportOptionsDialog::isImageIdEnabled() const
{
    return cbImageId && cbImageId->isChecked();
}

bool ReportOptionsDialog::isCoordinatesEnabled() const
{
    return cbCoordinates && cbCoordinates->isChecked();
}

bool ReportOptionsDialog::isPerimeterEnabled() const
{
    return cbPerimeter && cbPerimeter->isChecked();
}

bool ReportOptionsDialog::isAreaEnabled() const
{
    return cbArea && cbArea->isChecked();
}

bool ReportOptionsDialog::isTimestampEnabled() const
{
    return cbTimestamp && cbTimestamp->isChecked();
}

bool ReportOptionsDialog::isPhotosEnabled() const
{
    return cbPhotos && cbPhotos->isChecked();
}

QString ReportOptionsDialog::imageId() const
{
    return leImageId ? leImageId->text().trimmed() : QString();
}

QString ReportOptionsDialog::extentText() const
{
    return m_extentText;
}

QString ReportOptionsDialog::centerText() const
{
    return m_centerText;
}

double ReportOptionsDialog::perimeter() const
{
    return m_perimeterVal;
}

double ReportOptionsDialog::area() const
{
    return m_areaVal;
}

QDateTime ReportOptionsDialog::timestamp() const
{
    return m_timestamp;
}

QString ReportOptionsDialog::sentinelImageOriginalPath() const
{
    return leVvPath ? leVvPath->text().trimmed() : QString();
}

QString ReportOptionsDialog::sentinelImagePolyPath() const
{
    return leRgbPath ? leRgbPath->text().trimmed() : QString();
}

QString ReportOptionsDialog::detectionOverlayPath() const
{
    return leDetectionOverlayPath ? leDetectionOverlayPath->text().trimmed() : QString();
}

QString ReportOptionsDialog::outputFilePath() const
{
    return leOutputPath ? leOutputPath->text().trimmed() : QString();
}

void ReportOptionsDialog::setSpillForecastImages(const QStringList &paths)
{
    // Устанавливаем пути для каждого временного этапа (0, 24, 48, 72 часа)
    if (paths.size() >= 1 && leSpill0Path)
    {
        leSpill0Path->setText(paths[0]);
    }
    if (paths.size() >= 2 && leSpill24Path)
    {
        leSpill24Path->setText(paths[1]);
    }
    if (paths.size() >= 3 && leSpill48Path)
    {
        leSpill48Path->setText(paths[2]);
    }
    if (paths.size() >= 4 && leSpill72Path)
    {
        leSpill72Path->setText(paths[3]);
    }
}

bool ReportOptionsDialog::isSpillForecastEnabled() const
{
    return cbSpillForecast && cbSpillForecast->isChecked();
}

bool ReportOptionsDialog::isDetectionOverlayEnabled() const
{
    return cbDetectionOverlay && cbDetectionOverlay->isChecked();
}

// Получение списка путей к изображениям прогноза разлива
QStringList ReportOptionsDialog::spillForecastImages() const
{
    QStringList paths;
    if (leSpill0Path && !leSpill0Path->text().trimmed().isEmpty())
    {
        paths << leSpill0Path->text().trimmed();
    }
    if (leSpill24Path && !leSpill24Path->text().trimmed().isEmpty())
    {
        paths << leSpill24Path->text().trimmed();
    }
    if (leSpill48Path && !leSpill48Path->text().trimmed().isEmpty())
    {
        paths << leSpill48Path->text().trimmed();
    }
    if (leSpill72Path && !leSpill72Path->text().trimmed().isEmpty())
    {
        paths << leSpill72Path->text().trimmed();
    }
    return paths;
}
