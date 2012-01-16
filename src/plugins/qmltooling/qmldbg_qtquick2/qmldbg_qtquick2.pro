load(qt_module)

TARGET = qmldbg_qtquick2
QT       += declarative-private quick-private core-private gui-private opengl-private v8-private

load(qt_plugin)

DESTDIR  = $$QT.declarative.plugins/qmltooling

INCLUDEPATH *= $$PWD $$PWD/../shared

SOURCES += \
    qtquick2plugin.cpp \
    highlight.cpp \
    selectiontool.cpp \
    qquickviewinspector.cpp \
    ../shared/abstracttool.cpp \
    ../shared/abstractviewinspector.cpp

HEADERS += \
    qtquick2plugin.h \
    highlight.h \
    selectiontool.h \
    qquickviewinspector.h \
    ../shared/abstracttool.h \
    ../shared/abstractviewinspector.h \
    ../shared/qdeclarativeinspectorprotocol.h \
    ../shared/qmlinspectorconstants.h

target.path += $$[QT_INSTALL_PLUGINS]/qmltooling
INSTALLS += target
