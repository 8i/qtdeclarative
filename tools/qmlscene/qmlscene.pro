TEMPLATE = app
TARGET = qmlscene
DESTDIR= $$QT.declarative.bins

QT += declarative quick
!isEmpty(QT.widgets.name): QT += widgets

target.path = $$[QT_INSTALL_BINS]
INSTALLS += target

macx: CONFIG -= app_bundle

SOURCES += main.cpp

CONFIG += console declarative_debug

DEFINES += QML_RUNTIME_TESTING
