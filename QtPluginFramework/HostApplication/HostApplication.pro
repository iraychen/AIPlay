QT += core gui widgets

TARGET = HostApplication
TEMPLATE = app

# The following define makes your compiler emit warnings if you use
# any feature of Qt which has been marked as deprecated
DEFINES += QT_DEPRECATED_WARNINGS

# You can also make your code fail to compile if you use deprecated APIs.
# In order to do so, uncomment the following line.
DEFINES += QT_DISABLE_DEPRECATED_BEFORE=0x060000    # disables all the APIs deprecated before Qt 6.0.0

SOURCES += \
    main.cpp \
    MainWindow.cpp \
    PluginManagerDialog.cpp

HEADERS += \
    MainWindow.h \
    PluginManagerDialog.h

# Link with PluginCore
win32:CONFIG(release, debug|release): LIBS += -L$$PWD/../build/release/ -lPluginCore
else:win32:CONFIG(debug, debug|release): LIBS += -L$$PWD/../build/debug/ -lPluginCore
else:unix: LIBS += -L$$PWD/../build/release/ -lPluginCore

# Add runtime dependency for Windows
win32 {
    CONFIG(debug, debug|release) {
        QMAKE_POST_LINK += $$QMAKE_COPY $$shell_path($$PWD/../build/debug/PluginCore.dll) $$shell_path($$DESTDIR/) $$escape_expand(\\n\\t)
    } else {
        QMAKE_POST_LINK += $$QMAKE_COPY $$shell_path($$PWD/../build/release/PluginCore.dll) $$shell_path($$DESTDIR/) $$escape_expand(\\n\\t)
    }
}

INCLUDEPATH += $$PWD/../
DEPENDPATH += $$PWD/../

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

# Copy plugins and metadata directories to build directory
copydata.commands = $(COPY_DIR) $$PWD/../Plugins $$DESTDIR/plugins
first.depends = $(first) copydata
export(first.depends)
export(copydata.commands)
QMAKE_EXTRA_TARGETS += first copydata