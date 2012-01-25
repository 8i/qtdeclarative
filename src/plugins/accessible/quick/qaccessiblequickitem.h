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

#ifndef QACCESSIBLEQUICKITEM_H
#define QACCESSIBLEQUICKITEM_H

#include <QtQuick/QQuickItem>
#include <QtQuick/QQuickView>
#include "qdeclarativeaccessible.h"

QT_BEGIN_NAMESPACE

#ifndef QT_NO_ACCESSIBILITY

class QAccessibleQuickItem : public QDeclarativeAccessible
{
public:
    QAccessibleQuickItem(QQuickItem *item);

    QRect rect() const;
    QRect viewRect() const;

    bool clipsChildren() const;

    QAccessibleInterface *parent() const;
    QAccessibleInterface *child(int index) const;
    int childCount() const;
    int navigate(QAccessible::RelationFlag rel, int entry, QAccessibleInterface **target) const;
    int indexOfChild(const QAccessibleInterface *iface) const;
    QList<QQuickItem *> childItems() const;

    QAccessible::State state() const;
    QAccessible::Role role() const;
    QString text(QAccessible::Text) const;

    bool isAccessible() const;

protected:
    QQuickItem *item() const { return static_cast<QQuickItem*>(object()); }
};

QRect itemScreenRect(QQuickItem *item);


class QAccessibleQuickItemValueInterface: public QAccessibleQuickItem, public QAccessibleValueInterface
{
public:
    QAccessibleQuickItemValueInterface(QQuickItem *item) : QAccessibleQuickItem(item)
    {}

    void *interface_cast(QAccessible::InterfaceType t);

    QVariant currentValue() const;
    void setCurrentValue(const QVariant &value);
    QVariant maximumValue() const;
    QVariant minimumValue() const;
};

#endif // QT_NO_ACCESSIBILITY

QT_END_NAMESPACE

#endif // QACCESSIBLEQUICKITEM_H
