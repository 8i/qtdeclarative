SOURCES += \
    $$PWD/qdeclarativeinstruction.cpp \
    $$PWD/qdeclarativevmemetaobject.cpp \
    $$PWD/qdeclarativeengine.cpp \
    $$PWD/qdeclarativeexpression.cpp \
    $$PWD/qdeclarativebinding.cpp \
    $$PWD/qdeclarativeproperty.cpp \
    $$PWD/qdeclarativecomponent.cpp \
    $$PWD/qdeclarativeincubator.cpp \
    $$PWD/qdeclarativecontext.cpp \
    $$PWD/qdeclarativecustomparser.cpp \
    $$PWD/qdeclarativepropertyvaluesource.cpp \
    $$PWD/qdeclarativepropertyvalueinterceptor.cpp \
    $$PWD/qdeclarativeproxymetaobject.cpp \
    $$PWD/qdeclarativevme.cpp \
    $$PWD/qdeclarativecompiler.cpp \
    $$PWD/qdeclarativecompileddata.cpp \
    $$PWD/qdeclarativeboundsignal.cpp \
    $$PWD/qdeclarativemetatype.cpp \
    $$PWD/qdeclarativestringconverters.cpp \
    $$PWD/qdeclarativeparserstatus.cpp \
    $$PWD/qdeclarativetypeloader.cpp \
    $$PWD/qdeclarativeinfo.cpp \
    $$PWD/qdeclarativeerror.cpp \
    $$PWD/qdeclarativescript.cpp \
    $$PWD/qdeclarativerewrite.cpp \
    $$PWD/qdeclarativevaluetype.cpp \
    $$PWD/qdeclarativefastproperties.cpp \
    $$PWD/qdeclarativexmlhttprequest.cpp \
    $$PWD/qdeclarativesqldatabase.cpp \
    $$PWD/qdeclarativewatcher.cpp \
    $$PWD/qdeclarativecleanup.cpp \
    $$PWD/qdeclarativepropertycache.cpp \
    $$PWD/qdeclarativenotifier.cpp \
    $$PWD/qdeclarativeintegercache.cpp \
    $$PWD/qdeclarativetypenotavailable.cpp \
    $$PWD/qdeclarativetypenamecache.cpp \
    $$PWD/qdeclarativescriptstring.cpp \
    $$PWD/qdeclarativeworkerscript.cpp \
    $$PWD/qdeclarativeimageprovider.cpp \
    $$PWD/qdeclarativenetworkaccessmanagerfactory.cpp \
    $$PWD/qdeclarativedirparser.cpp \
    $$PWD/qdeclarativeextensionplugin.cpp \
    $$PWD/qdeclarativeimport.cpp \
    $$PWD/qdeclarativelist.cpp \

HEADERS += \
    $$PWD/qdeclarativeglobal_p.h \
    $$PWD/qdeclarativeinstruction_p.h \
    $$PWD/qdeclarativevmemetaobject_p.h \
    $$PWD/qdeclarative.h \
    $$PWD/qdeclarativebinding_p.h \
    $$PWD/qdeclarativebinding_p_p.h \
    $$PWD/qdeclarativeproperty.h \
    $$PWD/qdeclarativecomponent.h \
    $$PWD/qdeclarativecomponent_p.h \
    $$PWD/qdeclarativeincubator.h \
    $$PWD/qdeclarativeincubator_p.h \
    $$PWD/qdeclarativecustomparser_p.h \
    $$PWD/qdeclarativecustomparser_p_p.h \
    $$PWD/qdeclarativepropertyvaluesource.h \
    $$PWD/qdeclarativepropertyvalueinterceptor.h \
    $$PWD/qdeclarativeboundsignal_p.h \
    $$PWD/qdeclarativeparserstatus.h \
    $$PWD/qdeclarativeproxymetaobject_p.h \
    $$PWD/qdeclarativevme_p.h \
    $$PWD/qdeclarativecompiler_p.h \
    $$PWD/qdeclarativeengine_p.h \
    $$PWD/qdeclarativeexpression_p.h \
    $$PWD/qdeclarativeprivate.h \
    $$PWD/qdeclarativemetatype_p.h \
    $$PWD/qdeclarativeengine.h \
    $$PWD/qdeclarativecontext.h \
    $$PWD/qdeclarativeexpression.h \
    $$PWD/qdeclarativestringconverters_p.h \
    $$PWD/qdeclarativeinfo.h \
    $$PWD/qdeclarativeproperty_p.h \
    $$PWD/qdeclarativecontext_p.h \
    $$PWD/qdeclarativetypeloader_p.h \
    $$PWD/qdeclarativelist.h \
    $$PWD/qdeclarativelist_p.h \
    $$PWD/qdeclarativedata_p.h \
    $$PWD/qdeclarativeerror.h \
    $$PWD/qdeclarativescript_p.h \
    $$PWD/qdeclarativerewrite_p.h \
    $$PWD/qdeclarativevaluetype_p.h \
    $$PWD/qdeclarativefastproperties_p.h \
    $$PWD/qdeclarativexmlhttprequest_p.h \
    $$PWD/qdeclarativesqldatabase_p.h \
    $$PWD/qdeclarativewatcher_p.h \
    $$PWD/qdeclarativecleanup_p.h \
    $$PWD/qdeclarativepropertycache_p.h \
    $$PWD/qdeclarativenotifier_p.h \
    $$PWD/qdeclarativeintegercache_p.h \
    $$PWD/qdeclarativetypenotavailable_p.h \
    $$PWD/qdeclarativetypenamecache_p.h \
    $$PWD/qdeclarativescriptstring.h \
    $$PWD/qdeclarativeworkerscript_p.h \
    $$PWD/qdeclarativeguard_p.h \
    $$PWD/qdeclarativeimageprovider.h \
    $$PWD/qdeclarativenetworkaccessmanagerfactory.h \
    $$PWD/qdeclarativedirparser_p.h \
    $$PWD/qdeclarativeextensioninterface.h \
    $$PWD/qdeclarativeimport_p.h \
    $$PWD/qdeclarativeextensionplugin.h \
    $$PWD/qdeclarativenullablevalue_p_p.h \
    $$PWD/qdeclarativescriptstring_p.h

QT += sql
include(parser/parser.pri)
include(rewriter/rewriter.pri)
include(ftw/ftw.pri)
include(v4/v4.pri)
include(v8/v8.pri)
