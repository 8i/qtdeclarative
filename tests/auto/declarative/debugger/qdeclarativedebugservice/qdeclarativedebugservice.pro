CONFIG += testcase
TARGET = tst_qdeclarativedebugservice
macx:CONFIG -= app_bundle

HEADERS += ../shared/debugutil_p.h
SOURCES += tst_qdeclarativedebugservice.cpp \
           ../shared/debugutil.cpp

CONFIG += parallel_test declarative_debug

QT += core-private gui-private declarative-private network testlib
