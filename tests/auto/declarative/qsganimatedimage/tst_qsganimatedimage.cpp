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
#include <private/qsgrectangle_p.h>
#include <private/qsgimage_p.h>
#include <private/qsganimatedimage_p.h>
#include <QSignalSpy>
#include <QtDeclarative/qdeclarativecontext.h>

#include "../shared/testhttpserver.h"
#include "../shared/util.h"

class tst_qsganimatedimage : public QObject
{
    Q_OBJECT
public:
    tst_qsganimatedimage() {}

private slots:
    void play();
    void pause();
    void stopped();
    void setFrame();
    void frameCount();
    void mirror_running();
    void mirror_notRunning();
    void mirror_notRunning_data();
    void remote();
    void remote_data();
    void sourceSize();
    void sourceSizeReadOnly();
    void invalidSource();
    void qtbug_16520();
    void progressAndStatusChanges();

};

void tst_qsganimatedimage::play()
{
    QDeclarativeEngine engine;
    QDeclarativeComponent component(&engine, QUrl::fromLocalFile(TESTDATA("stickman.qml")));
    QSGAnimatedImage *anim = qobject_cast<QSGAnimatedImage *>(component.create());
    QVERIFY(anim);
    QVERIFY(anim->isPlaying());

    delete anim;
}

void tst_qsganimatedimage::pause()
{
    QDeclarativeEngine engine;
    QDeclarativeComponent component(&engine, QUrl::fromLocalFile(TESTDATA("stickmanpause.qml")));
    QSGAnimatedImage *anim = qobject_cast<QSGAnimatedImage *>(component.create());
    QVERIFY(anim);
    QVERIFY(anim->isPlaying());
    QVERIFY(anim->isPaused());

    delete anim;
}

void tst_qsganimatedimage::stopped()
{
    QDeclarativeEngine engine;
    QDeclarativeComponent component(&engine, QUrl::fromLocalFile(TESTDATA("stickmanstopped.qml")));
    QSGAnimatedImage *anim = qobject_cast<QSGAnimatedImage *>(component.create());
    QVERIFY(anim);
    QVERIFY(!anim->isPlaying());
    QCOMPARE(anim->currentFrame(), 0);

    delete anim;
}

void tst_qsganimatedimage::setFrame()
{
    QDeclarativeEngine engine;
    QDeclarativeComponent component(&engine, QUrl::fromLocalFile(TESTDATA("stickmanpause.qml")));
    QSGAnimatedImage *anim = qobject_cast<QSGAnimatedImage *>(component.create());
    QVERIFY(anim);
    QVERIFY(anim->isPlaying());
    QCOMPARE(anim->currentFrame(), 2);

    delete anim;
}

void tst_qsganimatedimage::frameCount()
{
    QDeclarativeEngine engine;
    QDeclarativeComponent component(&engine, QUrl::fromLocalFile(TESTDATA("colors.qml")));
    QSGAnimatedImage *anim = qobject_cast<QSGAnimatedImage *>(component.create());
    QVERIFY(anim);
    QVERIFY(anim->isPlaying());
    QCOMPARE(anim->frameCount(), 3);

    delete anim;
}

void tst_qsganimatedimage::mirror_running()
{
    // test where mirror is set to true after animation has started

    QSGView *canvas = new QSGView;
    canvas->show();

    canvas->setSource(QUrl::fromLocalFile(TESTDATA("hearts.qml")));
    QSGAnimatedImage *anim = qobject_cast<QSGAnimatedImage *>(canvas->rootObject());
    QVERIFY(anim);

    int width = anim->property("width").toInt();

    QCOMPARE(anim->currentFrame(), 0);
    QPixmap frame0 = QPixmap::fromImage(canvas->grabFrameBuffer());

    anim->setCurrentFrame(1);
    QPixmap frame1 = QPixmap::fromImage(canvas->grabFrameBuffer());

    anim->setCurrentFrame(0);

    QSignalSpy spy(anim, SIGNAL(frameChanged()));
    anim->setPlaying(true);

    QTRY_VERIFY(spy.count() == 1); spy.clear();
    anim->setProperty("mirror", true);

    QCOMPARE(anim->currentFrame(), 1);
    QPixmap frame1_flipped = QPixmap::fromImage(canvas->grabFrameBuffer());

    QTRY_VERIFY(spy.count() == 1); spy.clear();
    QCOMPARE(anim->currentFrame(), 0);  // animation only has 2 frames, should cycle back to first
    QPixmap frame0_flipped = QPixmap::fromImage(canvas->grabFrameBuffer());

    QSKIP("Skip while QTBUG-19351 and QTBUG-19252 are not resolved");

    QTransform transform;
    transform.translate(width, 0).scale(-1, 1.0);
    QPixmap frame0_expected = frame0.transformed(transform);
    QPixmap frame1_expected = frame1.transformed(transform);

    QCOMPARE(frame0_flipped, frame0_expected);
    QCOMPARE(frame1_flipped, frame1_expected);

    delete canvas;
}

void tst_qsganimatedimage::mirror_notRunning()
{
    QFETCH(QUrl, fileUrl);

    QSGView *canvas = new QSGView;
    canvas->show();

    canvas->setSource(fileUrl);
    QSGAnimatedImage *anim = qobject_cast<QSGAnimatedImage *>(canvas->rootObject());
    QVERIFY(anim);

    int width = anim->property("width").toInt();
    QPixmap screenshot = QPixmap::fromImage(canvas->grabFrameBuffer());

    QTransform transform;
    transform.translate(width, 0).scale(-1, 1.0);
    QPixmap expected = screenshot.transformed(transform);

    int frame = anim->currentFrame();
    bool playing = anim->isPlaying();
    bool paused = anim->isPlaying();

    anim->setProperty("mirror", true);
    screenshot = QPixmap::fromImage(canvas->grabFrameBuffer());

    QSKIP("Skip while QTBUG-19351 and QTBUG-19252 are not resolved");
    QCOMPARE(screenshot, expected);

    // mirroring should not change the current frame or playing status
    QCOMPARE(anim->currentFrame(), frame);
    QCOMPARE(anim->isPlaying(), playing);
    QCOMPARE(anim->isPaused(), paused);

    delete canvas;
}

void tst_qsganimatedimage::mirror_notRunning_data()
{
    QTest::addColumn<QUrl>("fileUrl");

    QTest::newRow("paused") << QUrl::fromLocalFile(TESTDATA("stickmanpause.qml"));
    QTest::newRow("stopped") << QUrl::fromLocalFile(TESTDATA("stickmanstopped.qml"));
}

void tst_qsganimatedimage::remote()
{
    QFETCH(QString, fileName);
    QFETCH(bool, paused);

    TestHTTPServer server(14449);
    QVERIFY(server.isValid());
    server.serveDirectory(TESTDATA(""));

    QDeclarativeEngine engine;
    QDeclarativeComponent component(&engine, QUrl("http://127.0.0.1:14449/" + fileName));
    QTRY_VERIFY(component.isReady());

    QSGAnimatedImage *anim = qobject_cast<QSGAnimatedImage *>(component.create());
    QVERIFY(anim);

    QTRY_VERIFY(anim->isPlaying());
    if (paused) {
        QTRY_VERIFY(anim->isPaused());
        QCOMPARE(anim->currentFrame(), 2);
    }
    QVERIFY(anim->status() != QSGAnimatedImage::Error);

    delete anim;
}

void tst_qsganimatedimage::sourceSize()
{
    QDeclarativeEngine engine;
    QDeclarativeComponent component(&engine, QUrl::fromLocalFile(TESTDATA("stickmanscaled.qml")));
    QSGAnimatedImage *anim = qobject_cast<QSGAnimatedImage *>(component.create());
    QVERIFY(anim);
    QCOMPARE(anim->width(),240.0);
    QCOMPARE(anim->height(),180.0);
    QCOMPARE(anim->sourceSize(),QSize(160,120));

    delete anim;
}

void tst_qsganimatedimage::sourceSizeReadOnly()
{
    QDeclarativeEngine engine;
    QDeclarativeComponent component(&engine, QUrl::fromLocalFile(TESTDATA("stickmanerror1.qml")));
    QVERIFY(component.isError());
    QCOMPARE(component.errors().at(0).description(), QString("Invalid property assignment: \"sourceSize\" is a read-only property"));
}

void tst_qsganimatedimage::remote_data()
{
    QTest::addColumn<QString>("fileName");
    QTest::addColumn<bool>("paused");

    QTest::newRow("playing") << "stickman.qml" << false;
    QTest::newRow("paused") << "stickmanpause.qml" << true;
}

void tst_qsganimatedimage::invalidSource()
{
    QDeclarativeEngine engine;
    QDeclarativeComponent component(&engine);
    component.setData("import QtQuick 2.0\n AnimatedImage { source: \"no-such-file.gif\" }", QUrl::fromLocalFile(""));
    QVERIFY(component.isReady());

    QTest::ignoreMessage(QtWarningMsg, "file::2:2: QML AnimatedImage: Error Reading Animated Image File file:no-such-file.gif");

    QSGAnimatedImage *anim = qobject_cast<QSGAnimatedImage *>(component.create());
    QVERIFY(anim);

    QVERIFY(!anim->isPlaying());
    QVERIFY(!anim->isPaused());
    QCOMPARE(anim->currentFrame(), 0);
    QCOMPARE(anim->frameCount(), 0);
    QTRY_VERIFY(anim->status() == 3);
}

void tst_qsganimatedimage::qtbug_16520()
{
    TestHTTPServer server(14449);
    QVERIFY(server.isValid());
    server.serveDirectory(TESTDATA(""));

    QDeclarativeEngine engine;
    QDeclarativeComponent component(&engine, QUrl::fromLocalFile(TESTDATA("qtbug-16520.qml")));
    QTRY_VERIFY(component.isReady());

    QSGRectangle *root = qobject_cast<QSGRectangle *>(component.create());
    QVERIFY(root);
    QSGAnimatedImage *anim = root->findChild<QSGAnimatedImage*>("anim");

    anim->setProperty("source", "http://127.0.0.1:14449/stickman.gif");

    QTRY_VERIFY(anim->opacity() == 0);
    QTRY_VERIFY(anim->opacity() == 1);

    delete anim;
}

void tst_qsganimatedimage::progressAndStatusChanges()
{
    TestHTTPServer server(14449);
    QVERIFY(server.isValid());
    server.serveDirectory(TESTDATA(""));

    QDeclarativeEngine engine;
    QString componentStr = "import QtQuick 2.0\nAnimatedImage { source: srcImage }";
    QDeclarativeContext *ctxt = engine.rootContext();
    ctxt->setContextProperty("srcImage", QUrl::fromLocalFile(TESTDATA("stickman.gif")));
    QDeclarativeComponent component(&engine);
    component.setData(componentStr.toLatin1(), QUrl::fromLocalFile(""));
    QSGImage *obj = qobject_cast<QSGImage*>(component.create());
    QVERIFY(obj != 0);
    QVERIFY(obj->status() == QSGImage::Ready);
    QTRY_VERIFY(obj->progress() == 1.0);

    QSignalSpy sourceSpy(obj, SIGNAL(sourceChanged(const QUrl &)));
    QSignalSpy progressSpy(obj, SIGNAL(progressChanged(qreal)));
    QSignalSpy statusSpy(obj, SIGNAL(statusChanged(QSGImageBase::Status)));

    // Loading local file
    ctxt->setContextProperty("srcImage", QUrl::fromLocalFile(TESTDATA("colors.gif")));
    QTRY_VERIFY(obj->status() == QSGImage::Ready);
    QTRY_VERIFY(obj->progress() == 1.0);
    QTRY_COMPARE(sourceSpy.count(), 1);
    QTRY_COMPARE(progressSpy.count(), 0);
    QTRY_COMPARE(statusSpy.count(), 0);

    // Loading remote file
    ctxt->setContextProperty("srcImage", "http://127.0.0.1:14449/stickman.gif");
    QTRY_VERIFY(obj->status() == QSGImage::Loading);
    QTRY_VERIFY(obj->progress() == 0.0);
    QTRY_VERIFY(obj->status() == QSGImage::Ready);
    QTRY_VERIFY(obj->progress() == 1.0);
    QTRY_COMPARE(sourceSpy.count(), 2);
    QTRY_VERIFY(progressSpy.count() > 1);
    QTRY_COMPARE(statusSpy.count(), 2);

    ctxt->setContextProperty("srcImage", "");
    QTRY_VERIFY(obj->status() == QSGImage::Null);
    QTRY_VERIFY(obj->progress() == 0.0);
    QTRY_COMPARE(sourceSpy.count(), 3);
    QTRY_VERIFY(progressSpy.count() > 2);
    QTRY_COMPARE(statusSpy.count(), 3);
}

QTEST_MAIN(tst_qsganimatedimage)

#include "tst_qsganimatedimage.moc"
