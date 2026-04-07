#ifndef OILSPREADANALYSISDIALOG_H
#define OILSPREADANALYSISDIALOG_H

#include <QDialog>
#include <QCheckBox>
#include <QVBoxLayout>
#include <QLabel>
#include <QLineEdit>
#include <QDateTimeEdit>
#include <QSpinBox>
#include <QDialogButtonBox>

class QLineEdit;
class QDateTimeEdit;
class QSpinBox;
class QPushButton;

/**
 * @class OilSpreadAnalysisDialog
 * @brief Диалоговое окно для настройки параметров анализа распространения нефти
 *
 * Предоставляет пользовательский интерфейс для настройки всех необходимых
 * параметров симуляции: пути к файлу разлива, времени начала, плотности частиц,
 * буферной зоны, разрешения береговой линии и шагов метеоданных.
 */
class OilSpreadAnalysisDialog : public QDialog
{
    Q_OBJECT

public:
    /**
     * @brief Конструктор диалогового окна
     * @param parent Родительский виджет
     */
    explicit OilSpreadAnalysisDialog(QWidget *parent = nullptr);

    /**
     * @brief Устанавливает путь к файлу разлива
     * @param value Путь к GeoJSON файлу
     */
    void setSpillGeoJsonPath(QString value);
    
    /**
     * @brief Устанавливает время начала разлива
     * @param value Время начала
     */
    void setStartTime(QDateTime value);
    
    /**
     * @brief Устанавливает плотность частиц
     * @param value Количество точек на км²
     */
    void setPointsPerKm2(int value);
    
    /**
     * @brief Устанавливает шаг метеоданных по широте
     * @param value Шаг в градусах
     */
    void setWeatherLatStep(double value);
    
    /**
     * @brief Устанавливает шаг метеоданных по долготе
     * @param value Шаг в градусах
     */
    void setWeatherLonStep(double value);

    /**
     * @brief Возвращает путь к файлу разлива
     * @return Путь к GeoJSON файлу
     */
    QString getSpillGeoJsonPath();
    
    /**
     * @brief Возвращает время начала разлива
     * @return Время начала
     */
    QDateTime getStartTime();
    
    /**
     * @brief Возвращает плотность частиц
     * @return Количество точек на км²
     */
    int getPointsPerKm2();
    
    /**
     * @brief Возвращает размер буферной зоны
     * @return Размер буферной зоны в метрах
     */
    double getByfferMeters();
    
    /**
     * @brief Возвращает флаг высокого разрешения береговой линии
     * @return true если используется высокое разрешение
     */
    bool getResLand();
    
    /**
     * @brief Возвращает шаг метеоданных по широте
     * @return Шаг в градусах
     */
    double getWeatherLatStep();
    
    /**
     * @brief Возвращает шаг метеоданных по долготе
     * @return Шаг в градусах
     */
    double getWeatherLonStep();

private:

    QLineEdit *m_spillPathEdit;             // Поле пути к файлу разлива
    QDateTimeEdit *m_startTimeEdit;         // Редактор времени начала разлива
    QSpinBox *m_pointsPerKm2Spin;           // Спинбокс плотности частиц
    QSpinBox *m_bufferMetersSpin;           // Спинбокс размера буферной зоны
    double bufferMeters;                    // Переменная для хранения размера буферной зоны
    QCheckBox *m_checkBoxResLand;           // Чекбокс высокого разрешения береговой линии
    QDoubleSpinBox *m_weatherLatStepSpin;   // Спинбокс шага метеоданных по широте
    QDoubleSpinBox *m_weatherLonStepSpin;   // Спинбокс шага метеоданных по долготе
};

#endif // OILSPREADANALYSISDIALOG_H
