/****************************************************************************
**
** Copyright (C) 2012 Nokia Corporation and/or its subsidiary(-ies).
** All rights reserved.
** Contact: http://www.qt-project.org/
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

#include "qsgengine.h"

#include <QtQuick/qquickcanvas.h>

#include <private/qobject_p.h>
#include <QtGui/QColor>

QT_BEGIN_NAMESPACE

class QSGEnginePrivate : public QObjectPrivate
{
public:
    QSGEnginePrivate()
        : canvas(0)
    {
    }

    QQuickCanvas *canvas;
};

/*!
    \class QSGEngine
    \deprecated
 */

QSGEngine::QSGEngine(QObject *parent) :
    QObject(*(new QSGEnginePrivate), parent)
{
}


QSGEngine::~QSGEngine()
{
}


void QSGEngine::setCanvas(QQuickCanvas *canvas)
{
    d_func()->canvas = canvas;
    connect(canvas, SIGNAL(afterRendering()), this, SIGNAL(afterRendering()), Qt::DirectConnection);
    connect(canvas, SIGNAL(beforeRendering()), this, SIGNAL(beforeRendering()), Qt::DirectConnection);
}

void QSGEngine::setClearBeforeRendering(bool enabled)
{
    d_func()->canvas->setClearBeforeRendering(enabled);
}

bool QSGEngine::clearBeforeRendering() const
{
    return d_func()->canvas->clearBeforeRendering();
}

QSGTexture *QSGEngine::createTextureFromImage(const QImage &image) const
{
    return d_func()->canvas->createTextureFromImage(image);
}

QSGTexture *QSGEngine::createTextureFromId(uint id, const QSize &size, TextureOptions options) const
{
    return d_func()->canvas->createTextureFromId(id, size, QQuickCanvas::CreateTextureOptions((int) options));
}

void QSGEngine::setClearColor(const QColor &color)
{
    d_func()->canvas->setClearColor(color);
}

QColor QSGEngine::clearColor() const
{
    return d_func()->canvas->clearColor();
}

QT_END_NAMESPACE
