/****************************************************************************
**
** Copyright (C) 2010 Nokia Corporation and/or its subsidiary(-ies).
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

#ifndef QQUICKCANVAS_H
#define QQUICKCANVAS_H

#include <QtCore/qmetatype.h>
#include <QtGui/qopengl.h>
#include <QtGui/qwindow.h>

QT_BEGIN_HEADER

QT_BEGIN_NAMESPACE

QT_MODULE(Declarative)

class QQuickItem;
class QSGEngine;
class QQuickCanvasPrivate;
class QOpenGLFramebufferObject;
class QDeclarativeIncubationController;

class Q_DECLARATIVE_EXPORT QQuickCanvas : public QWindow
{
Q_OBJECT
Q_DECLARE_PRIVATE(QQuickCanvas)
public:
    QQuickCanvas(QWindow *parent = 0);

    virtual ~QQuickCanvas();

    QQuickItem *rootItem() const;
    QQuickItem *activeFocusItem() const;

    QQuickItem *mouseGrabberItem() const;

    bool sendEvent(QQuickItem *, QEvent *);

    QVariant inputMethodQuery(Qt::InputMethodQuery query) const;

    QSGEngine *sceneGraphEngine() const;

    void setVSyncAnimations(bool enabled);
    bool vsyncAnimations() const;

    QImage grabFrameBuffer();

    void setRenderTarget(QOpenGLFramebufferObject *fbo);
    QOpenGLFramebufferObject *renderTarget() const;

    QDeclarativeIncubationController *incubationController() const;

Q_SIGNALS:
    void frameSwapped();
    void sceneGraphInitialized();

protected:
    QQuickCanvas(QQuickCanvasPrivate &dd, QWindow *parent = 0);

    virtual void exposeEvent(QExposeEvent *);
    virtual void resizeEvent(QResizeEvent *);

    virtual void showEvent(QShowEvent *);
    virtual void hideEvent(QHideEvent *);

    virtual void focusInEvent(QFocusEvent *);
    virtual void focusOutEvent(QFocusEvent *);

    virtual bool event(QEvent *);
    virtual void keyPressEvent(QKeyEvent *);
    virtual void keyReleaseEvent(QKeyEvent *);
    virtual void inputMethodEvent(QInputMethodEvent *);
    virtual void mousePressEvent(QMouseEvent *);
    virtual void mouseReleaseEvent(QMouseEvent *);
    virtual void mouseDoubleClickEvent(QMouseEvent *);
    virtual void mouseMoveEvent(QMouseEvent *);
#ifndef QT_NO_WHEELEVENT
    virtual void wheelEvent(QWheelEvent *);
#endif

private Q_SLOTS:
    void sceneGraphChanged();
    void maybeUpdate();
    void animationStarted();
    void animationStopped();

private:
    friend class QQuickItem;
    friend class QQuickCanvasRenderLoop;
    Q_DISABLE_COPY(QQuickCanvas)
};

QT_END_NAMESPACE

Q_DECLARE_METATYPE(QQuickCanvas *)

QT_END_HEADER

#endif // QQUICKCANVAS_H

