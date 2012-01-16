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

#include <qtest.h>
#include <private/qdeclarativepropertycache_p.h>
#include <QtDeclarative/qdeclarativeengine.h>
#include "../../shared/util.h"

class tst_qdeclarativepropertycache : public QObject
{
    Q_OBJECT
public:
    tst_qdeclarativepropertycache() {}

private slots:
    void properties();
    void propertiesDerived();
    void methods();
    void methodsDerived();
    void signalHandlers();
    void signalHandlersDerived();

private:
    QDeclarativeEngine engine;
};

class BaseObject : public QObject
{
    Q_OBJECT
    Q_PROPERTY(int propertyA READ propertyA NOTIFY propertyAChanged)
    Q_PROPERTY(QString propertyB READ propertyB NOTIFY propertyBChanged)
public:
    BaseObject(QObject *parent = 0) : QObject(parent) {}

    int propertyA() const { return 0; }
    QString propertyB() const { return QString(); }

public Q_SLOTS:
    void slotA() {}

Q_SIGNALS:
    void propertyAChanged();
    void propertyBChanged();
    void signalA();
};

class DerivedObject : public BaseObject
{
    Q_OBJECT
    Q_PROPERTY(int propertyC READ propertyC NOTIFY propertyCChanged)
    Q_PROPERTY(QString propertyD READ propertyD NOTIFY propertyDChanged)
public:
    DerivedObject(QObject *parent = 0) : BaseObject(parent) {}

    int propertyC() const { return 0; }
    QString propertyD() const { return QString(); }

public Q_SLOTS:
    void slotB() {}

Q_SIGNALS:
    void propertyCChanged();
    void propertyDChanged();
    void signalB();
};

void tst_qdeclarativepropertycache::properties()
{
    QDeclarativeEngine engine;
    DerivedObject object;
    const QMetaObject *metaObject = object.metaObject();

    QDeclarativeRefPointer<QDeclarativePropertyCache> cache(new QDeclarativePropertyCache(&engine, metaObject));
    QDeclarativePropertyData *data;

    QVERIFY(data = cache->property(QLatin1String("propertyA")));
    QCOMPARE(data->coreIndex, metaObject->indexOfProperty("propertyA"));

    QVERIFY(data = cache->property(QLatin1String("propertyB")));
    QCOMPARE(data->coreIndex, metaObject->indexOfProperty("propertyB"));

    QVERIFY(data = cache->property(QLatin1String("propertyC")));
    QCOMPARE(data->coreIndex, metaObject->indexOfProperty("propertyC"));

    QVERIFY(data = cache->property(QLatin1String("propertyD")));
    QCOMPARE(data->coreIndex, metaObject->indexOfProperty("propertyD"));
}

void tst_qdeclarativepropertycache::propertiesDerived()
{
    QDeclarativeEngine engine;
    DerivedObject object;
    const QMetaObject *metaObject = object.metaObject();

    QDeclarativeRefPointer<QDeclarativePropertyCache> parentCache(new QDeclarativePropertyCache(&engine, &BaseObject::staticMetaObject));
    QDeclarativeRefPointer<QDeclarativePropertyCache> cache(parentCache->copy());
    cache->append(&engine, object.metaObject());
    QDeclarativePropertyData *data;

    QVERIFY(data = cache->property(QLatin1String("propertyA")));
    QCOMPARE(data->coreIndex, metaObject->indexOfProperty("propertyA"));

    QVERIFY(data = cache->property(QLatin1String("propertyB")));
    QCOMPARE(data->coreIndex, metaObject->indexOfProperty("propertyB"));

    QVERIFY(data = cache->property(QLatin1String("propertyC")));
    QCOMPARE(data->coreIndex, metaObject->indexOfProperty("propertyC"));

    QVERIFY(data = cache->property(QLatin1String("propertyD")));
    QCOMPARE(data->coreIndex, metaObject->indexOfProperty("propertyD"));
}

void tst_qdeclarativepropertycache::methods()
{
    QDeclarativeEngine engine;
    DerivedObject object;
    const QMetaObject *metaObject = object.metaObject();

    QDeclarativeRefPointer<QDeclarativePropertyCache> cache(new QDeclarativePropertyCache(&engine, metaObject));
    QDeclarativePropertyData *data;

    QVERIFY(data = cache->property(QLatin1String("slotA")));
    QCOMPARE(data->coreIndex, metaObject->indexOfMethod("slotA()"));

    QVERIFY(data = cache->property(QLatin1String("slotB")));
    QCOMPARE(data->coreIndex, metaObject->indexOfMethod("slotB()"));

    QVERIFY(data = cache->property(QLatin1String("signalA")));
    QCOMPARE(data->coreIndex, metaObject->indexOfMethod("signalA()"));

    QVERIFY(data = cache->property(QLatin1String("signalB")));
    QCOMPARE(data->coreIndex, metaObject->indexOfMethod("signalB()"));

    QVERIFY(data = cache->property(QLatin1String("propertyAChanged")));
    QCOMPARE(data->coreIndex, metaObject->indexOfMethod("propertyAChanged()"));

    QVERIFY(data = cache->property(QLatin1String("propertyBChanged")));
    QCOMPARE(data->coreIndex, metaObject->indexOfMethod("propertyBChanged()"));

    QVERIFY(data = cache->property(QLatin1String("propertyCChanged")));
    QCOMPARE(data->coreIndex, metaObject->indexOfMethod("propertyCChanged()"));

    QVERIFY(data = cache->property(QLatin1String("propertyDChanged")));
    QCOMPARE(data->coreIndex, metaObject->indexOfMethod("propertyDChanged()"));
}

void tst_qdeclarativepropertycache::methodsDerived()
{
    QDeclarativeEngine engine;
    DerivedObject object;
    const QMetaObject *metaObject = object.metaObject();

    QDeclarativeRefPointer<QDeclarativePropertyCache> parentCache(new QDeclarativePropertyCache(&engine, &BaseObject::staticMetaObject));
    QDeclarativeRefPointer<QDeclarativePropertyCache> cache(parentCache->copy());
    cache->append(&engine, object.metaObject());
    QDeclarativePropertyData *data;

    QVERIFY(data = cache->property(QLatin1String("slotA")));
    QCOMPARE(data->coreIndex, metaObject->indexOfMethod("slotA()"));

    QVERIFY(data = cache->property(QLatin1String("slotB")));
    QCOMPARE(data->coreIndex, metaObject->indexOfMethod("slotB()"));

    QVERIFY(data = cache->property(QLatin1String("signalA")));
    QCOMPARE(data->coreIndex, metaObject->indexOfMethod("signalA()"));

    QVERIFY(data = cache->property(QLatin1String("signalB")));
    QCOMPARE(data->coreIndex, metaObject->indexOfMethod("signalB()"));

    QVERIFY(data = cache->property(QLatin1String("propertyAChanged")));
    QCOMPARE(data->coreIndex, metaObject->indexOfMethod("propertyAChanged()"));

    QVERIFY(data = cache->property(QLatin1String("propertyBChanged")));
    QCOMPARE(data->coreIndex, metaObject->indexOfMethod("propertyBChanged()"));

    QVERIFY(data = cache->property(QLatin1String("propertyCChanged")));
    QCOMPARE(data->coreIndex, metaObject->indexOfMethod("propertyCChanged()"));

    QVERIFY(data = cache->property(QLatin1String("propertyDChanged")));
    QCOMPARE(data->coreIndex, metaObject->indexOfMethod("propertyDChanged()"));
}

void tst_qdeclarativepropertycache::signalHandlers()
{
    QDeclarativeEngine engine;
    DerivedObject object;
    const QMetaObject *metaObject = object.metaObject();

    QDeclarativeRefPointer<QDeclarativePropertyCache> cache(new QDeclarativePropertyCache(&engine, metaObject));
    QDeclarativePropertyData *data;

    QVERIFY(data = cache->property(QLatin1String("onSignalA")));
    QCOMPARE(data->coreIndex, metaObject->indexOfMethod("signalA()"));

    QVERIFY(data = cache->property(QLatin1String("onSignalB")));
    QCOMPARE(data->coreIndex, metaObject->indexOfMethod("signalB()"));

    QVERIFY(data = cache->property(QLatin1String("onPropertyAChanged")));
    QCOMPARE(data->coreIndex, metaObject->indexOfMethod("propertyAChanged()"));

    QVERIFY(data = cache->property(QLatin1String("onPropertyBChanged")));
    QCOMPARE(data->coreIndex, metaObject->indexOfMethod("propertyBChanged()"));

    QVERIFY(data = cache->property(QLatin1String("onPropertyCChanged")));
    QCOMPARE(data->coreIndex, metaObject->indexOfMethod("propertyCChanged()"));

    QVERIFY(data = cache->property(QLatin1String("onPropertyDChanged")));
    QCOMPARE(data->coreIndex, metaObject->indexOfMethod("propertyDChanged()"));
}

void tst_qdeclarativepropertycache::signalHandlersDerived()
{
    QDeclarativeEngine engine;
    DerivedObject object;
    const QMetaObject *metaObject = object.metaObject();

    QDeclarativeRefPointer<QDeclarativePropertyCache> parentCache(new QDeclarativePropertyCache(&engine, &BaseObject::staticMetaObject));
    QDeclarativeRefPointer<QDeclarativePropertyCache> cache(parentCache->copy());
    cache->append(&engine, object.metaObject());
    QDeclarativePropertyData *data;

    QVERIFY(data = cache->property(QLatin1String("onSignalA")));
    QCOMPARE(data->coreIndex, metaObject->indexOfMethod("signalA()"));

    QVERIFY(data = cache->property(QLatin1String("onSignalB")));
    QCOMPARE(data->coreIndex, metaObject->indexOfMethod("signalB()"));

    QVERIFY(data = cache->property(QLatin1String("onPropertyAChanged")));
    QCOMPARE(data->coreIndex, metaObject->indexOfMethod("propertyAChanged()"));

    QVERIFY(data = cache->property(QLatin1String("onPropertyBChanged")));
    QCOMPARE(data->coreIndex, metaObject->indexOfMethod("propertyBChanged()"));

    QVERIFY(data = cache->property(QLatin1String("onPropertyCChanged")));
    QCOMPARE(data->coreIndex, metaObject->indexOfMethod("propertyCChanged()"));

    QVERIFY(data = cache->property(QLatin1String("onPropertyDChanged")));
    QCOMPARE(data->coreIndex, metaObject->indexOfMethod("propertyDChanged()"));
}

QTEST_MAIN(tst_qdeclarativepropertycache)

#include "tst_qdeclarativepropertycache.moc"
