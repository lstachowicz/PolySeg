QT       += core gui network

greaterThan(QT_MAJOR_VERSION, 4): QT += widgets

CONFIG += c++17 strict_c++

# Compiler warning flags (best practices)
gcc|clang {
    QMAKE_CXXFLAGS += -Wall
    QMAKE_CXXFLAGS += -Wextra
    QMAKE_CXXFLAGS += -Wpedantic
    QMAKE_CXXFLAGS += -Wshadow
    QMAKE_CXXFLAGS += -Wnon-virtual-dtor
    QMAKE_CXXFLAGS += -Wold-style-cast
    QMAKE_CXXFLAGS += -Wcast-align
    QMAKE_CXXFLAGS += -Wunused
    QMAKE_CXXFLAGS += -Woverloaded-virtual
    # warnings in Qt headers that cannot be fixed in user code
    # QMAKE_CXXFLAGS += -Wconversion
    # QMAKE_CXXFLAGS += -Wsign-conversion
    QMAKE_CXXFLAGS += -Wnull-dereference
    QMAKE_CXXFLAGS += -Wdouble-promotion
    QMAKE_CXXFLAGS += -Wformat=2
    QMAKE_CXXFLAGS += -Wimplicit-fallthrough
}

# MSVC warning flags
msvc {
    QMAKE_CXXFLAGS += /W4
    QMAKE_CXXFLAGS += /permissive-
}

# GCC-specific warnings
gcc:!clang {
    QMAKE_CXXFLAGS += -Wduplicated-cond
    QMAKE_CXXFLAGS += -Wduplicated-branches
    QMAKE_CXXFLAGS += -Wlogical-op
# warnings in Qt headers that cannot be fixed in user code
#    QMAKE_CXXFLAGS += -Wuseless-cast
}

# Treat warnings as errors in release builds (optional, uncomment if desired)
# CONFIG(release, debug|release): QMAKE_CXXFLAGS += -Werror

# Debug-specific flags
CONFIG(debug, debug|release) {
    gcc|clang {
        QMAKE_CXXFLAGS += -g3
        QMAKE_CXXFLAGS += -fno-omit-frame-pointer
        # Enable sanitizers for debug builds (uncomment as needed)
        # QMAKE_CXXFLAGS += -fsanitize=address,undefined
        # QMAKE_LFLAGS += -fsanitize=address,undefined
    }
    msvc {
        QMAKE_CXXFLAGS += /Zi
    }
}

# Release-specific flags
CONFIG(release, debug|release) {
    gcc|clang {
        QMAKE_CXXFLAGS += -O2
    }
    msvc {
        QMAKE_CXXFLAGS += /O2
    }
    DEFINES += NDEBUG
}

# Include paths
INCLUDEPATH += src

# You can make your code fail to compile if it uses deprecated APIs.
# In order to do so, uncomment the following line.
#DEFINES += QT_DISABLE_DEPRECATED_BEFORE=0x060800    # disables all APIs deprecated before 6.8

SOURCES += \
    src/main.cpp \
    src/mainwindow.cpp \
    src/modelcomparisondialog.cpp \
    src/modeldownloadmanager.cpp \
    src/modelregistrationdialog.cpp \
    src/polygoncanvas.cpp \
    src/projectconfig.cpp \
    src/pythonenvironmentmanager.cpp \
    src/settingstabbase.cpp \
    src/projectsettingstab.cpp \
    src/aimodelsettingstab.cpp \
    src/importexportsettingstab.cpp \
    src/settingsdialog.cpp \
    src/shortcuteditdialog.cpp \
    src/shortcutssettingstab.cpp \
    src/aipluginmanager.cpp \
    src/pluginwizard.cpp \
    src/wizardpages/welcomepage.cpp \
    src/wizardpages/pluginselectionpage.cpp \
    src/wizardpages/custompluginpage.cpp \
    src/wizardpages/modelselectionpage.cpp \
    src/wizardpages/pretrainedmodelpage.cpp \
    src/wizardpages/downloadpage.cpp \
    src/wizardpages/configurationpage.cpp \
    src/wizardpages/customconfigurationpage.cpp \
    src/wizardpages/summarypage.cpp

HEADERS += \
    src/mainwindow.h \
    src/modelcomparisondialog.h \
    src/modeldownloadmanager.h \
    src/modelregistrationdialog.h \
    src/polygoncanvas.h \
    src/projectconfig.h \
    src/pythonenvironmentmanager.h \
    src/settingstabbase.h \
    src/projectsettingstab.h \
    src/aimodelsettingstab.h \
    src/importexportsettingstab.h \
    src/settingsdialog.h \
    src/shortcuteditdialog.h \
    src/shortcutssettingstab.h \
    src/aipluginmanager.h \
    src/pluginwizard.h \
    src/wizardpages/welcomepage.h \
    src/wizardpages/pluginselectionpage.h \
    src/wizardpages/custompluginpage.h \
    src/wizardpages/modelselectionpage.h \
    src/wizardpages/pretrainedmodelpage.h \
    src/wizardpages/downloadpage.h \
    src/wizardpages/configurationpage.h \
    src/wizardpages/customconfigurationpage.h \
    src/wizardpages/summarypage.h

FORMS += \
    src/mainwindow.ui \
    src/settingsdialog.ui \
    src/aimodelsettingstab.ui \
    src/projectsettingstab.ui \
    src/importexportsettingstab.ui \
    src/shortcutssettingstab.ui \
    src/shortcuteditdialog.ui \
    src/modelcomparisondialog.ui \
    src/modelregistrationdialog.ui \
    src/wizardpages/welcomepage.ui \
    src/wizardpages/pluginselectionpage.ui \
    src/wizardpages/custompluginpage.ui \
    src/wizardpages/modelselectionpage.ui \
    src/wizardpages/pretrainedmodelpage.ui \
    src/wizardpages/downloadpage.ui \
    src/wizardpages/configurationpage.ui \
    src/wizardpages/customconfigurationpage.ui \
    src/wizardpages/summarypage.ui

RESOURCES += \
    resources/resources.qrc

# Default rules for deployment.
qnx: target.path = /tmp/$${TARGET}/bin
else: unix:!android: target.path = /opt/$${TARGET}/bin
!isEmpty(target.path): INSTALLS += target
