CONFIG += testcase
TARGET = tst_qdeclarativestates
macx:CONFIG -= app_bundle

SOURCES += tst_qdeclarativestates.cpp

DEFINES += SRCDIR=\\\"$$PWD\\\"

CONFIG += parallel_test
QT += core-private gui-private widgets-private v8-private declarative-private qtquick1-private testlib
