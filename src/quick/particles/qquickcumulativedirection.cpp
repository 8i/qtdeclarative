/****************************************************************************
**
** Copyright (C) 2012 Nokia Corporation and/or its subsidiary(-ies).
** All rights reserved.
** Contact: Nokia Corporation (qt-info@nokia.com)
**
** This file is part of the Declarative module of the Qt Toolkit.
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

#include "qquickcumulativedirection_p.h"
QT_BEGIN_NAMESPACE

/*!
    \qmlclass CumulativeDirection QQuickCumulativeDirection
    \inqmlmodule QtQuick.Particles 2
    \inherits Direction
    \brief The CumulativeDirection element allows you to specify a direction made of other directions

    The CumulativeDirection element will act as a direction that sums the directions within it.
*/
QQuickCumulativeDirection::QQuickCumulativeDirection(QObject *parent):QQuickDirection(parent)
{
}

QDeclarativeListProperty<QQuickDirection> QQuickCumulativeDirection::directions()
{
    return QDeclarativeListProperty<QQuickDirection>(this, m_directions);//TODO: Proper list property
}

const QPointF QQuickCumulativeDirection::sample(const QPointF &from)
{
    QPointF ret;
    foreach (QQuickDirection* dir, m_directions)
        ret += dir->sample(from);
    return ret;
}

QT_END_NAMESPACE
