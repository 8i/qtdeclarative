/****************************************************************************
**
** Copyright (C) 2012 Nokia Corporation and/or its subsidiary(-ies).
** All rights reserved.
** Contact: Nokia Corporation (qt-info@nokia.com)
**
** This file is part of the QtDeclarative module of the Qt Toolkit.
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

#include "QtQuick1/private/qdeclarativeanimatedimage_p.h"
#include "QtQuick1/private/qdeclarativeanimatedimage_p_p.h"

#ifndef QT_NO_MOVIE

#include <QtDeclarative/qdeclarativeinfo.h>
#include <QtDeclarative/private/qdeclarativeengine_p.h>

#include <QMovie>
#include <QNetworkRequest>
#include <QNetworkReply>

QT_BEGIN_NAMESPACE



/*!
    \qmlclass AnimatedImage QDeclarative1AnimatedImage
    \inqmlmodule QtQuick 1
    \inherits Image
    \since QtQuick 1.0
    \ingroup basic-visual-elements

    The AnimatedImage element extends the features of the \l Image element, providing
    a way to play animations stored as images containing a series of frames,
    such as those stored in GIF files.

    Information about the current frame and totla length of the animation can be
    obtained using the \l currentFrame and \l frameCount properties. You can
    start, pause and stop the animation by changing the values of the \l playing
    and \l paused properties.

    The full list of supported formats can be determined with QMovie::supportedFormats().

    \section1 Example Usage

    \beginfloatleft
    \image animatedimageitem.gif
    \endfloat

    The following QML shows how to display an animated image and obtain information
    about its state, such as the current frame and total number of frames.
    The result is an animated image with a simple progress indicator underneath it.

    \bold Note: Unlike images, animated images are not cached or shared internally.

    \clearfloat
    \snippet doc/src/snippets/qtquick1/animatedimage.qml document

    \sa BorderImage, Image
*/

/*!
    \qmlproperty url QtQuick1::AnimatedImage::source

    This property holds the URL that refers to the source image.

    AnimatedImage can handle any image format supported by Qt, loaded from any
    URL scheme supported by Qt.

    \sa QDeclarativeImageProvider
*/

/*!
    \qmlproperty bool QtQuick1::AnimatedImage::asynchronous

    Specifies that images on the local filesystem should be loaded
    asynchronously in a separate thread.  The default value is
    false, causing the user interface thread to block while the
    image is loaded.  Setting \a asynchronous to true is useful where
    maintaining a responsive user interface is more desirable
    than having images immediately visible.

    Note that this property is only valid for images read from the
    local filesystem.  Images loaded via a network resource (e.g. HTTP)
    are always loaded asynchonously.
*/

/*!
    \qmlproperty bool QtQuick1::AnimatedImage::mirror
    \since Quick 1.1

    This property holds whether the image should be horizontally inverted
    (effectively displaying a mirrored image).

    The default value is false.
*/

QDeclarative1AnimatedImage::QDeclarative1AnimatedImage(QDeclarativeItem *parent)
    : QDeclarative1Image(*(new QDeclarative1AnimatedImagePrivate), parent)
{
}

QDeclarative1AnimatedImage::~QDeclarative1AnimatedImage()
{
    Q_D(QDeclarative1AnimatedImage);
    delete d->_movie;
}

/*!
  \qmlproperty bool QtQuick1::AnimatedImage::paused
  This property holds whether the animated image is paused.

  By default, this property is false. Set it to true when you want to pause
  the animation.
*/
bool QDeclarative1AnimatedImage::isPaused() const
{
    Q_D(const QDeclarative1AnimatedImage);
    if(!d->_movie)
        return false;
    return d->_movie->state()==QMovie::Paused;
}

void QDeclarative1AnimatedImage::setPaused(bool pause)
{
    Q_D(QDeclarative1AnimatedImage);
    if(pause == d->paused)
        return;
    d->paused = pause;
    if(!d->_movie)
        return;
    d->_movie->setPaused(pause);
}
/*!
  \qmlproperty bool QtQuick1::AnimatedImage::playing
  This property holds whether the animated image is playing.

  By default, this property is true, meaning that the animation
  will start playing immediately.
*/
bool QDeclarative1AnimatedImage::isPlaying() const
{
    Q_D(const QDeclarative1AnimatedImage);
    if (!d->_movie)
        return false;
    return d->_movie->state()!=QMovie::NotRunning;
}

void QDeclarative1AnimatedImage::setPlaying(bool play)
{
    Q_D(QDeclarative1AnimatedImage);
    if(play == d->playing)
        return;
    d->playing = play;
    if (!d->_movie)
        return;
    if (play)
        d->_movie->start();
    else
        d->_movie->stop();
}

/*!
  \qmlproperty int QtQuick1::AnimatedImage::currentFrame
  \qmlproperty int QtQuick1::AnimatedImage::frameCount

  currentFrame is the frame that is currently visible. By monitoring this property
  for changes, you can animate other items at the same time as the image.

  frameCount is the number of frames in the animation. For some animation formats,
  frameCount is unknown and has a value of zero.
*/
int QDeclarative1AnimatedImage::currentFrame() const
{
    Q_D(const QDeclarative1AnimatedImage);
    if (!d->_movie)
        return d->preset_currentframe;
    return d->_movie->currentFrameNumber();
}

void QDeclarative1AnimatedImage::setCurrentFrame(int frame)
{
    Q_D(QDeclarative1AnimatedImage);
    if (!d->_movie) {
        d->preset_currentframe = frame;
        return;
    }
    d->_movie->jumpToFrame(frame);
}

int QDeclarative1AnimatedImage::frameCount() const
{
    Q_D(const QDeclarative1AnimatedImage);
    if (!d->_movie)
        return 0;
    return d->_movie->frameCount();
}

void QDeclarative1AnimatedImage::setSource(const QUrl &url)
{
    Q_D(QDeclarative1AnimatedImage);
    if (url == d->url)
        return;

    delete d->_movie;
    d->_movie = 0;

    if (d->reply) {
        d->reply->deleteLater();
        d->reply = 0;
    }

    d->url = url;
    emit sourceChanged(d->url);

    if (isComponentComplete())
        load();
}

void QDeclarative1AnimatedImage::load()
{
    Q_D(QDeclarative1AnimatedImage);

    QDeclarative1ImageBase::Status oldStatus = d->status;
    qreal oldProgress = d->progress;

    if (d->url.isEmpty()) {
        delete d->_movie;
        d->setPixmap(QPixmap());
        d->progress = 0;
        d->status = Null;
        if (d->status != oldStatus)
            emit statusChanged(d->status);
        if (d->progress != oldProgress)
            emit progressChanged(d->progress);
    } else {
#ifndef QT_NO_LOCALFILE_OPTIMIZED_QML
        QString lf = QDeclarativeEnginePrivate::urlToLocalFileOrQrc(d->url);
        if (!lf.isEmpty()) {
            //### should be unified with movieRequestFinished
            d->_movie = new QMovie(lf);
            if (!d->_movie->isValid()){
                qmlInfo(this) << "Error Reading Animated Image File " << d->url.toString();
                delete d->_movie;
                d->_movie = 0;
                d->status = Error;
                if (d->status != oldStatus)
                    emit statusChanged(d->status);
                return;
            }
            connect(d->_movie, SIGNAL(stateChanged(QMovie::MovieState)),
                    this, SLOT(playingStatusChanged()));
            connect(d->_movie, SIGNAL(frameChanged(int)),
                    this, SLOT(movieUpdate()));
            d->_movie->setCacheMode(QMovie::CacheAll);
            if(d->playing)
                d->_movie->start();
            else
                d->_movie->jumpToFrame(0);
            if(d->paused)
                d->_movie->setPaused(true);
            d->setPixmap(d->_movie->currentPixmap());
            d->status = Ready;
            d->progress = 1.0;
            if (d->status != oldStatus)
                emit statusChanged(d->status);
            if (d->progress != oldProgress)
                emit progressChanged(d->progress);
            return;
        }
#endif
        d->status = Loading;
        d->progress = 0;
        emit statusChanged(d->status);
        emit progressChanged(d->progress);
        QNetworkRequest req(d->url);
        req.setAttribute(QNetworkRequest::HttpPipeliningAllowedAttribute, true);
        d->reply = qmlEngine(this)->networkAccessManager()->get(req);
        QObject::connect(d->reply, SIGNAL(finished()),
                         this, SLOT(movieRequestFinished()));
        QObject::connect(d->reply, SIGNAL(downloadProgress(qint64,qint64)),
                         this, SLOT(requestProgress(qint64,qint64)));
    }
}

#define ANIMATEDIMAGE_MAXIMUM_REDIRECT_RECURSION 16

void QDeclarative1AnimatedImage::movieRequestFinished()
{
    Q_D(QDeclarative1AnimatedImage);

    d->redirectCount++;
    if (d->redirectCount < ANIMATEDIMAGE_MAXIMUM_REDIRECT_RECURSION) {
        QVariant redirect = d->reply->attribute(QNetworkRequest::RedirectionTargetAttribute);
        if (redirect.isValid()) {
            QUrl url = d->reply->url().resolved(redirect.toUrl());
            d->reply->deleteLater();
            d->reply = 0;
            setSource(url);
            return;
        }
    }
    d->redirectCount=0;

    d->_movie = new QMovie(d->reply);
    if (!d->_movie->isValid()){
#ifndef QT_NO_DEBUG_STREAM
        qmlInfo(this) << "Error Reading Animated Image File " << d->url;
#endif
        delete d->_movie;
        d->_movie = 0;
        d->status = Error;
        emit statusChanged(d->status);
        return;
    }
    connect(d->_movie, SIGNAL(stateChanged(QMovie::MovieState)),
            this, SLOT(playingStatusChanged()));
    connect(d->_movie, SIGNAL(frameChanged(int)),
            this, SLOT(movieUpdate()));
    d->_movie->setCacheMode(QMovie::CacheAll);
    if(d->playing)
        d->_movie->start();
    if (d->paused || !d->playing) {
        d->_movie->jumpToFrame(d->preset_currentframe);
        d->preset_currentframe = 0;
    }
    if(d->paused)
        d->_movie->setPaused(true);
    d->setPixmap(d->_movie->currentPixmap());
    d->status = Ready;
    emit statusChanged(d->status);
}

void QDeclarative1AnimatedImage::movieUpdate()
{
    Q_D(QDeclarative1AnimatedImage);
    d->setPixmap(d->_movie->currentPixmap());
    emit frameChanged();
}

void QDeclarative1AnimatedImage::playingStatusChanged()
{
    Q_D(QDeclarative1AnimatedImage);
    if((d->_movie->state() != QMovie::NotRunning) != d->playing){
        d->playing = (d->_movie->state() != QMovie::NotRunning);
        emit playingChanged();
    }
    if((d->_movie->state() == QMovie::Paused) != d->paused){
        d->playing = (d->_movie->state() == QMovie::Paused);
        emit pausedChanged();
    }
}

void QDeclarative1AnimatedImage::componentComplete()
{
    Q_D(QDeclarative1AnimatedImage);
    QDeclarativeItem::componentComplete(); // NOT QDeclarative1Image
    if (d->url.isValid())
        load();
    if (!d->reply) {
        setCurrentFrame(d->preset_currentframe);
        d->preset_currentframe = 0;
    }
}



QT_END_NAMESPACE

#endif // QT_NO_MOVIE
