#ifndef SARPROCESS_H
#define SARPROCESS_H

#include <QObject>
#include <QString>
#include <QSize>
#include <QDateTime>
#include <QVector>
#include <QDir>
#include <QProcess>
#include <QFileInfo>
#include <QImage>
#include <QtMath>
#include <algorithm>
#include <vector>
#include <gdal.h>
#include <gdal_priv.h>

/**
 * @struct SARProcessingResult
 * @brief Структура для хранения результатов обработки SAR-данных
 * 
 * Содержит информацию об успешности обработки, пути к выходным файлам,
 * геоданные и метаданные об изображении.
 */
struct SARProcessingResult 
{
    bool success = false;        // Флаг успешности обработки
    QString error;               // Текстовое описание ошибки при неудаче
    QString finalOut;            // Путь к финальному RGB GeoTIFF в формате COG
    QString vvOut;               // Путь к VV-каналу в формате TIFF
    QString outputPathRgb;       // Путь к PNG-превью для UI (RGB)
    QString outputPathVv;        // Путь к PNG-превью для UI (VV)
    QString basename;            // Базовое имя файла без расширения

    QVector<double> geotransform; // Геотрансформация (6 элементов GDAL)
    QSize geotiffSize;            // Размер изображения в пикселях
    QDateTime imageDataTime;      // Время получения данных спутником
};
Q_DECLARE_METATYPE(SARProcessingResult) 

/**
 * @class SarProcess
 * @brief Класс для обработки SAR-данных в отдельном потоке
 * 
 * Класс предназначен для обработки спутниковых радарных данных Sentinel-1.
 * Поддерживает два режима работы:
 * - ModePreprocessed: обработка предварительно обработанных GeoTIFF
 * - ModeRawSAFE: обработка исходных SAFE-пакетов с использованием SNAP
 * 
 * Класс работает в отдельном потоке и сообщает о завершении через сигнал finished.
 */
class SarProcess : public QObject
{
    Q_OBJECT

public:
    /**
     * @enum Mode
     * @brief Режимы обработки SAR-данных
     */
    enum Mode 
    { 
        ModePreprocessed,   // Обработка предварительно обработанного TIFF
        ModeRawSAFE         // Обработка исходного SAFE-пакета
    };
    Q_ENUM(Mode)

    /**
     * @brief Конструктор класса SarProcess
     * @param parent Родительский объект (по умолчанию nullptr)
     */
    explicit SarProcess(QObject *parent = nullptr);
    
    /**
     * @brief Деструктор класса
     */
    ~SarProcess() override = default;

    /**
     * @brief Установка пути к SNAP
     * @param path Путь к директории установки SNAP
     * 
     * Метод должен вызываться до запуска обработки в главном потоке.
     */
    void setSnapPath(const QString &path) { m_snapPath = path; }
    
    /**
     * @brief Установка API ключа Roboflow
     * @param key API ключ для Roboflow
     * 
     * Метод должен вызываться до запуска обработки в главном потоке.
     */
    void setRoboflowApiKey(const QString &key) { m_roboflowApiKey = key; }
    
    /**
     * @brief Установка исходной директории
     * @param dir Путь к исходной директории с данными
     * 
     * Метод должен вызываться до запуска обработки в главном потоке.
     */
    void setSourceDir(const QString &dir) { m_sourceDir = dir; }
    
    /**
     * @brief Установка целевой системы координат
     * @param crs Целевая система координат (по умолчанию "EPSG:3857")
     * 
     * Метод должен вызываться до запуска обработки в главном потоке.
     */
    void setTargetCrs(const QString &crs) { m_targetCrs = crs; }

public slots:
    /**
     * @brief Запуск обработки SAR-данных
     * @param inputPath Путь к входному файлу (TIFF или SAFE)
     * @param mode Режим обработки
     * 
     * Метод вызывается из MainWindow через invokeMethod для выполнения в отдельном потоке. 
     * По завершении генерирует сигнал finished.
     */
    void process(const QString &inputPath, Mode mode);

signals:
    /**
     * @brief Сигнал завершения обработки
     * @param result Результат обработки SAR-данных
     * 
     * Генерируется по завершении обработки данных в отдельном потоке.
     */
    void finished(const SARProcessingResult &result);

private:
    /**
     * @brief Обработка предварительно обработанного TIFF-файла
     * @param tiffPath Путь к TIFF-файлу
     * @return Результат обработки
     */
    SARProcessingResult processPreprocessedTiff(const QString &tiffPath);
    
    /**
     * @brief Обработка исходного SAFE-пакета
     * @param safePath Путь к SAFE-пакету
     * @return Результат обработки
     */
    SARProcessingResult processRawSAFE(const QString &safePath);
    
    /**
     * @brief Конвертация GeoTIFF в PNG для предпросмотра
     * @param inputPath Путь к входному GeoTIFF
     * @param outputPath Путь к выходному PNG
     * @param gamma Гамма-коррекция (по умолчанию 2.0)
     * @return true при успешной конвертации
     */
    bool convertGeoTiffToPNG(const QString &inputPath, const QString &outputPath, float gamma = 2.0f);
    
    /**
     * @brief Извлечение даты из имени файла
     * @param filename Имя файла
     * @return Дата получения данных
     */
    QDateTime parseDateFromFilename(const QString &filename);
    
    /**
     * @brief Запуск GPT утилиты SNAP
     * @param gptPath Путь к gpt.exe
     * @param args Аргументы командной строки
     * @return true при успешном выполнении
     */
    bool runGpt(const QString &gptPath, const QStringList &args);

    QString m_snapPath;                 // Путь к директории SNAP
    QString m_roboflowApiKey;           // API ключ Roboflow
    QString m_sourceDir;                // Исходная директория
    QString m_targetCrs = "EPSG:3857";  // Целевая система координат
};

#endif // SARPROCESS_H
