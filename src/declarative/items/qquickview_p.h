/****************************************************************************
**
** Copyright (C) 2011 Nokia Corporation and/or its subsidiary(-ies).
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

#ifndef QQUICKVIEW_P_H
#define QQUICKVIEW_P_H

#include "qquickview.h"

#include <QtCore/qurl.h>
#include <QtCore/qelapsedtimer.h>
#include <QtCore/qtimer.h>
#include <QtCore/qpointer.h>
#include <QtDeclarative/qdeclarativeengine.h>
#include "qquickcanvas_p.h"

#include "qquickitemchangelistener_p.h"

QT_BEGIN_HEADER

QT_BEGIN_NAMESPACE

QT_MODULE(Declarative)

class QDeclarativeContext;
class QDeclarativeError;
class QQuickItem;
class QDeclarativeComponent;

class QQuickViewPrivate : public QQuickCanvasPrivate,
                       public QQuickItemChangeListener
{
    Q_DECLARE_PUBLIC(QQuickView)
public:
    static QQuickViewPrivate* get(QQuickView *view) { return view->d_func(); }
    static const QQuickViewPrivate* get(const QQuickView *view) { return view->d_func(); }

    QQuickViewPrivate();
    ~QQuickViewPrivate();

    void execute();
    void itemGeometryChanged(QQuickItem *item, const QRectF &newGeometry, const QRectF &oldGeometry);
    void initResize();
    void updateSize();
    void setRootObject(QObject *);

    void init();

    QSize rootObjectSize() const;

    QPointer<QQuickItem> root;

    QUrl source;

    QDeclarativeEngine engine;
    QDeclarativeComponent *component;
    QBasicTimer resizetimer;

    QQuickView::ResizeMode resizeMode;
    QSize initialSize;
    QElapsedTimer frameTimer;
};

QT_END_NAMESPACE

QT_END_HEADER

#endif // QQUICKVIEW_P_H
