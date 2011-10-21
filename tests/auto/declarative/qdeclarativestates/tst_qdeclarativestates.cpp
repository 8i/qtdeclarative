/****************************************************************************
**
** Copyright (C) 2011 Nokia Corporation and/or its subsidiary(-ies).
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
#include <QtDeclarative/qdeclarativeengine.h>
#include <QtDeclarative/qdeclarativecomponent.h>
#include <QtDeclarative/qsgview.h>
#include <private/qsgstateoperations_p.h>
#include <private/qsganchors_p_p.h>
#include <private/qsgrectangle_p.h>
#include <private/qsgimage_p.h>
#include <private/qdeclarativepropertychanges_p.h>
#include <private/qdeclarativestategroup_p.h>
#include <private/qsgitem_p.h>
#include <private/qdeclarativeproperty_p.h>
#include "../shared/util.h"

class MyAttached : public QObject
{
    Q_OBJECT
    Q_PROPERTY(int foo READ foo WRITE setFoo)
public:
    MyAttached(QObject *parent) : QObject(parent), m_foo(13) {}

    int foo() const { return m_foo; }
    void setFoo(int f) { m_foo = f; }

private:
    int m_foo;
};

class MyRect : public QSGRectangle
{
   Q_OBJECT
   Q_PROPERTY(int propertyWithNotify READ propertyWithNotify WRITE setPropertyWithNotify NOTIFY oddlyNamedNotifySignal)
public:
    MyRect() {}

    void doSomething() { emit didSomething(); }
    
    int propertyWithNotify() const { return m_prop; }
    void setPropertyWithNotify(int i) { m_prop = i; emit oddlyNamedNotifySignal(); }

    static MyAttached *qmlAttachedProperties(QObject *o) {
        return new MyAttached(o);
    }
Q_SIGNALS:
    void didSomething();
    void oddlyNamedNotifySignal();

private:
    int m_prop;
};

QML_DECLARE_TYPE(MyRect)
QML_DECLARE_TYPEINFO(MyRect, QML_HAS_ATTACHED_PROPERTIES)

class tst_qdeclarativestates : public QObject
{
    Q_OBJECT
public:
    tst_qdeclarativestates() {}

private:
    static QByteArray fullDataPath(const QString &path);

private slots:
    void initTestCase();

    void basicChanges();
    void attachedPropertyChanges();
    void basicExtension();
    void basicBinding();
    void signalOverride();
    void signalOverrideCrash();
    void signalOverrideCrash2();
    void parentChange();
    void parentChangeErrors();
    void anchorChanges();
    void anchorChanges2();
    void anchorChanges3();
    void anchorChanges4();
    void anchorChanges5();
    void anchorChangesRTL();
    void anchorChangesRTL2();
    void anchorChangesRTL3();
    void anchorChangesCrash();
    void anchorRewindBug();
    void anchorRewindBug2();
    void script();
    void restoreEntryValues();
    void explicitChanges();
    void propertyErrors();
    void incorrectRestoreBug();
    void autoStateAtStartupRestoreBug();
    void deletingChange();
    void deletingState();
    void tempState();
    void illegalTempState();
    void nonExistantProperty();
    void reset();
    void illegalObjectCreation();
    void whenOrdering();
    void urlResolution();
    void unnamedWhen();
    void returnToBase();
    void extendsBug();
    void editProperties();
    void QTBUG_14830();
    void avoidFastForward();
};

void tst_qdeclarativestates::initTestCase()
{
    qmlRegisterType<MyRect>("Qt.test", 1, 0, "MyRectangle");
}

QByteArray tst_qdeclarativestates::fullDataPath(const QString &path)
{
    return QUrl::fromLocalFile(TESTDATA(path)).toString().toUtf8();
}

void tst_qdeclarativestates::basicChanges()
{
    QDeclarativeEngine engine;

    {
        QDeclarativeComponent rectComponent(&engine, TESTDATA("basicChanges.qml"));
        QSGRectangle *rect = qobject_cast<QSGRectangle*>(rectComponent.create());
        QSGItemPrivate *rectPrivate = QSGItemPrivate::get(rect);
        QVERIFY(rect != 0);

        QCOMPARE(rect->color(),QColor("red"));

        rectPrivate->setState("blue");
        QCOMPARE(rect->color(),QColor("blue"));

        rectPrivate->setState("");
        QCOMPARE(rect->color(),QColor("red"));
    }

    {
        QDeclarativeComponent rectComponent(&engine, TESTDATA("basicChanges2.qml"));
        QSGRectangle *rect = qobject_cast<QSGRectangle*>(rectComponent.create());
        QSGItemPrivate *rectPrivate = QSGItemPrivate::get(rect);
        QVERIFY(rect != 0);

        QCOMPARE(rect->color(),QColor("red"));

        rectPrivate->setState("blue");
        QCOMPARE(rect->color(),QColor("blue"));

        rectPrivate->setState("green");
        QCOMPARE(rect->color(),QColor("green"));

        rectPrivate->setState("");
        QCOMPARE(rect->color(),QColor("red"));

        rectPrivate->setState("green");
        QCOMPARE(rect->color(),QColor("green"));
    }

    {
        QDeclarativeComponent rectComponent(&engine, TESTDATA("basicChanges3.qml"));
        QSGRectangle *rect = qobject_cast<QSGRectangle*>(rectComponent.create());
        QSGItemPrivate *rectPrivate = QSGItemPrivate::get(rect);
        QVERIFY(rect != 0);

        QCOMPARE(rect->color(),QColor("red"));
        QCOMPARE(rect->border()->width(),1.0);

        rectPrivate->setState("blue");
        QCOMPARE(rect->color(),QColor("blue"));
        QCOMPARE(rect->border()->width(),1.0);

        rectPrivate->setState("bordered");
        QCOMPARE(rect->color(),QColor("red"));
        QCOMPARE(rect->border()->width(),2.0);

        rectPrivate->setState("");
        QCOMPARE(rect->color(),QColor("red"));
        QCOMPARE(rect->border()->width(),1.0);
        //### we should be checking that this is an implicit rather than explicit 1 (which currently fails)

        rectPrivate->setState("bordered");
        QCOMPARE(rect->color(),QColor("red"));
        QCOMPARE(rect->border()->width(),2.0);

        rectPrivate->setState("blue");
        QCOMPARE(rect->color(),QColor("blue"));
        QCOMPARE(rect->border()->width(),1.0);

    }

    {
        // Test basicChanges4.qml can magically connect to propertyWithNotify's notify
        // signal using 'onPropertyWithNotifyChanged' even though the signal name is
        // actually 'oddlyNamedNotifySignal'

        QDeclarativeComponent component(&engine, TESTDATA("basicChanges4.qml"));
        QVERIFY(component.isReady());

        MyRect *rect = qobject_cast<MyRect*>(component.create());
        QVERIFY(rect != 0);

        QMetaProperty prop = rect->metaObject()->property(rect->metaObject()->indexOfProperty("propertyWithNotify"));
        QVERIFY(prop.hasNotifySignal());
        QString notifySignal = QByteArray(prop.notifySignal().signature());
        QVERIFY(!notifySignal.startsWith("propertyWithNotifyChanged("));

        QCOMPARE(rect->color(), QColor(Qt::red));

        rect->setPropertyWithNotify(100);
        QCOMPARE(rect->color(), QColor(Qt::blue));
    }
}

void tst_qdeclarativestates::attachedPropertyChanges()
{
    QDeclarativeEngine engine;

    QDeclarativeComponent component(&engine, TESTDATA("attachedPropertyChanges.qml"));
    QVERIFY(component.isReady());

    QSGItem *item = qobject_cast<QSGItem*>(component.create());
    QVERIFY(item != 0);
    QCOMPARE(item->width(), 50.0);

    // Ensure attached property has been changed
    QObject *attObj = qmlAttachedPropertiesObject<MyRect>(item, false);
    QVERIFY(attObj);

    MyAttached *att = qobject_cast<MyAttached*>(attObj);
    QVERIFY(att);

    QCOMPARE(att->foo(), 1);
}

void tst_qdeclarativestates::basicExtension()
{
    QDeclarativeEngine engine;

    {
        QDeclarativeComponent rectComponent(&engine, TESTDATA("basicExtension.qml"));
        QSGRectangle *rect = qobject_cast<QSGRectangle*>(rectComponent.create());
        QSGItemPrivate *rectPrivate = QSGItemPrivate::get(rect);
        QVERIFY(rect != 0);

        QCOMPARE(rect->color(),QColor("red"));
        QCOMPARE(rect->border()->width(),1.0);

        rectPrivate->setState("blue");
        QCOMPARE(rect->color(),QColor("blue"));
        QCOMPARE(rect->border()->width(),1.0);

        rectPrivate->setState("bordered");
        QCOMPARE(rect->color(),QColor("blue"));
        QCOMPARE(rect->border()->width(),2.0);

        rectPrivate->setState("blue");
        QCOMPARE(rect->color(),QColor("blue"));
        QCOMPARE(rect->border()->width(),1.0);

        rectPrivate->setState("");
        QCOMPARE(rect->color(),QColor("red"));
        QCOMPARE(rect->border()->width(),1.0);

        rectPrivate->setState("bordered");
        QCOMPARE(rect->color(),QColor("blue"));
        QCOMPARE(rect->border()->width(),2.0);

        rectPrivate->setState("");
        QCOMPARE(rect->color(),QColor("red"));
        QCOMPARE(rect->border()->width(),1.0);
    }

    {
        QDeclarativeComponent rectComponent(&engine, TESTDATA("fakeExtension.qml"));
        QSGRectangle *rect = qobject_cast<QSGRectangle*>(rectComponent.create());
        QSGItemPrivate *rectPrivate = QSGItemPrivate::get(rect);
        QVERIFY(rect != 0);

        QCOMPARE(rect->color(),QColor("red"));

        rectPrivate->setState("blue");
        QCOMPARE(rect->color(),QColor("blue"));

        rectPrivate->setState("green");
        QCOMPARE(rect->color(),QColor("green"));

        rectPrivate->setState("blue");
        QCOMPARE(rect->color(),QColor("blue"));

        rectPrivate->setState("green");
        QCOMPARE(rect->color(),QColor("green"));

        rectPrivate->setState("");
        QCOMPARE(rect->color(),QColor("red"));

        rectPrivate->setState("green");
        QCOMPARE(rect->color(),QColor("green"));
    }
}

void tst_qdeclarativestates::basicBinding()
{
    QDeclarativeEngine engine;

    {
        QDeclarativeComponent rectComponent(&engine, TESTDATA("basicBinding.qml"));
        QSGRectangle *rect = qobject_cast<QSGRectangle*>(rectComponent.create());
        QSGItemPrivate *rectPrivate = QSGItemPrivate::get(rect);
        QVERIFY(rect != 0);

        QCOMPARE(rect->color(),QColor("red"));

        rectPrivate->setState("blue");
        QCOMPARE(rect->color(),QColor("blue"));

        rectPrivate->setState("");
        QCOMPARE(rect->color(),QColor("red"));

        rectPrivate->setState("blue");
        QCOMPARE(rect->color(),QColor("blue"));
        rect->setProperty("sourceColor", QColor("green"));
        QCOMPARE(rect->color(),QColor("green"));

        rectPrivate->setState("");
        QCOMPARE(rect->color(),QColor("red"));
        rect->setProperty("sourceColor", QColor("yellow"));
        QCOMPARE(rect->color(),QColor("red"));

        rectPrivate->setState("blue");
        QCOMPARE(rect->color(),QColor("yellow"));
    }

    {
        QDeclarativeComponent rectComponent(&engine, TESTDATA("basicBinding2.qml"));
        QSGRectangle *rect = qobject_cast<QSGRectangle*>(rectComponent.create());
        QSGItemPrivate *rectPrivate = QSGItemPrivate::get(rect);
        QVERIFY(rect != 0);

        QCOMPARE(rect->color(),QColor("red"));

        rectPrivate->setState("blue");
        QCOMPARE(rect->color(),QColor("blue"));

        rectPrivate->setState("");
        QCOMPARE(rect->color(),QColor("red"));

        rectPrivate->setState("blue");
        QCOMPARE(rect->color(),QColor("blue"));
        rect->setProperty("sourceColor", QColor("green"));
        QCOMPARE(rect->color(),QColor("blue"));

        rectPrivate->setState("");
        QCOMPARE(rect->color(),QColor("green"));
        rect->setProperty("sourceColor", QColor("yellow"));
        QCOMPARE(rect->color(),QColor("yellow"));

        rectPrivate->setState("blue");
        QCOMPARE(rect->color(),QColor("blue"));

        rectPrivate->setState("");
        QCOMPARE(rect->color(),QColor("yellow"));
    }

    {
        QDeclarativeComponent rectComponent(&engine, TESTDATA("basicBinding3.qml"));
        QSGRectangle *rect = qobject_cast<QSGRectangle*>(rectComponent.create());
        QSGItemPrivate *rectPrivate = QSGItemPrivate::get(rect);
        QVERIFY(rect != 0);

        QCOMPARE(rect->color(),QColor("red"));
        rect->setProperty("sourceColor", QColor("green"));
        QCOMPARE(rect->color(),QColor("green"));

        rectPrivate->setState("blue");
        QCOMPARE(rect->color(),QColor("blue"));
        rect->setProperty("sourceColor", QColor("red"));
        QCOMPARE(rect->color(),QColor("blue"));
        rect->setProperty("sourceColor2", QColor("yellow"));
        QCOMPARE(rect->color(),QColor("yellow"));

        rectPrivate->setState("");
        QCOMPARE(rect->color(),QColor("red"));
        rect->setProperty("sourceColor2", QColor("green"));
        QCOMPARE(rect->color(),QColor("red"));
        rect->setProperty("sourceColor", QColor("yellow"));
        QCOMPARE(rect->color(),QColor("yellow"));
    }

    {
        QDeclarativeComponent rectComponent(&engine, TESTDATA("basicBinding4.qml"));
        QSGRectangle *rect = qobject_cast<QSGRectangle*>(rectComponent.create());
        QSGItemPrivate *rectPrivate = QSGItemPrivate::get(rect);
        QVERIFY(rect != 0);

        QCOMPARE(rect->color(),QColor("red"));

        rectPrivate->setState("blue");
        QCOMPARE(rect->color(),QColor("blue"));
        rect->setProperty("sourceColor", QColor("yellow"));
        QCOMPARE(rect->color(),QColor("yellow"));

        rectPrivate->setState("green");
        QCOMPARE(rect->color(),QColor("green"));
        rect->setProperty("sourceColor", QColor("purple"));
        QCOMPARE(rect->color(),QColor("green"));

        rectPrivate->setState("blue");
        QCOMPARE(rect->color(),QColor("purple"));

        rectPrivate->setState("green");
        QCOMPARE(rect->color(),QColor("green"));

        rectPrivate->setState("");
        QCOMPARE(rect->color(),QColor("red"));
    }
}

void tst_qdeclarativestates::signalOverride()
{
    QDeclarativeEngine engine;

    {
        QDeclarativeComponent rectComponent(&engine, TESTDATA("signalOverride.qml"));
        MyRect *rect = qobject_cast<MyRect*>(rectComponent.create());
        QVERIFY(rect != 0);

        QCOMPARE(rect->color(),QColor("red"));
        rect->doSomething();
        QCOMPARE(rect->color(),QColor("blue"));

        QSGItemPrivate::get(rect)->setState("green");
        rect->doSomething();
        QCOMPARE(rect->color(),QColor("green"));
    }

    {
        QDeclarativeComponent rectComponent(&engine, TESTDATA("signalOverride2.qml"));
        MyRect *rect = qobject_cast<MyRect*>(rectComponent.create());
        QVERIFY(rect != 0);

        QCOMPARE(rect->color(),QColor("white"));
        rect->doSomething();
        QCOMPARE(rect->color(),QColor("blue"));

        QSGRectangle *innerRect = qobject_cast<QSGRectangle*>(rect->findChild<QSGRectangle*>("extendedRect"));
        QSGItemPrivate::get(innerRect)->setState("green");
        rect->doSomething();
        QCOMPARE(rect->color(),QColor("blue"));
        QCOMPARE(innerRect->color(),QColor("green"));
        QCOMPARE(innerRect->property("extendedColor").value<QColor>(),QColor("green"));
    }
}

void tst_qdeclarativestates::signalOverrideCrash()
{
    QDeclarativeEngine engine;

    QDeclarativeComponent rectComponent(&engine, TESTDATA("signalOverrideCrash.qml"));
    MyRect *rect = qobject_cast<MyRect*>(rectComponent.create());
    QVERIFY(rect != 0);

    QSGItemPrivate::get(rect)->setState("overridden");
    rect->doSomething();
}

void tst_qdeclarativestates::signalOverrideCrash2()
{
    QDeclarativeEngine engine;

    QDeclarativeComponent rectComponent(&engine, TESTDATA("signalOverrideCrash2.qml"));
    QSGRectangle *rect = qobject_cast<QSGRectangle*>(rectComponent.create());
    QVERIFY(rect != 0);

    QSGItemPrivate::get(rect)->setState("state1");
    QSGItemPrivate::get(rect)->setState("state2");
    QSGItemPrivate::get(rect)->setState("state1");

    delete rect;
}

void tst_qdeclarativestates::parentChange()
{
    QDeclarativeEngine engine;

    {
        QDeclarativeComponent rectComponent(&engine, TESTDATA("parentChange1.qml"));
        QSGRectangle *rect = qobject_cast<QSGRectangle*>(rectComponent.create());
        QVERIFY(rect != 0);

        QSGRectangle *innerRect = qobject_cast<QSGRectangle*>(rect->findChild<QSGRectangle*>("MyRect"));
        QVERIFY(innerRect != 0);

        QDeclarativeListReference list(rect, "states");
        QDeclarativeState *state = qobject_cast<QDeclarativeState*>(list.at(0));
        QVERIFY(state != 0);

        qmlExecuteDeferred(state);
        QSGParentChange *pChange = qobject_cast<QSGParentChange*>(state->operationAt(0));
        QVERIFY(pChange != 0);
        QSGItem *nParent = qobject_cast<QSGItem*>(rect->findChild<QSGItem*>("NewParent"));
        QVERIFY(nParent != 0);

        QCOMPARE(pChange->parent(), nParent);

        QSGItemPrivate::get(rect)->setState("reparented");
        QCOMPARE(innerRect->rotation(), qreal(0));
        QCOMPARE(innerRect->scale(), qreal(1));
        QCOMPARE(innerRect->x(), qreal(-133));
        QCOMPARE(innerRect->y(), qreal(-300));
    }

    {
        QDeclarativeComponent rectComponent(&engine, TESTDATA("parentChange2.qml"));
        QSGRectangle *rect = qobject_cast<QSGRectangle*>(rectComponent.create());
        QVERIFY(rect != 0);
        QSGItemPrivate *rectPrivate = QSGItemPrivate::get(rect);
        QSGRectangle *innerRect = qobject_cast<QSGRectangle*>(rect->findChild<QSGRectangle*>("MyRect"));
        QVERIFY(innerRect != 0);

        rectPrivate->setState("reparented");
        QCOMPARE(innerRect->rotation(), qreal(15));
        QCOMPARE(innerRect->scale(), qreal(.5));
        QCOMPARE(QString("%1").arg(innerRect->x()), QString("%1").arg(-19.9075));
        QCOMPARE(QString("%1").arg(innerRect->y()), QString("%1").arg(-8.73433));
    }

    {
        QDeclarativeComponent rectComponent(&engine, TESTDATA("parentChange3.qml"));
        QSGRectangle *rect = qobject_cast<QSGRectangle*>(rectComponent.create());
        QVERIFY(rect != 0);
        QSGItemPrivate *rectPrivate = QSGItemPrivate::get(rect);
        QSGRectangle *innerRect = qobject_cast<QSGRectangle*>(rect->findChild<QSGRectangle*>("MyRect"));
        QVERIFY(innerRect != 0);

        rectPrivate->setState("reparented");
        QCOMPARE(innerRect->rotation(), qreal(-37));
        QCOMPARE(innerRect->scale(), qreal(.25));
        QCOMPARE(QString("%1").arg(innerRect->x()), QString("%1").arg(-217.305));
        QCOMPARE(QString("%1").arg(innerRect->y()), QString("%1").arg(-164.413));

        rectPrivate->setState("");
        QCOMPARE(innerRect->rotation(), qreal(0));
        QCOMPARE(innerRect->scale(), qreal(1));
        QCOMPARE(innerRect->x(), qreal(5));
        //do a non-qFuzzyCompare fuzzy compare
        QVERIFY(innerRect->y() < qreal(0.00001) && innerRect->y() > qreal(-0.00001));
    }

    {
        QDeclarativeComponent rectComponent(&engine, TESTDATA("parentChange6.qml"));
        QSGRectangle *rect = qobject_cast<QSGRectangle*>(rectComponent.create());
        QVERIFY(rect != 0);

        QSGRectangle *innerRect = qobject_cast<QSGRectangle*>(rect->findChild<QSGRectangle*>("MyRect"));
        QVERIFY(innerRect != 0);

        QSGItemPrivate::get(rect)->setState("reparented");
        QCOMPARE(innerRect->rotation(), qreal(180));
        QCOMPARE(innerRect->scale(), qreal(1));
        QCOMPARE(innerRect->x(), qreal(-105));
        QCOMPARE(innerRect->y(), qreal(-105));
    }
}

void tst_qdeclarativestates::parentChangeErrors()
{
    QDeclarativeEngine engine;

    {
        QDeclarativeComponent rectComponent(&engine, TESTDATA("parentChange4.qml"));
        QSGRectangle *rect = qobject_cast<QSGRectangle*>(rectComponent.create());
        QVERIFY(rect != 0);

        QSGRectangle *innerRect = qobject_cast<QSGRectangle*>(rect->findChild<QSGRectangle*>("MyRect"));
        QVERIFY(innerRect != 0);

        QTest::ignoreMessage(QtWarningMsg, fullDataPath("parentChange4.qml") + ":25:9: QML ParentChange: Unable to preserve appearance under non-uniform scale");
        QSGItemPrivate::get(rect)->setState("reparented");
        QCOMPARE(innerRect->rotation(), qreal(0));
        QCOMPARE(innerRect->scale(), qreal(1));
        QCOMPARE(innerRect->x(), qreal(5));
        QCOMPARE(innerRect->y(), qreal(5));
    }

    {
        QDeclarativeComponent rectComponent(&engine, TESTDATA("parentChange5.qml"));
        QSGRectangle *rect = qobject_cast<QSGRectangle*>(rectComponent.create());
        QVERIFY(rect != 0);

        QSGRectangle *innerRect = qobject_cast<QSGRectangle*>(rect->findChild<QSGRectangle*>("MyRect"));
        QVERIFY(innerRect != 0);

        QTest::ignoreMessage(QtWarningMsg, fullDataPath("parentChange5.qml") + ":25:9: QML ParentChange: Unable to preserve appearance under complex transform");
        QSGItemPrivate::get(rect)->setState("reparented");
        QCOMPARE(innerRect->rotation(), qreal(0));
        QCOMPARE(innerRect->scale(), qreal(1));
        QCOMPARE(innerRect->x(), qreal(5));
        QCOMPARE(innerRect->y(), qreal(5));
    }
}

void tst_qdeclarativestates::anchorChanges()
{
    QDeclarativeEngine engine;

    QDeclarativeComponent rectComponent(&engine, TESTDATA("anchorChanges1.qml"));
    QSGRectangle *rect = qobject_cast<QSGRectangle*>(rectComponent.create());
    QVERIFY(rect != 0);
    QSGItemPrivate *rectPrivate = QSGItemPrivate::get(rect);

    QSGRectangle *innerRect = qobject_cast<QSGRectangle*>(rect->findChild<QSGRectangle*>("MyRect"));
    QVERIFY(innerRect != 0);

    QDeclarativeListReference list(rect, "states");
    QDeclarativeState *state = qobject_cast<QDeclarativeState*>(list.at(0));
    QVERIFY(state != 0);

    qmlExecuteDeferred(state);
    QSGAnchorChanges *aChanges = qobject_cast<QSGAnchorChanges*>(state->operationAt(0));
    QVERIFY(aChanges != 0);

    rectPrivate->setState("right");
    QCOMPARE(innerRect->x(), qreal(150));
    QCOMPARE(aChanges->object(), qobject_cast<QSGItem*>(innerRect));
    QCOMPARE(QSGItemPrivate::get(aChanges->object())->anchors()->left().anchorLine, QSGAnchorLine::Invalid);  //### was reset (how do we distinguish from not set at all)
    QCOMPARE(QSGItemPrivate::get(aChanges->object())->anchors()->right().item, rectPrivate->right().item);
    QCOMPARE(QSGItemPrivate::get(aChanges->object())->anchors()->right().anchorLine, rectPrivate->right().anchorLine);

    rectPrivate->setState("");
    QCOMPARE(innerRect->x(), qreal(5));

    delete rect;
}

void tst_qdeclarativestates::anchorChanges2()
{
    QDeclarativeEngine engine;

    QDeclarativeComponent rectComponent(&engine, TESTDATA("anchorChanges2.qml"));
    QSGRectangle *rect = qobject_cast<QSGRectangle*>(rectComponent.create());
    QVERIFY(rect != 0);
    QSGItemPrivate *rectPrivate = QSGItemPrivate::get(rect);

    QSGRectangle *innerRect = qobject_cast<QSGRectangle*>(rect->findChild<QSGRectangle*>("MyRect"));
    QVERIFY(innerRect != 0);

    rectPrivate->setState("right");
    QCOMPARE(innerRect->x(), qreal(150));

    rectPrivate->setState("");
    QCOMPARE(innerRect->x(), qreal(5));

    delete rect;
}

void tst_qdeclarativestates::anchorChanges3()
{
    QDeclarativeEngine engine;

    QDeclarativeComponent rectComponent(&engine, TESTDATA("anchorChanges3.qml"));
    QSGRectangle *rect = qobject_cast<QSGRectangle*>(rectComponent.create());
    QVERIFY(rect != 0);
    QSGItemPrivate *rectPrivate = QSGItemPrivate::get(rect);

    QSGRectangle *innerRect = qobject_cast<QSGRectangle*>(rect->findChild<QSGRectangle*>("MyRect"));
    QVERIFY(innerRect != 0);

    QSGItem *leftGuideline = qobject_cast<QSGItem*>(rect->findChild<QSGItem*>("LeftGuideline"));
    QVERIFY(leftGuideline != 0);

    QSGItem *bottomGuideline = qobject_cast<QSGItem*>(rect->findChild<QSGItem*>("BottomGuideline"));
    QVERIFY(bottomGuideline != 0);

    QDeclarativeListReference list(rect, "states");
    QDeclarativeState *state = qobject_cast<QDeclarativeState*>(list.at(0));
    QVERIFY(state != 0);

    qmlExecuteDeferred(state);
    QSGAnchorChanges *aChanges = qobject_cast<QSGAnchorChanges*>(state->operationAt(0));
    QVERIFY(aChanges != 0);

    rectPrivate->setState("reanchored");
    QCOMPARE(aChanges->object(), qobject_cast<QSGItem*>(innerRect));
    QCOMPARE(QSGItemPrivate::get(aChanges->object())->anchors()->left().item, QSGItemPrivate::get(leftGuideline)->left().item);
    QCOMPARE(QSGItemPrivate::get(aChanges->object())->anchors()->left().anchorLine, QSGItemPrivate::get(leftGuideline)->left().anchorLine);
    QCOMPARE(QSGItemPrivate::get(aChanges->object())->anchors()->right().item, rectPrivate->right().item);
    QCOMPARE(QSGItemPrivate::get(aChanges->object())->anchors()->right().anchorLine, rectPrivate->right().anchorLine);
    QCOMPARE(QSGItemPrivate::get(aChanges->object())->anchors()->top().item, rectPrivate->top().item);
    QCOMPARE(QSGItemPrivate::get(aChanges->object())->anchors()->top().anchorLine, rectPrivate->top().anchorLine);
    QCOMPARE(QSGItemPrivate::get(aChanges->object())->anchors()->bottom().item, QSGItemPrivate::get(bottomGuideline)->bottom().item);
    QCOMPARE(QSGItemPrivate::get(aChanges->object())->anchors()->bottom().anchorLine, QSGItemPrivate::get(bottomGuideline)->bottom().anchorLine);

    QCOMPARE(innerRect->x(), qreal(10));
    QCOMPARE(innerRect->y(), qreal(0));
    QCOMPARE(innerRect->width(), qreal(190));
    QCOMPARE(innerRect->height(), qreal(150));

    rectPrivate->setState("");
    QCOMPARE(innerRect->x(), qreal(0));
    QCOMPARE(innerRect->y(), qreal(10));
    QCOMPARE(innerRect->width(), qreal(150));
    QCOMPARE(innerRect->height(), qreal(190));

    delete rect;
}

void tst_qdeclarativestates::anchorChanges4()
{
    QDeclarativeEngine engine;

    QDeclarativeComponent rectComponent(&engine, TESTDATA("anchorChanges4.qml"));
    QSGRectangle *rect = qobject_cast<QSGRectangle*>(rectComponent.create());
    QVERIFY(rect != 0);

    QSGRectangle *innerRect = qobject_cast<QSGRectangle*>(rect->findChild<QSGRectangle*>("MyRect"));
    QVERIFY(innerRect != 0);

    QSGItem *leftGuideline = qobject_cast<QSGItem*>(rect->findChild<QSGItem*>("LeftGuideline"));
    QVERIFY(leftGuideline != 0);

    QSGItem *bottomGuideline = qobject_cast<QSGItem*>(rect->findChild<QSGItem*>("BottomGuideline"));
    QVERIFY(bottomGuideline != 0);

    QDeclarativeListReference list(rect, "states");
    QDeclarativeState *state = qobject_cast<QDeclarativeState*>(list.at(0));
    QVERIFY(state != 0);

    qmlExecuteDeferred(state);
    QSGAnchorChanges *aChanges = qobject_cast<QSGAnchorChanges*>(state->operationAt(0));
    QVERIFY(aChanges != 0);

    QSGItemPrivate::get(rect)->setState("reanchored");
    QCOMPARE(aChanges->object(), qobject_cast<QSGItem*>(innerRect));
    QCOMPARE(QSGItemPrivate::get(aChanges->object())->anchors()->horizontalCenter().item, QSGItemPrivate::get(bottomGuideline)->horizontalCenter().item);
    QCOMPARE(QSGItemPrivate::get(aChanges->object())->anchors()->horizontalCenter().anchorLine, QSGItemPrivate::get(bottomGuideline)->horizontalCenter().anchorLine);
    QCOMPARE(QSGItemPrivate::get(aChanges->object())->anchors()->verticalCenter().item, QSGItemPrivate::get(leftGuideline)->verticalCenter().item);
    QCOMPARE(QSGItemPrivate::get(aChanges->object())->anchors()->verticalCenter().anchorLine, QSGItemPrivate::get(leftGuideline)->verticalCenter().anchorLine);

    delete rect;
}

void tst_qdeclarativestates::anchorChanges5()
{
    QDeclarativeEngine engine;

    QDeclarativeComponent rectComponent(&engine, TESTDATA("anchorChanges5.qml"));
    QSGRectangle *rect = qobject_cast<QSGRectangle*>(rectComponent.create());
    QVERIFY(rect != 0);

    QSGRectangle *innerRect = qobject_cast<QSGRectangle*>(rect->findChild<QSGRectangle*>("MyRect"));
    QVERIFY(innerRect != 0);

    QSGItem *leftGuideline = qobject_cast<QSGItem*>(rect->findChild<QSGItem*>("LeftGuideline"));
    QVERIFY(leftGuideline != 0);

    QSGItem *bottomGuideline = qobject_cast<QSGItem*>(rect->findChild<QSGItem*>("BottomGuideline"));
    QVERIFY(bottomGuideline != 0);

    QDeclarativeListReference list(rect, "states");
    QDeclarativeState *state = qobject_cast<QDeclarativeState*>(list.at(0));
    QVERIFY(state != 0);

    qmlExecuteDeferred(state);
    QSGAnchorChanges *aChanges = qobject_cast<QSGAnchorChanges*>(state->operationAt(0));
    QVERIFY(aChanges != 0);

    QSGItemPrivate::get(rect)->setState("reanchored");
    QCOMPARE(aChanges->object(), qobject_cast<QSGItem*>(innerRect));
    //QCOMPARE(aChanges->anchors()->horizontalCenter().item, bottomGuideline->horizontalCenter().item);
    //QCOMPARE(aChanges->anchors()->horizontalCenter().anchorLine, bottomGuideline->horizontalCenter().anchorLine);
    //QCOMPARE(aChanges->anchors()->baseline().item, leftGuideline->baseline().item);
    //QCOMPARE(aChanges->anchors()->baseline().anchorLine, leftGuideline->baseline().anchorLine);

    delete rect;
}

void mirrorAnchors(QSGItem *item) {
    QSGItemPrivate *itemPrivate = QSGItemPrivate::get(item);
    itemPrivate->setLayoutMirror(true);
}

qreal offsetRTL(QSGItem *anchorItem, QSGItem *item) {
    return anchorItem->width()+2*anchorItem->x()-item->width();
}

void tst_qdeclarativestates::anchorChangesRTL()
{
    QDeclarativeEngine engine;

    QDeclarativeComponent rectComponent(&engine, TESTDATA("anchorChanges1.qml"));
    QSGRectangle *rect = qobject_cast<QSGRectangle*>(rectComponent.create());
    QVERIFY(rect != 0);
    QSGItemPrivate *rectPrivate = QSGItemPrivate::get(rect);

    QSGRectangle *innerRect = qobject_cast<QSGRectangle*>(rect->findChild<QSGRectangle*>("MyRect"));
    QVERIFY(innerRect != 0);
    mirrorAnchors(innerRect);

    QDeclarativeListReference list(rect, "states");
    QDeclarativeState *state = qobject_cast<QDeclarativeState*>(list.at(0));
    QVERIFY(state != 0);

    qmlExecuteDeferred(state);
    QSGAnchorChanges *aChanges = qobject_cast<QSGAnchorChanges*>(state->operationAt(0));
    QVERIFY(aChanges != 0);

    rectPrivate->setState("right");
    QCOMPARE(innerRect->x(), offsetRTL(rect, innerRect) - qreal(150));
    QCOMPARE(aChanges->object(), qobject_cast<QSGItem*>(innerRect));
    QCOMPARE(QSGItemPrivate::get(aChanges->object())->anchors()->left().anchorLine, QSGAnchorLine::Invalid);  //### was reset (how do we distinguish from not set at all)
    QCOMPARE(QSGItemPrivate::get(aChanges->object())->anchors()->right().item, rectPrivate->right().item);
    QCOMPARE(QSGItemPrivate::get(aChanges->object())->anchors()->right().anchorLine, rectPrivate->right().anchorLine);

    rectPrivate->setState("");
    QCOMPARE(innerRect->x(), offsetRTL(rect, innerRect) -qreal(5));

    delete rect;
}

void tst_qdeclarativestates::anchorChangesRTL2()
{
    QDeclarativeEngine engine;

    QDeclarativeComponent rectComponent(&engine, TESTDATA("anchorChanges2.qml"));
    QSGRectangle *rect = qobject_cast<QSGRectangle*>(rectComponent.create());
    QVERIFY(rect != 0);
    QSGItemPrivate *rectPrivate = QSGItemPrivate::get(rect);

    QSGRectangle *innerRect = qobject_cast<QSGRectangle*>(rect->findChild<QSGRectangle*>("MyRect"));
    QVERIFY(innerRect != 0);
    mirrorAnchors(innerRect);

    rectPrivate->setState("right");
    QCOMPARE(innerRect->x(), offsetRTL(rect, innerRect) - qreal(150));

    rectPrivate->setState("");
    QCOMPARE(innerRect->x(), offsetRTL(rect, innerRect) - qreal(5));

    delete rect;
}

void tst_qdeclarativestates::anchorChangesRTL3()
{
    QDeclarativeEngine engine;

    QDeclarativeComponent rectComponent(&engine, TESTDATA("anchorChanges3.qml"));
    QSGRectangle *rect = qobject_cast<QSGRectangle*>(rectComponent.create());
    QVERIFY(rect != 0);
    QSGItemPrivate *rectPrivate = QSGItemPrivate::get(rect);

    QSGRectangle *innerRect = qobject_cast<QSGRectangle*>(rect->findChild<QSGRectangle*>("MyRect"));
    QVERIFY(innerRect != 0);
    mirrorAnchors(innerRect);

    QSGItem *leftGuideline = qobject_cast<QSGItem*>(rect->findChild<QSGItem*>("LeftGuideline"));
    QVERIFY(leftGuideline != 0);

    QSGItem *bottomGuideline = qobject_cast<QSGItem*>(rect->findChild<QSGItem*>("BottomGuideline"));
    QVERIFY(bottomGuideline != 0);

    QDeclarativeListReference list(rect, "states");
    QDeclarativeState *state = qobject_cast<QDeclarativeState*>(list.at(0));
    QVERIFY(state != 0);

    qmlExecuteDeferred(state);
    QSGAnchorChanges *aChanges = qobject_cast<QSGAnchorChanges*>(state->operationAt(0));
    QVERIFY(aChanges != 0);

    rectPrivate->setState("reanchored");
    QCOMPARE(aChanges->object(), qobject_cast<QSGItem*>(innerRect));
    QCOMPARE(QSGItemPrivate::get(aChanges->object())->anchors()->left().item, QSGItemPrivate::get(leftGuideline)->left().item);
    QCOMPARE(QSGItemPrivate::get(aChanges->object())->anchors()->left().anchorLine, QSGItemPrivate::get(leftGuideline)->left().anchorLine);
    QCOMPARE(QSGItemPrivate::get(aChanges->object())->anchors()->right().item, rectPrivate->right().item);
    QCOMPARE(QSGItemPrivate::get(aChanges->object())->anchors()->right().anchorLine, rectPrivate->right().anchorLine);
    QCOMPARE(QSGItemPrivate::get(aChanges->object())->anchors()->top().item, rectPrivate->top().item);
    QCOMPARE(QSGItemPrivate::get(aChanges->object())->anchors()->top().anchorLine, rectPrivate->top().anchorLine);
    QCOMPARE(QSGItemPrivate::get(aChanges->object())->anchors()->bottom().item, QSGItemPrivate::get(bottomGuideline)->bottom().item);
    QCOMPARE(QSGItemPrivate::get(aChanges->object())->anchors()->bottom().anchorLine, QSGItemPrivate::get(bottomGuideline)->bottom().anchorLine);

    QCOMPARE(innerRect->x(), offsetRTL(leftGuideline, innerRect) - qreal(10));
    QCOMPARE(innerRect->y(), qreal(0));
    // between left side of parent and leftGuideline.x: 10, which has width 0
    QCOMPARE(innerRect->width(), qreal(10));
    QCOMPARE(innerRect->height(), qreal(150));

    rectPrivate->setState("");
    QCOMPARE(innerRect->x(), offsetRTL(rect, innerRect) - qreal(0));
    QCOMPARE(innerRect->y(), qreal(10));
    // between right side of parent and left side of rightGuideline.x: 150, which has width 0
    QCOMPARE(innerRect->width(), qreal(50));
    QCOMPARE(innerRect->height(), qreal(190));

    delete rect;
}

//QTBUG-9609
void tst_qdeclarativestates::anchorChangesCrash()
{
    QDeclarativeEngine engine;

    QDeclarativeComponent rectComponent(&engine, TESTDATA("anchorChangesCrash.qml"));
    QSGRectangle *rect = qobject_cast<QSGRectangle*>(rectComponent.create());
    QVERIFY(rect != 0);

    QSGItemPrivate::get(rect)->setState("reanchored");

    delete rect;
}

// QTBUG-12273
void tst_qdeclarativestates::anchorRewindBug()
{
    QSGView *view = new QSGView;
    view->setSource(QUrl::fromLocalFile(TESTDATA("anchorRewindBug.qml")));

    view->show();
    view->requestActivateWindow();

    QTest::qWaitForWindowShown(view);

    QSGRectangle *rect = qobject_cast<QSGRectangle*>(view->rootObject());
    QVERIFY(rect != 0);

    QSGItem * column = rect->findChild<QSGItem*>("column");

    QVERIFY(column != 0);
    QVERIFY(!QSGItemPrivate::get(column)->heightValid);
    QVERIFY(!QSGItemPrivate::get(column)->widthValid);
    QCOMPARE(column->height(), 200.0);
    QSGItemPrivate::get(rect)->setState("reanchored");

    // column height and width should stay implicit
    // and column's implicit resizing should still work
    QVERIFY(!QSGItemPrivate::get(column)->heightValid);
    QVERIFY(!QSGItemPrivate::get(column)->widthValid);
    QTRY_COMPARE(column->height(), 100.0);

    QSGItemPrivate::get(rect)->setState("");

    // column height and width should stay implicit
    // and column's implicit resizing should still work
    QVERIFY(!QSGItemPrivate::get(column)->heightValid);
    QVERIFY(!QSGItemPrivate::get(column)->widthValid);
    QTRY_COMPARE(column->height(), 200.0);

    delete view;
}

// QTBUG-11834
void tst_qdeclarativestates::anchorRewindBug2()
{
    QDeclarativeEngine engine;

    QDeclarativeComponent rectComponent(&engine, TESTDATA("anchorRewindBug2.qml"));
    QSGRectangle *rect = qobject_cast<QSGRectangle*>(rectComponent.create());
    QVERIFY(rect != 0);

    QSGRectangle *mover = rect->findChild<QSGRectangle*>("mover");

    QVERIFY(mover != 0);
    QCOMPARE(mover->y(), qreal(0.0));
    QCOMPARE(mover->width(), qreal(50.0));

    QSGItemPrivate::get(rect)->setState("anchored");
    QCOMPARE(mover->y(), qreal(250.0));
    QCOMPARE(mover->width(), qreal(200.0));

    QSGItemPrivate::get(rect)->setState("");
    QCOMPARE(mover->y(), qreal(0.0));
    QCOMPARE(mover->width(), qreal(50.0));

    delete rect;
}

void tst_qdeclarativestates::script()
{
    QDeclarativeEngine engine;

    {
        QDeclarativeComponent rectComponent(&engine, TESTDATA("script.qml"));
        QSGRectangle *rect = qobject_cast<QSGRectangle*>(rectComponent.create());
        QVERIFY(rect != 0);
        QSGItemPrivate *rectPrivate = QSGItemPrivate::get(rect);
        QCOMPARE(rect->color(),QColor("red"));

        rectPrivate->setState("blue");
        QCOMPARE(rect->color(),QColor("blue"));

        rectPrivate->setState("");
        QCOMPARE(rect->color(),QColor("blue")); // a script isn't reverted
    }
}

void tst_qdeclarativestates::restoreEntryValues()
{
    QDeclarativeEngine engine;

    QDeclarativeComponent rectComponent(&engine, TESTDATA("restoreEntryValues.qml"));
    QSGRectangle *rect = qobject_cast<QSGRectangle*>(rectComponent.create());
    QVERIFY(rect != 0);
    QSGItemPrivate *rectPrivate = QSGItemPrivate::get(rect);
    QCOMPARE(rect->color(),QColor("red"));

    rectPrivate->setState("blue");
    QCOMPARE(rect->color(),QColor("blue"));

    rectPrivate->setState("");
    QCOMPARE(rect->color(),QColor("blue"));
}

void tst_qdeclarativestates::explicitChanges()
{
    QDeclarativeEngine engine;

    QDeclarativeComponent rectComponent(&engine, TESTDATA("explicit.qml"));
    QSGRectangle *rect = qobject_cast<QSGRectangle*>(rectComponent.create());
    QVERIFY(rect != 0);
    QSGItemPrivate *rectPrivate = QSGItemPrivate::get(rect);
    QDeclarativeListReference list(rect, "states");
    QDeclarativeState *state = qobject_cast<QDeclarativeState*>(list.at(0));
    QVERIFY(state != 0);

    qmlExecuteDeferred(state);
    QDeclarativePropertyChanges *changes = qobject_cast<QDeclarativePropertyChanges*>(rect->findChild<QDeclarativePropertyChanges*>("changes"));
    QVERIFY(changes != 0);
    QVERIFY(changes->isExplicit());

    QCOMPARE(rect->color(),QColor("red"));

    rectPrivate->setState("blue");
    QCOMPARE(rect->color(),QColor("blue"));

    rect->setProperty("sourceColor", QColor("green"));
    QCOMPARE(rect->color(),QColor("blue"));

    rectPrivate->setState("");
    QCOMPARE(rect->color(),QColor("red"));
    rect->setProperty("sourceColor", QColor("yellow"));
    QCOMPARE(rect->color(),QColor("red"));

    rectPrivate->setState("blue");
    QCOMPARE(rect->color(),QColor("yellow"));
}

void tst_qdeclarativestates::propertyErrors()
{
    QDeclarativeEngine engine;
    QDeclarativeComponent rectComponent(&engine, TESTDATA("propertyErrors.qml"));
    QSGRectangle *rect = qobject_cast<QSGRectangle*>(rectComponent.create());
    QVERIFY(rect != 0);

    QCOMPARE(rect->color(),QColor("red"));

    QTest::ignoreMessage(QtWarningMsg, fullDataPath("propertyErrors.qml") + ":8:9: QML PropertyChanges: Cannot assign to non-existent property \"colr\"");
    QTest::ignoreMessage(QtWarningMsg, fullDataPath("propertyErrors.qml") + ":8:9: QML PropertyChanges: Cannot assign to read-only property \"activeFocus\"");
    QSGItemPrivate::get(rect)->setState("blue");
}

void tst_qdeclarativestates::incorrectRestoreBug()
{
    QDeclarativeEngine engine;

    QDeclarativeComponent rectComponent(&engine, TESTDATA("basicChanges.qml"));
    QSGRectangle *rect = qobject_cast<QSGRectangle*>(rectComponent.create());
    QVERIFY(rect != 0);
    QSGItemPrivate *rectPrivate = QSGItemPrivate::get(rect);
    QCOMPARE(rect->color(),QColor("red"));

    rectPrivate->setState("blue");
    QCOMPARE(rect->color(),QColor("blue"));

    rectPrivate->setState("");
    QCOMPARE(rect->color(),QColor("red"));

    // make sure if we change the base state value, we then restore to it correctly
    rect->setColor(QColor("green"));

    rectPrivate->setState("blue");
    QCOMPARE(rect->color(),QColor("blue"));

    rectPrivate->setState("");
    QCOMPARE(rect->color(),QColor("green"));
}

void tst_qdeclarativestates::autoStateAtStartupRestoreBug()
{
    QDeclarativeEngine engine;

    QDeclarativeComponent component(&engine, TESTDATA("autoStateAtStartupRestoreBug.qml"));
    QObject *obj = component.create();

    QVERIFY(obj != 0);
    QCOMPARE(obj->property("test").toInt(), 3);

    obj->setProperty("input", 2);

    QCOMPARE(obj->property("test").toInt(), 9);

    delete obj;
}

void tst_qdeclarativestates::deletingChange()
{
    QDeclarativeEngine engine;

    QDeclarativeComponent rectComponent(&engine, TESTDATA("deleting.qml"));
    QSGRectangle *rect = qobject_cast<QSGRectangle*>(rectComponent.create());
    QVERIFY(rect != 0);
    QSGItemPrivate *rectPrivate = QSGItemPrivate::get(rect);
    rectPrivate->setState("blue");
    QCOMPARE(rect->color(),QColor("blue"));
    QCOMPARE(rect->radius(),qreal(5));

    rectPrivate->setState("");
    QCOMPARE(rect->color(),QColor("red"));
    QCOMPARE(rect->radius(),qreal(0));

    QDeclarativePropertyChanges *pc = rect->findChild<QDeclarativePropertyChanges*>("pc1");
    QVERIFY(pc != 0);
    delete pc;

    QDeclarativeState *state = rect->findChild<QDeclarativeState*>();
    QVERIFY(state != 0);
    qmlExecuteDeferred(state);
    QCOMPARE(state->operationCount(), 1);

    rectPrivate->setState("blue");
    QCOMPARE(rect->color(),QColor("red"));
    QCOMPARE(rect->radius(),qreal(5));

    delete rect;
}

void tst_qdeclarativestates::deletingState()
{
    QDeclarativeEngine engine;

    QDeclarativeComponent rectComponent(&engine, TESTDATA("deletingState.qml"));
    QSGRectangle *rect = qobject_cast<QSGRectangle*>(rectComponent.create());
    QVERIFY(rect != 0);

    QDeclarativeStateGroup *sg = rect->findChild<QDeclarativeStateGroup*>();
    QVERIFY(sg != 0);
    QVERIFY(sg->findState("blue") != 0);

    sg->setState("blue");
    QCOMPARE(rect->color(),QColor("blue"));

    sg->setState("");
    QCOMPARE(rect->color(),QColor("red"));

    QDeclarativeState *state = rect->findChild<QDeclarativeState*>();
    QVERIFY(state != 0);
    delete state;

    QVERIFY(sg->findState("blue") == 0);

    //### should we warn that state doesn't exist
    sg->setState("blue");
    QCOMPARE(rect->color(),QColor("red"));

    delete rect;
}

void tst_qdeclarativestates::tempState()
{
    QDeclarativeEngine engine;

    QDeclarativeComponent rectComponent(&engine, TESTDATA("legalTempState.qml"));
    QSGRectangle *rect = qobject_cast<QSGRectangle*>(rectComponent.create());
    QVERIFY(rect != 0);
    QSGItemPrivate *rectPrivate = QSGItemPrivate::get(rect);
    QTest::ignoreMessage(QtDebugMsg, "entering placed");
    QTest::ignoreMessage(QtDebugMsg, "entering idle");
    rectPrivate->setState("placed");
    QCOMPARE(rectPrivate->state(), QLatin1String("idle"));
}

void tst_qdeclarativestates::illegalTempState()
{
    QDeclarativeEngine engine;

    QDeclarativeComponent rectComponent(&engine, TESTDATA("illegalTempState.qml"));
    QSGRectangle *rect = qobject_cast<QSGRectangle*>(rectComponent.create());
    QVERIFY(rect != 0);
    QSGItemPrivate *rectPrivate = QSGItemPrivate::get(rect);
    QTest::ignoreMessage(QtWarningMsg, "<Unknown File>: QML StateGroup: Can't apply a state change as part of a state definition.");
    rectPrivate->setState("placed");
    QCOMPARE(rectPrivate->state(), QLatin1String("placed"));
}

void tst_qdeclarativestates::nonExistantProperty()
{
    QDeclarativeEngine engine;

    QDeclarativeComponent rectComponent(&engine, TESTDATA("nonExistantProp.qml"));
    QSGRectangle *rect = qobject_cast<QSGRectangle*>(rectComponent.create());
    QVERIFY(rect != 0);
    QSGItemPrivate *rectPrivate = QSGItemPrivate::get(rect);
    QTest::ignoreMessage(QtWarningMsg, fullDataPath("nonExistantProp.qml") + ":9:9: QML PropertyChanges: Cannot assign to non-existent property \"colr\"");
    rectPrivate->setState("blue");
    QCOMPARE(rectPrivate->state(), QLatin1String("blue"));
}

void tst_qdeclarativestates::reset()
{
    QDeclarativeEngine engine;

    QDeclarativeComponent c(&engine, TESTDATA("reset.qml"));
    QSGRectangle *rect = qobject_cast<QSGRectangle*>(c.create());
    QVERIFY(rect != 0);

    QSGImage *image = rect->findChild<QSGImage*>();
    QVERIFY(image != 0);
    QCOMPARE(image->width(), qreal(40.));
    QCOMPARE(image->height(), qreal(20.));

    QSGItemPrivate::get(rect)->setState("state1");

    QCOMPARE(image->width(), 20.0);
    QCOMPARE(image->height(), qreal(20.));
}

void tst_qdeclarativestates::illegalObjectCreation()
{
    QDeclarativeEngine engine;

    QDeclarativeComponent component(&engine, TESTDATA("illegalObj.qml"));
    QList<QDeclarativeError> errors = component.errors();
    QVERIFY(errors.count() == 1);
    const QDeclarativeError &error = errors.at(0);
    QCOMPARE(error.line(), 9);
    QCOMPARE(error.column(), 23);
    QCOMPARE(error.description().toUtf8().constData(), "PropertyChanges does not support creating state-specific objects.");
}

void tst_qdeclarativestates::whenOrdering()
{
    QDeclarativeEngine engine;

    QDeclarativeComponent c(&engine, TESTDATA("whenOrdering.qml"));
    QSGRectangle *rect = qobject_cast<QSGRectangle*>(c.create());
    QVERIFY(rect != 0);
    QSGItemPrivate *rectPrivate = QSGItemPrivate::get(rect);

    QCOMPARE(rectPrivate->state(), QLatin1String(""));
    rect->setProperty("condition2", true);
    QCOMPARE(rectPrivate->state(), QLatin1String("state2"));
    rect->setProperty("condition1", true);
    QCOMPARE(rectPrivate->state(), QLatin1String("state1"));
    rect->setProperty("condition2", false);
    QCOMPARE(rectPrivate->state(), QLatin1String("state1"));
    rect->setProperty("condition2", true);
    QCOMPARE(rectPrivate->state(), QLatin1String("state1"));
    rect->setProperty("condition1", false);
    rect->setProperty("condition2", false);
    QCOMPARE(rectPrivate->state(), QLatin1String(""));
}

void tst_qdeclarativestates::urlResolution()
{
    QDeclarativeEngine engine;

    QDeclarativeComponent c(&engine, TESTDATA("urlResolution.qml"));
    QSGRectangle *rect = qobject_cast<QSGRectangle*>(c.create());
    QVERIFY(rect != 0);

    QSGItem *myType = rect->findChild<QSGItem*>("MyType");
    QSGImage *image1 = rect->findChild<QSGImage*>("image1");
    QSGImage *image2 = rect->findChild<QSGImage*>("image2");
    QSGImage *image3 = rect->findChild<QSGImage*>("image3");
    QVERIFY(myType != 0 && image1 != 0 && image2 != 0 && image3 != 0);

    QSGItemPrivate::get(myType)->setState("SetImageState");
    QUrl resolved = QUrl::fromLocalFile(TESTDATA("Implementation/images/qt-logo.png"));
    QCOMPARE(image1->source(), resolved);
    QCOMPARE(image2->source(), resolved);
    QCOMPARE(image3->source(), resolved);
}

void tst_qdeclarativestates::unnamedWhen()
{
    QDeclarativeEngine engine;

    QDeclarativeComponent c(&engine, TESTDATA("unnamedWhen.qml"));
    QSGRectangle *rect = qobject_cast<QSGRectangle*>(c.create());
    QVERIFY(rect != 0);
    QSGItemPrivate *rectPrivate = QSGItemPrivate::get(rect);

    QCOMPARE(rectPrivate->state(), QLatin1String(""));
    QCOMPARE(rect->property("stateString").toString(), QLatin1String(""));
    rect->setProperty("triggerState", true);
    QCOMPARE(rectPrivate->state(), QLatin1String("anonymousState1"));
    QCOMPARE(rect->property("stateString").toString(), QLatin1String("inState"));
    rect->setProperty("triggerState", false);
    QCOMPARE(rectPrivate->state(), QLatin1String(""));
    QCOMPARE(rect->property("stateString").toString(), QLatin1String(""));
}

void tst_qdeclarativestates::returnToBase()
{
    QDeclarativeEngine engine;

    QDeclarativeComponent c(&engine, TESTDATA("returnToBase.qml"));
    QSGRectangle *rect = qobject_cast<QSGRectangle*>(c.create());
    QVERIFY(rect != 0);
    QSGItemPrivate *rectPrivate = QSGItemPrivate::get(rect);

    QCOMPARE(rectPrivate->state(), QLatin1String(""));
    QCOMPARE(rect->property("stateString").toString(), QLatin1String(""));
    rect->setProperty("triggerState", true);
    QCOMPARE(rectPrivate->state(), QLatin1String("anonymousState1"));
    QCOMPARE(rect->property("stateString").toString(), QLatin1String("inState"));
    rect->setProperty("triggerState", false);
    QCOMPARE(rectPrivate->state(), QLatin1String(""));
    QCOMPARE(rect->property("stateString").toString(), QLatin1String("originalState"));
}

//QTBUG-12559
void tst_qdeclarativestates::extendsBug()
{
    QDeclarativeEngine engine;

    QDeclarativeComponent c(&engine, TESTDATA("extendsBug.qml"));
    QSGRectangle *rect = qobject_cast<QSGRectangle*>(c.create());
    QVERIFY(rect != 0);
    QSGItemPrivate *rectPrivate = QSGItemPrivate::get(rect);
    QSGRectangle *greenRect = rect->findChild<QSGRectangle*>("greenRect");

    rectPrivate->setState("b");
    QCOMPARE(greenRect->x(), qreal(100));
    QCOMPARE(greenRect->y(), qreal(100));
}

void tst_qdeclarativestates::editProperties()
{
    QDeclarativeEngine engine;

    QDeclarativeComponent c(&engine, TESTDATA("editProperties.qml"));
    QSGRectangle *rect = qobject_cast<QSGRectangle*>(c.create());
    QVERIFY(rect != 0);

    QSGItemPrivate *rectPrivate = QSGItemPrivate::get(rect);

    QDeclarativeStateGroup *stateGroup = rectPrivate->_states();
    QVERIFY(stateGroup != 0);
    qmlExecuteDeferred(stateGroup);

    QDeclarativeState *blueState = stateGroup->findState("blue");
    QVERIFY(blueState != 0);
    qmlExecuteDeferred(blueState);

    QDeclarativePropertyChanges *propertyChangesBlue = qobject_cast<QDeclarativePropertyChanges*>(blueState->operationAt(0));
    QVERIFY(propertyChangesBlue != 0);

    QDeclarativeState *greenState = stateGroup->findState("green");
    QVERIFY(greenState != 0);
    qmlExecuteDeferred(greenState);

    QDeclarativePropertyChanges *propertyChangesGreen = qobject_cast<QDeclarativePropertyChanges*>(greenState->operationAt(0));
    QVERIFY(propertyChangesGreen != 0);

    QSGRectangle *childRect = rect->findChild<QSGRectangle*>("rect2");
    QVERIFY(childRect != 0);
    QCOMPARE(childRect->width(), qreal(402));
    QVERIFY(QDeclarativePropertyPrivate::binding(QDeclarativeProperty(childRect, "width")));
    QCOMPARE(childRect->height(), qreal(200));

    rectPrivate->setState("blue");
    QCOMPARE(childRect->width(), qreal(50));
    QCOMPARE(childRect->height(), qreal(40));
    QVERIFY(!QDeclarativePropertyPrivate::binding(QDeclarativeProperty(childRect, "width")));
    QVERIFY(blueState->bindingInRevertList(childRect, "width"));


    rectPrivate->setState("green");
    QCOMPARE(childRect->width(), qreal(200));
    QCOMPARE(childRect->height(), qreal(100));
    QVERIFY(greenState->bindingInRevertList(childRect, "width"));


    rectPrivate->setState("");


    QCOMPARE(propertyChangesBlue->actions().length(), 2);
    QVERIFY(propertyChangesBlue->containsValue("width"));
    QVERIFY(!propertyChangesBlue->containsProperty("x"));
    QCOMPARE(propertyChangesBlue->value("width").toInt(), 50);
    QVERIFY(!propertyChangesBlue->value("x").isValid());

    propertyChangesBlue->changeValue("width", 60);
    QCOMPARE(propertyChangesBlue->value("width").toInt(), 60);
    QCOMPARE(propertyChangesBlue->actions().length(), 2);


    propertyChangesBlue->changeExpression("width", "myRectangle.width / 2");
    QVERIFY(!propertyChangesBlue->containsValue("width"));
    QVERIFY(propertyChangesBlue->containsExpression("width"));
    QCOMPARE(propertyChangesBlue->value("width").toInt(), 0);
    QCOMPARE(propertyChangesBlue->actions().length(), 2);

    propertyChangesBlue->changeValue("width", 50);
    QVERIFY(propertyChangesBlue->containsValue("width"));
    QVERIFY(!propertyChangesBlue->containsExpression("width"));
    QCOMPARE(propertyChangesBlue->value("width").toInt(), 50);
    QCOMPARE(propertyChangesBlue->actions().length(), 2);

    QVERIFY(QDeclarativePropertyPrivate::binding(QDeclarativeProperty(childRect, "width")));
    rectPrivate->setState("blue");
    QCOMPARE(childRect->width(), qreal(50));
    QCOMPARE(childRect->height(), qreal(40));

    propertyChangesBlue->changeValue("width", 60);
    QCOMPARE(propertyChangesBlue->value("width").toInt(), 60);
    QCOMPARE(propertyChangesBlue->actions().length(), 2);
    QCOMPARE(childRect->width(), qreal(60));
    QVERIFY(!QDeclarativePropertyPrivate::binding(QDeclarativeProperty(childRect, "width")));

    propertyChangesBlue->changeExpression("width", "myRectangle.width / 2");
    QVERIFY(!propertyChangesBlue->containsValue("width"));
    QVERIFY(propertyChangesBlue->containsExpression("width"));
    QCOMPARE(propertyChangesBlue->value("width").toInt(), 0);
    QCOMPARE(propertyChangesBlue->actions().length(), 2);
    QVERIFY(QDeclarativePropertyPrivate::binding(QDeclarativeProperty(childRect, "width")));
    QCOMPARE(childRect->width(), qreal(200));

    propertyChangesBlue->changeValue("width", 50);
    QCOMPARE(childRect->width(), qreal(50));

    rectPrivate->setState("");
    QCOMPARE(childRect->width(), qreal(402));
    QVERIFY(QDeclarativePropertyPrivate::binding(QDeclarativeProperty(childRect, "width")));

    QCOMPARE(propertyChangesGreen->actions().length(), 2);
    rectPrivate->setState("green");
    QCOMPARE(childRect->width(), qreal(200));
    QCOMPARE(childRect->height(), qreal(100));
    QVERIFY(QDeclarativePropertyPrivate::binding(QDeclarativeProperty(childRect, "width")));
    QVERIFY(greenState->bindingInRevertList(childRect, "width"));
    QCOMPARE(propertyChangesGreen->actions().length(), 2);


    propertyChangesGreen->removeProperty("height");
    QVERIFY(!QDeclarativePropertyPrivate::binding(QDeclarativeProperty(childRect, "height")));
    QCOMPARE(childRect->height(), qreal(200));

    QVERIFY(greenState->bindingInRevertList(childRect, "width"));
    QVERIFY(greenState->containsPropertyInRevertList(childRect, "width"));
    propertyChangesGreen->removeProperty("width");
    QVERIFY(QDeclarativePropertyPrivate::binding(QDeclarativeProperty(childRect, "width")));
    QCOMPARE(childRect->width(), qreal(402));
    QVERIFY(!greenState->bindingInRevertList(childRect, "width"));
    QVERIFY(!greenState->containsPropertyInRevertList(childRect, "width"));

    propertyChangesBlue->removeProperty("width");
    QCOMPARE(childRect->width(), qreal(402));

    rectPrivate->setState("blue");
    QCOMPARE(childRect->width(), qreal(402));
    QCOMPARE(childRect->height(), qreal(40));
}

void tst_qdeclarativestates::QTBUG_14830()
{
    QDeclarativeEngine engine;

    QDeclarativeComponent c(&engine, TESTDATA("QTBUG-14830.qml"));
    QSGRectangle *rect = qobject_cast<QSGRectangle*>(c.create());
    QVERIFY(rect != 0);
    QSGItem *item = rect->findChild<QSGItem*>("area");

    QCOMPARE(item->width(), qreal(171));
}

void tst_qdeclarativestates::avoidFastForward()
{
    QDeclarativeEngine engine;

    //shouldn't fast forward if there isn't a transition
    QDeclarativeComponent c(&engine, TESTDATA("avoidFastForward.qml"));
    QSGRectangle *rect = qobject_cast<QSGRectangle*>(c.create());
    QVERIFY(rect != 0);

    QSGItemPrivate *rectPrivate = QSGItemPrivate::get(rect);
    rectPrivate->setState("a");
    QCOMPARE(rect->property("updateCount").toInt(), 1);
}

QTEST_MAIN(tst_qdeclarativestates)

#include "tst_qdeclarativestates.moc"
