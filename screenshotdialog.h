#ifndef SCREENSHOTDIALOG_H
#define SCREENSHOTDIALOG_H

#include <QCheckBox>
#include <QLineEdit>
#include <QDateTime>
#include <QFileDialog>
#include <QFormLayout>
#include <QPushButton>
#include <QSpinBox>
#include <QVBoxLayout>

/**
 * @brief Структура параметров скриншота
 *
 * Содержит настройки для создания скриншота карты:
 * - Разрешение (DPI)
 * - Флаг добавления масштаба к изображению
 * - Путь к файлу для сохранения
 */
struct ScreenshotOptions
{
    int dpi;            // Разрешение в DPI
    bool addScale;      // Добавлять ли масштаб к изображению
    QString filePath;   // Путь к файлу для сохранения
};

/**
 * @brief Класс диалогового окна создания скриншота карты
 *
 * Класс предоставляет интерфейс для настройки параметров создания скриншота:
 * - Выбор разрешения (DPI)
 * - Опция добавления масштаба к изображению
 * - Выбор пути для сохранения файла
 */
class ScreenshotDialog : public QDialog
{
    Q_OBJECT

public:
    /**
     * @brief Конструктор диалогового окна
     *
     * Инициализирует интерфейс с настройками по умолчанию.
     *
     * @param parent Родительский виджет
     */
    explicit ScreenshotDialog(QWidget* parent = nullptr);

    /**
     * @brief Получить параметры скриншота
     *
     * Возвращает структуру с настройками, выбранными пользователем.
     *
     * @return Структура ScreenshotOptions с параметрами
     */
    ScreenshotOptions getOptions() const;

private slots:
    /**
     * @brief Слот для выбора файла сохранения
     *
     * Открывает диалог выбора файла и обновляет поле пути.
     */
    void browseFile();

private:
    QSpinBox* mDpiSpinBox = nullptr;           // Спинбокс для выбора разрешения DPI
    QCheckBox* mAddScaleCheckBox = nullptr;    // Чекбокс добавления масштаба
    QLineEdit* mFilePathEdit = nullptr;        // Поле ввода пути к файлу
};

#endif // SCREENSHOTDIALOG_H
