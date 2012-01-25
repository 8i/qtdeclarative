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

#include "qquickaccessibleattached_p.h"

#ifndef QT_NO_ACCESSIBILITY

#include "private/qquickitem_p.h"

QT_BEGIN_NAMESPACE

/*!
    \qmlclass Accessible QQuickAccessibleAttached
    \brief Attached property to enable accessibility of QML items.

    \inqmlmodule QtQuick 2
    \ingroup qml-basic-interaction-elements
    \ingroup accessibility

    This class is part of \l {Accessibility for Qt Quick Applications}.

    Items the user interacts with or that give information to the user
    need to expose their information in a semantic way.
    Then assistive tools can make use of that information to enable
    users to interact with the application in various ways.

    This enables Qt Quick applications to be used with screen-readers for example.

    The most important properties to set are \l name and \l role.

    \sa Accessibility
*/

/*!
    \qmlproperty string QtQuick2::Accessible::name

    This property sets an accessible name.
    For a button for example, this should have a binding to its text.
    In general this property should be set to a simple and concise
    but human readable name. Do not include the type of control
    you want to represent but just the name.
*/

/*!
    \qmlproperty string QtQuick2::Accessible::description

    This property sets an accessible description.
    Similar to the name it describes the item. The description
    can be a little more verbose and tell what the item does,
    for example the functionallity of the button it describes.
*/

/*!
    \qmlproperty enumeration QtQuick2::Accessible::role

    This flags sets the semantic type of the widget.
    A button for example would have "Button" as type.
    The value must be one of \l QAccessible::Role .
    Example:
    \qml
    Item {
        id: myButton
        Text {
            id: label
            // ...
        }
        Accessible.name: label.text
        Accessible.role: Accessible.Button
    }
    \endqml

    Some roles have special semantics.
    In order to implement check boxes for example a "checked" property is expected.

    \table
    \header
        \o \bold {Role}
        \o \bold {Expected property}
        \o

    \row
       \o CheckBox
       \o checked
       \o The check state of the check box.
    \row
       \o RadioButton
       \o checked
       \o The selected state of the radio button.
    \row
       \o Button
       \o checkable
       \o Whether the button is checkable.
    \row
       \o Button
       \o checked
       \o Whether the button is checked (only if checkable is true).

    \endtable

*/

QQuickAccessibleAttached::QQuickAccessibleAttached(QObject *parent)
    : QObject(parent)
{
    Q_ASSERT(parent);
    QQuickItem *item = qobject_cast<QQuickItem*>(parent);
    if (!item)
        return;

    // Enable accessibility for items with accessible content. This also
    // enables accessibility for the ancestors of souch items.
    item->d_func()->setAccessibleFlagAndListener();
    QAccessible::updateAccessibility(item, 0, QAccessible::ObjectCreated);
}

QQuickAccessibleAttached::~QQuickAccessibleAttached()
{
}

QQuickAccessibleAttached *QQuickAccessibleAttached::qmlAttachedProperties(QObject *obj)
{
    return new QQuickAccessibleAttached(obj);
}

QT_END_NAMESPACE

#endif
