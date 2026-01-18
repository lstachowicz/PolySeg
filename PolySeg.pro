QT       += core gui

greaterThan(QT_MAJOR_VERSION, 4): QT += widgets

CONFIG += c++17

# Include paths
INCLUDEPATH += src

# You can make your code fail to compile if it uses deprecated APIs.
# In order to do so, uncomment the following line.
#DEFINES += QT_DISABLE_DEPRECATED_BEFORE=0x060000    # disables all the APIs deprecated before Qt 6.0.0

SOURCES += \
    src/main.cpp \
    src/mainwindow.cpp \
    src/polygoncanvas.cpp \
    src/projectconfig.cpp \
    src/settingstabbase.cpp \
    src/projectsettingstab.cpp \
    src/aimodelsettingstab.cpp \
    src/importexportsettingstab.cpp \
    src/settingsdialog.cpp \
    src/modelcomparisondialog.cpp \
    src/shortcutseditor.cpp

HEADERS += \
    src/mainwindow.h \
    src/polygoncanvas.h \
    src/projectconfig.h \
    src/settingstabbase.h \
    src/projectsettingstab.h \
    src/aimodelsettingstab.h \
    src/importexportsettingstab.h \
    src/settingsdialog.h \
    src/modelcomparisondialog.h \
    src/shortcutseditor.h

FORMS += \
    src/mainwindow.ui

# Default rules for deployment.
qnx: target.path = /tmp/$${TARGET}/bin
else: unix:!android: target.path = /opt/$${TARGET}/bin
!isEmpty(target.path): INSTALLS += target
