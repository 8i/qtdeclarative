load(qt_module)

TARGET     = QtDeclarative
QPRO_PWD   = $$PWD

CONFIG += module
MODULE_PRI += ../../modules/qt_declarative.pri

QT = core-private gui gui-private network sql v8-private

DEFINES   += QT_BUILD_DECLARATIVE_LIB QT_NO_URL_CAST_FROM_STRING QT_NO_INTEGER_EVENT_COORDINATES
win32-msvc*|win32-icc:QMAKE_LFLAGS += /BASE:0x66000000
win32-msvc*:DEFINES *= _CRT_SECURE_NO_WARNINGS
solaris-cc*:QMAKE_CXXFLAGS_RELEASE -= -O2

unix|win32-g++*:QMAKE_PKGCONFIG_REQUIRES = QtCore QtGui

exists("qdeclarative_enable_gcov") {
    QMAKE_CXXFLAGS = -fprofile-arcs -ftest-coverage -fno-elide-constructors
    LIBS += -lgcov
}

load(qt_module_config)

HEADERS += qtdeclarativeversion.h

#INCLUDEPATH -= $$QMAKE_INCDIR_QT/$$TARGET
#DESTDIR=.

#modules
include(util/util.pri)
include(qml/qml.pri)
include(debugger/debugger.pri)
include(scenegraph/scenegraph.pri)
include(items/items.pri)
include(particles/particles.pri)
include(designer/designer.pri)
include(animations/animations.pri)

HEADERS += qtquick2_p.h
SOURCES += qtquick2.cpp
