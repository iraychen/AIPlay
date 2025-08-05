QT += core gui widgets sql

TARGET = SqlServerBackup
TEMPLATE = lib
CONFIG += plugin

# The following define makes your compiler emit warnings if you use
# any feature of Qt which has been marked as deprecated
DEFINES += QT_DEPRECATED_WARNINGS

# You can also make your code fail to compile if you use deprecated APIs.
# In order to do so, uncomment the following line.
DEFINES += QT_DISABLE_DEPRECATED_BEFORE=0x060000    # disables all the APIs deprecated before Qt 6.0.0

SOURCES += \
    SqlServerBackupPlugin.cpp

HEADERS += \
    SqlServerBackupPlugin.h

DISTFILES += \
    SqlServerBackup.json

# Link with PluginCore
win32:CONFIG(release, debug|release): LIBS += -L$$PWD/../../build/release/ -lPluginCore
else:win32:CONFIG(debug, debug|release): LIBS += -L$$PWD/../../build/debug/ -lPluginCore
else:unix: LIBS += -L$$PWD/../../build/release/ -lPluginCore

INCLUDEPATH += $$PWD/../../
DEPENDPATH += $$PWD/../../

# Output directory
CONFIG(debug, debug|release) {
    DESTDIR = $$PWD/../../build/debug/plugins
} else {
    DESTDIR = $$PWD/../../build/release/plugins
}

OBJECTS_DIR = $$DESTDIR/.obj
MOC_DIR = $$DESTDIR/.moc
RCC_DIR = $$DESTDIR/.qrc
UI_DIR = $$DESTDIR/.ui

# Copy metadata to build directory
metadata.path = $$DESTDIR/../metadata
metadata.files = SqlServerBackup.json
INSTALLS += metadata

# Copy metadata during build
win32 {
    QMAKE_POST_LINK += $$QMAKE_COPY $$shell_path($$PWD/SqlServerBackup.json) $$shell_path($$DESTDIR/../metadata/) $$escape_expand(\\n\\t)
} else {
    QMAKE_POST_LINK += $$QMAKE_COPY $$PWD/SqlServerBackup.json $$DESTDIR/../metadata/ $$escape_expand(\\n\\t)
}