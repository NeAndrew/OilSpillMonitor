#include "screenshotdialog.h"

// Конструктор диалогового окна создания скриншота
ScreenshotDialog::ScreenshotDialog(QWidget* parent)
    : QDialog(parent)
{
    // Установка заголовка и размера окна
    setWindowTitle("Параметры скриншота");
    resize(400, 150);

    // Создание основного макета
    auto* layout = new QVBoxLayout(this);
    auto* formLayout = new QFormLayout();

    // Создание и настройка спинбокса для выбора разрешения DPI
    mDpiSpinBox = new QSpinBox();
    mDpiSpinBox->setRange(72, 2400);  // Диапазон от стандартного до высокого разрешения
    mDpiSpinBox->setValue(300);       // Значение по умолчанию
    mDpiSpinBox->setSuffix(" DPI");
    formLayout->addRow("Разрешение:", mDpiSpinBox);

    // Создание чекбокса для опции добавления масштаба к изображению
    mAddScaleCheckBox = new QCheckBox("Добавить масштаб к изображению");
    mAddScaleCheckBox->setChecked(true);  // Включено по умолчанию
    formLayout->addRow(mAddScaleCheckBox);

    // Создание дополнительного макета
    auto* fileLayout = new QHBoxLayout();
    mFilePathEdit = new QLineEdit();

    // Установка пути по умолчанию в домашнюю директорию с текущей датой и временем
    mFilePathEdit->setText(QDir::home().filePath(QString("map_%1.png").arg(QDateTime::currentDateTime().toString("yyyyMMdd_hhmmss"))));
    QPushButton* browseButton = new QPushButton("...");  // Кнопка для выбора файла
    
    // Подключение сигнала нажатия кнопки к слоту выбора файла
    connect(browseButton, &QPushButton::clicked, this, &ScreenshotDialog::browseFile);
    fileLayout->addWidget(mFilePathEdit);
    fileLayout->addWidget(browseButton);
    formLayout->addRow("Файл:", fileLayout);

    // Добавление формы настроек в основной макет
    layout->addLayout(formLayout);

    // Создание горизонтального макета для кнопок OK и Отмена
    auto* buttonLayout = new QHBoxLayout();
    buttonLayout->addStretch();
    QPushButton* okButton = new QPushButton("OK");
    QPushButton* cancelButton = new QPushButton("Отмена");

    // Подключение кнопок к стандартным слотам принятия и отклонения диалога
    connect(okButton, &QPushButton::clicked, this, &QDialog::accept);
    connect(cancelButton, &QPushButton::clicked, this, &QDialog::reject);

    buttonLayout->addWidget(okButton);
    buttonLayout->addWidget(cancelButton);
    layout->addLayout(buttonLayout);
}

ScreenshotOptions ScreenshotDialog::getOptions() const
{
    // Создание структуры параметров и заполнение её значениями из элементов интерфейса
    ScreenshotOptions opts;
    opts.dpi = mDpiSpinBox->value();              // Получение выбранного разрешения
    opts.addScale = mAddScaleCheckBox->isChecked(); // Получение состояния чекбокса масштаба
    opts.filePath = mFilePathEdit->text();        // Получение пути к файлу
    return opts;
}

// Слот для открытия диалога выбора файла сохранения
void ScreenshotDialog::browseFile()
{
    // Получение текущего пути из поля ввода
    QString initialPath = mFilePathEdit->text();
    // Извлечение директории из текущего пути для начального положения диалога
    QString dir = QFileInfo(initialPath).absoluteDir().path();
    // Открытие диалога сохранения файла с фильтром PNG и JPEG
    QString path = QFileDialog::getSaveFileName(this, "Сохранить карту", dir, "PNG (*.png);;JPEG (*.jpg *.jpeg)", nullptr);

    // Если пользователь выбрал файл (не отменил диалог)
    if (!path.isEmpty())
    {
        // Обновление поля ввода пути выбранным файлом
        mFilePathEdit->setText(path);
    }
}
