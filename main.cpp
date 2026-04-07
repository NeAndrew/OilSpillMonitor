#include "mainwindow.h"

// Библиотеки Qt
#include <QApplication>
#include <QTranslator>
#include <QLocale>
#include <QDir>

// Библиотеки QGIS API
#include <qgsapplication.h>
#include <qgsprocessingregistry.h>
#include <qgsnativealgorithms.h>

// Переменная для хранения перевода (должна быть объявлена глобально)
static QTranslator *g_qgisTranslator = nullptr;

namespace
{
const QString QGIS_TRANSLATION_PATH = "/usr/share/qgis/i18n/";
const QString QGIS_PREFIX_PATH = "/usr";
const QString QGIS_TRANSLATION_FILE_BASE = "qgis";
}

QString getApplicationStyleSheet()
{
    return R"(
        QPushButton {
            background-color: white;
            color: #2b2b2b;
            border: 2px solid #9ACEEB;
            border-radius: 16px;
            padding: 6px 14px;
        }

        QPushButton:hover {
            background-color: #9ACEEB;
            color: white;
        }

        QPushButton:pressed {
            background-color: #d0d0d0;
            border-color: #9ACEEB;
            color: #2b2b2b;
        }

        QPushButton:disabled {
            background-color: #f2f2f2;
            border-color: #c0c0c0;
            color: #9a9a9a;
        }

        QToolButton {
            background-color: white;
            color: #2b2b2b;
            border: 2px solid #9ACEEB;
            border-radius: 16px;
            padding: 6px 14px;
        }

        QToolButton:hover {
            background-color: #9ACEEB;
            color: white;
        }

        QToolButton:pressed {
            background-color: #d0d0d0;
            border-color: #9ACEEB;
            color: #2b2b2b;
        }

        QToolButton:disabled {
            background-color: #f2f2f2;
            border-color: #c0c0c0;
            color: #9a9a9a;
        }
    )";
}

void setupApplicationStyle(QApplication& app)
{
    app.setStyleSheet(getApplicationStyleSheet());
}

// Пытаемся загрузить перевод QGIS 
void setupTranslation(QApplication& app)
{
    QLocale locale = QLocale::system();
    QString localeName = locale.name();
    QString shortLocale = localeName.split('_').first();

    g_qgisTranslator = new QTranslator();
    QString translationFile = QGIS_TRANSLATION_FILE_BASE + "_" + shortLocale;

    if (g_qgisTranslator->load(translationFile, QGIS_TRANSLATION_PATH))
    {
        app.installTranslator(g_qgisTranslator);
        qDebug() << "Перевод QGIS загружен:" << translationFile;
    }
    else
    {
        qWarning() << "Не удалось загрузить перевод QGIS:" << translationFile;
        delete g_qgisTranslator;
        g_qgisTranslator = nullptr;
    }
}

void configureApplication(QgsApplication& app)
{
    app.setPrefixPath(QGIS_PREFIX_PATH, true);

    setupApplicationStyle(app);
    setupTranslation(app);

    QgsApplication::initQgis();
    QgsApplication::processingRegistry()->addProvider(new QgsNativeAlgorithms());
}

int main(int argc, char *argv[])
{
    QgsApplication app(argc, argv, true);

    configureApplication(app);

    MainWindow window;
    window.show();

    return app.exec();
}
