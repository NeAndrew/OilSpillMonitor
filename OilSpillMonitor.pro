QT += core gui xml gui-private svg network sql charts network concurrent printsupport

unix { INCLUDEPATH += /usr/lib/gcc/x86_64-linux-gnu/12/include }

greaterThan(QT_MAJOR_VERSION, 4): QT += widgets

CONFIG += c++17

DEFINES += SRCDIR=\\\"$$PWD/\\\"

SOURCES += \
    EditTool/qgslockedfeature.cpp \
    EditTool/qgsvertexeditor.cpp \
    EditTool/qgsvertextool.cpp \
    advancedslider.cpp \
    copernicuslogindialog.cpp \
    detectionrenderer.cpp \
    geojsonexporter.cpp \
    modelmanager.cpp \
    reportoptionsdialog.cpp \
    maplayermanager.cpp \
    detectionoildialog.cpp \
    maskedmenu.cpp \
    oilspreadanalysis.cpp \
    oilspreadanalysisdialog.cpp \
    reportgenerator.cpp \
    sarprocess.cpp \
    screenshotdialog.cpp \
    drawlinestringtool.cpp \
    drawpointtool.cpp \
    drawpolygontool.cpp \
    main.cpp \
    mainwindow.cpp \
    measurementtool.cpp \
    selectbyrectangletool.cpp \
    sentineldownloaddialog.cpp

HEADERS += \
    EditTool/qgslockedfeature.h \
    EditTool/qgsvertexeditor.h \
    EditTool/qgsvertextool.h \
    advancedslider.h \
    copernicuslogindialog.h \
    detectionrenderer.h \
    geojsonexporter.h \
    modelmanager.h \
    reportoptionsdialog.h \
    maplayermanager.h \
    detectionoildialog.h \
    maskedmenu.h \
    oilspreadanalysis.h \
    oilspreadanalysisdialog.h \
    reportgenerator.h \
    sarprocess.h \
    screenshotdialog.h \
    drawlinestringtool.h \
    drawpointtool.h \
    drawpolygontool.h \
    mainwindow.h \
    measurementtool.h \
    selectbyrectangletool.h \
    sentineldownloaddialog.h

FORMS += \
    mainwindow.ui

unix{
DEFINES += CORE_EXPORT=
DEFINES += GUI_EXPORT=
}
!unix{
INCLUDEPATH += $(QGIS_ROOT)/include
DEFINES += CORE_EXPORT=__declspec(dllimport)
DEFINES += GUI_EXPORT=__declspec(dllimport)
}

qnx: target.path = /tmp/$${TARGET}/bin
else: unix:!android: target.path = /opt/$${TARGET}/bin
!isEmpty(target.path): INSTALLS += target

#QGIS API (from source)
#unix:!macx: LIBS += -L$$PWD/../../../dev/cpp/QGIS/build-master/output/lib/ -lqgis_core
#unix:!macx: LIBS += -L$$PWD/../../../dev/cpp/QGIS/build-master/output/lib/ -lqgis_gui
#unix:!macx: LIBS += -L$$PWD/../../../dev/cpp/QGIS/build-master/output/lib/ -lqgis_analysis

#INCLUDEPATH += $$PWD/../../../dev/cpp/QGIS/src/core
#DEPENDPATH += $$PWD/../../../dev/cpp/QGIS/src/core
#INCLUDEPATH += $$PWD/../../../dev/cpp/QGIS/src/gui
#DEPENDPATH += $$PWD/../../../dev/cpp/QGIS/src/gui

#QGIS API (пакет libqgis-dev)
unix:!macx: LIBS += -L/usr/lib/ -lqgis_core
unix:!macx: LIBS += -L/usr/lib/ -lqgis_gui
unix:!macx: LIBS += -L/usr/lib/ -lqgis_app
unix:!macx: LIBS += -L/usr/lib/ -lqgis_analysis
INCLUDEPATH += /usr/include/qgis
DEPENDPATH += /usr/include/qgis

#unix:!macx: LIBS += -L$$PWD/../../../../../usr/lib/ -lqgis_core
#unix:!macx: LIBS += -L$$PWD/../../../../../usr/lib/ -lqgis_gui
#unix:!macx: LIBS += -L$$PWD/../../../../../usr/lib/ -lqgis_app
#unix:!macx: LIBS += -L$$PWD/../../../../../usr/lib/ -lqgis_analysis
#INCLUDEPATH += $$PWD/../../../../../usr/include/qgis
#DEPENDPATH += $$PWD/../../../../../usr/include/qgis

#GDAL
#unix:!macx: LIBS += -L$$PWD/../../../../../lib/x86_64-linux-gnu/ -lgdal
#INCLUDEPATH += $$PWD/../../../../../usr/include/gdal
#DEPENDPATH += $$PWD/../../../../../usr/include/gdal
unix:!macx: LIBS += -L/lib/x86_64-linux-gnu/ -lgdal
INCLUDEPATH += /usr/include/gdal
DEPENDPATH += /usr/include/gdal


# qt5keychain
#INCLUDEPATH += $$PWD/../../../../../usr/include/qt5keychain
#INCLUDEPATH += $$PWD/../../../../../usr/include/Qca-qt5/QtCrypto
#DEPENDPATH += $$PWD/../../../../../usr/include/Qca-qt5/QtCrypto
INCLUDEPATH += /usr/include/qt5keychain
INCLUDEPATH += /usr/include/Qca-qt5/QtCrypto
DEPENDPATH += /usr/include/Qca-qt5/QtCrypto


RESOURCES += \
    res.qrc
