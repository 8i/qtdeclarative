CONFIG += testcase
TARGET = tst_qdeclarativelistreference
macx:CONFIG -= app_bundle

SOURCES += tst_qdeclarativelistreference.cpp

testDataFiles.files = data
testDataFiles.path = .
DEPLOYMENT += testDataFiles

CONFIG += parallel_test

QT += core-private gui-private declarative-private testlib
