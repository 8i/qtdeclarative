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

#include "qquickscreen_p.h"

#include "qquickitem.h"
#include "qquickitem_p.h"
#include "qquickcanvas.h"

#include <QScreen>

QT_BEGIN_NAMESPACE

/*!
    \qmlclass Screen QQuickScreenAttached
    \inqmlmodule QtQuick.Window 2
    \brief The Screen attached object provides information about the Screen an Item is displayed on.

    The Screen attached object is only valid inside Item or Item derived elements. Inside these elements
    it refers to the screen that the element is currently being displayed on.
*/

/*!
    \qmlattachedproperty int QtQuickWindow2::Screen::width
    \readonly

    This contains the width of the screen in pixels.
*/
/*!
    \qmlattachedproperty int QtQuickWindow2::Screen::height
    \readonly

    This contains the height of the screen in pixels.
*/
/*!
    \qmlattachedproperty Qt::ScreenOrientation QtQuickWindow2::Screen::primaryOrientation
    \readonly

    This contains the primary orientation of the screen. This can only change if the screen changes.
*/
/*!
    \qmlattachedproperty Qt::ScreenOrientation QtQuickWindow2::Screen::currentOrientation
    \readonly

    This contains the current orientation of the screen.
*/
/*!
    \qmlattachedmethod int QtQuickWindow2::Screen::angleBetween(Qt::ScreenOrientation a, Qt::ScreenOrientation b)

    Returns the rotation angle, in degrees, between the two specified angles.
*/

QQuickScreenAttached::QQuickScreenAttached(QObject* attachee)
    : QObject(attachee)
    , m_screen(0)
{
    m_attachee = qobject_cast<QQuickItem*>(attachee);

    if (m_attachee) {
        QQuickItemPrivate::get(m_attachee)->screenAttached = this;

        if (m_attachee->canvas()) //It might not be assigned to a canvas yet
            canvasChanged(m_attachee->canvas());
    }
}

int QQuickScreenAttached::width() const
{
    if (!m_screen)
        return 0;
    return m_screen->size().width();
}

int QQuickScreenAttached::height() const
{
    if (!m_screen)
        return 0;
    return m_screen->size().height();
}

Qt::ScreenOrientation QQuickScreenAttached::primaryOrientation() const
{
    if (!m_screen)
        return Qt::UnknownOrientation;
    return m_screen->primaryOrientation();
}

Qt::ScreenOrientation QQuickScreenAttached::currentOrientation() const
{
    if (!m_screen)
        return Qt::UnknownOrientation;
    return m_screen->currentOrientation();
}

int QQuickScreenAttached::angleBetween(Qt::ScreenOrientation a, Qt::ScreenOrientation b)
{
    return QScreen::angleBetween(a,b);
}

void QQuickScreenAttached::canvasChanged(QQuickCanvas* c)//Called by QQuickItemPrivate::initCanvas
{
    QScreen* screen = c ? c->screen() : 0;
    if (screen != m_screen) {
        QScreen* oldScreen = m_screen;
        m_screen = screen;

        if (oldScreen) {
            disconnect(oldScreen, SIGNAL(sizeChanged(QSize)),
                    this, SIGNAL(widthChanged()));
            disconnect(oldScreen, SIGNAL(sizeChanged(QSize)),
                    this, SIGNAL(heightChanged()));
            disconnect(oldScreen, SIGNAL(currentOrientationChanged(Qt::ScreenOrientation)),
                    this, SIGNAL(currentOrientationChanged()));
        }

        if (!screen)
            return; //Don't bother emitting signals, because the new values are garbage anyways

        if (!oldScreen || screen->size() != oldScreen->size()) {
            emit widthChanged();
            emit heightChanged();
        }
        if (!oldScreen || screen->currentOrientation() != oldScreen->currentOrientation())
            emit currentOrientationChanged();
        if (!oldScreen || screen->primaryOrientation() != oldScreen->primaryOrientation())
            emit primaryOrientationChanged();


        connect(screen, SIGNAL(sizeChanged(QSize)),
                this, SIGNAL(widthChanged()));
        connect(screen, SIGNAL(sizeChanged(QSize)),
                this, SIGNAL(heightChanged()));
        connect(screen, SIGNAL(currentOrientationChanged(Qt::ScreenOrientation)),
                this, SIGNAL(currentOrientationChanged()));
    }
}

QT_END_NAMESPACE
