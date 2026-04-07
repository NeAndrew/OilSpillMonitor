#include "sentineldownloaddialog.h"


// Конструктор диалогового окна
SentinelDownloadDialog::SentinelDownloadDialog(QWidget *parent,
                                               const QString &copernicusLogin,
                                               const QString &copernicusPassword,
                                               QgsMapCanvas *mapCanvas)
    : QDialog(parent)
    , m_networkManager(new QNetworkAccessManager(this))
    , m_authSuccess(false)
    , m_mapCanvas(mapCanvas)
    , m_rubberBand(nullptr)
    , m_waitingForMapSelection(false)
    , m_copernicusLogin(copernicusLogin)
    , m_copernicusPassword(copernicusPassword)
{
    setWindowTitle("Скачать снимки Sentinel-1 (Copernicus API) [EPSG:3857]");
    resize(450, 520); // Уменьшили ширину с 520 на 450

    // Группа границ области в координатах Web Mercator
    QGroupBox *bboxGroup = new QGroupBox("Границы области (Web Mercator, EPSG:3857, метры)");
    QGridLayout *bboxLayout = new QGridLayout;
    const double MAX_MERCATOR = 20037508.34;

    // Создание полей ввода координат с пределами Web Mercator
    spinMinX = new QDoubleSpinBox; spinMinX->setRange(-MAX_MERCATOR, MAX_MERCATOR); spinMinX->setDecimals(2); spinMinX->setValue(4110000.0);
    spinMinY = new QDoubleSpinBox; spinMinY->setRange(-MAX_MERCATOR, MAX_MERCATOR); spinMinY->setDecimals(2); spinMinY->setValue(7300000.0);
    spinMaxX = new QDoubleSpinBox; spinMaxX->setRange(-MAX_MERCATOR, MAX_MERCATOR); spinMaxX->setDecimals(2); spinMaxX->setValue(4220000.0);
    spinMaxY = new QDoubleSpinBox; spinMaxY->setRange(-MAX_MERCATOR, MAX_MERCATOR); spinMaxY->setDecimals(2); spinMaxY->setValue(7450000.0);

    // Размещение полей координат в сетке с русскими метками
    bboxLayout->addWidget(new QLabel("Мин. X (м):"), 0, 0); bboxLayout->addWidget(spinMinX, 0, 1);
    bboxLayout->addWidget(new QLabel("Мин. Y (м):"), 0, 2); bboxLayout->addWidget(spinMinY, 0, 3);
    bboxLayout->addWidget(new QLabel("Макс. X (м):"), 1, 0); bboxLayout->addWidget(spinMaxX, 1, 1);
    bboxLayout->addWidget(new QLabel("Макс. Y (м):"), 1, 2); bboxLayout->addWidget(spinMaxY, 1, 3);

    // Примечание о конвертации координат
    QLabel *noteLabel = new QLabel("Примечание: Ввод в метрах. Автоматически конвертируется в WGS84 для поиска.");
    noteLabel->setStyleSheet("color: gray; font-size: 10px;");
    bboxLayout->addWidget(noteLabel, 2, 0, 1, 4);
    bboxGroup->setLayout(bboxLayout);

    // Кнопка загрузки границ с выпадающим меню
    btnLoadBbox = new QToolButton();
    btnLoadBbox->setText("Загрузить охват");
    btnLoadBbox->setToolTip("Загрузить область интереса из файла или выбрать на карте");
    btnLoadBbox->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
    btnLoadBbox->setStyleSheet("QToolButton::menu-indicator { image: none; }");
    btnLoadBbox->setPopupMode(QToolButton::InstantPopup);

    // Создание меню для кнопки загрузки
    menuLoadBbox = new QMenu(this);

    QAction *actFromFile = new QAction("Из файла (GeoJSON/KML/SHP)...", this);
    QAction *actFromMap  = new QAction("Выбрать на карте QGIS (клавиша 'A' для принятия)", this);

    // Подключение обработчиков для действий меню
    connect(actFromFile, &QAction::triggered, this, &SentinelDownloadDialog::onBboxLoadFromFile);
    connect(actFromMap, &QAction::triggered, this, &SentinelDownloadDialog::onBboxLoadFromMap);

    menuLoadBbox->addAction(actFromFile);
    menuLoadBbox->addAction(actFromMap);

    // Отключение выбора на карте если нет canvas
    if (!m_mapCanvas)
    {
        actFromMap->setEnabled(false);
        actFromMap->setToolTip("Требуется ссылка на QgsMapCanvas");
    }

    btnLoadBbox->setMenu(menuLoadBbox);
    connect(btnLoadBbox, &QPushButton::clicked, btnLoadBbox, [this]()
    {
        menuLoadBbox->exec(btnLoadBbox->mapToGlobal(btnLoadBbox->rect().bottomLeft()));
    });

    QHBoxLayout *bboxControlsLayout = new QHBoxLayout;
    bboxControlsLayout->addWidget(btnLoadBbox);

    // Группа периода съёмки
    QGroupBox *dateGroup = new QGroupBox("Период съемки");
    QVBoxLayout *dateLayout = new QVBoxLayout;
    dateStart = new QDateEdit; dateStart->setCalendarPopup(true); dateStart->setDate(QDate::currentDate().addDays(-30));
    dateEnd = new QDateEdit; dateEnd->setCalendarPopup(true); dateEnd->setDate(QDate::currentDate());
    dateLayout->addWidget(new QLabel("Дата начала:")); dateLayout->addWidget(dateStart);
    dateLayout->addWidget(new QLabel("Дата окончания:")); dateLayout->addWidget(dateEnd);
    dateGroup->setLayout(dateLayout);

    // Путь сохранения файлов
    QString defaultDir = QStandardPaths::writableLocation(QStandardPaths::DownloadLocation) + "/sentinel_cdse";
    QHBoxLayout *pathLayout = new QHBoxLayout;
    editSavePath = new QLineEdit; editSavePath->setText(defaultDir);
    btnBrowse = new QPushButton("..."); btnBrowse->setFixedWidth(30); btnBrowse->setText("..."); // Изменили на три точки
    pathLayout->addWidget(new QLabel("Папка:")); pathLayout->addWidget(editSavePath); pathLayout->addWidget(btnBrowse);

    // Элементы управления
    btnStart = new QPushButton("Начать поиск и скачивание");
    btnStart->setStyleSheet("font-weight: bold; padding: 6px;");
    labelStatus = new QLabel("Ожидание авторизации..."); labelStatus->setAlignment(Qt::AlignCenter);
    progressBar = new QProgressBar; progressBar->setVisible(false); progressBar->setRange(0, 100);

    // Основной компоновщик диалога
    QVBoxLayout *mainLayout = new QVBoxLayout(this);
    mainLayout->addWidget(bboxGroup);
    mainLayout->addLayout(bboxControlsLayout);
    mainLayout->addWidget(dateGroup);
    mainLayout->addLayout(pathLayout);
    mainLayout->addWidget(btnStart);
    mainLayout->addWidget(labelStatus);
    mainLayout->addWidget(progressBar);

    // Подключение сигналов
    connect(btnBrowse, &QPushButton::clicked, this, [this]()
    {
        QString dir = QFileDialog::getExistingDirectory(this, "Выберите папку", editSavePath->text());
        if (!dir.isEmpty()) editSavePath->setText(dir);
    });

    connect(btnStart, &QPushButton::clicked, this, &SentinelDownloadDialog::onStartClicked);

    // Запуск авторизации при инициализации
    authenticate(m_copernicusLogin, m_copernicusPassword);
}

// Деструктор - очистка ресурсов
SentinelDownloadDialog::~SentinelDownloadDialog()
{
    if (m_rubberBand) delete m_rubberBand;
    if (m_mapCanvas && m_waitingForMapSelection) {
        m_mapCanvas->removeEventFilter(this);
    }
}

// Перехват клавиш на карте (A = захват, ESC = отмена)
bool SentinelDownloadDialog::eventFilter(QObject *watched, QEvent *event)
{
    if (watched == m_mapCanvas && m_waitingForMapSelection)
    {
        if (event->type() == QEvent::KeyPress)
        {
            QKeyEvent *keyEvent = static_cast<QKeyEvent*>(event);

            // Клавиша 'A' (или 'Ф' в русской раскладке) — захват охвата
            if (keyEvent->key() == Qt::Key_A || keyEvent->key() == Qt::Key_F)
            {
                captureMapExtent();
                return true; // Событие обработано
            }
            // Клавиша ESC — отмена
            if (keyEvent->key() == Qt::Key_Escape)
            {
                finishMapSelection();
                labelStatus->setText("Выбор области отменён");
                return true;
            }
        }
    }
    return QDialog::eventFilter(watched, event);
}

// Обработка клавиши ESC в диалоге
void SentinelDownloadDialog::keyPressEvent(QKeyEvent *event)
{
    if (event->key() == Qt::Key_Escape && m_waitingForMapSelection)
    {
        finishMapSelection();
        labelStatus->setText("Выбор области отменён");
        event->accept();
        return;
    }
    QDialog::keyPressEvent(event);
}

// Преобразование координат из EPSG:3857 в EPSG:4326
bool SentinelDownloadDialog::transformBboxToWGS84(double minX_3857, double minY_3857, double maxX_3857, double maxY_3857, QRectF &outBboxWGS84)
{
    try
    {
        QgsCoordinateReferenceSystem crsSource(QStringLiteral("EPSG:3857"));
        QgsCoordinateReferenceSystem crsDest(QStringLiteral("EPSG:4326"));

        if (!crsSource.isValid() || !crsDest.isValid())
        {
            return false;
        }

        QgsCoordinateTransform transform(crsSource, crsDest, QgsProject::instance());

        QgsPointXY ptSouthWest(minX_3857, minY_3857);
        QgsPointXY ptNorthEast(maxX_3857, maxY_3857);

        QgsPointXY ptSW_WGS84 = transform.transform(ptSouthWest);
        QgsPointXY ptNE_WGS84 = transform.transform(ptNorthEast);

        double minLon = ptSW_WGS84.x();
        double minLat = ptSW_WGS84.y();
        double maxLon = ptNE_WGS84.x();
        double maxLat = ptNE_WGS84.y();

        // Упорядочивание координат
        if (minLon > maxLon) std::swap(minLon, maxLon);
        if (minLat > maxLat) std::swap(minLat, maxLat);

        outBboxWGS84 = QRectF(minLon, minLat, maxLon - minLon, maxLat - minLat);

        return true;
    }
    catch (const QgsCsException &e)
    {
        qWarning() << "Исключение:" << e.what();
        return false;
    }
    catch (...)
    {
        qWarning() << "Неизвестная ошибка при трансформации координат";
        return false;
    }
}

// Преобразование прямоугольника QGIS в QRectF
QRectF SentinelDownloadDialog::qgsRectToQRectF(const QgsRectangle &rect)
{
    return QRectF(rect.xMinimum(), rect.yMinimum(), rect.xMaximum() - rect.xMinimum(), rect.yMaximum() - rect.yMinimum());
}

// Авторизация через OAuth2 Password Grant
void SentinelDownloadDialog::authenticate(const QString& username, const QString& password)
{
    labelStatus->setText("Авторизация...");

    // URL для получения токена доступа
    QUrl url("https://identity.dataspace.copernicus.eu/auth/realms/CDSE/protocol/openid-connect/token");
    QNetworkRequest request(url);
    request.setHeader(QNetworkRequest::ContentTypeHeader, "application/x-www-form-urlencoded");

    QUrlQuery query;
    query.addQueryItem("grant_type", "password");
    query.addQueryItem("client_id", "cdse-public");
    query.addQueryItem("username", username.trimmed());
    query.addQueryItem("password", password.trimmed());

    QNetworkReply* reply = m_networkManager->post(request, query.toString().toUtf8());
    connect(reply, &QNetworkReply::finished, this, [this, reply]()
    {
        onAuthReplyFinished(reply);
        reply->deleteLater();
    });
}

// Обработчик ответа авторизации
void SentinelDownloadDialog::onAuthReplyFinished(QNetworkReply* reply)
{
    if (reply->error() != QNetworkReply::NoError)
    {
        onAuthFinished(false, "", "Ошибка сети: " + reply->errorString());
        return;
    }

    QJsonParseError err;
    QJsonDocument doc = QJsonDocument::fromJson(reply->readAll(), &err);
    if (err.error != QJsonParseError::NoError || !doc.isObject())
    {
        onAuthFinished(false, "", "Ошибка парсинга JSON ответа");
        return;
    }

    QJsonObject obj = doc.object();
    if (obj.contains("access_token"))
    {
        m_accessToken = obj["access_token"].toString();
        int expiresIn = obj["expires_in"].toInt(1800);
        qDebug() << "Аутентификация прошла успешно. Срок действия токена истекает через:" << expiresIn << "секунд";        onAuthFinished(true, m_accessToken, "Авторизация успешна");
    }
    else
    {
        QString errMsg = obj["error_description"].toString(obj["error"].toString("Unknown error"));
        onAuthFinished(false, "", "Ошибка аутентификации: " + errMsg);
    }
}

// Завершение процесса авторизации
void SentinelDownloadDialog::onAuthFinished(bool success, const QString &token, const QString &message)
{
    Q_UNUSED(token);
    m_authSuccess = success;
    labelStatus->setText(message);
    btnStart->setEnabled(success);
    if (!success)
    {
        QMessageBox::critical(this, "Ошибка авторизации", message);
    }
}

// Обработчик нажатия кнопки начала поиска
void SentinelDownloadDialog::onStartClicked()
{
    if (!m_authSuccess)
    {
        QMessageBox::warning(this, "Предупреждение", "Авторизация не пройдена.");
        return;
    }

    // Проверка корректности координат
    double minX = spinMinX->value(), minY = spinMinY->value();
    double maxX = spinMaxX->value(), maxY = spinMaxY->value();
    if (minX >= maxX || minY >= maxY)
    {
        QMessageBox::critical(this, "Ошибка", "Некорректные координаты BBOX.");
        return;
    }

    // Преобразование координат в WGS84
    QRectF bboxWGS84;
    if (!transformBboxToWGS84(minX, minY, maxX, maxY, bboxWGS84))
    {
        QMessageBox::critical(this, "Ошибка", "Не удалось преобразовать координаты.");
        return;
    }

    // Сохранение параметров для поиска
    m_pendingBbox = bboxWGS84;
    m_pendingDateStart = dateStart->date();
    m_pendingDateEnd = dateEnd->date();
    m_pendingSavePath = editSavePath->text();

    // Настройка UI для процесса поиска
    btnStart->setEnabled(false);
    progressBar->setVisible(true);
    progressBar->setRange(0, 0);
    labelStatus->setText("Поиск снимков...");

    searchSentinelData();
}

// Поиск данных через OData API
void SentinelDownloadDialog::searchSentinelData()
{
    QString filter = buildODataFilter(m_pendingBbox, m_pendingDateStart, m_pendingDateEnd);

    // URL для поиска продуктов Sentinel-1
    QUrl url("https://catalogue.dataspace.copernicus.eu/odata/v1/Products");
    QUrlQuery query;
    query.addQueryItem("$filter", filter);
    query.addQueryItem("$top", "20");
    query.addQueryItem("$orderby", "ContentDate/Start desc");
    url.setQuery(query);

    QNetworkRequest request(url);
    request.setRawHeader("Authorization", "Bearer " + m_accessToken.toUtf8());
    request.setRawHeader("Accept", "application/json");

    qDebug() << "URL:" << url.toString();

    QNetworkReply* reply = m_networkManager->get(request);
    connect(reply, &QNetworkReply::finished, this, [this, reply]() {
        onSearchReplyFinished(reply);
        reply->deleteLater();
    });
}

// Построение фильтра OData для поиска
QString SentinelDownloadDialog::buildODataFilter(const QRectF& bboxWGS84, const QDate& start, const QDate& end)
{
    double lonMin = bboxWGS84.left();
    double latMin = bboxWGS84.top();
    double lonMax = bboxWGS84.right();
    double latMax = bboxWGS84.bottom();

    // Упорядочивание координат
    if (lonMin > lonMax) std::swap(lonMin, lonMax);
    if (latMin > latMax) std::swap(latMin, latMax);

    // Создание полигона в формате WKT
    QString polygon = QString("POLYGON((%1 %2, %3 %2, %3 %4, %1 %4, %1 %2))")
            .arg(lonMin, 0, 'f', 6)
            .arg(latMin, 0, 'f', 6)
            .arg(lonMax, 0, 'f', 6)
            .arg(latMax, 0, 'f', 6);

    // Конвертация дат в UTC
    QDateTime dtStart(start, QTime(0,0,0), Qt::UTC);
    QDateTime dtEnd(end.addDays(1), QTime(0,0,0), Qt::UTC);

    QString dateStartStr = dtStart.toString("yyyy-MM-ddTHH:mm:ss.000Z");
    QString dateEndStr   = dtEnd.toString("yyyy-MM-ddTHH:mm:ss.000Z");

    return QString(
                "Collection/Name eq 'SENTINEL-1' and contains(Name,'GRDH') and "
                "OData.CSC.Intersects(area=geography'SRID=4326;%1') and "
                "ContentDate/Start ge %2 and ContentDate/Start le %3"
                ).arg(polygon, dateStartStr, dateEndStr);
}

// Обработчик ответа поиска
void SentinelDownloadDialog::onSearchReplyFinished(QNetworkReply* reply)
{
    if (reply->error() != QNetworkReply::NoError)
    {
        onError("Ошибка поиска: " + reply->errorString());
        return;
    }

    QJsonParseError err;
    QJsonDocument doc = QJsonDocument::fromJson(reply->readAll(), &err);
    if (err.error != QJsonParseError::NoError || !doc.isObject())
    {
        onError("Ошибка парсинга поиска");
        return;
    }

    QJsonObject root = doc.object();
    QJsonArray value = root["value"].toArray();

    if (value.isEmpty())
    {
        QMessageBox::information(this, "Результат", "Снимки не найдены.");
        btnStart->setEnabled(true);
        progressBar->setVisible(false);
        labelStatus->setText("Готов к работе");
        return;
    }

    // Заполнение списка найденных продуктов
    m_searchItems.clear();
    for (const QJsonValue& val : std::as_const(value))
    {
        QJsonObject prod = val.toObject();
        SearchItem item;
        item.id = prod["Id"].toString();
        item.name = prod["Name"].toString();
        item.size = QString::number(prod["ContentLength"].toDouble() / (1024.0*1024.0), 'f', 1) + " MB";
        m_searchItems.append(item);
    }

    showSearchResults();
}

// Отображение результатов поиска в диалоге выбора
void SentinelDownloadDialog::showSearchResults()
{
    QDialog dialog(this);
    dialog.setWindowTitle("Выбор снимка Sentinel-1");
    dialog.resize(600, 400);
    QVBoxLayout* layout = new QVBoxLayout(&dialog);

    // Создание таблицы с результатами
    QTableWidget* table = new QTableWidget(m_searchItems.size(), 3);
    table->setHorizontalHeaderLabels({"ID", "Имя продукта", "Размер"});
    table->horizontalHeader()->setSectionResizeMode(1, QHeaderView::Stretch);
    table->setSelectionBehavior(QAbstractItemView::SelectRows);
    table->setSelectionMode(QAbstractItemView::SingleSelection);
    table->setEditTriggers(QAbstractItemView::NoEditTriggers);

    // Заполнение таблицы
    for (int i = 0; i < m_searchItems.size(); ++i)
    {
        const auto& item = m_searchItems[i];
        table->setItem(i, 0, new QTableWidgetItem(item.id));
        table->setItem(i, 1, new QTableWidgetItem(item.name));
        table->setItem(i, 2, new QTableWidgetItem(item.size));
    }

    QDialogButtonBox* buttons = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel);
    layout->addWidget(table);
    layout->addWidget(buttons);
    connect(buttons, &QDialogButtonBox::accepted, &dialog, &QDialog::accept);
    connect(buttons, &QDialogButtonBox::rejected, &dialog, &QDialog::reject);

    // Обработка выбора продукта
    if (dialog.exec() == QDialog::Accepted && table->currentRow() >= 0) {
        const SearchItem& selected = m_searchItems[table->currentRow()];

        QString filename = sanitizeFilename(selected.name) + ".zip";
        QString fullPath = QDir(m_pendingSavePath).filePath(filename);

        // Проверка существования файла
        if (QFile::exists(fullPath))
        {
            int ret = QMessageBox::question(this, "Файл существует", QString("Файл %1 уже существует.\nПерезаписать?").arg(filename));
            if (ret != QMessageBox::Yes)
            {
                btnStart->setEnabled(true);
                progressBar->setVisible(false);
                labelStatus->setText("Готов к работе");
                return;
            }
        }

        m_downloadPath = fullPath;
        labelStatus->setText("Загрузка...");
        requestDownload(selected.id, filename);
    }
    else
    {
        btnStart->setEnabled(true);
        progressBar->setVisible(false);
        labelStatus->setText("Готов к работе");
    }
}

// Очистка имени файла от недопустимых символов
QString SentinelDownloadDialog::sanitizeFilename(const QString& name)
{
    QString res = name;
    res.replace(QRegExp("[<>:\"/\\\\|?*]"), "_");
    return res;
}

// Запрос на скачивание выбранного продукта
void SentinelDownloadDialog::requestDownload(const QString& productId, const QString& filename)
{
    // Блокируем кнопку загрузки границ на время скачивания
    btnLoadBbox->setEnabled(false);
    
    // URL для скачивания файла продукта
    QUrl url(QString("https://catalogue.dataspace.copernicus.eu/odata/v1/Products(%1)/$value").arg(productId));

    QNetworkRequest request(url);
    request.setRawHeader("Authorization", "Bearer " + m_accessToken.toUtf8());
    request.setRawHeader("Accept", "application/octet-stream");
    request.setAttribute(QNetworkRequest::RedirectPolicyAttribute, true);

    // Создание директории если не существует
    QDir dir(m_pendingSavePath);
    if (!dir.exists())
    {
        dir.mkpath(".");
    }

    // Создание файла для записи
    QFile* file = new QFile(m_downloadPath, this);
    if (!file->open(QIODevice::WriteOnly))
    {
        onError("Не удалось создать файл: " + m_downloadPath);
        delete file;
        return;
    }

    QNetworkReply* reply = m_networkManager->get(request);

    // Сохранение указателя на файл в свойствах ответа
    reply->setProperty("targetFile", QVariant::fromValue(file));

    // Подключение обработчика записи данных по мере поступления
    connect(reply, &QNetworkReply::readyRead, this, [reply]()
    {
        QFile* file = reply->property("targetFile").value<QFile*>();
        if (file && file->isOpen())
        {
            file->write(reply->readAll());
        }
    });

    connect(reply, &QNetworkReply::downloadProgress, this, &SentinelDownloadDialog::onDownloadProgress);

    // Обработка завершения скачивания
    connect(reply, &QNetworkReply::finished, this, [this, reply, filename]()
    {
        onDownloadReplyFinished(reply);
        QFile* file = reply->property("targetFile").value<QFile*>();
        if (file) {
            file->close();
            if (reply->error() != QNetworkReply::NoError)
            {
                file->remove(); // Удаление повреждённого файла
            }
            delete file;
        }
        reply->deleteLater();
    });
}

// Обработчик прогресса скачивания
void SentinelDownloadDialog::onDownloadProgress(qint64 received, qint64 total)
{
    if (total > 0)
    {
        progressBar->setRange(0, 100);
        progressBar->setValue(static_cast<int>(100 * received / total));

        // Отображение прогресса в мегабайтах
        double mbRecv = received / (1024.0 * 1024.0);
        double mbTotal = total / (1024.0 * 1024.0);
        labelStatus->setText(QString("Загрузка: %1 / %2 МБ").arg(mbRecv, 0, 'f', 1).arg(mbTotal, 0, 'f', 1));
    }
    else
    {
        progressBar->setRange(0, 0); // Неопределённый прогресс
    }
}

// Обработчик завершения скачивания
void SentinelDownloadDialog::onDownloadReplyFinished(QNetworkReply* reply)
{
    if (reply->error() != QNetworkReply::NoError)
    {
        if (reply->error() == QNetworkReply::ContentAccessDenied)
        {
            onDownloadFinished(false, "", "Ошибка доступа (возможно, истёк токен): " + reply->errorString());
        }
        else
        {
            onDownloadFinished(false, "", "Ошибка загрузки: " + reply->errorString());
        }
        return;
    }

    // Проверка HTTP статуса
    int status = reply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt();
    if (status >= 400)
    {
        onDownloadFinished(false, "", QString("HTTP ошибка %1").arg(status));
        return;
    }

    onDownloadFinished(true, m_downloadPath, "Загрузка завершена успешно!");
}

// Завершение процесса скачивания
void SentinelDownloadDialog::onDownloadFinished(bool success, const QString &path, const QString &msg)
{
    btnStart->setEnabled(true);
    progressBar->setVisible(false);
    progressBar->setRange(0, 100);
    labelStatus->setText(msg);
    
    // Включаем кнопку загрузки границ обратно
    btnLoadBbox->setEnabled(true);

    if (success)
    {
        QMessageBox::information(this, "Успешное выполнение", QString("Файл сохранён:\n%1").arg(path));
    }
    else
    {
        QMessageBox::critical(this, "Ошибка", msg);
    }
}

// Обработчик общих ошибок
void SentinelDownloadDialog::onError(const QString &err)
{
    btnStart->setEnabled(true);
    progressBar->setVisible(false);
    labelStatus->setText("Ошибка!");
    
    // Включаем кнопку загрузки границ обратно
    btnLoadBbox->setEnabled(true);
    
    QMessageBox::critical(this, "Ошибка", err);
}

// Обработчик нажатия кнопки загрузки границ
void SentinelDownloadDialog::onBboxLoadClicked()
{
    menuLoadBbox->exec(btnLoadBbox->mapToGlobal(btnLoadBbox->rect().bottomLeft()));
}

// Загрузка границ из файла
void SentinelDownloadDialog::onBboxLoadFromFile()
{
    QString filter = "Векторные файлы (*.geojson *.json *.kml *.shp *.gpkg *.gml);;Все файлы (*)";
    QString filePath = QFileDialog::getOpenFileName(this, "Выберите файл с охватом", QDir::homePath(), filter);
    if (filePath.isEmpty())
    {
        return;
    }

    QRectF bboxWGS84;
    if (loadBboxFromFile(filePath, bboxWGS84))
    {
        try
        {
            // Преобразование из WGS84 в EPSG:3857 для отображения в UI
            QgsCoordinateReferenceSystem crsSrc(QStringLiteral("EPSG:4326"));
            QgsCoordinateReferenceSystem crsDst(QStringLiteral("EPSG:3857"));
            QgsCoordinateTransform transform(crsSrc, crsDst, QgsProject::instance());

            QgsPointXY minWgs(bboxWGS84.left(), bboxWGS84.top());
            QgsPointXY maxWgs(bboxWGS84.right(), bboxWGS84.bottom());

            QgsPointXY min3857 = transform.transform(minWgs);
            QgsPointXY max3857 = transform.transform(maxWgs);

            // Установка значений в поля ввода
            spinMinX->setValue(min3857.x()); spinMinY->setValue(min3857.y());
            spinMaxX->setValue(max3857.x()); spinMaxY->setValue(max3857.y());

            labelStatus->setText("Охват загружен из файла");
            qDebug() << "BBOX из файла (3857):" << min3857.x() << min3857.y()
                     << max3857.x() << max3857.y();
        }
        catch (...)
        {
            QMessageBox::warning(this, "Ошибка трансформации", "Не удалось преобразовать координаты в EPSG:3857");
        }
    }
    else
    {
        QMessageBox::critical(this, "Ошибка", "Не удалось извлечь геометрию из файла.\n" "Поддерживаются: GeoJSON, KML, Shapefile, GPKG с полигонами/прямоугольниками.");
    }
}

// Загрузка границ из векторного файла
bool SentinelDownloadDialog::loadBboxFromFile(const QString &filePath, QRectF &outBboxWGS84)
{
    QgsVectorLayer layer(filePath, "temp_coverage", "ogr");
    if (!layer.isValid())
    {
        return false;
    }

    QgsRectangle extent;
    bool hasFeatures = false;

    // Итерация по всем объектам слоя
    QgsFeatureIterator it = layer.getFeatures();
    QgsFeature feature;
    while (it.nextFeature(feature))
    {
        QgsGeometry geom = feature.geometry();
        if (geom.isNull() || geom.isEmpty())
        {
            continue;
        }

        QgsRectangle featRect = geom.boundingBox();
        if (!hasFeatures)
        {
            extent = featRect;
            hasFeatures = true;
        }
        else
        {
            extent.combineExtentWith(featRect); // Объединение экстентов
        }
    }

    if (!hasFeatures || extent.isEmpty())
    {
        return false;
    }

    // Преобразование в WGS84 если необходимо
    QgsCoordinateReferenceSystem layerCrs = layer.crs();
    if (layerCrs.authid() != QStringLiteral("EPSG:4326"))
    {
        try
        {
            QgsCoordinateTransform transform(layerCrs, QgsCoordinateReferenceSystem(QStringLiteral("EPSG:4326")), QgsProject::instance());
            extent = transform.transformBoundingBox(extent);
        }
        catch (...)
        {
            return false;
        }
    }

    outBboxWGS84 = qgsRectToQRectF(extent);
    return true;
}

// Загрузка границ из карты QGIS с интерактивным выбором
void SentinelDownloadDialog::onBboxLoadFromMap()
{
    if (!m_mapCanvas)
    {
        QMessageBox::warning(this, "Ошибка", "QgsMapCanvas не передан в диалог");
        return;
    }

    // Скрываем диалог, чтобы пользователь видел карту
    hide();

    // Создаём RubberBand для визуализации текущего охвата
    m_rubberBand = new QgsRubberBand(m_mapCanvas, Qgis::GeometryType::Polygon);
    m_rubberBand->setColor(QColor(0, 170, 255, 80));   // голубой полупрозрачный
    m_rubberBand->setStrokeColor(QColor(0, 170, 255, 200));
    m_rubberBand->setWidth(2);
    m_rubberBand->reset(Qgis::GeometryType::Polygon);

    // Рисуем прямоугольник по текущему extent canvas
    QgsRectangle currentExtent = m_mapCanvas->extent();
    m_rubberBand->addPoint(QgsPointXY(currentExtent.xMinimum(), currentExtent.yMinimum()), false);
    m_rubberBand->addPoint(QgsPointXY(currentExtent.xMaximum(), currentExtent.yMinimum()), false);
    m_rubberBand->addPoint(QgsPointXY(currentExtent.xMaximum(), currentExtent.yMaximum()), false);
    m_rubberBand->addPoint(QgsPointXY(currentExtent.xMinimum(), currentExtent.yMaximum()), true);

    m_waitingForMapSelection = true;

    // Устанавливаем event filter для перехвата клавиш
    m_mapCanvas->installEventFilter(this);

    // Обновляем RubberBand при изменении extent (панорамирование/зум)
    connect(m_mapCanvas, &QgsMapCanvas::extentsChanged, this, [this]()
    {
        if (!m_waitingForMapSelection || !m_rubberBand)
        {
            return;
        }

        QgsRectangle extent = m_mapCanvas->extent();
        m_rubberBand->reset(Qgis::GeometryType::Polygon);
        m_rubberBand->addPoint(QgsPointXY(extent.xMinimum(), extent.yMinimum()), false);
        m_rubberBand->addPoint(QgsPointXY(extent.xMaximum(), extent.yMinimum()), false);
        m_rubberBand->addPoint(QgsPointXY(extent.xMaximum(), extent.yMaximum()), false);
        m_rubberBand->addPoint(QgsPointXY(extent.xMinimum(), extent.yMaximum()), true);
    });

    // Инструкция пользователю
    QMessageBox::information(nullptr, "Выбор области на карте",
                             "- Настройте вид карты (панорамирование/масштаб)\n"
                             "- Нажмите клавишу [A] для захвата охвата\n"
                             "- Нажмите [ESC] для отмены");

    labelStatus->setText("Настройте карту и нажмите [A]...");
}

// Захват охвата по нажатию клавиши 'A'
void SentinelDownloadDialog::captureMapExtent()
{
    if (!m_waitingForMapSelection || !m_mapCanvas) return;

    QgsRectangle currentExtent = m_mapCanvas->extent();
    QgsCoordinateReferenceSystem canvasCrs = m_mapCanvas->mapSettings().destinationCrs();

    try
    {
        // Преобразование из CRS карты в WGS84
        QgsCoordinateTransform transform(canvasCrs, QgsCoordinateReferenceSystem(QStringLiteral("EPSG:4326")), QgsProject::instance());
        QgsRectangle extentWGS84 = transform.transformBoundingBox(currentExtent);

        QRectF bboxWGS84 = qgsRectToQRectF(extentWGS84);
        setBboxFromWGS84(bboxWGS84.left(), bboxWGS84.top(), bboxWGS84.right(), bboxWGS84.bottom());

        emit mapSelectionFinished(bboxWGS84);

    }
    catch (const QgsCsException &e)
    {
        QMessageBox::warning(nullptr, "Ошибка", "Не удалось преобразовать координаты: " + e.what());
    }
    catch (const std::exception &e)
    {
        QMessageBox::warning(nullptr, "Ошибка", "Не удалось преобразовать координаты: " + QString::fromUtf8(e.what()));
    }
    catch (...)
    {
        QMessageBox::warning(nullptr, "Ошибка", "Не удалось преобразовать координаты");
    }

    finishMapSelection();
}

// Публичный слот: установка границ из координат WGS84
void SentinelDownloadDialog::setBboxFromWGS84(double minLon, double minLat, double maxLon, double maxLat)
{
    try
    {
        // Преобразование из WGS84 в EPSG:3857 для отображения в UI
        QgsCoordinateReferenceSystem crsSrc(QStringLiteral("EPSG:4326"));
        QgsCoordinateReferenceSystem crsDst(QStringLiteral("EPSG:3857"));
        QgsCoordinateTransform transform(crsSrc, crsDst, QgsProject::instance());

        QgsPointXY minWgs(minLon, minLat);
        QgsPointXY maxWgs(maxLon, maxLat);

        QgsPointXY min3857 = transform.transform(minWgs);
        QgsPointXY max3857 = transform.transform(maxWgs);

        // Установка значений в поля ввода
        spinMinX->setValue(min3857.x()); spinMinY->setValue(min3857.y());
        spinMaxX->setValue(max3857.x()); spinMaxY->setValue(max3857.y());

        labelStatus->setText("Охват выбран на карте");
        if (!isVisible())
        {
            show();
        }
    }
    catch (...)
    {
        QMessageBox::critical(this, "Ошибка", "Не удалось преобразовать координаты");
        if (!isVisible())
        {
            show();
        }
    }
}

// Завершение режима выбора на карте
void SentinelDownloadDialog::finishMapSelection()
{
    m_waitingForMapSelection = false;

    // Удаление RubberBand
    if (m_rubberBand)
    {
        delete m_rubberBand;
        m_rubberBand = nullptr;
    }

    // Отключение от карты
    if (m_mapCanvas)
    {
        disconnect(m_mapCanvas, &QgsMapCanvas::extentsChanged, this, nullptr);
        m_mapCanvas->removeEventFilter(this);
    }

    // Показ диалога и обновление статуса
    if (!isVisible())
    {
        show();
    }
    labelStatus->setText("Готов к работе");
}
