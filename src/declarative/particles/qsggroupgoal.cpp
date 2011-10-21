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

#include "qsggroupgoal_p.h"
#include <private/qsgspriteengine_p.h>
#include <private/qsgsprite_p.h>
#include "qsgimageparticle_p.h"
#include <QDebug>

QT_BEGIN_NAMESPACE

/*!
    \qmlclass GroupGoal QSGGroupGoalAffector
    \inqmlmodule QtQuick.Particles 2
    \inherits Affector
    \brief The GroupGoal Affector allows you to change the state of a group of a particle.

*/
/*!
    \qmlproperty string QtQuick.Particles2::GroupGoal::goalState

    The name of the group which the affected particles should move to.

    Groups can have defined durations and transitions between them, setting goalState
    will cause it to disregard any path weightings (including 0) and head down the path
    which will reach the goalState quickest. It will pass through intermediate groups
    on that path for their respective durations.
*/
/*!
    \qmlproperty bool QtQuick.Particles2::GroupGoal::jump

    If true, affected particles will jump directly to the target group instead of taking the
    the shortest valid path to get there. They will also not finish their current state,
    but immediately move to the beginning of the goal state.

    Default is false.
*/

QSGGroupGoalAffector::QSGGroupGoalAffector(QSGItem *parent) :
    QSGParticleAffector(parent), m_jump(false)
{
}

void QSGGroupGoalAffector::setGoalState(QString arg)
{
    if (m_goalState != arg) {
        m_goalState = arg;
        emit goalStateChanged(arg);
    }
}

bool QSGGroupGoalAffector::affectParticle(QSGParticleData *d, qreal dt)
{
    Q_UNUSED(dt);
    QSGStochasticEngine *engine = m_system->stateEngine;
    bool notUsingEngine = false;
    if (!engine)
        notUsingEngine = true;

    int index = d->systemIndex;
    int goalIdx = m_system->groupIds[m_goalState];
    if (notUsingEngine){//no stochastic states defined. So cut out the engine
        //TODO: It's possible to move to a group that is intermediate and not used by painters or emitters - but right now that will redirect to the default group
        m_system->moveGroups(d, goalIdx);
        return true;
    }else if (engine->curState(index) != goalIdx){
        engine->setGoal(goalIdx, index, m_jump);
        return true;
    }
    return false;
}

QT_END_NAMESPACE
