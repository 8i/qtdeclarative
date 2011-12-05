/****************************************************************************
**
** Copyright (C) 2011 Nokia Corporation and/or its subsidiary(-ies).
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

#ifndef QQuickANGLEDDIRECTION_H
#define QQuickANGLEDDIRECTION_H
#include "qquickdirection_p.h"
QT_BEGIN_HEADER

QT_BEGIN_NAMESPACE

class QQuickAngleDirection : public QQuickDirection
{
    Q_OBJECT
    Q_PROPERTY(qreal angle READ angle WRITE setAngle NOTIFY angleChanged)
    Q_PROPERTY(qreal magnitude READ magnitude WRITE setMagnitude NOTIFY magnitudeChanged)
    Q_PROPERTY(qreal angleVariation READ angleVariation WRITE setAngleVariation NOTIFY angleVariationChanged)
    Q_PROPERTY(qreal magnitudeVariation READ magnitudeVariation WRITE setMagnitudeVariation NOTIFY magnitudeVariationChanged)
public:
    explicit QQuickAngleDirection(QObject *parent = 0);
    const QPointF sample(const QPointF &from);
    qreal angle() const
    {
        return m_angle;
    }

    qreal magnitude() const
    {
        return m_magnitude;
    }

    qreal angleVariation() const
    {
        return m_angleVariation;
    }

    qreal magnitudeVariation() const
    {
        return m_magnitudeVariation;
    }

signals:

    void angleChanged(qreal arg);

    void magnitudeChanged(qreal arg);

    void angleVariationChanged(qreal arg);

    void magnitudeVariationChanged(qreal arg);

public slots:
void setAngle(qreal arg)
{
    if (m_angle != arg) {
        m_angle = arg;
        emit angleChanged(arg);
    }
}

void setMagnitude(qreal arg)
{
    if (m_magnitude != arg) {
        m_magnitude = arg;
        emit magnitudeChanged(arg);
    }
}

void setAngleVariation(qreal arg)
{
    if (m_angleVariation != arg) {
        m_angleVariation = arg;
        emit angleVariationChanged(arg);
    }
}

void setMagnitudeVariation(qreal arg)
{
    if (m_magnitudeVariation != arg) {
        m_magnitudeVariation = arg;
        emit magnitudeVariationChanged(arg);
    }
}

private:
qreal m_angle;
qreal m_magnitude;
qreal m_angleVariation;
qreal m_magnitudeVariation;
};

QT_END_NAMESPACE
QT_END_HEADER
#endif // QQuickANGLEDDIRECTION_H
