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

#ifndef QDECLARATIVEACCESSIBLE_H
#define QDECLARATIVEACCESSIBLE_H

#include <QtGui/qaccessibleobject.h>
#include <QtGui/qaccessible2.h>
//#include <QtQuick1/qdeclarativeview.h>
//#include <QtQuick1/qdeclarativeitem.h>
#include <QtDeclarative/qdeclarativeproperty.h>

//#include <private/qdeclarativeaccessible_p.h>

QT_BEGIN_NAMESPACE

#ifndef QT_NO_ACCESSIBILITY

/*
    -- Declarative Accessibility Overview. --

    * Item interface classes:
    QAccessibleDeclarativeItem for QtQuick1
    QAccessibleQuickItem for for QtQuick2
    Common base class: QDeclarativeAccessible

    * View interface classes.

    These are the root of the QML accessible tree and connects it to the widget hierarchy.

    QAccessbileDeclarativeView is the root for the QGraphicsView implementation
    QAccessbileQuickView is the root for the SceneGraph implementation

*/
class QDeclarativeAccessible: public QAccessibleObject, public QAccessibleActionInterface
{
public:
    ~QDeclarativeAccessible();

    virtual QRect viewRect() const = 0;
    QAccessibleInterface *childAt(int, int) const;
    QAccessible::State state() const;

    QStringList actionNames() const;
    void doAction(const QString &actionName);
    QStringList keyBindingsForAction(const QString &actionName) const;

protected:
    virtual bool clipsChildren() const = 0;
    // For subclasses, use instantiateObject factory method outside the class.
    QDeclarativeAccessible(QObject *object);
};

#endif // QT_NO_ACCESSIBILITY

QT_END_NAMESPACE

#endif // QDECLARATIVEACCESSIBLE_H
