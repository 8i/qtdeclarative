load(qt_module)

TARGET     = QtQuickTest
QPRO_PWD   = $$PWD

CONFIG += module
CONFIG += dll warn_on declarative_debug
MODULE_PRI += ../../modules/qt_qmltest.pri

QT += testlib-private declarative testlib quick qtquick1 widgets widgets-private gui
DEFINES += QT_BUILD_QUICK_TEST_LIB QT_NO_URL_CAST_FROM_STRING


load(qt_module_config)

# Install qmltestcase.prf into the Qt mkspecs so that "CONFIG += qmltestcase"
# can be used in customer applications to build against QtQuickTest.
feature.path = $$[QT_INSTALL_DATA]/mkspecs/features
feature.files = $$PWD/features/qmltestcase.prf
INSTALLS += feature

INCLUDEPATH += $$PWD/QtQuickTest
INCLUDEPATH += $$PWD

SOURCES += \
    $$PWD/quicktest.cpp \
    $$PWD/quicktestevent.cpp \
    $$PWD/quicktestresult.cpp
HEADERS += \
    $$PWD/quicktestglobal.h \
    $$PWD/quicktest.h \
    $$PWD/quicktestevent_p.h \
    $$PWD/quicktestresult_p.h \
    $$PWD/qtestoptions_p.h


DEFINES += QT_BUILD_QUICK_TEST_LIB

