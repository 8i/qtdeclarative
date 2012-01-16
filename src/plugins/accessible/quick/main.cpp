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


#include "qdeclarativeaccessible.h"
#include "qaccessiblequickview.h"
#include "qaccessiblequickitem.h"

#include <QtQuick/QQuickView>
#include <QtQuick/QQuickItem>
#include <QtQuick/private/qquickaccessibleattached_p.h>

#include <qaccessibleplugin.h>
#include <qvariant.h>
#include <qplugin.h>
#include <qaccessible.h>

#ifndef QT_NO_ACCESSIBILITY

QT_BEGIN_NAMESPACE

class AccessibleQuickFactory : public QAccessiblePlugin
{
public:
    AccessibleQuickFactory();

    QStringList keys() const;
    QAccessibleInterface *create(const QString &classname, QObject *object);
};

AccessibleQuickFactory::AccessibleQuickFactory()
{
}

QStringList AccessibleQuickFactory::keys() const
{
    QStringList list;
    list << QLatin1String("QQuickView");
    list << QLatin1String("QQuickItem");
    return list;
}

QAccessibleInterface *AccessibleQuickFactory::create(const QString &classname, QObject *object)
{
    if (classname == QLatin1String("QQuickView")) {
        return new QAccessibleQuickView(qobject_cast<QQuickView *>(object)); // FIXME
    } else if (classname == QLatin1String("QQuickItem")) {
        QQuickItem * item = qobject_cast<QQuickItem *>(object);
        Q_ASSERT(item);

        QVariant v = QQuickAccessibleAttached::property(item, "role");
        bool ok;
        QAccessible::Role role = (QAccessible::Role)v.toInt(&ok);
        if (!ok)    // Not sure if this check is needed.
            return new QAccessibleQuickItem(item);

        switch (role) {
        case QAccessible::Slider:
        case QAccessible::SpinBox:
        case QAccessible::Dial:
            return new QAccessibleQuickItemValueInterface(item);
        default:
            return new QAccessibleQuickItem(item);
        }
    }

    return 0;
}

Q_EXPORT_STATIC_PLUGIN(AccessibleQuickFactory)
Q_EXPORT_PLUGIN2(qtaccessiblequick, AccessibleQuickFactory)

QT_END_NAMESPACE

#endif // QT_NO_ACCESSIBILITY
