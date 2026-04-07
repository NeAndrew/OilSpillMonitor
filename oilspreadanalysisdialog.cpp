#include "oilspreadanalysisdialog.h"

OilSpreadAnalysisDialog::OilSpreadAnalysisDialog(QWidget *parent)
    : QDialog(parent)
{
    setWindowTitle(tr("Параметры анализа разлива нефти"));

    m_spillPathEdit = new QLineEdit(this);
    m_spillPathEdit->setEnabled(0);

    m_startTimeEdit = new QDateTimeEdit(QDateTime::currentDateTime(), this);
    m_startTimeEdit->setDisplayFormat("dd.MM.yyyy HH:mm");
    m_startTimeEdit->setCalendarPopup(true);
    m_startTimeEdit->setMaximumDateTime(QDateTime::currentDateTime().addDays(10));

    m_pointsPerKm2Spin = new QSpinBox(this);
    m_pointsPerKm2Spin->setRange(1, 10);
    m_pointsPerKm2Spin->setValue(1); // значение по умолчанию

    m_bufferMetersSpin = new QSpinBox(this);
    m_bufferMetersSpin->setRange(100000, 1000000);
    m_bufferMetersSpin->setValue(100000); // значение по умолчанию

    m_checkBoxResLand = new QCheckBox(this);
    m_checkBoxResLand->setCheckState(Qt::CheckState::Checked);

    m_weatherLatStepSpin = new QDoubleSpinBox(this);
    m_weatherLatStepSpin->setRange(0.01, 1.0);
    m_weatherLatStepSpin->setValue(0.1);
    m_weatherLatStepSpin->setSingleStep(0.01);
    m_weatherLatStepSpin->setDecimals(2);

    m_weatherLonStepSpin = new QDoubleSpinBox(this);
    m_weatherLonStepSpin->setRange(0.01, 1.0);
    m_weatherLonStepSpin->setValue(0.1);
    m_weatherLonStepSpin->setSingleStep(0.01);
    m_weatherLonStepSpin->setDecimals(2);

    // Создание компоновки для элементов управления
    auto pathLayout = new QHBoxLayout;
    pathLayout->addWidget(new QLabel(tr("Файл разлива (GeoJSON):")));
    pathLayout->addWidget(m_spillPathEdit);

    auto timeLayout = new QHBoxLayout;
    timeLayout->addWidget(new QLabel(tr("Время начала:")));
    timeLayout->addWidget(m_startTimeEdit);

    auto densityLayout = new QHBoxLayout;
    densityLayout->addWidget(new QLabel(tr("Точек на км²:")));
    densityLayout->addWidget(m_pointsPerKm2Spin);

    auto bufferLayout = new QHBoxLayout;
    bufferLayout->addWidget(new QLabel(tr("Буферная зона метеоданных (м):")));
    bufferLayout->addWidget(m_bufferMetersSpin);

    auto checkLayout = new QHBoxLayout;
    checkLayout->addWidget(new QLabel(tr("Использовать береговую линию высокого разрешения")));
    checkLayout->addWidget(m_checkBoxResLand);

    auto weatherLatLayout = new QHBoxLayout;
    weatherLatLayout->addWidget(new QLabel(tr("Шаг метеоданных по широте (градусы):")));
    weatherLatLayout->addWidget(m_weatherLatStepSpin);

    auto weatherLonLayout = new QHBoxLayout;
    weatherLonLayout->addWidget(new QLabel(tr("Шаг метеоданных по долготе (градусы):")));
    weatherLonLayout->addWidget(m_weatherLonStepSpin);

    // Основная компоновка диалога
    auto mainLayout = new QVBoxLayout;
    mainLayout->addLayout(pathLayout);
    mainLayout->addLayout(timeLayout);
    mainLayout->addLayout(densityLayout);
    mainLayout->addLayout(bufferLayout);
    mainLayout->addLayout(checkLayout);
    mainLayout->addLayout(weatherLatLayout);
    mainLayout->addLayout(weatherLonLayout);

    // Добавление кнопок OK/Cancel
    auto buttonBox = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel, this);
    connect(buttonBox, &QDialogButtonBox::accepted, this, &QDialog::accept);
    connect(buttonBox, &QDialogButtonBox::rejected, this, &QDialog::reject);
    mainLayout->addWidget(buttonBox);

    setLayout(mainLayout);
    resize(500, 200);
}


// Геттеры и сеттеры для параметров диалога
QString OilSpreadAnalysisDialog::getSpillGeoJsonPath()
{
    return m_spillPathEdit->text();
}

QDateTime OilSpreadAnalysisDialog::getStartTime()
{
    return m_startTimeEdit->dateTime();
}

int OilSpreadAnalysisDialog::getPointsPerKm2()
{
    return m_pointsPerKm2Spin->value();
}

void OilSpreadAnalysisDialog::setSpillGeoJsonPath(QString value)
{
    m_spillPathEdit->setText(value);
}

void OilSpreadAnalysisDialog::setStartTime(QDateTime value)
{
    m_startTimeEdit->setDateTime(value);
}

void OilSpreadAnalysisDialog::setPointsPerKm2(int value)
{
    m_pointsPerKm2Spin->setValue(value);
}

double OilSpreadAnalysisDialog::getByfferMeters()
{
    return m_bufferMetersSpin->value();
}

bool OilSpreadAnalysisDialog::getResLand()
{
    return m_checkBoxResLand->isChecked();
}

void OilSpreadAnalysisDialog::setWeatherLatStep(double value)
{
    m_weatherLatStepSpin->setValue(value);
}

void OilSpreadAnalysisDialog::setWeatherLonStep(double value)
{
    m_weatherLonStepSpin->setValue(value);
}

double OilSpreadAnalysisDialog::getWeatherLatStep()
{
    return m_weatherLatStepSpin->value();
}

double OilSpreadAnalysisDialog::getWeatherLonStep()
{
    return m_weatherLonStepSpin->value();
}
