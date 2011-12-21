INCLUDEPATH += $$PWD/../../../3rdparty/javascriptcore

include(script.pri)

HEADERS += \
    $$PWD/qv8_p.h \
    $$PWD/qv8debug_p.h \
    $$PWD/qv8profiler_p.h \
    $$PWD/qv8stringwrapper_p.h \
    $$PWD/qv8engine_p.h \
    $$PWD/qv8gccallback_p.h \
    $$PWD/qv8sequencewrapper_p.h \
    $$PWD/qv8sequencewrapper_p_p.h \
    $$PWD/qv8contextwrapper_p.h \
    $$PWD/qv8qobjectwrapper_p.h \
    $$PWD/qv8typewrapper_p.h \
    $$PWD/qv8listwrapper_p.h \
    $$PWD/qv8variantwrapper_p.h \
    $$PWD/qv8variantresource_p.h \
    $$PWD/qv8valuetypewrapper_p.h \
    $$PWD/qv8include_p.h \
    $$PWD/qv8worker_p.h \
    $$PWD/qv8bindings_p.h \
    $$PWD/../../../3rdparty/javascriptcore/DateMath.h \
    $$PWD/qv8engine_impl_p.h \
    $$PWD/qv8domerrors_p.h \
    $$PWD/qdeclarativebuiltinfunctions_p.h

SOURCES += \
    $$PWD/qv8stringwrapper.cpp \
    $$PWD/qv8engine.cpp \
    $$PWD/qv8sequencewrapper.cpp \
    $$PWD/qv8contextwrapper.cpp \
    $$PWD/qv8qobjectwrapper.cpp \
    $$PWD/qv8typewrapper.cpp \
    $$PWD/qv8listwrapper.cpp \
    $$PWD/qv8variantwrapper.cpp \
    $$PWD/qv8valuetypewrapper.cpp \
    $$PWD/qv8include.cpp \
    $$PWD/qv8worker.cpp \
    $$PWD/qv8bindings.cpp \
    $$PWD/../../../3rdparty/javascriptcore/DateMath.cpp \
    $$PWD/qv8domerrors.cpp \
    $$PWD/qdeclarativebuiltinfunctions.cpp
