CONFIG += testcase
TARGET = tst_qdeclarativecomponent
macx:CONFIG -= app_bundle

SOURCES += tst_qdeclarativecomponent.cpp

include (../../shared/util.pri)

testDataFiles.files = data
testDataFiles.path = .
DEPLOYMENT += testDataFiles

CONFIG += parallel_test

QT += core-private gui-private declarative-private network testlib
