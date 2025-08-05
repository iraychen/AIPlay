QT += core gui widgets

TARGET = PluginCore
TEMPLATE = lib
CONFIG += shared

DEFINES += PLUGINCORE_LIBRARY

# The following define makes your compiler emit warnings if you use
# any feature of Qt which has been marked as deprecated
DEFINES += QT_DEPRECATED_WARNINGS

# You can also make your code fail to compile if you use deprecated APIs.
# In order to do so, uncomment the following line.
DEFINES += QT_DISABLE_DEPRECATED_BEFORE=0x060000    # disables all the APIs deprecated before Qt 6.0.0

SOURCES += \
    ConfigManager.cpp \
    ExceptionHandler.cpp \
    LogManager.cpp \
    PermissionManager.cpp \
    PluginCommunication.cpp \
    PluginManager.cpp \
    PluginMetadata.cpp

HEADERS += \
    ConfigManager.h \
    ExceptionHandler.h \
    IPlugin.h \
    LogManager.h \
    PermissionManager.h \
    PluginCommunication.h \
    PluginManager.h \
    PluginMetadata.h

unix {
    target.path = /usr/lib
    INSTALLS += target
}

# Output directory
CONFIG(debug, debug|release) {
    DESTDIR = $$PWD/../build/debug
} else {
    DESTDIR = $$PWD/../build/release
}

OBJECTS_DIR = $$DESTDIR/.obj
MOC_DIR = $$DESTDIR/.moc
RCC_DIR = $$DESTDIR/.qrc
UI_DIR = $$DESTDIR/.ui