#ifndef DETECTIONOILDIALOG_H
#define DETECTIONOILDIALOG_H

// Библиотеки Qt
#include <QDialog>
#include <QLabel>
#include <QSlider>
#include <QComboBox>
#include <QTextEdit>
#include <QPushButton>
#include <QHBoxLayout>
#include <QGroupBox>
#include <QMessageBox>
#include <QFileDialog>
#include <QTimer>
#include <QClipboard>
#include <QApplication>

#include "modelmanager.h"
#include "detectionrenderer.h"
#include "geojsonexporter.h"

/**
 * @brief Диалоговое окно для отображения результатов обнаружения разливов нефти
 *
 * Класс предоставляет интерфейс для визуализации результатов обнаружения разливов нефти
 * на спутниковых изображениях. Позволяет настраивать порог уверенности обнаружения,
 * переключать режимы отображения (подписи/формы) и экспортировать результат в GeoJSON.
 *
 * Основные возможности:
 * - Визуализация обнаруженных нефтяных пятен с настраиваемым порогом уверенности
 * - Два режима отображения: подписи и формаы
 * - Экспорт результатов в формате GeoJSON
 * - Просмотр JSON ответа от Roboflow API
 */
class DetectionOilDialog : public QDialog
{
    Q_OBJECT

public:
    /**
     * @brief Конструктор диалогового окна
     *
     * @param imagePath Путь к изображению для отображения
     * @param geoTransform Геотрансформация растра
     * @param rasterSize Размер исходного растра
     * @param parent Родительский виджет
     */
    explicit DetectionOilDialog(const QString &imagePath,
                                const QString &userApiKey,
                                const QVector<double> &geoTransform,
                                const QSize &rasterSize,
                                QWidget *parent = nullptr);

private slots:
    /**
     * @brief Слот обработки изменения порога уверенности
     * @param value Значение порога (0-100)
     */
    void onConfidenceChanged(int value);
    
    /**
     * @brief Слот перерисовки изображения с детекциями
     */
    void onRedraw();
    
    /**
     * @brief Слот экспорта результатов в GeoJSON формат
     */
    void onAddLayerToProject();
    
    /**
     * @brief Слот переключения модели детекции
     * @param index Индекс выбранной модели
     */
    void onSwitchModel(int index);
    
    /**
     * @brief Слот инициализации пользовательского интерфейса
     */
    void setupUI();
    
    /**
     * @brief Слот обработки ответа от модели
     * @param modelIndex Индекс модели
     * @param predictions Массив предсказаний
     */
    void onModelResponseReceived(int modelIndex, const QJsonArray &predictions);
    
    /**
     * @brief Слот обработки ошибки модели
     * @param modelIndex Индекс модели
     * @param error Текст ошибки
     */
    void onModelErrorOccurred(int modelIndex, const QString &error);
    
    /**
     * @brief Слот завершения всех запросов к моделям
     */
    void onAllModelsCompleted();

signals:
    /**
     * @brief Сигнал сохранения GeoJSON слоя
     * @param geojsonPath Путь к сохраненному файлу
     */
    void layerSaved(const QString &geojsonPath);
    
    /**
     * @brief Сигнал сохранения изображения с детекциями
     * @param imagePath Путь к сохраненному изображению
     */
    void detectionImageSaved(const QString &imagePath);

private:
    void initializeComponents();    // Инициализация компонентов
    void connectSignals();          // Подключение сигналов
    void sendRequestsToAllModels(); // Отправка запросов к моделям
    QPixmap renderImageWithOverlay() const; // Отрисовка изображения с наложением
    
    // UI компоненты
    QLabel *imageLabel;
    QSlider *confidenceSlider;
    QComboBox *drawModeCombo;
    QComboBox *modelCombo;
    QTextEdit *jsonView;
    QPushButton *copyButton;
    QPushButton *addLayerButton;
    
    // Данные
    QString imagePath;           // Путь к изображению
    QString mApiKey;             // API ключ Roboflow
    QJsonArray predictions;      // Массив предсказаний
    double confidenceThreshold;  // Порог уверенности
    GeoTransform m_geoTransform; // Геотрансформация
    
    // Компоненты
    ModelManager *m_modelManager;   // Менеджер моделей
    DetectionRenderer *m_renderer;  // Рендер
    GeoJsonExporter *m_exporter;    // Экспортер GeoJSON

};

#endif // DETECTIONOILDIALOG_H
