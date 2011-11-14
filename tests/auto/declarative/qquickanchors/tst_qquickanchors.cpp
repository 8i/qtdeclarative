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
#include <QSignalSpy>
#include <private/qquickitem_p.h>
#include <QtDeclarative/qdeclarativeengine.h>
#include <QtDeclarative/qdeclarativecomponent.h>
#include <QtDeclarative/qquickview.h>
#include <QtDeclarative/private/qquickrectangle_p.h>
#include <QtDeclarative/private/qquicktext_p.h>
#include <QtDeclarative/private/qquickanchors_p_p.h>
#include <QtDeclarative/private/qquickitem_p.h>
#include "../shared/util.h"

Q_DECLARE_METATYPE(QQuickAnchors::Anchor)
Q_DECLARE_METATYPE(QQuickAnchorLine::AnchorLine)

class tst_qquickanchors : public QObject
{
    Q_OBJECT
public:
    tst_qquickanchors() {}

private slots:
    void basicAnchors();
    void basicAnchorsRTL();
    void loops();
    void illegalSets();
    void illegalSets_data();
    void reset();
    void reset_data();
    void resetConvenience();
    void nullItem();
    void nullItem_data();
    void crash1();
    void centerIn();
    void centerInRTL();
    void centerInRotation();
    void hvCenter();
    void hvCenterRTL();
    void fill();
    void fillRTL();
    void margins();
    void marginsRTL();
};

/*
   Find an item with the specified objectName.
*/
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

void tst_qquickanchors::basicAnchors()
{
    QQuickView *view = new QQuickView;
    view->setSource(QUrl::fromLocalFile(TESTDATA("anchors.qml")));

    qApp->processEvents();

    //sibling horizontal
    QCOMPARE(findItem<QQuickRectangle>(view->rootObject(), QLatin1String("rect1"))->x(), 26.0);
    QCOMPARE(findItem<QQuickRectangle>(view->rootObject(), QLatin1String("rect2"))->x(), 122.0);
    QCOMPARE(findItem<QQuickRectangle>(view->rootObject(), QLatin1String("rect3"))->x(), 74.0);
    QCOMPARE(findItem<QQuickRectangle>(view->rootObject(), QLatin1String("rect4"))->x(), 16.0);
    QCOMPARE(findItem<QQuickRectangle>(view->rootObject(), QLatin1String("rect5"))->x(), 112.0);
    QCOMPARE(findItem<QQuickRectangle>(view->rootObject(), QLatin1String("rect6"))->x(), 64.0);

    //parent horizontal
    QCOMPARE(findItem<QQuickRectangle>(view->rootObject(), QLatin1String("rect7"))->x(), 0.0);
    QCOMPARE(findItem<QQuickRectangle>(view->rootObject(), QLatin1String("rect8"))->x(), 240.0);
    QCOMPARE(findItem<QQuickRectangle>(view->rootObject(), QLatin1String("rect9"))->x(), 120.0);
    QCOMPARE(findItem<QQuickRectangle>(view->rootObject(), QLatin1String("rect10"))->x(), -10.0);
    QCOMPARE(findItem<QQuickRectangle>(view->rootObject(), QLatin1String("rect11"))->x(), 230.0);
    QCOMPARE(findItem<QQuickRectangle>(view->rootObject(), QLatin1String("rect12"))->x(), 110.0);

    //vertical
    QCOMPARE(findItem<QQuickRectangle>(view->rootObject(), QLatin1String("rect13"))->y(), 20.0);
    QCOMPARE(findItem<QQuickRectangle>(view->rootObject(), QLatin1String("rect14"))->y(), 155.0);

    //stretch
    QCOMPARE(findItem<QQuickRectangle>(view->rootObject(), QLatin1String("rect15"))->x(), 26.0);
    QCOMPARE(findItem<QQuickRectangle>(view->rootObject(), QLatin1String("rect15"))->width(), 96.0);
    QCOMPARE(findItem<QQuickRectangle>(view->rootObject(), QLatin1String("rect16"))->x(), 26.0);
    QCOMPARE(findItem<QQuickRectangle>(view->rootObject(), QLatin1String("rect16"))->width(), 192.0);
    QCOMPARE(findItem<QQuickRectangle>(view->rootObject(), QLatin1String("rect17"))->x(), -70.0);
    QCOMPARE(findItem<QQuickRectangle>(view->rootObject(), QLatin1String("rect17"))->width(), 192.0);

    //vertical stretch
    QCOMPARE(findItem<QQuickRectangle>(view->rootObject(), QLatin1String("rect18"))->y(), 20.0);
    QCOMPARE(findItem<QQuickRectangle>(view->rootObject(), QLatin1String("rect18"))->height(), 40.0);

    //more parent horizontal
    QCOMPARE(findItem<QQuickRectangle>(view->rootObject(), QLatin1String("rect19"))->x(), 115.0);
    QCOMPARE(findItem<QQuickRectangle>(view->rootObject(), QLatin1String("rect20"))->x(), 235.0);
    QCOMPARE(findItem<QQuickRectangle>(view->rootObject(), QLatin1String("rect21"))->x(), -5.0);

    //centerIn
    QCOMPARE(findItem<QQuickRectangle>(view->rootObject(), QLatin1String("rect22"))->x(), 69.0);
    QCOMPARE(findItem<QQuickRectangle>(view->rootObject(), QLatin1String("rect22"))->y(), 5.0);

     //margins
    QCOMPARE(findItem<QQuickRectangle>(view->rootObject(), QLatin1String("rect23"))->x(), 31.0);
    QCOMPARE(findItem<QQuickRectangle>(view->rootObject(), QLatin1String("rect23"))->y(), 5.0);
    QCOMPARE(findItem<QQuickRectangle>(view->rootObject(), QLatin1String("rect23"))->width(), 86.0);
    QCOMPARE(findItem<QQuickRectangle>(view->rootObject(), QLatin1String("rect23"))->height(), 10.0);

    // offsets
    QCOMPARE(findItem<QQuickRectangle>(view->rootObject(), QLatin1String("rect24"))->x(), 26.0);
    QCOMPARE(findItem<QQuickRectangle>(view->rootObject(), QLatin1String("rect25"))->y(), 60.0);
    QCOMPARE(findItem<QQuickRectangle>(view->rootObject(), QLatin1String("rect26"))->y(), 5.0);

    //baseline
    QQuickText *text1 = findItem<QQuickText>(view->rootObject(), QLatin1String("text1"));
    QQuickText *text2 = findItem<QQuickText>(view->rootObject(), QLatin1String("text2"));
    QCOMPARE(text1->y(), text2->y());

    delete view;
}

QQuickItem* childItem(QQuickItem *parentItem, const char * itemString) {
    return findItem<QQuickItem>(parentItem, QLatin1String(itemString));
}

qreal offsetMasterRTL(QQuickItem *rootItem, const char * itemString) {
    QQuickItem* masterItem = findItem<QQuickItem>(rootItem,  QLatin1String("masterRect"));
    return masterItem->width()+2*masterItem->x()-findItem<QQuickItem>(rootItem,  QLatin1String(itemString))->width();
}

qreal offsetParentRTL(QQuickItem *rootItem, const char * itemString) {
    return rootItem->width()+2*rootItem->x()-findItem<QQuickItem>(rootItem,  QLatin1String(itemString))->width();
}

void mirrorAnchors(QQuickItem *item) {
    QQuickItemPrivate *itemPrivate = QQuickItemPrivate::get(item);
    itemPrivate->setLayoutMirror(true);
}

void tst_qquickanchors::basicAnchorsRTL()
{
    QQuickView *view = new QQuickView;
    view->setSource(QUrl::fromLocalFile(TESTDATA("anchors.qml")));

    qApp->processEvents();

    QQuickItem* rootItem = qobject_cast<QQuickItem*>(view->rootObject());
    foreach (QObject *child, rootItem->children()) {
        bool mirrored = QQuickItemPrivate::get(qobject_cast<QQuickItem*>(child))->anchors()->property("mirrored").toBool();
        QCOMPARE(mirrored, false);
    }

    foreach (QObject *child, rootItem->children())
        mirrorAnchors(qobject_cast<QQuickItem*>(child));

    foreach (QObject *child, rootItem->children()) {
        bool mirrored = QQuickItemPrivate::get(qobject_cast<QQuickItem*>(child))->anchors()->property("mirrored").toBool();
        QCOMPARE(mirrored, true);
    }

    //sibling horizontal
    QCOMPARE(childItem(rootItem, "rect1")->x(), offsetMasterRTL(rootItem, "rect1")-26.0);
    QCOMPARE(childItem(rootItem, "rect2")->x(), offsetMasterRTL(rootItem, "rect2")-122.0);
    QCOMPARE(childItem(rootItem, "rect3")->x(), offsetMasterRTL(rootItem, "rect3")-74.0);
    QCOMPARE(childItem(rootItem, "rect4")->x(), offsetMasterRTL(rootItem, "rect4")-16.0);
    QCOMPARE(childItem(rootItem, "rect5")->x(), offsetMasterRTL(rootItem, "rect5")-112.0);
    QCOMPARE(childItem(rootItem, "rect6")->x(), offsetMasterRTL(rootItem, "rect6")-64.0);

    //parent horizontal
    QCOMPARE(childItem(rootItem, "rect7")->x(), offsetParentRTL(rootItem, "rect7")-0.0);
    QCOMPARE(childItem(rootItem, "rect8")->x(), offsetParentRTL(rootItem, "rect8")-240.0);
    QCOMPARE(childItem(rootItem, "rect9")->x(), offsetParentRTL(rootItem, "rect9")-120.0);
    QCOMPARE(childItem(rootItem, "rect10")->x(), offsetParentRTL(rootItem, "rect10")+10.0);
    QCOMPARE(childItem(rootItem, "rect11")->x(), offsetParentRTL(rootItem, "rect11")-230.0);
    QCOMPARE(childItem(rootItem, "rect12")->x(), offsetParentRTL(rootItem, "rect12")-110.0);

    //vertical
    QCOMPARE(childItem(rootItem, "rect13")->y(), 20.0);
    QCOMPARE(childItem(rootItem, "rect14")->y(), 155.0);

    //stretch
    QCOMPARE(childItem(rootItem, "rect15")->x(), offsetMasterRTL(rootItem, "rect15")-26.0);
    QCOMPARE(childItem(rootItem, "rect15")->width(), 96.0);
    QCOMPARE(childItem(rootItem, "rect16")->x(), offsetMasterRTL(rootItem, "rect16")-26.0);
    QCOMPARE(childItem(rootItem, "rect16")->width(), 192.0);
    QCOMPARE(childItem(rootItem, "rect17")->x(), offsetMasterRTL(rootItem, "rect17")+70.0);
    QCOMPARE(childItem(rootItem, "rect17")->width(), 192.0);

    //vertical stretch
    QCOMPARE(childItem(rootItem, "rect18")->y(), 20.0);
    QCOMPARE(childItem(rootItem, "rect18")->height(), 40.0);

    //more parent horizontal
    QCOMPARE(childItem(rootItem, "rect19")->x(), offsetParentRTL(rootItem, "rect19")-115.0);
    QCOMPARE(childItem(rootItem, "rect20")->x(), offsetParentRTL(rootItem, "rect20")-235.0);
    QCOMPARE(childItem(rootItem, "rect21")->x(), offsetParentRTL(rootItem, "rect21")+5.0);

    //centerIn
    QCOMPARE(childItem(rootItem, "rect22")->x(), offsetMasterRTL(rootItem, "rect22")-69.0);
    QCOMPARE(childItem(rootItem, "rect22")->y(), 5.0);

     //margins
    QCOMPARE(childItem(rootItem, "rect23")->x(), offsetMasterRTL(rootItem, "rect23")-31.0);
    QCOMPARE(childItem(rootItem, "rect23")->y(), 5.0);
    QCOMPARE(childItem(rootItem, "rect23")->width(), 86.0);
    QCOMPARE(childItem(rootItem, "rect23")->height(), 10.0);

    // offsets
    QCOMPARE(childItem(rootItem, "rect24")->x(), offsetMasterRTL(rootItem, "rect24")-26.0);
    QCOMPARE(childItem(rootItem, "rect25")->y(), 60.0);
    QCOMPARE(childItem(rootItem, "rect26")->y(), 5.0);

    //baseline
    QQuickText *text1 = findItem<QQuickText>(rootItem, QLatin1String("text1"));
    QQuickText *text2 = findItem<QQuickText>(rootItem, QLatin1String("text2"));
    QCOMPARE(text1->y(), text2->y());

    delete view;
}

// mostly testing that we don't crash
void tst_qquickanchors::loops()
{
    {
        QUrl source(QUrl::fromLocalFile(TESTDATA("loop1.qml")));

        QString expect = source.toString() + ":6:5: QML Text: Possible anchor loop detected on horizontal anchor.";
        QTest::ignoreMessage(QtWarningMsg, expect.toLatin1());
        QTest::ignoreMessage(QtWarningMsg, expect.toLatin1());
        QTest::ignoreMessage(QtWarningMsg, expect.toLatin1());

        QQuickView *view = new QQuickView;
        view->setSource(source);
        qApp->processEvents();

        delete view;
    }

    {
        QUrl source(QUrl::fromLocalFile(TESTDATA("loop2.qml")));

        QString expect = source.toString() + ":8:3: QML Image: Possible anchor loop detected on horizontal anchor.";
        QTest::ignoreMessage(QtWarningMsg, expect.toLatin1());

        QQuickView *view = new QQuickView;
        view->setSource(source);
        qApp->processEvents();

        delete view;
    }
}

void tst_qquickanchors::illegalSets()
{
    QFETCH(QString, qml);
    QFETCH(QString, warning);

    QTest::ignoreMessage(QtWarningMsg, warning.toLatin1());

    QDeclarativeEngine engine;
    QDeclarativeComponent component(&engine);
    component.setData(QByteArray("import QtQuick 1.0\n" + qml.toUtf8()), QUrl::fromLocalFile(""));
    if (!component.isReady())
        qWarning() << "Test errors:" << component.errors();
    QVERIFY(component.isReady());
    QObject *o = component.create();
    delete o;
}

void tst_qquickanchors::illegalSets_data()
{
    QTest::addColumn<QString>("qml");
    QTest::addColumn<QString>("warning");

    QTest::newRow("H - too many anchors")
        << "Rectangle { id: rect; Rectangle { anchors.left: rect.left; anchors.right: rect.right; anchors.horizontalCenter: rect.horizontalCenter } }"
        << "file::2:23: QML Rectangle: Cannot specify left, right, and hcenter anchors.";

    foreach (const QString &side, QStringList() << "left" << "right") {
        QTest::newRow("H - anchor to V")
            << QString("Rectangle { Rectangle { anchors.%1: parent.top } }").arg(side)
            << "file::2:13: QML Rectangle: Cannot anchor a horizontal edge to a vertical edge.";

        QTest::newRow("H - anchor to non parent/sibling")
            << QString("Rectangle { Item { Rectangle { id: rect } } Rectangle { anchors.%1: rect.%1 } }").arg(side)
            << "file::2:45: QML Rectangle: Cannot anchor to an item that isn't a parent or sibling.";

        QTest::newRow("H - anchor to self")
            << QString("Rectangle { id: rect; anchors.%1: rect.%1 }").arg(side)
            << "file::2:1: QML Rectangle: Cannot anchor item to self.";
    }


    QTest::newRow("V - too many anchors")
        << "Rectangle { id: rect; Rectangle { anchors.top: rect.top; anchors.bottom: rect.bottom; anchors.verticalCenter: rect.verticalCenter } }"
        << "file::2:23: QML Rectangle: Cannot specify top, bottom, and vcenter anchors.";

    QTest::newRow("V - too many anchors with baseline")
        << "Rectangle { Text { id: text1; text: \"Hello\" } Text { anchors.baseline: text1.baseline; anchors.top: text1.top; } }"
        << "file::2:47: QML Text: Baseline anchor cannot be used in conjunction with top, bottom, or vcenter anchors.";

    foreach (const QString &side, QStringList() << "top" << "bottom" << "baseline") {

        QTest::newRow("V - anchor to H")
            << QString("Rectangle { Rectangle { anchors.%1: parent.left } }").arg(side)
            << "file::2:13: QML Rectangle: Cannot anchor a vertical edge to a horizontal edge.";

        QTest::newRow("V - anchor to non parent/sibling")
            << QString("Rectangle { Item { Rectangle { id: rect } } Rectangle { anchors.%1: rect.%1 } }").arg(side)
            << "file::2:45: QML Rectangle: Cannot anchor to an item that isn't a parent or sibling.";

        QTest::newRow("V - anchor to self")
            << QString("Rectangle { id: rect; anchors.%1: rect.%1 }").arg(side)
            << "file::2:1: QML Rectangle: Cannot anchor item to self.";
    }


    QTest::newRow("centerIn - anchor to non parent/sibling")
        << "Rectangle { Item { Rectangle { id: rect } } Rectangle { anchors.centerIn: rect} }"
        << "file::2:45: QML Rectangle: Cannot anchor to an item that isn't a parent or sibling.";


    QTest::newRow("fill - anchor to non parent/sibling")
        << "Rectangle { Item { Rectangle { id: rect } } Rectangle { anchors.fill: rect} }"
        << "file::2:45: QML Rectangle: Cannot anchor to an item that isn't a parent or sibling.";
}

void tst_qquickanchors::reset()
{
    QFETCH(QString, side);
    QFETCH(QQuickAnchorLine::AnchorLine, anchorLine);
    QFETCH(QQuickAnchors::Anchor, usedAnchor);

    QQuickItem *baseItem = new QQuickItem;

    QQuickAnchorLine anchor;
    anchor.item = baseItem;
    anchor.anchorLine = anchorLine;

    QQuickItem *item = new QQuickItem;
    QQuickItemPrivate *itemPrivate = QQuickItemPrivate::get(item);

    const QMetaObject *meta = itemPrivate->anchors()->metaObject();
    QMetaProperty p = meta->property(meta->indexOfProperty(side.toUtf8().constData()));

    QVERIFY(p.write(itemPrivate->anchors(), qVariantFromValue(anchor)));
    QCOMPARE(itemPrivate->anchors()->usedAnchors().testFlag(usedAnchor), true);

    QVERIFY(p.reset(itemPrivate->anchors()));
    QCOMPARE(itemPrivate->anchors()->usedAnchors().testFlag(usedAnchor), false);

    delete item;
    delete baseItem;
}

void tst_qquickanchors::reset_data()
{
    QTest::addColumn<QString>("side");
    QTest::addColumn<QQuickAnchorLine::AnchorLine>("anchorLine");
    QTest::addColumn<QQuickAnchors::Anchor>("usedAnchor");

    QTest::newRow("left") << "left" << QQuickAnchorLine::Left << QQuickAnchors::LeftAnchor;
    QTest::newRow("top") << "top" << QQuickAnchorLine::Top << QQuickAnchors::TopAnchor;
    QTest::newRow("right") << "right" << QQuickAnchorLine::Right << QQuickAnchors::RightAnchor;
    QTest::newRow("bottom") << "bottom" << QQuickAnchorLine::Bottom << QQuickAnchors::BottomAnchor;

    QTest::newRow("hcenter") << "horizontalCenter" << QQuickAnchorLine::HCenter << QQuickAnchors::HCenterAnchor;
    QTest::newRow("vcenter") << "verticalCenter" << QQuickAnchorLine::VCenter << QQuickAnchors::VCenterAnchor;
    QTest::newRow("baseline") << "baseline" << QQuickAnchorLine::Baseline << QQuickAnchors::BaselineAnchor;
}

void tst_qquickanchors::resetConvenience()
{
    QQuickItem *baseItem = new QQuickItem;
    QQuickItem *item = new QQuickItem;
    QQuickItemPrivate *itemPrivate = QQuickItemPrivate::get(item);

    //fill
    itemPrivate->anchors()->setFill(baseItem);
    QVERIFY(itemPrivate->anchors()->fill() == baseItem);
    itemPrivate->anchors()->resetFill();
    QVERIFY(itemPrivate->anchors()->fill() == 0);

    //centerIn
    itemPrivate->anchors()->setCenterIn(baseItem);
    QVERIFY(itemPrivate->anchors()->centerIn() == baseItem);
    itemPrivate->anchors()->resetCenterIn();
    QVERIFY(itemPrivate->anchors()->centerIn() == 0);

    delete item;
    delete baseItem;
}

void tst_qquickanchors::nullItem()
{
    QFETCH(QString, side);

    QQuickAnchorLine anchor;
    QQuickItem *item = new QQuickItem;
    QQuickItemPrivate *itemPrivate = QQuickItemPrivate::get(item);

    const QMetaObject *meta = itemPrivate->anchors()->metaObject();
    QMetaProperty p = meta->property(meta->indexOfProperty(side.toUtf8().constData()));

    QTest::ignoreMessage(QtWarningMsg, "<Unknown File>: QML Item: Cannot anchor to a null item.");
    QVERIFY(p.write(itemPrivate->anchors(), qVariantFromValue(anchor)));

    delete item;
}

void tst_qquickanchors::nullItem_data()
{
    QTest::addColumn<QString>("side");

    QTest::newRow("left") << "left";
    QTest::newRow("top") << "top";
    QTest::newRow("right") << "right";
    QTest::newRow("bottom") << "bottom";

    QTest::newRow("hcenter") << "horizontalCenter";
    QTest::newRow("vcenter") << "verticalCenter";
    QTest::newRow("baseline") << "baseline";
}

//QTBUG-5428
void tst_qquickanchors::crash1()
{
    QUrl source(QUrl::fromLocalFile(TESTDATA("crash1.qml")));

    QString expect = source.toString() + ":3:1: QML Column: Cannot specify top, bottom, verticalCenter, fill or centerIn anchors for items inside Column";

    QTest::ignoreMessage(QtWarningMsg, expect.toLatin1());

    QQuickView *view = new QQuickView(source);
    qApp->processEvents();

    delete view;
}

void tst_qquickanchors::fill()
{
    QQuickView *view = new QQuickView(QUrl::fromLocalFile(TESTDATA("fill.qml")));

    qApp->processEvents();
    QQuickRectangle* rect = findItem<QQuickRectangle>(view->rootObject(), QLatin1String("filler"));
    QQuickItemPrivate *rectPrivate = QQuickItemPrivate::get(rect);
    QCOMPARE(rect->x(), 0.0 + 10.0);
    QCOMPARE(rect->y(), 0.0 + 30.0);
    QCOMPARE(rect->width(), 200.0 - 10.0 - 20.0);
    QCOMPARE(rect->height(), 200.0 - 30.0 - 40.0);
    //Alter Offsets (tests QTBUG-6631)
    rectPrivate->anchors()->setLeftMargin(20.0);
    rectPrivate->anchors()->setRightMargin(0.0);
    rectPrivate->anchors()->setBottomMargin(0.0);
    rectPrivate->anchors()->setTopMargin(10.0);
    QCOMPARE(rect->x(), 0.0 + 20.0);
    QCOMPARE(rect->y(), 0.0 + 10.0);
    QCOMPARE(rect->width(), 200.0 - 20.0);
    QCOMPARE(rect->height(), 200.0 - 10.0);

    delete view;
}

void tst_qquickanchors::fillRTL()
{
    QQuickView *view = new QQuickView(QUrl::fromLocalFile(TESTDATA("fill.qml")));

    qApp->processEvents();
    QQuickRectangle* rect = findItem<QQuickRectangle>(view->rootObject(), QLatin1String("filler"));
    QQuickItemPrivate *rectPrivate = QQuickItemPrivate::get(rect);
    mirrorAnchors(rect);

    QCOMPARE(rect->x(), 0.0 + 20.0);
    QCOMPARE(rect->y(), 0.0 + 30.0);
    QCOMPARE(rect->width(), 200.0 - 10.0 - 20.0);
    QCOMPARE(rect->height(), 200.0 - 30.0 - 40.0);
    //Alter Offsets (tests QTBUG-6631)
    rectPrivate->anchors()->setLeftMargin(20.0);
    rectPrivate->anchors()->setRightMargin(0.0);
    rectPrivate->anchors()->setBottomMargin(0.0);
    rectPrivate->anchors()->setTopMargin(10.0);
    QCOMPARE(rect->x(), 0.0 + 0.0);
    QCOMPARE(rect->y(), 0.0 + 10.0);
    QCOMPARE(rect->width(), 200.0 - 20.0);
    QCOMPARE(rect->height(), 200.0 - 10.0);

    delete view;
}

void tst_qquickanchors::centerIn()
{
    QQuickView *view = new QQuickView(QUrl::fromLocalFile(TESTDATA("centerin.qml")));

    qApp->processEvents();
    QQuickRectangle* rect = findItem<QQuickRectangle>(view->rootObject(), QLatin1String("centered"));
    QQuickItemPrivate *rectPrivate = QQuickItemPrivate::get(rect);

    QCOMPARE(rect->x(), 75.0 + 10);
    QCOMPARE(rect->y(), 75.0 + 30);
    //Alter Offsets (tests QTBUG-6631)
    rectPrivate->anchors()->setHorizontalCenterOffset(-20.0);
    rectPrivate->anchors()->setVerticalCenterOffset(-10.0);
    QCOMPARE(rect->x(), 75.0 - 20.0);
    QCOMPARE(rect->y(), 75.0 - 10.0);

    delete view;
}

void tst_qquickanchors::centerInRTL()
{
    QQuickView *view = new QQuickView(QUrl::fromLocalFile(TESTDATA("centerin.qml")));

    qApp->processEvents();
    QQuickRectangle* rect = findItem<QQuickRectangle>(view->rootObject(), QLatin1String("centered"));
    QQuickItemPrivate *rectPrivate = QQuickItemPrivate::get(rect);
    mirrorAnchors(rect);

    QCOMPARE(rect->x(), 75.0 - 10);
    QCOMPARE(rect->y(), 75.0 + 30);
    //Alter Offsets (tests QTBUG-6631)
    rectPrivate->anchors()->setHorizontalCenterOffset(-20.0);
    rectPrivate->anchors()->setVerticalCenterOffset(-10.0);
    QCOMPARE(rect->x(), 75.0 + 20.0);
    QCOMPARE(rect->y(), 75.0 - 10.0);

    delete view;
}

//QTBUG-12441
void tst_qquickanchors::centerInRotation()
{
    QQuickView *view = new QQuickView(QUrl::fromLocalFile(TESTDATA("centerinRotation.qml")));

    qApp->processEvents();
    QQuickRectangle* outer = findItem<QQuickRectangle>(view->rootObject(), QLatin1String("outer"));
    QQuickRectangle* inner = findItem<QQuickRectangle>(view->rootObject(), QLatin1String("inner"));

    QEXPECT_FAIL("", "QTBUG-12441", Abort);
    QCOMPARE(outer->x(), qreal(49.5));
    QCOMPARE(outer->y(), qreal(49.5));
    QCOMPARE(inner->x(), qreal(25.5));
    QCOMPARE(inner->y(), qreal(25.5));

    delete view;
}

void tst_qquickanchors::hvCenter()
{
    QQuickView *view = new QQuickView(QUrl::fromLocalFile(TESTDATA("hvCenter.qml")));

    qApp->processEvents();
    QQuickRectangle* rect = findItem<QQuickRectangle>(view->rootObject(), QLatin1String("centered"));
    QQuickItemPrivate *rectPrivate = QQuickItemPrivate::get(rect);

    // test QTBUG-10999
    QCOMPARE(rect->x(), 10.0);
    QCOMPARE(rect->y(), 19.0);

    rectPrivate->anchors()->setHorizontalCenterOffset(-5.0);
    rectPrivate->anchors()->setVerticalCenterOffset(5.0);
    QCOMPARE(rect->x(), 10.0 - 5.0);
    QCOMPARE(rect->y(), 19.0 + 5.0);

    delete view;
}

void tst_qquickanchors::hvCenterRTL()
{
    QQuickView *view = new QQuickView(QUrl::fromLocalFile(TESTDATA("hvCenter.qml")));

    qApp->processEvents();
    QQuickRectangle* rect = findItem<QQuickRectangle>(view->rootObject(), QLatin1String("centered"));
    QQuickItemPrivate *rectPrivate = QQuickItemPrivate::get(rect);
    mirrorAnchors(rect);

    // test QTBUG-10999
    QCOMPARE(rect->x(), 10.0);
    QCOMPARE(rect->y(), 19.0);

    rectPrivate->anchors()->setHorizontalCenterOffset(-5.0);
    rectPrivate->anchors()->setVerticalCenterOffset(5.0);
    QCOMPARE(rect->x(), 10.0 + 5.0);
    QCOMPARE(rect->y(), 19.0 + 5.0);

    delete view;
}
void tst_qquickanchors::margins()
{
    QQuickView *view = new QQuickView(QUrl::fromLocalFile(TESTDATA("margins.qml")));

    qApp->processEvents();
    QQuickRectangle* rect = findItem<QQuickRectangle>(view->rootObject(), QLatin1String("filler"));
    QQuickItemPrivate *rectPrivate = QQuickItemPrivate::get(rect);
    QCOMPARE(rect->x(), 5.0);
    QCOMPARE(rect->y(), 6.0);
    QCOMPARE(rect->width(), 200.0 - 5.0 - 10.0);
    QCOMPARE(rect->height(), 200.0 - 6.0 - 10.0);

    rectPrivate->anchors()->setTopMargin(0.0);
    rectPrivate->anchors()->setMargins(20.0);

    QCOMPARE(rect->x(), 5.0);
    QCOMPARE(rect->y(), 20.0);
    QCOMPARE(rect->width(), 200.0 - 5.0 - 20.0);
    QCOMPARE(rect->height(), 200.0 - 20.0 - 20.0);

    delete view;
}

void tst_qquickanchors::marginsRTL()
{
    QQuickView *view = new QQuickView(QUrl::fromLocalFile(TESTDATA("margins.qml")));

    QQuickRectangle* rect = findItem<QQuickRectangle>(view->rootObject(), QLatin1String("filler"));
    QQuickItemPrivate *rectPrivate = QQuickItemPrivate::get(rect);
    mirrorAnchors(rect);

    QCOMPARE(rect->x(), 10.0);
    QCOMPARE(rect->y(), 6.0);
    QCOMPARE(rect->width(), 200.0 - 5.0 - 10.0);
    QCOMPARE(rect->height(), 200.0 - 6.0 - 10.0);

    rectPrivate->anchors()->setTopMargin(0.0);
    rectPrivate->anchors()->setMargins(20.0);

    QCOMPARE(rect->x(), 20.0);
    QCOMPARE(rect->y(), 20.0);
    QCOMPARE(rect->width(), 200.0 - 5.0 - 20.0);
    QCOMPARE(rect->height(), 200.0 - 20.0 - 20.0);

    delete view;
}


QTEST_MAIN(tst_qquickanchors)

#include "tst_qquickanchors.moc"
