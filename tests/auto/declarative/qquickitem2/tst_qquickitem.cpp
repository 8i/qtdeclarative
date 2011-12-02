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
#include <QtTest/QSignalSpy>
#include <QtDeclarative/qdeclarativeengine.h>
#include <QtDeclarative/qdeclarativecomponent.h>
#include <QtDeclarative/qdeclarativecontext.h>
#include <QtDeclarative/qquickview.h>
#include <private/qquickrectangle_p.h>
#include <private/qquickitem_p.h>
#include "../shared/util.h"

class tst_QQuickItem : public QObject
{
    Q_OBJECT
public:
    tst_QQuickItem();

private slots:
    void initTestCase();
    void keys();
    void keysProcessingOrder();
    void keyNavigation();
    void keyNavigation_RightToLeft();
    void keyNavigation_skipNotVisible();
    void keyNavigation_implicitSetting();
    void layoutMirroring();
    void layoutMirroringIllegalParent();
    void smooth();
    void clip();
    void mapCoordinates();
    void mapCoordinates_data();
    void propertyChanges();
    void transforms();
    void transforms_data();
    void childrenRect();
    void childrenRectBug();
    void childrenRectBug2();
    void childrenRectBug3();

    void childrenProperty();
    void resourcesProperty();

    void transformCrash();
    void implicitSize();
    void qtbug_16871();
private:
    QDeclarativeEngine engine;
};

template<typename T>
T *findItem(QQuickItem *parent, const QString &objectName)
{
    if (!parent)
        return 0;

    const QMetaObject &mo = T::staticMetaObject;
    //qDebug() << parent->QQuickItem::children().count() << "children";
    for (int i = 0; i < parent->childItems().count(); ++i) {
        QQuickItem *item = qobject_cast<QQuickItem*>(parent->childItems().at(i));
        if (!item)
            continue;
        //qDebug() << "try" << item;
        if (mo.cast(item) && (objectName.isEmpty() || item->objectName() == objectName))
            return static_cast<T*>(item);
        item = findItem<T>(item, objectName);
        if (item)
            return static_cast<T*>(item);
    }

    return 0;
}

class KeysTestObject : public QObject
{
    Q_OBJECT

    Q_PROPERTY(bool processLast READ processLast NOTIFY processLastChanged)

public:
    KeysTestObject() : mKey(0), mModifiers(0), mForwardedKey(0), mLast(false) {}

    void reset() {
        mKey = 0;
        mText = QString();
        mModifiers = 0;
        mForwardedKey = 0;
    }

    bool processLast() const { return mLast; }
    void setProcessLast(bool b) {
        if (b != mLast) {
            mLast = b;
            emit processLastChanged();
        }
    }

public slots:
    void keyPress(int key, QString text, int modifiers) {
        mKey = key;
        mText = text;
        mModifiers = modifiers;
    }
    void keyRelease(int key, QString text, int modifiers) {
        mKey = key;
        mText = text;
        mModifiers = modifiers;
    }
    void forwardedKey(int key) {
        mForwardedKey = key;
    }

signals:
    void processLastChanged();

public:
    int mKey;
    QString mText;
    int mModifiers;
    int mForwardedKey;
    bool mLast;

private:
};

class KeyTestItem : public QQuickItem
{
    Q_OBJECT
public:
    KeyTestItem(QQuickItem *parent=0) : QQuickItem(parent), mKey(0) {}

protected:
    void keyPressEvent(QKeyEvent *e) {
        mKey = e->key();

        if (e->key() == Qt::Key_A)
            e->accept();
        else
            e->ignore();
    }

    void keyReleaseEvent(QKeyEvent *e) {
        if (e->key() == Qt::Key_B)
            e->accept();
        else
            e->ignore();
    }

public:
    int mKey;
};

QML_DECLARE_TYPE(KeyTestItem);


tst_QQuickItem::tst_QQuickItem()
{
}

void tst_QQuickItem::initTestCase()
{
    qmlRegisterType<KeyTestItem>("Test",1,0,"KeyTestItem");
}

void tst_QQuickItem::keys()
{
    QQuickView *canvas = new QQuickView(0);
    canvas->setBaseSize(QSize(240,320));

    KeysTestObject *testObject = new KeysTestObject;
    canvas->rootContext()->setContextProperty("keysTestObject", testObject);

    canvas->rootContext()->setContextProperty("enableKeyHanding", QVariant(true));
    canvas->rootContext()->setContextProperty("forwardeeVisible", QVariant(true));

    canvas->setSource(QUrl::fromLocalFile(TESTDATA("keystest.qml")));
    canvas->show();
    canvas->requestActivateWindow();
    QTest::qWaitForWindowShown(canvas);
    QTRY_VERIFY(QGuiApplication::focusWindow() == canvas);

    QVERIFY(canvas->rootObject());
    QCOMPARE(canvas->rootObject()->property("isEnabled").toBool(), true);

    QKeyEvent key(QEvent::KeyPress, Qt::Key_A, Qt::NoModifier, "A", false, 1);
    QApplication::sendEvent(canvas, &key);
    QCOMPARE(testObject->mKey, int(Qt::Key_A));
    QCOMPARE(testObject->mForwardedKey, int(Qt::Key_A));
    QCOMPARE(testObject->mText, QLatin1String("A"));
    QVERIFY(testObject->mModifiers == Qt::NoModifier);
    QVERIFY(!key.isAccepted());

    testObject->reset();

    key = QKeyEvent(QEvent::KeyRelease, Qt::Key_A, Qt::ShiftModifier, "A", false, 1);
    QApplication::sendEvent(canvas, &key);
    QCOMPARE(testObject->mKey, int(Qt::Key_A));
    QCOMPARE(testObject->mForwardedKey, int(Qt::Key_A));
    QCOMPARE(testObject->mText, QLatin1String("A"));
    QVERIFY(testObject->mModifiers == Qt::ShiftModifier);
    QVERIFY(key.isAccepted());

    testObject->reset();

    key = QKeyEvent(QEvent::KeyPress, Qt::Key_Return, Qt::NoModifier, "", false, 1);
    QApplication::sendEvent(canvas, &key);
    QCOMPARE(testObject->mKey, int(Qt::Key_Return));
    QCOMPARE(testObject->mForwardedKey, int(Qt::Key_Return));
    QCOMPARE(testObject->mText, QLatin1String("Return"));
    QVERIFY(testObject->mModifiers == Qt::NoModifier);
    QVERIFY(key.isAccepted());

    testObject->reset();

    key = QKeyEvent(QEvent::KeyPress, Qt::Key_0, Qt::NoModifier, "0", false, 1);
    QApplication::sendEvent(canvas, &key);
    QCOMPARE(testObject->mKey, int(Qt::Key_0));
    QCOMPARE(testObject->mForwardedKey, int(Qt::Key_0));
    QCOMPARE(testObject->mText, QLatin1String("0"));
    QVERIFY(testObject->mModifiers == Qt::NoModifier);
    QVERIFY(key.isAccepted());

    testObject->reset();

    key = QKeyEvent(QEvent::KeyPress, Qt::Key_9, Qt::NoModifier, "9", false, 1);
    QApplication::sendEvent(canvas, &key);
    QCOMPARE(testObject->mKey, int(Qt::Key_9));
    QCOMPARE(testObject->mForwardedKey, int(Qt::Key_9));
    QCOMPARE(testObject->mText, QLatin1String("9"));
    QVERIFY(testObject->mModifiers == Qt::NoModifier);
    QVERIFY(!key.isAccepted());

    testObject->reset();

    key = QKeyEvent(QEvent::KeyPress, Qt::Key_Tab, Qt::NoModifier, "", false, 1);
    QApplication::sendEvent(canvas, &key);
    QCOMPARE(testObject->mKey, int(Qt::Key_Tab));
    QCOMPARE(testObject->mForwardedKey, int(Qt::Key_Tab));
    QCOMPARE(testObject->mText, QLatin1String("Tab"));
    QVERIFY(testObject->mModifiers == Qt::NoModifier);
    QVERIFY(key.isAccepted());

    testObject->reset();

    key = QKeyEvent(QEvent::KeyPress, Qt::Key_Backtab, Qt::NoModifier, "", false, 1);
    QApplication::sendEvent(canvas, &key);
    QCOMPARE(testObject->mKey, int(Qt::Key_Backtab));
    QCOMPARE(testObject->mForwardedKey, int(Qt::Key_Backtab));
    QCOMPARE(testObject->mText, QLatin1String("Backtab"));
    QVERIFY(testObject->mModifiers == Qt::NoModifier);
    QVERIFY(key.isAccepted());

    testObject->reset();

    canvas->rootContext()->setContextProperty("forwardeeVisible", QVariant(false));
    key = QKeyEvent(QEvent::KeyPress, Qt::Key_A, Qt::NoModifier, "A", false, 1);
    QApplication::sendEvent(canvas, &key);
    QCOMPARE(testObject->mKey, int(Qt::Key_A));
    QCOMPARE(testObject->mForwardedKey, 0);
    QCOMPARE(testObject->mText, QLatin1String("A"));
    QVERIFY(testObject->mModifiers == Qt::NoModifier);
    QVERIFY(!key.isAccepted());

    testObject->reset();

    canvas->rootContext()->setContextProperty("enableKeyHanding", QVariant(false));
    QCOMPARE(canvas->rootObject()->property("isEnabled").toBool(), false);

    key = QKeyEvent(QEvent::KeyPress, Qt::Key_Return, Qt::NoModifier, "", false, 1);
    QApplication::sendEvent(canvas, &key);
    QCOMPARE(testObject->mKey, 0);
    QVERIFY(!key.isAccepted());

    canvas->rootContext()->setContextProperty("enableKeyHanding", QVariant(true));
    QCOMPARE(canvas->rootObject()->property("isEnabled").toBool(), true);

    key = QKeyEvent(QEvent::KeyPress, Qt::Key_Return, Qt::NoModifier, "", false, 1);
    QApplication::sendEvent(canvas, &key);
    QCOMPARE(testObject->mKey, int(Qt::Key_Return));
    QVERIFY(key.isAccepted());

    delete canvas;
    delete testObject;
}

void tst_QQuickItem::keysProcessingOrder()
{
    QQuickView *canvas = new QQuickView(0);
    canvas->setBaseSize(QSize(240,320));

    KeysTestObject *testObject = new KeysTestObject;
    canvas->rootContext()->setContextProperty("keysTestObject", testObject);

    canvas->setSource(QUrl::fromLocalFile(TESTDATA("keyspriority.qml")));
    canvas->show();
    canvas->requestActivateWindow();
    QTest::qWaitForWindowShown(canvas);
    QTRY_VERIFY(QGuiApplication::focusWindow() == canvas);

    KeyTestItem *testItem = qobject_cast<KeyTestItem*>(canvas->rootObject());
    QVERIFY(testItem);

    QKeyEvent key(QEvent::KeyPress, Qt::Key_A, Qt::NoModifier, "A", false, 1);
    QApplication::sendEvent(canvas, &key);
    QCOMPARE(testObject->mKey, int(Qt::Key_A));
    QCOMPARE(testObject->mText, QLatin1String("A"));
    QVERIFY(testObject->mModifiers == Qt::NoModifier);
    QVERIFY(key.isAccepted());

    testObject->reset();

    testObject->setProcessLast(true);

    key = QKeyEvent(QEvent::KeyPress, Qt::Key_A, Qt::NoModifier, "A", false, 1);
    QApplication::sendEvent(canvas, &key);
    QCOMPARE(testObject->mKey, 0);
    QVERIFY(key.isAccepted());

    testObject->reset();

    key = QKeyEvent(QEvent::KeyPress, Qt::Key_B, Qt::NoModifier, "B", false, 1);
    QApplication::sendEvent(canvas, &key);
    QCOMPARE(testObject->mKey, int(Qt::Key_B));
    QCOMPARE(testObject->mText, QLatin1String("B"));
    QVERIFY(testObject->mModifiers == Qt::NoModifier);
    QVERIFY(!key.isAccepted());

    testObject->reset();

    key = QKeyEvent(QEvent::KeyRelease, Qt::Key_B, Qt::NoModifier, "B", false, 1);
    QApplication::sendEvent(canvas, &key);
    QCOMPARE(testObject->mKey, 0);
    QVERIFY(key.isAccepted());

    delete canvas;
    delete testObject;
}

QQuickItemPrivate *childPrivate(QQuickItem *rootItem, const char * itemString)
{
    QQuickItem *item = findItem<QQuickItem>(rootItem, QString(QLatin1String(itemString)));
    QQuickItemPrivate* itemPrivate = QQuickItemPrivate::get(item);
    return itemPrivate;
}

QVariant childProperty(QQuickItem *rootItem, const char * itemString, const char * property)
{
    QQuickItem *item = findItem<QQuickItem>(rootItem, QString(QLatin1String(itemString)));
    return item->property(property);
}

bool anchorsMirrored(QQuickItem *rootItem, const char * itemString)
{
    QQuickItem *item = findItem<QQuickItem>(rootItem, QString(QLatin1String(itemString)));
    QQuickItemPrivate* itemPrivate = QQuickItemPrivate::get(item);
    return itemPrivate->anchors()->mirrored();
}

void tst_QQuickItem::layoutMirroring()
{
    QQuickView *canvas = new QQuickView(0);
    canvas->setSource(QUrl::fromLocalFile(TESTDATA("layoutmirroring.qml")));
    canvas->show();

    QQuickItem *rootItem = qobject_cast<QQuickItem*>(canvas->rootObject());
    QVERIFY(rootItem);
    QQuickItemPrivate *rootPrivate = QQuickItemPrivate::get(rootItem);
    QVERIFY(rootPrivate);

    QCOMPARE(childPrivate(rootItem, "mirrored1")->effectiveLayoutMirror, true);
    QCOMPARE(childPrivate(rootItem, "mirrored2")->effectiveLayoutMirror, true);
    QCOMPARE(childPrivate(rootItem, "notMirrored1")->effectiveLayoutMirror, false);
    QCOMPARE(childPrivate(rootItem, "notMirrored2")->effectiveLayoutMirror, false);
    QCOMPARE(childPrivate(rootItem, "inheritedMirror1")->effectiveLayoutMirror, true);
    QCOMPARE(childPrivate(rootItem, "inheritedMirror2")->effectiveLayoutMirror, true);

    QCOMPARE(anchorsMirrored(rootItem, "mirrored1"), true);
    QCOMPARE(anchorsMirrored(rootItem, "mirrored2"), true);
    QCOMPARE(anchorsMirrored(rootItem, "notMirrored1"), false);
    QCOMPARE(anchorsMirrored(rootItem, "notMirrored2"), false);
    QCOMPARE(anchorsMirrored(rootItem, "inheritedMirror1"), true);
    QCOMPARE(anchorsMirrored(rootItem, "inheritedMirror2"), true);

    QCOMPARE(childPrivate(rootItem, "mirrored1")->inheritedLayoutMirror, true);
    QCOMPARE(childPrivate(rootItem, "mirrored2")->inheritedLayoutMirror, false);
    QCOMPARE(childPrivate(rootItem, "notMirrored1")->inheritedLayoutMirror, true);
    QCOMPARE(childPrivate(rootItem, "notMirrored2")->inheritedLayoutMirror, false);
    QCOMPARE(childPrivate(rootItem, "inheritedMirror1")->inheritedLayoutMirror, true);
    QCOMPARE(childPrivate(rootItem, "inheritedMirror2")->inheritedLayoutMirror, true);

    QCOMPARE(childPrivate(rootItem, "mirrored1")->isMirrorImplicit, false);
    QCOMPARE(childPrivate(rootItem, "mirrored2")->isMirrorImplicit, false);
    QCOMPARE(childPrivate(rootItem, "notMirrored1")->isMirrorImplicit, false);
    QCOMPARE(childPrivate(rootItem, "notMirrored2")->isMirrorImplicit, true);
    QCOMPARE(childPrivate(rootItem, "inheritedMirror1")->isMirrorImplicit, true);
    QCOMPARE(childPrivate(rootItem, "inheritedMirror2")->isMirrorImplicit, true);

    QCOMPARE(childPrivate(rootItem, "mirrored1")->inheritMirrorFromParent, true);
    QCOMPARE(childPrivate(rootItem, "mirrored2")->inheritMirrorFromParent, false);
    QCOMPARE(childPrivate(rootItem, "notMirrored1")->inheritMirrorFromParent, true);
    QCOMPARE(childPrivate(rootItem, "notMirrored2")->inheritMirrorFromParent, false);
    QCOMPARE(childPrivate(rootItem, "inheritedMirror1")->inheritMirrorFromParent, true);
    QCOMPARE(childPrivate(rootItem, "inheritedMirror2")->inheritMirrorFromParent, true);

    QCOMPARE(childPrivate(rootItem, "mirrored1")->inheritMirrorFromItem, true);
    QCOMPARE(childPrivate(rootItem, "mirrored2")->inheritMirrorFromItem, false);
    QCOMPARE(childPrivate(rootItem, "notMirrored1")->inheritMirrorFromItem, false);
    QCOMPARE(childPrivate(rootItem, "notMirrored2")->inheritMirrorFromItem, false);
    QCOMPARE(childPrivate(rootItem, "inheritedMirror1")->inheritMirrorFromItem, false);
    QCOMPARE(childPrivate(rootItem, "inheritedMirror2")->inheritMirrorFromItem, false);

    // load dynamic content using Loader that needs to inherit mirroring
    rootItem->setProperty("state", "newContent");
    QCOMPARE(childPrivate(rootItem, "notMirrored3")->effectiveLayoutMirror, false);
    QCOMPARE(childPrivate(rootItem, "inheritedMirror3")->effectiveLayoutMirror, true);

    QCOMPARE(childPrivate(rootItem, "notMirrored3")->inheritedLayoutMirror, true);
    QCOMPARE(childPrivate(rootItem, "inheritedMirror3")->inheritedLayoutMirror, true);

    QCOMPARE(childPrivate(rootItem, "notMirrored3")->isMirrorImplicit, false);
    QCOMPARE(childPrivate(rootItem, "inheritedMirror3")->isMirrorImplicit, true);

    QCOMPARE(childPrivate(rootItem, "notMirrored3")->inheritMirrorFromParent, true);
    QCOMPARE(childPrivate(rootItem, "inheritedMirror3")->inheritMirrorFromParent, true);

    QCOMPARE(childPrivate(rootItem, "notMirrored3")->inheritMirrorFromItem, false);
    QCOMPARE(childPrivate(rootItem, "notMirrored3")->inheritMirrorFromItem, false);

    // disable inheritance
    rootItem->setProperty("childrenInherit", false);

    QCOMPARE(childPrivate(rootItem, "inheritedMirror1")->effectiveLayoutMirror, false);
    QCOMPARE(childPrivate(rootItem, "inheritedMirror2")->effectiveLayoutMirror, false);
    QCOMPARE(childPrivate(rootItem, "mirrored1")->effectiveLayoutMirror, true);
    QCOMPARE(childPrivate(rootItem, "notMirrored1")->effectiveLayoutMirror, false);

    QCOMPARE(childPrivate(rootItem, "inheritedMirror1")->inheritedLayoutMirror, false);
    QCOMPARE(childPrivate(rootItem, "inheritedMirror2")->inheritedLayoutMirror, false);
    QCOMPARE(childPrivate(rootItem, "mirrored1")->inheritedLayoutMirror, false);
    QCOMPARE(childPrivate(rootItem, "notMirrored1")->inheritedLayoutMirror, false);

    // re-enable inheritance
    rootItem->setProperty("childrenInherit", true);

    QCOMPARE(childPrivate(rootItem, "inheritedMirror1")->effectiveLayoutMirror, true);
    QCOMPARE(childPrivate(rootItem, "inheritedMirror2")->effectiveLayoutMirror, true);
    QCOMPARE(childPrivate(rootItem, "mirrored1")->effectiveLayoutMirror, true);
    QCOMPARE(childPrivate(rootItem, "notMirrored1")->effectiveLayoutMirror, false);

    QCOMPARE(childPrivate(rootItem, "inheritedMirror1")->inheritedLayoutMirror, true);
    QCOMPARE(childPrivate(rootItem, "inheritedMirror2")->inheritedLayoutMirror, true);
    QCOMPARE(childPrivate(rootItem, "mirrored1")->inheritedLayoutMirror, true);
    QCOMPARE(childPrivate(rootItem, "notMirrored1")->inheritedLayoutMirror, true);

    //
    // dynamic parenting
    //
    QQuickItem *parentItem1 = new QQuickItem();
    QQuickItemPrivate::get(parentItem1)->effectiveLayoutMirror = true; // LayoutMirroring.enabled: true
    QQuickItemPrivate::get(parentItem1)->isMirrorImplicit = false;
    QQuickItemPrivate::get(parentItem1)->inheritMirrorFromItem = true; // LayoutMirroring.childrenInherit: true
    QQuickItemPrivate::get(parentItem1)->resolveLayoutMirror();

    // inherit in constructor
    QQuickItem *childItem1 = new QQuickItem(parentItem1);
    QCOMPARE(QQuickItemPrivate::get(childItem1)->effectiveLayoutMirror, true);
    QCOMPARE(QQuickItemPrivate::get(childItem1)->inheritMirrorFromParent, true);

    // inherit through a parent change
    QQuickItem *childItem2 = new QQuickItem();
    QCOMPARE(QQuickItemPrivate::get(childItem2)->effectiveLayoutMirror, false);
    QCOMPARE(QQuickItemPrivate::get(childItem2)->inheritMirrorFromParent, false);
    childItem2->setParentItem(parentItem1);
    QCOMPARE(QQuickItemPrivate::get(childItem2)->effectiveLayoutMirror, true);
    QCOMPARE(QQuickItemPrivate::get(childItem2)->inheritMirrorFromParent, true);

    // stop inherting through a parent change
    QQuickItem *parentItem2 = new QQuickItem();
    QQuickItemPrivate::get(parentItem2)->effectiveLayoutMirror = true; // LayoutMirroring.enabled: true
    QQuickItemPrivate::get(parentItem2)->resolveLayoutMirror();
    childItem2->setParentItem(parentItem2);
    QCOMPARE(QQuickItemPrivate::get(childItem2)->effectiveLayoutMirror, false);
    QCOMPARE(QQuickItemPrivate::get(childItem2)->inheritMirrorFromParent, false);

    delete parentItem1;
    delete parentItem2;
}

void tst_QQuickItem::layoutMirroringIllegalParent()
{
    QDeclarativeComponent component(&engine);
    component.setData("import QtQuick 2.0; QtObject { LayoutMirroring.enabled: true; LayoutMirroring.childrenInherit: true }", QUrl::fromLocalFile(""));
    QTest::ignoreMessage(QtWarningMsg, "file::1:21: QML QtObject: LayoutDirection attached property only works with Items");
    QObject *object = component.create();
    QVERIFY(object != 0);
}

void tst_QQuickItem::keyNavigation()
{
    QQuickView *canvas = new QQuickView(0);
    canvas->setBaseSize(QSize(240,320));

    canvas->setSource(QUrl::fromLocalFile(TESTDATA("keynavigationtest.qml")));
    canvas->show();
    canvas->requestActivateWindow();
    QTest::qWaitForWindowShown(canvas);
    QTRY_VERIFY(QGuiApplication::focusWindow() == canvas);

    QQuickItem *item = findItem<QQuickItem>(canvas->rootObject(), "item1");
    QVERIFY(item);
    QVERIFY(item->hasActiveFocus());

    QVariant result;
    QVERIFY(QMetaObject::invokeMethod(canvas->rootObject(), "verify",
            Q_RETURN_ARG(QVariant, result)));
    QVERIFY(result.toBool());

    // right
    QKeyEvent key(QEvent::KeyPress, Qt::Key_Right, Qt::NoModifier, "", false, 1);
    QApplication::sendEvent(canvas, &key);
    QVERIFY(key.isAccepted());

    item = findItem<QQuickItem>(canvas->rootObject(), "item2");
    QVERIFY(item);
    QVERIFY(item->hasActiveFocus());

    // down
    key = QKeyEvent(QEvent::KeyPress, Qt::Key_Down, Qt::NoModifier, "", false, 1);
    QApplication::sendEvent(canvas, &key);
    QVERIFY(key.isAccepted());

    item = findItem<QQuickItem>(canvas->rootObject(), "item4");
    QVERIFY(item);
    QVERIFY(item->hasActiveFocus());

    // left
    key = QKeyEvent(QEvent::KeyPress, Qt::Key_Left, Qt::NoModifier, "", false, 1);
    QApplication::sendEvent(canvas, &key);
    QVERIFY(key.isAccepted());

    item = findItem<QQuickItem>(canvas->rootObject(), "item3");
    QVERIFY(item);
    QVERIFY(item->hasActiveFocus());

    // up
    key = QKeyEvent(QEvent::KeyPress, Qt::Key_Up, Qt::NoModifier, "", false, 1);
    QApplication::sendEvent(canvas, &key);
    QVERIFY(key.isAccepted());

    item = findItem<QQuickItem>(canvas->rootObject(), "item1");
    QVERIFY(item);
    QVERIFY(item->hasActiveFocus());

    // tab
    key = QKeyEvent(QEvent::KeyPress, Qt::Key_Tab, Qt::NoModifier, "", false, 1);
    QApplication::sendEvent(canvas, &key);
    QVERIFY(key.isAccepted());

    item = findItem<QQuickItem>(canvas->rootObject(), "item2");
    QVERIFY(item);
    QVERIFY(item->hasActiveFocus());

    // backtab
    key = QKeyEvent(QEvent::KeyPress, Qt::Key_Backtab, Qt::NoModifier, "", false, 1);
    QApplication::sendEvent(canvas, &key);
    QVERIFY(key.isAccepted());

    item = findItem<QQuickItem>(canvas->rootObject(), "item1");
    QVERIFY(item);
    QVERIFY(item->hasActiveFocus());

    delete canvas;
}

void tst_QQuickItem::keyNavigation_RightToLeft()
{
    QQuickView *canvas = new QQuickView(0);
    canvas->setBaseSize(QSize(240,320));

    canvas->setSource(QUrl::fromLocalFile(TESTDATA("keynavigationtest.qml")));
    canvas->show();
    canvas->requestActivateWindow();
    QTest::qWaitForWindowShown(canvas);
    QTRY_VERIFY(QGuiApplication::focusWindow() == canvas);

    QQuickItem *rootItem = qobject_cast<QQuickItem*>(canvas->rootObject());
    QVERIFY(rootItem);
    QQuickItemPrivate* rootItemPrivate = QQuickItemPrivate::get(rootItem);

    rootItemPrivate->effectiveLayoutMirror = true; // LayoutMirroring.mirror: true
    rootItemPrivate->isMirrorImplicit = false;
    rootItemPrivate->inheritMirrorFromItem = true; // LayoutMirroring.inherit: true
    rootItemPrivate->resolveLayoutMirror();

    QEvent wa(QEvent::WindowActivate);
    QApplication::sendEvent(canvas, &wa);
    QFocusEvent fe(QEvent::FocusIn);
    QApplication::sendEvent(canvas, &fe);

    QQuickItem *item = findItem<QQuickItem>(canvas->rootObject(), "item1");
    QVERIFY(item);
    QVERIFY(item->hasActiveFocus());

    QVariant result;
    QVERIFY(QMetaObject::invokeMethod(canvas->rootObject(), "verify",
            Q_RETURN_ARG(QVariant, result)));
    QVERIFY(result.toBool());

    // right
    QKeyEvent key(QEvent::KeyPress, Qt::Key_Left, Qt::NoModifier, "", false, 1);
    QApplication::sendEvent(canvas, &key);
    QVERIFY(key.isAccepted());

    item = findItem<QQuickItem>(canvas->rootObject(), "item2");
    QVERIFY(item);
    QVERIFY(item->hasActiveFocus());

    // left
    key = QKeyEvent(QEvent::KeyPress, Qt::Key_Right, Qt::NoModifier, "", false, 1);
    QApplication::sendEvent(canvas, &key);
    QVERIFY(key.isAccepted());

    item = findItem<QQuickItem>(canvas->rootObject(), "item1");
    QVERIFY(item);
    QVERIFY(item->hasActiveFocus());

    delete canvas;
}

void tst_QQuickItem::keyNavigation_skipNotVisible()
{
    QQuickView *canvas = new QQuickView(0);
    canvas->setBaseSize(QSize(240,320));

    canvas->setSource(QUrl::fromLocalFile(TESTDATA("keynavigationtest.qml")));
    canvas->show();
    canvas->requestActivateWindow();
    QTest::qWaitForWindowShown(canvas);
    QTRY_VERIFY(QGuiApplication::focusWindow() == canvas);

    QQuickItem *item = findItem<QQuickItem>(canvas->rootObject(), "item1");
    QVERIFY(item);
    QVERIFY(item->hasActiveFocus());

    // Set item 2 to not visible
    item = findItem<QQuickItem>(canvas->rootObject(), "item2");
    QVERIFY(item);
    item->setVisible(false);
    QVERIFY(!item->isVisible());

    // right
    QKeyEvent key(QEvent::KeyPress, Qt::Key_Right, Qt::NoModifier, "", false, 1);
    QApplication::sendEvent(canvas, &key);
    QVERIFY(key.isAccepted());

    item = findItem<QQuickItem>(canvas->rootObject(), "item1");
    QVERIFY(item);
    QVERIFY(item->hasActiveFocus());

    // tab
    key = QKeyEvent(QEvent::KeyPress, Qt::Key_Tab, Qt::NoModifier, "", false, 1);
    QApplication::sendEvent(canvas, &key);
    QVERIFY(key.isAccepted());

    item = findItem<QQuickItem>(canvas->rootObject(), "item3");
    QVERIFY(item);
    QVERIFY(item->hasActiveFocus());

    // backtab
    key = QKeyEvent(QEvent::KeyPress, Qt::Key_Backtab, Qt::NoModifier, "", false, 1);
    QApplication::sendEvent(canvas, &key);
    QVERIFY(key.isAccepted());

    item = findItem<QQuickItem>(canvas->rootObject(), "item1");
    QVERIFY(item);
    QVERIFY(item->hasActiveFocus());

    //Set item 3 to not visible
    item = findItem<QQuickItem>(canvas->rootObject(), "item3");
    QVERIFY(item);
    item->setVisible(false);
    QVERIFY(!item->isVisible());

    // tab
    key = QKeyEvent(QEvent::KeyPress, Qt::Key_Tab, Qt::NoModifier, "", false, 1);
    QApplication::sendEvent(canvas, &key);
    QVERIFY(key.isAccepted());

    item = findItem<QQuickItem>(canvas->rootObject(), "item4");
    QVERIFY(item);
    QVERIFY(item->hasActiveFocus());

    // backtab
    key = QKeyEvent(QEvent::KeyPress, Qt::Key_Backtab, Qt::NoModifier, "", false, 1);
    QApplication::sendEvent(canvas, &key);
    QVERIFY(key.isAccepted());

    item = findItem<QQuickItem>(canvas->rootObject(), "item1");
    QVERIFY(item);
    QVERIFY(item->hasActiveFocus());

    delete canvas;
}

void tst_QQuickItem::keyNavigation_implicitSetting()
{
    QQuickView *canvas = new QQuickView(0);
    canvas->setBaseSize(QSize(240,320));

    canvas->setSource(QUrl::fromLocalFile(TESTDATA("keynavigationtest_implicit.qml")));
    canvas->show();
    canvas->requestActivateWindow();
    QTest::qWaitForWindowShown(canvas);
    QTRY_VERIFY(QGuiApplication::focusWindow() == canvas);

    QEvent wa(QEvent::WindowActivate);
    QApplication::sendEvent(canvas, &wa);
    QFocusEvent fe(QEvent::FocusIn);
    QApplication::sendEvent(canvas, &fe);

    QQuickItem *item = findItem<QQuickItem>(canvas->rootObject(), "item1");
    QVERIFY(item);
    QVERIFY(item->hasActiveFocus());

    QVariant result;
    QVERIFY(QMetaObject::invokeMethod(canvas->rootObject(), "verify",
            Q_RETURN_ARG(QVariant, result)));
    QVERIFY(result.toBool());

    // right
    QKeyEvent key(QEvent::KeyPress, Qt::Key_Right, Qt::NoModifier, "", false, 1);
    QApplication::sendEvent(canvas, &key);
    QVERIFY(key.isAccepted());

    item = findItem<QQuickItem>(canvas->rootObject(), "item2");
    QVERIFY(item);
    QVERIFY(item->hasActiveFocus());

    // back to item1
    key = QKeyEvent(QEvent::KeyPress, Qt::Key_Left, Qt::NoModifier, "", false, 1);
    QApplication::sendEvent(canvas, &key);
    QVERIFY(key.isAccepted());

    item = findItem<QQuickItem>(canvas->rootObject(), "item1");
    QVERIFY(item);
    QVERIFY(item->hasActiveFocus());

    // down
    key = QKeyEvent(QEvent::KeyPress, Qt::Key_Down, Qt::NoModifier, "", false, 1);
    QApplication::sendEvent(canvas, &key);
    QVERIFY(key.isAccepted());

    item = findItem<QQuickItem>(canvas->rootObject(), "item3");
    QVERIFY(item);
    QVERIFY(item->hasActiveFocus());

    // move to item4
    key = QKeyEvent(QEvent::KeyPress, Qt::Key_Right, Qt::NoModifier, "", false, 1);
    QApplication::sendEvent(canvas, &key);
    QVERIFY(key.isAccepted());

    item = findItem<QQuickItem>(canvas->rootObject(), "item4");
    QVERIFY(item);
    QVERIFY(item->hasActiveFocus());

    // left
    key = QKeyEvent(QEvent::KeyPress, Qt::Key_Left, Qt::NoModifier, "", false, 1);
    QApplication::sendEvent(canvas, &key);
    QVERIFY(key.isAccepted());

    item = findItem<QQuickItem>(canvas->rootObject(), "item3");
    QVERIFY(item);
    QVERIFY(item->hasActiveFocus());

    // back to item4
    key = QKeyEvent(QEvent::KeyPress, Qt::Key_Right, Qt::NoModifier, "", false, 1);
    QApplication::sendEvent(canvas, &key);
    QVERIFY(key.isAccepted());

    item = findItem<QQuickItem>(canvas->rootObject(), "item4");
    QVERIFY(item);
    QVERIFY(item->hasActiveFocus());

    // up
    key = QKeyEvent(QEvent::KeyPress, Qt::Key_Up, Qt::NoModifier, "", false, 1);
    QApplication::sendEvent(canvas, &key);
    QVERIFY(key.isAccepted());

    item = findItem<QQuickItem>(canvas->rootObject(), "item2");
    QVERIFY(item);
    QVERIFY(item->hasActiveFocus());

    // back to item4
    key = QKeyEvent(QEvent::KeyPress, Qt::Key_Down, Qt::NoModifier, "", false, 1);
    QApplication::sendEvent(canvas, &key);
    QVERIFY(key.isAccepted());

    item = findItem<QQuickItem>(canvas->rootObject(), "item4");
    QVERIFY(item);
    QVERIFY(item->hasActiveFocus());

    // tab
    key = QKeyEvent(QEvent::KeyPress, Qt::Key_Tab, Qt::NoModifier, "", false, 1);
    QApplication::sendEvent(canvas, &key);
    QVERIFY(key.isAccepted());

    item = findItem<QQuickItem>(canvas->rootObject(), "item1");
    QVERIFY(item);
    QVERIFY(item->hasActiveFocus());

    // back to item4
    key = QKeyEvent(QEvent::KeyPress, Qt::Key_Backtab, Qt::NoModifier, "", false, 1);
    QApplication::sendEvent(canvas, &key);
    QVERIFY(key.isAccepted());

    item = findItem<QQuickItem>(canvas->rootObject(), "item4");
    QVERIFY(item);
    QVERIFY(item->hasActiveFocus());

    // backtab
    key = QKeyEvent(QEvent::KeyPress, Qt::Key_Backtab, Qt::NoModifier, "", false, 1);
    QApplication::sendEvent(canvas, &key);
    QVERIFY(key.isAccepted());

    item = findItem<QQuickItem>(canvas->rootObject(), "item3");
    QVERIFY(item);
    QVERIFY(item->hasActiveFocus());

    delete canvas;
}

void tst_QQuickItem::smooth()
{
    QDeclarativeComponent component(&engine);
    component.setData("import QtQuick 2.0; Item { smooth: false; }", QUrl::fromLocalFile(""));
    QQuickItem *item = qobject_cast<QQuickItem*>(component.create());
    QSignalSpy spy(item, SIGNAL(smoothChanged(bool)));

    QVERIFY(item);
    QVERIFY(!item->smooth());

    item->setSmooth(true);
    QVERIFY(item->smooth());
    QCOMPARE(spy.count(),1);
    QList<QVariant> arguments = spy.first();
    QVERIFY(arguments.count() == 1);
    QVERIFY(arguments.at(0).toBool() == true);

    item->setSmooth(true);
    QCOMPARE(spy.count(),1);

    item->setSmooth(false);
    QVERIFY(!item->smooth());
    QCOMPARE(spy.count(),2);
    item->setSmooth(false);
    QCOMPARE(spy.count(),2);

    delete item;
}

void tst_QQuickItem::clip()
{
    QDeclarativeComponent component(&engine);
    component.setData("import QtQuick 2.0\nItem { clip: false\n }", QUrl::fromLocalFile(""));
    QQuickItem *item = qobject_cast<QQuickItem*>(component.create());
    QSignalSpy spy(item, SIGNAL(clipChanged(bool)));

    QVERIFY(item);
    QVERIFY(!item->clip());

    item->setClip(true);
    QVERIFY(item->clip());

    QList<QVariant> arguments = spy.first();
    QVERIFY(arguments.count() == 1);
    QVERIFY(arguments.at(0).toBool() == true);

    QCOMPARE(spy.count(),1);
    item->setClip(true);
    QCOMPARE(spy.count(),1);

    item->setClip(false);
    QVERIFY(!item->clip());
    QCOMPARE(spy.count(),2);
    item->setClip(false);
    QCOMPARE(spy.count(),2);

    delete item;
}

void tst_QQuickItem::mapCoordinates()
{
    QFETCH(int, x);
    QFETCH(int, y);

    QQuickView *canvas = new QQuickView(0);
    canvas->setBaseSize(QSize(300, 300));
    canvas->setSource(QUrl::fromLocalFile(TESTDATA("mapCoordinates.qml")));
    canvas->show();
    qApp->processEvents();

    QQuickItem *root = qobject_cast<QQuickItem*>(canvas->rootObject());
    QVERIFY(root != 0);
    QQuickItem *a = findItem<QQuickItem>(canvas->rootObject(), "itemA");
    QVERIFY(a != 0);
    QQuickItem *b = findItem<QQuickItem>(canvas->rootObject(), "itemB");
    QVERIFY(b != 0);

    QVariant result;

    QVERIFY(QMetaObject::invokeMethod(root, "mapAToB",
            Q_RETURN_ARG(QVariant, result), Q_ARG(QVariant, x), Q_ARG(QVariant, y)));
    QCOMPARE(result.value<QPointF>(), qobject_cast<QQuickItem*>(a)->mapToItem(b, QPointF(x, y)));

    QVERIFY(QMetaObject::invokeMethod(root, "mapAFromB",
            Q_RETURN_ARG(QVariant, result), Q_ARG(QVariant, x), Q_ARG(QVariant, y)));
    QCOMPARE(result.value<QPointF>(), qobject_cast<QQuickItem*>(a)->mapFromItem(b, QPointF(x, y)));

    QVERIFY(QMetaObject::invokeMethod(root, "mapAToNull",
            Q_RETURN_ARG(QVariant, result), Q_ARG(QVariant, x), Q_ARG(QVariant, y)));
    QCOMPARE(result.value<QPointF>(), qobject_cast<QQuickItem*>(a)->mapToScene(QPointF(x, y)));

    QVERIFY(QMetaObject::invokeMethod(root, "mapAFromNull",
            Q_RETURN_ARG(QVariant, result), Q_ARG(QVariant, x), Q_ARG(QVariant, y)));
    QCOMPARE(result.value<QPointF>(), qobject_cast<QQuickItem*>(a)->mapFromScene(QPointF(x, y)));

    QString warning1 = QUrl::fromLocalFile(TESTDATA("mapCoordinates.qml")).toString() + ":48:5: QML Item: mapToItem() given argument \"1122\" which is neither null nor an Item";
    QString warning2 = QUrl::fromLocalFile(TESTDATA("mapCoordinates.qml")).toString() + ":48:5: QML Item: mapFromItem() given argument \"1122\" which is neither null nor an Item";

    QTest::ignoreMessage(QtWarningMsg, qPrintable(warning1));
    QVERIFY(QMetaObject::invokeMethod(root, "checkMapAToInvalid",
            Q_RETURN_ARG(QVariant, result), Q_ARG(QVariant, x), Q_ARG(QVariant, y)));
    QVERIFY(result.toBool());

    QTest::ignoreMessage(QtWarningMsg, qPrintable(warning2));
    QVERIFY(QMetaObject::invokeMethod(root, "checkMapAFromInvalid",
            Q_RETURN_ARG(QVariant, result), Q_ARG(QVariant, x), Q_ARG(QVariant, y)));
    QVERIFY(result.toBool());

    delete canvas;
}

void tst_QQuickItem::mapCoordinates_data()
{
    QTest::addColumn<int>("x");
    QTest::addColumn<int>("y");

    for (int i=-20; i<=20; i+=10)
        QTest::newRow(QTest::toString(i)) << i << i;
}

void tst_QQuickItem::transforms_data()
{
    QTest::addColumn<QByteArray>("qml");
    QTest::addColumn<QTransform>("transform");
    QTest::newRow("translate") << QByteArray("Translate { x: 10; y: 20 }")
        << QTransform(1,0,0,0,1,0,10,20,1);
    QTest::newRow("rotation") << QByteArray("Rotation { angle: 90 }")
        << QTransform(0,1,0,-1,0,0,0,0,1);
    QTest::newRow("scale") << QByteArray("Scale { xScale: 1.5; yScale: -2  }")
        << QTransform(1.5,0,0,0,-2,0,0,0,1);
    QTest::newRow("sequence") << QByteArray("[ Translate { x: 10; y: 20 }, Scale { xScale: 1.5; yScale: -2  } ]")
        << QTransform(1,0,0,0,1,0,10,20,1) * QTransform(1.5,0,0,0,-2,0,0,0,1);
}

void tst_QQuickItem::transforms()
{
    QFETCH(QByteArray, qml);
    QFETCH(QTransform, transform);
    QDeclarativeComponent component(&engine);
    component.setData("import QtQuick 2.0\nItem { transform: "+qml+"}", QUrl::fromLocalFile(""));
    QQuickItem *item = qobject_cast<QQuickItem*>(component.create());
    QVERIFY(item);
    QCOMPARE(item->itemTransform(0,0), transform);
}

void tst_QQuickItem::childrenProperty()
{
    QDeclarativeComponent component(&engine, TESTDATA("childrenProperty.qml"));

    QObject *o = component.create();
    QVERIFY(o != 0);

    QCOMPARE(o->property("test1").toBool(), true);
    QCOMPARE(o->property("test2").toBool(), true);
    QCOMPARE(o->property("test3").toBool(), true);
    QCOMPARE(o->property("test4").toBool(), true);
    QCOMPARE(o->property("test5").toBool(), true);
    delete o;
}

void tst_QQuickItem::resourcesProperty()
{
    QDeclarativeComponent component(&engine, TESTDATA("resourcesProperty.qml"));

    QObject *o = component.create();
    QVERIFY(o != 0);

    QCOMPARE(o->property("test1").toBool(), true);
    QCOMPARE(o->property("test2").toBool(), true);
    QCOMPARE(o->property("test3").toBool(), true);
    QCOMPARE(o->property("test4").toBool(), true);
    QCOMPARE(o->property("test5").toBool(), true);
    delete o;
}

void tst_QQuickItem::propertyChanges()
{
    QQuickView *canvas = new QQuickView(0);
    canvas->setBaseSize(QSize(300, 300));
    canvas->setSource(QUrl::fromLocalFile(TESTDATA("propertychanges.qml")));
    canvas->show();
    canvas->requestActivateWindow();
    QTest::qWaitForWindowShown(canvas);
    QTRY_VERIFY(QGuiApplication::focusWindow() == canvas);

    QQuickItem *item = findItem<QQuickItem>(canvas->rootObject(), "item");
    QQuickItem *parentItem = findItem<QQuickItem>(canvas->rootObject(), "parentItem");

    QVERIFY(item);
    QVERIFY(parentItem);

    QSignalSpy parentSpy(item, SIGNAL(parentChanged(QQuickItem *)));
    QSignalSpy widthSpy(item, SIGNAL(widthChanged()));
    QSignalSpy heightSpy(item, SIGNAL(heightChanged()));
    QSignalSpy baselineOffsetSpy(item, SIGNAL(baselineOffsetChanged(qreal)));
    QSignalSpy childrenRectSpy(parentItem, SIGNAL(childrenRectChanged(QRectF)));
    QSignalSpy focusSpy(item, SIGNAL(focusChanged(bool)));
    QSignalSpy wantsFocusSpy(parentItem, SIGNAL(activeFocusChanged(bool)));
    QSignalSpy childrenChangedSpy(parentItem, SIGNAL(childrenChanged()));
    QSignalSpy xSpy(item, SIGNAL(xChanged()));
    QSignalSpy ySpy(item, SIGNAL(yChanged()));

    item->setParentItem(parentItem);
    item->setWidth(100.0);
    item->setHeight(200.0);
    item->setFocus(true);
    item->setBaselineOffset(10.0);

    QCOMPARE(item->parentItem(), parentItem);
    QCOMPARE(parentSpy.count(),1);
    QList<QVariant> parentArguments = parentSpy.first();
    QVERIFY(parentArguments.count() == 1);
    QCOMPARE(item->parentItem(), qvariant_cast<QQuickItem *>(parentArguments.at(0)));
    QCOMPARE(childrenChangedSpy.count(),1);

    item->setParentItem(parentItem);
    QCOMPARE(childrenChangedSpy.count(),1);

    QCOMPARE(item->width(), 100.0);
    QCOMPARE(widthSpy.count(),1);

    QCOMPARE(item->height(), 200.0);
    QCOMPARE(heightSpy.count(),1);

    QCOMPARE(item->baselineOffset(), 10.0);
    QCOMPARE(baselineOffsetSpy.count(),1);
    QList<QVariant> baselineOffsetArguments = baselineOffsetSpy.first();
    QVERIFY(baselineOffsetArguments.count() == 1);
    QCOMPARE(item->baselineOffset(), baselineOffsetArguments.at(0).toReal());

    QCOMPARE(parentItem->childrenRect(), QRectF(0.0,0.0,100.0,200.0));
    QCOMPARE(childrenRectSpy.count(),1);
    QList<QVariant> childrenRectArguments = childrenRectSpy.at(0);
    QVERIFY(childrenRectArguments.count() == 1);
    QCOMPARE(parentItem->childrenRect(), childrenRectArguments.at(0).toRectF());

    QCOMPARE(item->hasActiveFocus(), true);
    QCOMPARE(focusSpy.count(),1);
    QList<QVariant> focusArguments = focusSpy.first();
    QVERIFY(focusArguments.count() == 1);
    QCOMPARE(focusArguments.at(0).toBool(), true);

    QCOMPARE(parentItem->hasActiveFocus(), false);
    QCOMPARE(parentItem->hasFocus(), false);
    QCOMPARE(wantsFocusSpy.count(),0);

    item->setX(10.0);
    QCOMPARE(item->x(), 10.0);
    QCOMPARE(xSpy.count(), 1);

    item->setY(10.0);
    QCOMPARE(item->y(), 10.0);
    QCOMPARE(ySpy.count(), 1);

    delete canvas;
}

void tst_QQuickItem::childrenRect()
{
    QQuickView *canvas = new QQuickView(0);
    canvas->setSource(QUrl::fromLocalFile(TESTDATA("childrenRect.qml")));
    canvas->setBaseSize(QSize(240,320));
    canvas->show();

    QQuickItem *o = canvas->rootObject();
    QQuickItem *item = o->findChild<QQuickItem*>("testItem");
    QCOMPARE(item->width(), qreal(0));
    QCOMPARE(item->height(), qreal(0));

    o->setProperty("childCount", 1);
    QCOMPARE(item->width(), qreal(10));
    QCOMPARE(item->height(), qreal(20));

    o->setProperty("childCount", 5);
    QCOMPARE(item->width(), qreal(50));
    QCOMPARE(item->height(), qreal(100));

    o->setProperty("childCount", 0);
    QCOMPARE(item->width(), qreal(0));
    QCOMPARE(item->height(), qreal(0));

    delete o;
    delete canvas;
}

// QTBUG-11383
void tst_QQuickItem::childrenRectBug()
{
    QQuickView *canvas = new QQuickView(0);
    canvas->setSource(QUrl::fromLocalFile(TESTDATA("childrenRectBug.qml")));
    canvas->show();

    QQuickItem *o = canvas->rootObject();
    QQuickItem *item = o->findChild<QQuickItem*>("theItem");
    QCOMPARE(item->width(), qreal(200));
    QCOMPARE(item->height(), qreal(100));
    QCOMPARE(item->x(), qreal(100));

    delete canvas;
}

// QTBUG-11465
void tst_QQuickItem::childrenRectBug2()
{
    QQuickView *canvas = new QQuickView(0);
    canvas->setSource(QUrl::fromLocalFile(TESTDATA("childrenRectBug2.qml")));
    canvas->show();

    QQuickRectangle *rect = qobject_cast<QQuickRectangle*>(canvas->rootObject());
    QVERIFY(rect);
    QQuickItem *item = rect->findChild<QQuickItem*>("theItem");
    QCOMPARE(item->width(), qreal(100));
    QCOMPARE(item->height(), qreal(110));
    QCOMPARE(item->x(), qreal(130));

    QQuickItemPrivate *rectPrivate = QQuickItemPrivate::get(rect);
    rectPrivate->setState("row");
    QCOMPARE(item->width(), qreal(210));
    QCOMPARE(item->height(), qreal(50));
    QCOMPARE(item->x(), qreal(75));

    delete canvas;
}

// QTBUG-12722
void tst_QQuickItem::childrenRectBug3()
{
    QQuickView *canvas = new QQuickView(0);
    canvas->setSource(QUrl::fromLocalFile(TESTDATA("childrenRectBug3.qml")));
    canvas->show();

    //don't crash on delete
    delete canvas;
}

// QTBUG-13893
void tst_QQuickItem::transformCrash()
{
    QQuickView *canvas = new QQuickView(0);
    canvas->setSource(QUrl::fromLocalFile(TESTDATA("transformCrash.qml")));
    canvas->show();

    delete canvas;
}

void tst_QQuickItem::implicitSize()
{
    QQuickView *canvas = new QQuickView(0);
    canvas->setSource(QUrl::fromLocalFile(TESTDATA("implicitsize.qml")));
    canvas->show();

    QQuickItem *item = qobject_cast<QQuickItem*>(canvas->rootObject());
    QVERIFY(item);
    QCOMPARE(item->width(), qreal(80));
    QCOMPARE(item->height(), qreal(60));

    QCOMPARE(item->implicitWidth(), qreal(200));
    QCOMPARE(item->implicitHeight(), qreal(100));

    QMetaObject::invokeMethod(item, "resetSize");

    QCOMPARE(item->width(), qreal(200));
    QCOMPARE(item->height(), qreal(100));

    QMetaObject::invokeMethod(item, "changeImplicit");

    QCOMPARE(item->implicitWidth(), qreal(150));
    QCOMPARE(item->implicitHeight(), qreal(80));
    QCOMPARE(item->width(), qreal(150));
    QCOMPARE(item->height(), qreal(80));

    delete canvas;
}

void tst_QQuickItem::qtbug_16871()
{
    QDeclarativeComponent component(&engine, TESTDATA("qtbug_16871.qml"));
    QObject *o = component.create();
    QVERIFY(o != 0);
    delete o;
}

QTEST_MAIN(tst_QQuickItem)

#include "tst_qquickitem.moc"
