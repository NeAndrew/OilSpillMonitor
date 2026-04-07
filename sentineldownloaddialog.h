#pragma once

// Библиотеки Qt
#include <QUrlQuery>
#include <QStandardPaths>
#include <QToolButton>
#include <QPushButton>
#include <QLineEdit>
#include <QDateEdit>
#include <QDoubleSpinBox>
#include <QLabel>
#include <QProgressBar>
#include <QFileDialog>
#include <QGridLayout>
#include <QGroupBox>
#include <QMessageBox>
#include <QDialogButtonBox>
#include <QHeaderView>
#include <QMenu>
#include <QTableWidgetItem>
#include <QNetworkReply>
#include <QJsonArray>

// Библиотеки QGIS API
#include <qgsvectorlayer.h>
#include <qgsmapcanvas.h>
#include <qgsrubberband.h>

/**
 * @brief Класс диалогового окна для загрузки спутниковых снимков Sentinel-1
 *
 * Реализует интерфейс для поиска и загрузки данных Sentinel-1 через Copernicus API.
 * Предоставляет возможности:
 * - Ввод границ области в координатах Web Mercator (EPSG:3857)
 * - Интерактивный выбор области на карте QGIS
 * - Загрузка границ из GeoJSON/KML/SHP файлов
 * - Настройка периода съёмки
 * - Аутентификация в системе Copernicus
 * - Поиск и отображение доступных снимков
 * - Скачивание выбранных продуктов с отображением прогресса
 */
class SentinelDownloadDialog : public QDialog
{
    Q_OBJECT

public:
    /**
     * @brief Конструктор диалогового окна
     * @param parent Родительский виджет (по умолчанию nullptr)
     * @param copernicusLogin Логин для доступа к Copernicus Data Space
     * @param copernicusPassword Пароль для доступа к Copernicus Data Space
     * @param mapCanvas Указатель на карту QGIS для интерактивного выбора области
     */
    explicit SentinelDownloadDialog(QWidget *parent = nullptr,
                                    const QString &copernicusLogin = "",
                                    const QString &copernicusPassword = "",
                                    QgsMapCanvas *mapCanvas = nullptr);
    
    /**
     * @brief Деструктор
     */
    ~SentinelDownloadDialog() override;

public slots:
    /**
     * @brief Установка границ области из координат WGS84
     * @param minLon Минимальная долгота
     * @param minLat Минимальная широта
     * @param maxLon Максимальная долгота
     * @param maxLat Максимальная широта
     */
    void setBboxFromWGS84(double minLon, double minLat, double maxLon, double maxLat);

signals:
    /**
     * @brief Сигнал запроса интерактивного выбора области на карте
     */
    void requestMapSelection();
    
    /**
     * @brief Сигнал завершения выбора области на карте
     * @param bboxWGS84 Выбранные границы в координатах WGS84
     */
    void mapSelectionFinished(const QRectF &bboxWGS84);

private slots:
    /**
     * @brief Обработчик нажатия кнопки начала поиска/загрузки
     */
    void onStartClicked();
    
    /**
     * @brief Обработчик завершения аутентификации
     * @param success Флаг успешной аутентификации
     * @param token Токен доступа
     * @param message Сообщение об ошибке или успехе
     */
    void onAuthFinished(bool success, const QString &token, const QString &message);
    
    /**
     * @brief Обработчик прогресса скачивания файла
     * @param received Количество полученных байт
     * @param total Общий размер файла
     */
    void onDownloadProgress(qint64 received, qint64 total);
    
    /**
     * @brief Обработчик завершения скачивания файла
     * @param success Флаг успешного скачивания
     * @param path Путь к сохранённому файлу
     * @param msg Сообщение о результате
     */
    void onDownloadFinished(bool success, const QString &path, const QString &msg);
    
    /**
     * @brief Обработчик ошибок
     * @param err Текст ошибки
     */
    void onError(const QString &err);

    /**
     * @brief Обработчик нажатия кнопки загрузки границ
     */
    void onBboxLoadClicked();
    
    /**
     * @brief Загрузка границ из файла
     */
    void onBboxLoadFromFile();
    
    /**
     * @brief Загрузка границ из выбора на карте
     */
    void onBboxLoadFromMap();
    
    /**
     * @brief Отображение результатов поиска снимков
     */
    void showSearchResults();


private:
    /**
     * @brief Преобразование границ из EPSG:3857 в WGS84
     * @param minX_3857 Минимальная X в EPSG:3857
     * @param minY_3857 Минимальная Y в EPSG:3857
     * @param maxX_3857 Максимальная X в EPSG:3857
     * @param maxY_3857 Максимальная Y в EPSG:3857
     * @param outBboxWGS84 Выходные границы в WGS84
     * @return true при успешном преобразовании
     */
    bool transformBboxToWGS84(double minX_3857, double minY_3857,
                              double maxX_3857, double maxY_3857,
                              QRectF &outBboxWGS84);
    
    /**
     * @brief Преобразование прямоугольника QGIS в QRectF
     * @param rect Прямоугольник QGIS
     * @return Прямоугольник Qt
     */
    QRectF qgsRectToQRectF(const QgsRectangle &rect);
    
    /**
     * @brief Аутентификация в системе Copernicus
     * @param username Имя пользователя
     * @param password Пароль
     */
    void authenticate(const QString& username, const QString& password);
    
    /**
     * @brief Поиск данных Sentinel по параметрам
     */
    void searchSentinelData();
    
    /**
     * @brief Запрос на скачивание продукта
     * @param productId ID продукта
     * @param filename Имя файла для сохранения
     */
    void requestDownload(const QString& productId, const QString& filename);

    /**
     * @brief Обработчик ответа аутентификации
     * @param reply Сетевой ответ
     */
    void onAuthReplyFinished(QNetworkReply* reply);
    
    /**
     * @brief Обработчик ответа поиска
     * @param reply Сетевой ответ
     */
    void onSearchReplyFinished(QNetworkReply* reply);
    
    /**
     * @brief Обработчик ответа скачивания
     * @param reply Сетевой ответ
     */
    void onDownloadReplyFinished(QNetworkReply* reply);

    /**
     * @brief Построение фильтра OData для поиска
     * @param bboxWGS84 Границы в WGS84
     * @param start Дата начала
     * @param end Дата окончания
     * @return Строка фильтра OData
     */
    QString buildODataFilter(const QRectF& bboxWGS84, const QDate& start, const QDate& end);
    
    /**
     * @brief Очистка имени файла от недопустимых символов
     * @param name Исходное имя
     * @return Очищенное имя
     */
    QString sanitizeFilename(const QString& name);
    
    /**
     * @brief Загрузка границ из файла
     * @param filePath Путь к файлу
     * @param outBboxWGS84 Выходные границы в WGS84
     * @return true при успешной загрузке
     */
    bool loadBboxFromFile(const QString &filePath, QRectF &outBboxWGS84);
    
    /**
     * @brief Завершение выбора области на карте
     */
    void finishMapSelection();

    /**
     * @brief Обработчик нажатия клавиш в режиме выбора на карте
     * @param event Событие клавиатуры
     */
    void onMapKeyPress(QKeyEvent *event);
    
    /**
     * @brief Захват текущего экстента карты
     */
    void captureMapExtent();

    /**
     * @brief Переопределение события нажатия клавиши
     * @param event Событие клавиатуры
     */
    void keyPressEvent(QKeyEvent *event) override;


    /**
     * @brief Переопределение фильтра событий для перехвата клавиш
     * @param watched Наблюдаемый объект
     * @param event Событие
     * @return true если событие обработано
     */
    bool eventFilter(QObject *watched, QEvent *event) override;

    bool m_mapSelectionMode;        // Флаг режима выбора на карте
    QDoubleSpinBox *spinMinX;       // Поле ввода минимальной X координаты
    QDoubleSpinBox *spinMinY;       // Поле ввода минимальной Y координаты
    QDoubleSpinBox *spinMaxX;       // Поле ввода максимальной X координаты
    QDoubleSpinBox *spinMaxY;       // Поле ввода максимальной Y координаты
    QDateEdit *dateStart;           // Поле выбора даты начала
    QDateEdit *dateEnd;             // Поле выбора даты окончания
    QLineEdit *editSavePath;        // Поле пути сохранения
    QPushButton *btnStart;          // Кнопка начала поиска/загрузки
    QPushButton *btnBrowse;         // Кнопка выбора папки
    QLabel *labelStatus;            // Метка статуса операции
    QProgressBar *progressBar;      // Индикатор прогресса
    QToolButton *btnLoadBbox = nullptr; // Кнопка загрузки границ области
    QMenu *menuLoadBbox;            // Меню кнопки загрузки границ

    
    QNetworkAccessManager* m_networkManager;  // Менеджер сетевых запросов
    QString m_accessToken;                    // Токен доступа к API
    bool m_authSuccess;                       // Флаг успешной аутентификации
    QRectF m_pendingBbox;                     // Ожидаемые границы области
    QDate m_pendingDateStart;                 // Ожидаемая дата начала
    QDate m_pendingDateEnd;                   // Ожидаемая дата окончания
    QString m_pendingSavePath;                // Ожидаемый путь сохранения

    
    /**
     * @brief Структура элемента результатов поиска
     */
    struct SearchItem
    {
        QString id;       // ID продукта
        QString name;     // Имя продукта для отображения
        QString size;     // Размер файла
    };
    
    QVector<SearchItem> m_searchItems;  // Список найденных продуктов
    QString m_downloadPath;             //  Путь для сохранения текущего файла
    
    QgsMapCanvas *m_mapCanvas;          // Карта
    QgsRubberBand *m_rubberBand;        // Визуализация выделения на карте
    bool m_waitingForMapSelection;      // Флаг ожидания выбора области
    QString m_copernicusLogin;          // Логин Copernicus
    QString m_copernicusPassword;       // Пароль Copernicus
};
