/****************************************************************************
**
** Copyright (C) 2009 Nokia Corporation and/or its subsidiary(-ies).
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

#include <QtTest/QtTest>
#include <QtTest/QSignalSpy>
#include <private/qquickpincharea_p.h>
#include <QtQuick/private/qquickrectangle_p.h>
#include <QtQuick/qquickview.h>
#include <QtDeclarative/qdeclarativecontext.h>
#include "../../shared/util.h"

class tst_QQuickPinchArea: public QObject
{
    Q_OBJECT
public:
    tst_QQuickPinchArea() : device(0) { }
private slots:
    void initTestCase();
    void cleanupTestCase();
    void pinchProperties();
    void scale();
    void pan();
    void retouch();

private:
    QQuickView *createView();
    QTouchDevice *device;
};
void tst_QQuickPinchArea::initTestCase()
{
    if (!device) {
        device = new QTouchDevice;
        device->setType(QTouchDevice::TouchScreen);
        QWindowSystemInterface::registerTouchDevice(device);
    }
}

void tst_QQuickPinchArea::cleanupTestCase()
{

}
void tst_QQuickPinchArea::pinchProperties()
{
    QQuickView *canvas = createView();
    canvas->setSource(QUrl::fromLocalFile(TESTDATA("pinchproperties.qml")));
    canvas->show();
    canvas->requestActivateWindow();
    QVERIFY(canvas->rootObject() != 0);

    QQuickPinchArea *pinchArea = canvas->rootObject()->findChild<QQuickPinchArea*>("pincharea");
    QQuickPinch *pinch = pinchArea->pinch();
    QVERIFY(pinchArea != 0);
    QVERIFY(pinch != 0);

    // target
    QQuickItem *blackRect = canvas->rootObject()->findChild<QQuickItem*>("blackrect");
    QVERIFY(blackRect != 0);
    QVERIFY(blackRect == pinch->target());
    QQuickItem *rootItem = qobject_cast<QQuickItem*>(canvas->rootObject());
    QVERIFY(rootItem != 0);
    QSignalSpy targetSpy(pinch, SIGNAL(targetChanged()));
    pinch->setTarget(rootItem);
    QCOMPARE(targetSpy.count(),1);
    pinch->setTarget(rootItem);
    QCOMPARE(targetSpy.count(),1);

    // axis
    QCOMPARE(pinch->axis(), QQuickPinch::XandYAxis);
    QSignalSpy axisSpy(pinch, SIGNAL(dragAxisChanged()));
    pinch->setAxis(QQuickPinch::XAxis);
    QCOMPARE(pinch->axis(), QQuickPinch::XAxis);
    QCOMPARE(axisSpy.count(),1);
    pinch->setAxis(QQuickPinch::XAxis);
    QCOMPARE(axisSpy.count(),1);

    // minimum and maximum drag properties
    QSignalSpy xminSpy(pinch, SIGNAL(minimumXChanged()));
    QSignalSpy xmaxSpy(pinch, SIGNAL(maximumXChanged()));
    QSignalSpy yminSpy(pinch, SIGNAL(minimumYChanged()));
    QSignalSpy ymaxSpy(pinch, SIGNAL(maximumYChanged()));

    QCOMPARE(pinch->xmin(), 0.0);
    QCOMPARE(pinch->xmax(), rootItem->width()-blackRect->width());
    QCOMPARE(pinch->ymin(), 0.0);
    QCOMPARE(pinch->ymax(), rootItem->height()-blackRect->height());

    pinch->setXmin(10);
    pinch->setXmax(10);
    pinch->setYmin(10);
    pinch->setYmax(10);

    QCOMPARE(pinch->xmin(), 10.0);
    QCOMPARE(pinch->xmax(), 10.0);
    QCOMPARE(pinch->ymin(), 10.0);
    QCOMPARE(pinch->ymax(), 10.0);

    QCOMPARE(xminSpy.count(),1);
    QCOMPARE(xmaxSpy.count(),1);
    QCOMPARE(yminSpy.count(),1);
    QCOMPARE(ymaxSpy.count(),1);

    pinch->setXmin(10);
    pinch->setXmax(10);
    pinch->setYmin(10);
    pinch->setYmax(10);

    QCOMPARE(xminSpy.count(),1);
    QCOMPARE(xmaxSpy.count(),1);
    QCOMPARE(yminSpy.count(),1);
    QCOMPARE(ymaxSpy.count(),1);

    // minimum and maximum scale properties
    QSignalSpy scaleMinSpy(pinch, SIGNAL(minimumScaleChanged()));
    QSignalSpy scaleMaxSpy(pinch, SIGNAL(maximumScaleChanged()));

    QCOMPARE(pinch->minimumScale(), 1.0);
    QCOMPARE(pinch->maximumScale(), 2.0);

    pinch->setMinimumScale(0.5);
    pinch->setMaximumScale(1.5);

    QCOMPARE(pinch->minimumScale(), 0.5);
    QCOMPARE(pinch->maximumScale(), 1.5);

    QCOMPARE(scaleMinSpy.count(),1);
    QCOMPARE(scaleMaxSpy.count(),1);

    pinch->setMinimumScale(0.5);
    pinch->setMaximumScale(1.5);

    QCOMPARE(scaleMinSpy.count(),1);
    QCOMPARE(scaleMaxSpy.count(),1);

    // minimum and maximum rotation properties
    QSignalSpy rotMinSpy(pinch, SIGNAL(minimumRotationChanged()));
    QSignalSpy rotMaxSpy(pinch, SIGNAL(maximumRotationChanged()));

    QCOMPARE(pinch->minimumRotation(), 0.0);
    QCOMPARE(pinch->maximumRotation(), 90.0);

    pinch->setMinimumRotation(-90.0);
    pinch->setMaximumRotation(45.0);

    QCOMPARE(pinch->minimumRotation(), -90.0);
    QCOMPARE(pinch->maximumRotation(), 45.0);

    QCOMPARE(rotMinSpy.count(),1);
    QCOMPARE(rotMaxSpy.count(),1);

    pinch->setMinimumRotation(-90.0);
    pinch->setMaximumRotation(45.0);

    QCOMPARE(rotMinSpy.count(),1);
    QCOMPARE(rotMaxSpy.count(),1);

    delete canvas;
}

QTouchEvent::TouchPoint makeTouchPoint(int id, QPoint p, QQuickView *v, QQuickItem *i)
{
    QTouchEvent::TouchPoint touchPoint(id);
    touchPoint.setPos(i->mapFromScene(p));
    touchPoint.setScreenPos(v->mapToGlobal(p));
    touchPoint.setScenePos(p);
    return touchPoint;
}

void tst_QQuickPinchArea::scale()
{
    QQuickView *canvas = createView();
    canvas->setSource(QUrl::fromLocalFile(TESTDATA("pinchproperties.qml")));
    canvas->show();
    canvas->requestActivateWindow();
    QTest::qWaitForWindowShown(canvas);
    QVERIFY(canvas->rootObject() != 0);
    qApp->processEvents();

    QQuickPinchArea *pinchArea = canvas->rootObject()->findChild<QQuickPinchArea*>("pincharea");
    QQuickPinch *pinch = pinchArea->pinch();
    QVERIFY(pinchArea != 0);
    QVERIFY(pinch != 0);

    QQuickItem *root = qobject_cast<QQuickItem*>(canvas->rootObject());
    QVERIFY(root != 0);

    // target
    QQuickItem *blackRect = canvas->rootObject()->findChild<QQuickItem*>("blackrect");
    QVERIFY(blackRect != 0);

    QPoint p1(80, 80);
    QPoint p2(100, 100);

    QTest::touchEvent(canvas, device).press(0, p1, canvas);
    QTest::touchEvent(canvas, device).stationary(0).press(1, p2, canvas);
    p1 -= QPoint(10,10);
    p2 += QPoint(10,10);
    QTest::touchEvent(canvas, device).move(0, p1,canvas).move(1, p2,canvas);

    QCOMPARE(root->property("scale").toReal(), 1.0);

    p1 -= QPoint(10,10);
    p2 += QPoint(10,10);
    QTest::touchEvent(canvas, device).move(0, p1,canvas).move(1, p2,canvas);

    QCOMPARE(root->property("scale").toReal(), 1.5);
    QCOMPARE(root->property("center").toPointF(), QPointF(40, 40)); // blackrect is at 50,50
    QCOMPARE(blackRect->scale(), 1.5);

    // scale beyond bound
    p1 -= QPoint(50,50);
    p2 += QPoint(50,50);
    QTest::touchEvent(canvas, device).move(0, p1, canvas).move(1, p2, canvas);

    QCOMPARE(blackRect->scale(), 2.0);

    QTest::touchEvent(canvas, device).release(0, p1, canvas).release(1, p2, canvas);

    delete canvas;
}

void tst_QQuickPinchArea::pan()
{
    QQuickView *canvas = createView();
    canvas->setSource(QUrl::fromLocalFile(TESTDATA("pinchproperties.qml")));
    canvas->show();
    canvas->requestActivateWindow();
    QTest::qWaitForWindowShown(canvas);
    QVERIFY(canvas->rootObject() != 0);
    qApp->processEvents();

    QQuickPinchArea *pinchArea = canvas->rootObject()->findChild<QQuickPinchArea*>("pincharea");
    QQuickPinch *pinch = pinchArea->pinch();
    QVERIFY(pinchArea != 0);
    QVERIFY(pinch != 0);

    QQuickItem *root = qobject_cast<QQuickItem*>(canvas->rootObject());
    QVERIFY(root != 0);

    // target
    QQuickItem *blackRect = canvas->rootObject()->findChild<QQuickItem*>("blackrect");
    QVERIFY(blackRect != 0);

    QPoint p1(80, 80);
    QPoint p2(100, 100);

    QTest::touchEvent(canvas, device).press(0, p1, canvas);
    QTest::touchEvent(canvas, device).stationary(0).press(1, p2, canvas);
    p1 += QPoint(10,10);
    p2 += QPoint(10,10);
    QTest::touchEvent(canvas, device).move(0, p1, canvas).move(1, p2, canvas);

    QCOMPARE(root->property("scale").toReal(), 1.0);

    p1 += QPoint(10,10);
    p2 += QPoint(10,10);
    QTest::touchEvent(canvas, device).move(0, p1, canvas).move(1, p2, canvas);

    QCOMPARE(root->property("center").toPointF(), QPointF(60, 60)); // blackrect is at 50,50

    QCOMPARE(blackRect->x(), 60.0);
    QCOMPARE(blackRect->y(), 60.0);

    // pan x beyond bound
    p1 += QPoint(100,100);
    p2 += QPoint(100,100);
    QTest::touchEvent(canvas, device).move(0, p1, canvas).move(1, p2, canvas);

    QCOMPARE(blackRect->x(), 140.0);
    QCOMPARE(blackRect->y(), 160.0);

    QTest::touchEvent(canvas, device).release(0, p1, canvas).release(1, p2, canvas);

    delete canvas;
}

// test pinch, release one point, touch again to continue pinch
void tst_QQuickPinchArea::retouch()
{
    QQuickView *canvas = createView();
    canvas->setSource(QUrl::fromLocalFile(TESTDATA("pinchproperties.qml")));
    canvas->show();
    canvas->requestActivateWindow();
    QTest::qWaitForWindowShown(canvas);
    QVERIFY(canvas->rootObject() != 0);
    qApp->processEvents();

    QQuickPinchArea *pinchArea = canvas->rootObject()->findChild<QQuickPinchArea*>("pincharea");
    QQuickPinch *pinch = pinchArea->pinch();
    QVERIFY(pinchArea != 0);
    QVERIFY(pinch != 0);

    QQuickItem *root = qobject_cast<QQuickItem*>(canvas->rootObject());
    QVERIFY(root != 0);

    QSignalSpy startedSpy(pinchArea, SIGNAL(pinchStarted(QQuickPinchEvent *)));
    QSignalSpy finishedSpy(pinchArea, SIGNAL(pinchFinished(QQuickPinchEvent *)));

    // target
    QQuickItem *blackRect = canvas->rootObject()->findChild<QQuickItem*>("blackrect");
    QVERIFY(blackRect != 0);

    QPoint p1(80, 80);
    QPoint p2(100, 100);

    QTest::touchEvent(canvas, device).press(0, p1, canvas);
    QTest::touchEvent(canvas, device).stationary(0).press(1, p2, canvas);
    p1 -= QPoint(10,10);
    p2 += QPoint(10,10);
    QTest::touchEvent(canvas, device).move(0, p1, canvas).move(1, p2, canvas);

    QCOMPARE(root->property("scale").toReal(), 1.0);

    p1 -= QPoint(10,10);
    p2 += QPoint(10,10);
    QTest::touchEvent(canvas, device).move(0, p1, canvas).move(1, p2, canvas);

    QCOMPARE(startedSpy.count(), 1);

    QCOMPARE(root->property("scale").toReal(), 1.5);
    QCOMPARE(root->property("center").toPointF(), QPointF(40, 40)); // blackrect is at 50,50
    QCOMPARE(blackRect->scale(), 1.5);

    QCOMPARE(canvas->rootObject()->property("pointCount").toInt(), 2);

    QCOMPARE(startedSpy.count(), 1);
    QCOMPARE(finishedSpy.count(), 0);

    QTest::touchEvent(canvas, device).stationary(0).release(1, p2, canvas);

    QCOMPARE(startedSpy.count(), 1);
    QCOMPARE(finishedSpy.count(), 0);

    QCOMPARE(canvas->rootObject()->property("pointCount").toInt(), 1);

    QTest::touchEvent(canvas, device).stationary(0).press(1, p2, canvas);
    p1 -= QPoint(10,10);
    p2 += QPoint(10,10);
    QTest::touchEvent(canvas, device).move(0, p1, canvas).move(1, p2, canvas);

    // Lifting and retouching results in onPinchStarted being called again
    QCOMPARE(startedSpy.count(), 2);
    QCOMPARE(finishedSpy.count(), 0);

    QCOMPARE(canvas->rootObject()->property("pointCount").toInt(), 2);

    QTest::touchEvent(canvas, device).release(0, p1, canvas).release(1, p2, canvas);

    QCOMPARE(startedSpy.count(), 2);
    QCOMPARE(finishedSpy.count(), 1);

    delete canvas;
}


QQuickView *tst_QQuickPinchArea::createView()
{
    QQuickView *canvas = new QQuickView(0);
    canvas->setGeometry(0,0,240,320);

    return canvas;
}

QTEST_MAIN(tst_QQuickPinchArea)

#include "tst_qquickpincharea.moc"
