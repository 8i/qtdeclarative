CONFIG += testcase
TARGET = tst_qdeclarativeflipable
macx:CONFIG -= app_bundle

SOURCES += tst_qdeclarativeflipable.cpp

DEFINES += SRCDIR=\\\"$$PWD\\\"

CONFIG += parallel_test

QT += core-private gui-private widgets-private v8-private declarative-private qtquick1-private testlib
contains(QT_CONFIG,xcb):CONFIG+=insignificant_test  # QTBUG-21012 fails on exit (X11-specific)
