load(qt_module)

TARGET = qmldbg_inspector
QT       += declarative-private

load(qt_plugin)

DESTDIR  = $$QT.declarative.plugins/qmltooling
QTDIR_build:REQUIRES += "contains(QT_CONFIG, declarative)"

SOURCES += \
    qdeclarativeinspectorplugin.cpp \
    qdeclarativeviewinspector.cpp \
    editor/abstractliveedittool.cpp \
    editor/liveselectiontool.cpp \
    editor/livelayeritem.cpp \
    editor/livesingleselectionmanipulator.cpp \
    editor/liverubberbandselectionmanipulator.cpp \
    editor/liveselectionrectangle.cpp \
    editor/liveselectionindicator.cpp \
    editor/boundingrecthighlighter.cpp \
    editor/subcomponentmasklayeritem.cpp \
    editor/zoomtool.cpp \
    editor/colorpickertool.cpp \
    editor/qmltoolbar.cpp \
    editor/toolbarcolorbox.cpp

HEADERS += \
    qdeclarativeinspectorplugin.h \
    qdeclarativeinspectorprotocol.h \
    qdeclarativeviewinspector_p.h \
    qdeclarativeviewinspector_p_p.h \
    qmlinspectorconstants_p.h \
    editor/abstractliveedittool_p.h \
    editor/liveselectiontool_p.h \
    editor/livelayeritem_p.h \
    editor/livesingleselectionmanipulator_p.h \
    editor/liverubberbandselectionmanipulator_p.h \
    editor/liveselectionrectangle_p.h \
    editor/liveselectionindicator_p.h \
    editor/boundingrecthighlighter_p.h \
    editor/subcomponentmasklayeritem_p.h \
    editor/zoomtool_p.h \
    editor/colorpickertool_p.h \
    editor/qmltoolbar_p.h \
    editor/toolbarcolorbox_p.h

RESOURCES += editor/editor.qrc

target.path += $$[QT_INSTALL_PLUGINS]/qmltooling
INSTALLS += target

symbian:TARGET.UID3=0x20031E90
