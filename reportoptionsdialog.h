#ifndef REPORTOPTIONSDIALOG_H
#define REPORTOPTIONSDIALOG_H

#include <QCheckBox>
#include <QDateTime>
#include <QDialog>
#include <QLabel>
#include <QLineEdit>
#include <QDialogButtonBox>
#include <QFileDialog>
#include <QFormLayout>
#include <QGridLayout>
#include <QGroupBox>
#include <QHBoxLayout>
#include <QPushButton>
#include <QScrollArea>

/**
 * @brief Класс реализует диалоговое окно выбора параметров для составления отчёта
 *
 * Позволяет включать/отключать параметры через чекбоксы:
 * - ID снимка (редактируемый, отключаемый для ручных полигонов)
 * - Координаты (охват слоя и центр)
 * - Периметр
 * - Площадь
 * - Время
 * - Фотографии (vv.png, rgb.png)
 * - Прогноз разлива
 */
class ReportOptionsDialog : public QDialog
{
    Q_OBJECT

public:
    /**
     * @brief Конструктор диалогового окна
     * @param parent Родительский виджет (по умолчанию nullptr)
     */
    explicit ReportOptionsDialog(QWidget *parent = nullptr);

    /**
     * @brief Установка ID снимка
     * @param id Идентификатор спутникового снимка
     */
    void setImageId(const QString &id);
    
    /**
     * @brief Установка координат
     * @param extent Область охвата слоя
     * @param center Центральные координаты
     */
    void setCoordinates(const QString &extent, const QString &center);
    
    /**
     * @brief Установка периметра пятна
     * @param meters Периметр в метрах
     */
    void setPerimeter(double meters);
    
    /**
     * @brief Установка площади пятна
     * @param sqMeters Площадь в квадратных метрах
     */
    void setArea(double sqMeters);
    
    /**
     * @brief Установка временной метки
     * @param dt Время обнаружения пятна
     */
    void setTimestamp(const QDateTime &dt);
    
    /**
     * @brief Установка пути к оригинальному снимку Sentinel
     * @param path Путь к файлу vv.png
     */
    void setSentinelImageOriginalPath(const QString &path);
    
    /**
     * @brief Установка пути к снимку с полигоном
     * @param path Путь к файлу rgb.png
     */
    void setSentinelImagePolyPath(const QString &path);
    
    /**
     * @brief Установка пути к изображению с детекцией
     * @param path Путь к файлу с обнаруженным пятном
     */
    void setDetectionOverlayPath(const QString &path);
    
    /**
     * @brief Установка путей к изображениям прогноза разлива
     * @param paths Список путей к файлам spill_0, spill_24, spill_48, spill_72
     */
    void setSpillForecastImages(const QStringList &paths);

    /**
     * @brief Проверка использования ID снимка
     * @return true если ID снимка включён в отчёт
     */
    bool isImageIdEnabled() const;
    
    /**
     * @brief Проверка использования координат
     * @return true если координаты включены в отчёт
     */
    bool isCoordinatesEnabled() const;
    
    /**
     * @brief Проверка использования периметра
     * @return true если периметр включён в отчёт
     */
    bool isPerimeterEnabled() const;
    
    /**
     * @brief Проверка использования площади
     * @return true если площадь включена в отчёт
     */
    bool isAreaEnabled() const;
    
    /**
     * @brief Проверка использования временной метки
     * @return true если временная метка включена в отчёт
     */
    bool isTimestampEnabled() const;
    
    /**
     * @brief Проверка использования фотографий
     * @return true если фотографии включены в отчёт
     */
    bool isPhotosEnabled() const;
    
    /**
     * @brief Проверка использования прогноза разлива
     * @return true если прогноз разлива включён в отчёт
     */
    bool isSpillForecastEnabled() const;
    
    /**
     * @brief Проверка использования изображения с детекцией
     * @return true если изображение с детекцией включено в отчёт
     */
    bool isDetectionOverlayEnabled() const;
    
    /**
     * @brief Получение ID снимка из формы
     * @return ID снимка введённый пользователем
     */
    QString imageId() const;
    
    /**
     * @brief Получение текста области охвата
     * @return Текст с координатами области
     */
    QString extentText() const;
    
    /**
     * @brief Получение текста центральных координат
     * @return Текст с центральными координатами
     */
    QString centerText() const;
    
    /**
     * @brief Получение периметра из формы
     * @return Периметр в метрах
     */
    double perimeter() const;
    
    /**
     * @brief Получение площади из формы
     * @return Площадь в квадратных метрах
     */
    double area() const;
    
    /**
     * @brief Получение временной метки из формы
     * @return Время обнаружения пятна
     */
    QDateTime timestamp() const;

    /**
     * @brief Получение пути к оригинальному снимку Sentinel
     * @return Путь к файлу vv.png или оригиналу
     */
    QString sentinelImageOriginalPath() const;
    
    /**
     * @brief Получение пути к снимку с полигоном
     * @return Путь к файлу rgb.png или с полигоном
     */
    QString sentinelImagePolyPath() const;
    
    /**
     * @brief Получение пути к изображению с детекцией/прогнозом
     * @return Путь к файлу с результатами детекции
     */
    QString detectionOverlayPath() const;
    
    /**
     * @brief Получение путей к изображениям прогноза разлива
     * @return Список путей к файлам spill_0, spill_24, spill_48, spill_72
     */
    QStringList spillForecastImages() const;
    
    /**
     * @brief Получение пути для сохранения PDF отчёта
     * @return Путь к выходному PDF файлу
     */
    QString outputFilePath() const;

private:
    /**
     * @brief Настройка пользовательского интерфейса диалога
     */
    void setupUI();

    QCheckBox *cbImageId = nullptr;             // Чекбокс включения ID снимка
    QCheckBox *cbCoordinates = nullptr;         // Чекбокс включения координат
    QCheckBox *cbPerimeter = nullptr;           // Чекбокс включения периметра
    QCheckBox *cbArea = nullptr;                // Чекбокс включения площади
    QCheckBox *cbTimestamp = nullptr;           // Чекбокс включения временной метки
    QCheckBox *cbPhotos = nullptr;              // Чекбокс включения фотографий
    QCheckBox *cbSpillForecast = nullptr;       // Чекбокс включения прогноза разлива
    QCheckBox *cbDetectionOverlay = nullptr;    // Чекбокс включения детекции
    
    QLineEdit *leImageId = nullptr;             // Поле ввода ID снимка
    QLabel *lbExtent = nullptr;                 // Метка области охвата
    QLabel *lbCenter = nullptr;                 // Метка центральных координат
    QLabel *lbPerimeter = nullptr;              // Метка периметра
    QLabel *lbArea = nullptr;                   // Метка площади
    QLabel *lbTimestamp = nullptr;              // Метка временной метки

    QLineEdit *leVvPath = nullptr;              // Поле пути к vv.png
    QLineEdit *leRgbPath = nullptr;             // Поле пути к rgb.png
    QLineEdit *leDetectionOverlayPath = nullptr; // Поле пути к детекции
    QLineEdit *leSpill0Path = nullptr;          // Поле пути к spill_0.png
    QLineEdit *leSpill24Path = nullptr;         // Поле пути к spill_24.png
    QLineEdit *leSpill48Path = nullptr;         // Поле пути к spill_48.png
    QLineEdit *leSpill72Path = nullptr;         // Поле пути к spill_72.png
    QLineEdit *leOutputPath = nullptr;          // Поле пути к выходному PDF
    
    QString m_extentText;                       // Текст области охвата
    QString m_centerText;                       // Текст центральных координат
    double m_perimeterVal = 0;                  // Значение периметра
    double m_areaVal = 0;                       // Значение площади
    QDateTime m_timestamp;                      // Временная метка
};

#endif // REPORTOPTIONSDIALOG_H
