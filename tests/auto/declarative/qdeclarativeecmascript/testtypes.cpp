/****************************************************************************
**
** Copyright (C) 2012 Nokia Corporation and/or its subsidiary(-ies).
** All rights reserved.
** Contact: Nokia Corporation (qt-info@nokia.com)
**
** This file is part of the test suite of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:LGPL$
** GNU Lesser General Public License Usage
** This file may be used under the terms of the GNU Lesser General Public
** License version 2.1 as published by the Free Software Foundation and
** appearing in the file LICENSE.LGPL included in the packaging of this
** file. Please review the following information to ensure the GNU Lesser
** General Public License version 2.1 requirements will be met:
** http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Nokia gives you certain additional
** rights. These rights are described in the Nokia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU General
** Public License version 3.0 as published by the Free Software Foundation
** and appearing in the file LICENSE.GPL included in the packaging of this
** file. Please review the following information to ensure the GNU General
** Public License version 3.0 requirements will be met:
** http://www.gnu.org/copyleft/gpl.html.
**
** Other Usage
** Alternatively, this file may be used in accordance with the terms and
** conditions contained in a signed written agreement between you and Nokia.
**
**
**
**
**
** $QT_END_LICENSE$
**
****************************************************************************/
#include "testtypes.h"
#include <QWidget>
#include <QPlainTextEdit>
#include <QDeclarativeEngine>
#include <QJSEngine>

class BaseExtensionObject : public QObject
{
    Q_OBJECT
    Q_PROPERTY(int baseExtendedProperty READ extendedProperty WRITE setExtendedProperty NOTIFY extendedPropertyChanged)
public:
    BaseExtensionObject(QObject *parent) : QObject(parent), m_value(0) {}

    int extendedProperty() const { return m_value; }
    void setExtendedProperty(int v) { m_value = v; emit extendedPropertyChanged(); }

signals:
    void extendedPropertyChanged();
private:
    int m_value;
};

class ExtensionObject : public QObject
{
    Q_OBJECT
    Q_PROPERTY(int extendedProperty READ extendedProperty WRITE setExtendedProperty NOTIFY extendedPropertyChanged)
public:
    ExtensionObject(QObject *parent) : QObject(parent), m_value(0) {}

    int extendedProperty() const { return m_value; }
    void setExtendedProperty(int v) { m_value = v; emit extendedPropertyChanged(); }

signals:
    void extendedPropertyChanged();
private:
    int m_value;
};

class DefaultPropertyExtensionObject : public QObject
{
    Q_OBJECT
    Q_CLASSINFO("DefaultProperty", "firstProperty")
public:
    DefaultPropertyExtensionObject(QObject *parent) : QObject(parent) {}
};

class QWidgetDeclarativeUI : public QObject
{
    Q_OBJECT

    Q_PROPERTY(int width READ width WRITE setWidth NOTIFY widthChanged)

signals:
    void widthChanged();

public:
    QWidgetDeclarativeUI(QObject *other) : QObject(other) { }

public:
    int width() const { return 0; }
    void setWidth(int) { }
};

void MyQmlObject::v8function(QDeclarativeV8Function *args)
{
    const char *error = "Exception thrown from within QObject slot";
    v8::ThrowException(v8::Exception::Error(v8::String::New(error)));
}

static QJSValue script_api(QDeclarativeEngine *engine, QJSEngine *scriptEngine)
{
    Q_UNUSED(engine)

    static int testProperty = 13;
    QJSValue v = scriptEngine->newObject();
    v.setProperty("scriptTestProperty", testProperty++);
    return v;
}

static QJSValue readonly_script_api(QDeclarativeEngine *engine, QJSEngine *scriptEngine)
{
    Q_UNUSED(engine)

    static int testProperty = 42;
    QJSValue v = scriptEngine->newObject();
    v.setProperty("scriptTestProperty", testProperty++);

    // now freeze it so that it's read-only
    QJSValue freezeFunction = scriptEngine->evaluate("(function(obj) { return Object.freeze(obj); })");
    v = freezeFunction.call(QJSValue(), (QJSValueList() << v));

    return v;
}

static QObject *qobject_api(QDeclarativeEngine *engine, QJSEngine *scriptEngine)
{
    Q_UNUSED(engine)
    Q_UNUSED(scriptEngine)

    testQObjectApi *o = new testQObjectApi();
    o->setQObjectTestProperty(20);
    o->setQObjectTestWritableProperty(50);
    return o;
}

static QObject *qobject_api_engine_parent(QDeclarativeEngine *engine, QJSEngine *scriptEngine)
{
    Q_UNUSED(scriptEngine)

    static int testProperty = 26;
    testQObjectApi *o = new testQObjectApi(engine);
    o->setQObjectTestProperty(testProperty++);
    return o;
}

void registerTypes()
{
    qmlRegisterType<MyQmlObject>("Qt.test", 1,0, "MyQmlObjectAlias");
    qmlRegisterType<MyQmlObject>("Qt.test", 1,0, "MyQmlObject");
    qmlRegisterType<MyDeferredObject>("Qt.test", 1,0, "MyDeferredObject");
    qmlRegisterType<MyQmlContainer>("Qt.test", 1,0, "MyQmlContainer");
    qmlRegisterExtendedType<MyBaseExtendedObject, BaseExtensionObject>("Qt.test", 1,0, "MyBaseExtendedObject");
    qmlRegisterExtendedType<MyExtendedObject, ExtensionObject>("Qt.test", 1,0, "MyExtendedObject");
    qmlRegisterType<MyTypeObject>("Qt.test", 1,0, "MyTypeObject");
    qmlRegisterType<MyDerivedObject>("Qt.test", 1,0, "MyDerivedObject");
    qmlRegisterType<NumberAssignment>("Qt.test", 1,0, "NumberAssignment");
    qmlRegisterExtendedType<DefaultPropertyExtendedObject, DefaultPropertyExtensionObject>("Qt.test", 1,0, "DefaultPropertyExtendedObject");
    qmlRegisterType<OverrideDefaultPropertyObject>("Qt.test", 1,0, "OverrideDefaultPropertyObject");
    qmlRegisterType<MyRevisionedClass>("Qt.test",1,0,"MyRevisionedClass");
    qmlRegisterType<MyDeleteObject>("Qt.test", 1,0, "MyDeleteObject");
    qmlRegisterType<MyRevisionedClass,1>("Qt.test",1,1,"MyRevisionedClass");

    // test scarce resource property binding post-evaluation optimisation
    // and for testing memory usage in property var circular reference test
    qmlRegisterType<ScarceResourceObject>("Qt.test", 1,0, "MyScarceResourceObject");

    // Register the uncreatable base class
    qmlRegisterRevision<MyRevisionedBaseClassRegistered,1>("Qt.test",1,1);
    // MyRevisionedSubclass 1.0 uses MyRevisionedClass revision 0
    qmlRegisterType<MyRevisionedSubclass>("Qt.test",1,0,"MyRevisionedSubclass");
    // MyRevisionedSubclass 1.1 uses MyRevisionedClass revision 1
    qmlRegisterType<MyRevisionedSubclass,1>("Qt.test",1,1,"MyRevisionedSubclass");

    qmlRegisterExtendedType<QWidget,QWidgetDeclarativeUI>("Qt.test",1,0,"QWidget");
    qmlRegisterType<QPlainTextEdit>("Qt.test",1,0,"QPlainTextEdit");

    qRegisterMetaType<MyQmlObject::MyType>("MyQmlObject::MyType");

    qmlRegisterModuleApi("Qt.test",1,0,script_api);             // register (script) module API for an existing uri which contains elements
    qmlRegisterModuleApi("Qt.test",1,0,qobject_api);            // register (qobject) for an existing uri for which another module API was previously regd.  Should replace!
    qmlRegisterModuleApi("Qt.test.scriptApi",1,0,script_api);   // register (script) module API for a uri which doesn't contain elements
    qmlRegisterModuleApi("Qt.test.scriptApi",2,0,readonly_script_api); // register (script) module API for a uri which doesn't contain elements - will be made read-only
    qmlRegisterModuleApi("Qt.test.qobjectApi",1,0,qobject_api); // register (qobject) module API for a uri which doesn't contain elements
    qmlRegisterModuleApi("Qt.test.qobjectApi",1,3,qobject_api); // register (qobject) module API for a uri which doesn't contain elements, minor version set
    qmlRegisterModuleApi("Qt.test.qobjectApi",2,0,qobject_api); // register (qobject) module API for a uri which doesn't contain elements, major version set
    qmlRegisterModuleApi("Qt.test.qobjectApiParented",1,0,qobject_api_engine_parent); // register (parented qobject) module API for a uri which doesn't contain elements

    qRegisterMetaType<MyQmlObject::MyType>("MyEnum2");
    qRegisterMetaType<Qt::MouseButtons>("Qt::MouseButtons");

    qmlRegisterType<CircularReferenceObject>("Qt.test", 1, 0, "CircularReferenceObject");
    qmlRegisterType<CircularReferenceHandle>("Qt.test", 1, 0, "CircularReferenceHandle");

    qmlRegisterType<MyDynamicCreationDestructionObject>("Qt.test", 1, 0, "MyDynamicCreationDestructionObject");
    qmlRegisterType<WriteCounter>("Qt.test", 1, 0, "WriteCounter");

    qmlRegisterType<MySequenceConversionObject>("Qt.test", 1, 0, "MySequenceConversionObject");
}

#include "testtypes.moc"
