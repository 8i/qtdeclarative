TARGET = app
QT += declarative widgets qtquick1

CONFIG += declarative_debug
macx:CONFIG -= app_bundle

SOURCES += main.cpp

OTHER_FILES += qtquick1.qml qtquick2.qml
