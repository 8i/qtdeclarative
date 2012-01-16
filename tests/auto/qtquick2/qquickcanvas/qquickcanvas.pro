CONFIG += testcase
TARGET = tst_qquickcanvas
SOURCES += tst_qquickcanvas.cpp

include (../../shared/util.pri)

macx:CONFIG -= app_bundle

CONFIG += parallel_test
QT += core-private gui-private declarative-private quick-private testlib

testData.files = data
testData.path = .
DEPLOYMENT += testData

OTHER_FILES += \
    data/AnimationsWhileHidden.qml \
    data/Headless.qml


