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

#include "private/qdeclarativetranslate_p.h"
#include <private/qgraphicstransform_p.h>
#include <QDebug>
#include <QtCore/qmath.h>

QT_BEGIN_NAMESPACE



class QDeclarative1TranslatePrivate : public QGraphicsTransformPrivate
{
public:
    QDeclarative1TranslatePrivate()
        : x(0), y(0) {}
    qreal x;
    qreal y;
};

/*!
    Constructs an empty QDeclarative1Translate object with the given \a parent.
*/
QDeclarative1Translate::QDeclarative1Translate(QObject *parent)
    : QGraphicsTransform(*new QDeclarative1TranslatePrivate, parent)
{
}

/*!
    Destroys the graphics scale.
*/
QDeclarative1Translate::~QDeclarative1Translate()
{
}

/*!
    \property QDeclarative1Translate::x
    \brief the horizontal translation.

    The translation can be any real number; the default value is 0.0.

    \sa y
*/
qreal QDeclarative1Translate::x() const
{
    Q_D(const QDeclarative1Translate);
    return d->x;
}
void QDeclarative1Translate::setX(qreal x)
{
    Q_D(QDeclarative1Translate);
    if (d->x == x)
        return;
    d->x = x;
    update();
    emit xChanged();
}

/*!
    \property QDeclarative1Translate::y
    \brief the vertical translation.

    The translation can be any real number; the default value is 0.0.

    \sa x
*/
qreal QDeclarative1Translate::y() const
{
    Q_D(const QDeclarative1Translate);
    return d->y;
}
void QDeclarative1Translate::setY(qreal y)
{
    Q_D(QDeclarative1Translate);
    if (d->y == y)
        return;
    d->y = y;
    update();
    emit yChanged();
}

void QDeclarative1Translate::applyTo(QMatrix4x4 *matrix) const
{
    Q_D(const QDeclarative1Translate);
    matrix->translate(d->x, d->y, 0);
}



QT_END_NAMESPACE
