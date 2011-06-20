/****************************************************************************
**
** Copyright (C) 2011 Nokia Corporation and/or its subsidiary(-ies).
** All rights reserved.
** Contact: Nokia Corporation (qt-info@nokia.com)
**
** This file is part of the Declarative module of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:LGPL$
** No Commercial Usage
** This file contains pre-release code and may not be distributed.
** You may use this file in accordance with the terms and conditions
** contained in the Technology Preview License Agreement accompanying
** this package.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU Lesser General Public License version 2.1 requirements
** will be met: http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Nokia gives you certain additional
** rights.  These rights are described in the Nokia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
** If you have questions regarding the use of this file, please contact
** Nokia at qt-info@nokia.com.
**
**
**
**
**
**
**
**
** $QT_END_LICENSE$
**
****************************************************************************/

#ifndef ATTRACTORAFFECTOR_H
#define ATTRACTORAFFECTOR_H
#include "qsgparticleaffector_p.h"

QT_BEGIN_HEADER

QT_BEGIN_NAMESPACE

QT_MODULE(Declarative)

class QSGPointAttractorAffector : public QSGParticleAffector
{
    Q_OBJECT
    //Like Gravitational singularity, but linear to distance instead of quadratic
    //And affects ds/dt, not da/dt
    Q_PROPERTY(qreal strength READ strength WRITE setStrength NOTIFY strengthChanged)
    Q_PROPERTY(qreal x READ x WRITE setX NOTIFY xChanged)
    Q_PROPERTY(qreal y READ y WRITE setY NOTIFY yChanged)
    Q_PROPERTY(PhysicsAffects physics READ physics WRITE setPhysics NOTIFY physicsChanged)
    Q_PROPERTY(Proportion proportionalToDistance READ proportionalToDistance WRITE setProportionalToDistance NOTIFY proportionalToDistanceChanged)
    Q_ENUMS(PhysicsAffects)
    Q_ENUMS(Proportion)

public:
    enum Proportion{
        Linear,
        Quadratic
    };

    enum PhysicsAffects {
        Position,
        Velocity,
        Acceleration
    };

    explicit QSGPointAttractorAffector(QSGItem *parent = 0);

    qreal strength() const
    {
        return m_strength;
    }

    qreal x() const
    {
        return m_x;
    }

    qreal y() const
    {
        return m_y;
    }

    PhysicsAffects physics() const
    {
        return m_physics;
    }

    Proportion proportionalToDistance() const
    {
        return m_proportionalToDistance;
    }

signals:

    void strengthChanged(qreal arg);

    void xChanged(qreal arg);

    void yChanged(qreal arg);

    void physicsChanged(PhysicsAffects arg);

    void proportionalToDistanceChanged(Proportion arg);

public slots:
void setStrength(qreal arg)
{
    if (m_strength != arg) {
        m_strength = arg;
        emit strengthChanged(arg);
    }
}

void setX(qreal arg)
{
    if (m_x != arg) {
        m_x = arg;
        emit xChanged(arg);
    }
}

void setY(qreal arg)
{
    if (m_y != arg) {
        m_y = arg;
        emit yChanged(arg);
    }
}
void setPhysics(PhysicsAffects arg)
{
    if (m_physics != arg) {
        m_physics = arg;
        emit physicsChanged(arg);
    }
}

void setProportionalToDistance(Proportion arg)
{
    if (m_proportionalToDistance != arg) {
        m_proportionalToDistance = arg;
        emit proportionalToDistanceChanged(arg);
    }
}

protected:
    virtual bool affectParticle(QSGParticleData *d, qreal dt);
private:
qreal m_strength;
qreal m_x;
qreal m_y;
PhysicsAffects m_physics;
Proportion m_proportionalToDistance;
};

QT_END_NAMESPACE
QT_END_HEADER
#endif // ATTRACTORAFFECTOR_H
