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

#include "qsgwander_p.h"
#include "qsgparticlesystem_p.h"//for ParticlesVertices
QT_BEGIN_NAMESPACE

QSGWanderAffector::QSGWanderAffector(QSGItem *parent) :
    QSGParticleAffector(parent), m_xVariance(0), m_yVariance(0), m_pace(0)
    , m_physics(Velocity)
{
    m_needsReset = true;
}

QSGWanderAffector::~QSGWanderAffector()
{
    for(QHash<int, WanderData*>::const_iterator iter=m_wanderData.constBegin();
        iter != m_wanderData.constEnd(); iter++)
        delete (*iter);
}

WanderData* QSGWanderAffector::getData(int idx)
{
    if(m_wanderData.contains(idx))
        return m_wanderData[idx];
    WanderData* d = new WanderData;
    d->x_vel = 0;
    d->y_vel = 0;
    d->x_peak = m_xVariance;
    d->y_peak = m_yVariance;
    d->x_var = m_pace * qreal(qrand()) / RAND_MAX;
    d->y_var = m_pace * qreal(qrand()) / RAND_MAX;

    m_wanderData.insert(idx, d);
    return d;
}

void QSGWanderAffector::reset(int systemIdx)
{
    if(m_wanderData.contains(systemIdx))
        delete m_wanderData[systemIdx];
    m_wanderData.remove(systemIdx);
}

bool QSGWanderAffector::affectParticle(QSGParticleData* data, qreal dt)
{
    /*TODO: Add a mode which does basically this - picking a direction, going in it (random speed) and then going back
    WanderData* d = getData(data->systemIndex);
    if (m_xVariance != 0.) {
        if ((d->x_vel > d->x_peak && d->x_var > 0.0) || (d->x_vel < -d->x_peak && d->x_var < 0.0)) {
            d->x_var = -d->x_var;
            d->x_peak = m_xVariance + m_xVariance * qreal(qrand()) / RAND_MAX;
        }
        d->x_vel += d->x_var * dt;
    }
    qreal dx = dt * d->x_vel;

    if (m_yVariance != 0.) {
        if ((d->y_vel > d->y_peak && d->y_var > 0.0) || (d->y_vel < -d->y_peak && d->y_var < 0.0)) {
            d->y_var = -d->y_var;
            d->y_peak = m_yVariance + m_yVariance * qreal(qrand()) / RAND_MAX;
        }
        d->y_vel += d->y_var * dt;
    }
    qreal dy = dt * d->x_vel;

    //### Should we be amending vel instead?
    ParticleVertex* p = &(data->pv);
    p->x += dx;

    p->y += dy;
    return true;
    */
    qreal dx = dt * m_pace * (2 * qreal(qrand())/RAND_MAX - 1);
    qreal dy = dt * m_pace * (2 * qreal(qrand())/RAND_MAX - 1);
    qreal newX, newY;
    switch(m_physics){
    case Position:
        newX = data->curX() + dx;
        if(m_xVariance > qAbs(newX) )
            data->x += dx;
        newY = data->curY() + dy;
        if(m_yVariance > qAbs(newY) )
            data->y += dy;
        break;
    default:
    case Velocity:
        newX = data->curSX() + dx;
        if(m_xVariance > qAbs(newX) )
            data->setInstantaneousSX(newX);
        newY = data->curSY() + dy;
        if(m_yVariance > qAbs(newY) )
            data->setInstantaneousSY(newY);
        break;
    case Acceleration:
        newX = data->ax + dx;
        if(m_xVariance > qAbs(newX) )
            data->setInstantaneousAX(newX);
        newY = data->ay + dy;
        if(m_yVariance > qAbs(newY) )
            data->setInstantaneousAY(newY);
        break;
    }
    return true;
}
QT_END_NAMESPACE
