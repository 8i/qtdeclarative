/****************************************************************************
**
** Copyright (C) 2011 Nokia Corporation and/or its subsidiary(-ies).
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

#include "private/qparallelanimationgroup2_p.h"

QT_BEGIN_NAMESPACE

QParallelAnimationGroup2::QParallelAnimationGroup2()
    : QAnimationGroup2()
    , m_previousLoop(0)
    , m_previousCurrentTime(0)
{
}

QParallelAnimationGroup2::~QParallelAnimationGroup2()
{
}

int QParallelAnimationGroup2::duration() const
{
    int ret = 0;

    for (QAbstractAnimation2 *animation = firstChild(); animation; animation = animation->nextSibling()) {
        int currentDuration = animation->totalDuration();
        //this takes care of the case where a parallel animation group has controlled and uncontrolled
        //animations, and the uncontrolled stop before the controlled
        if (currentDuration == -1)
            currentDuration = uncontrolledAnimationFinishTime(animation);
        if (currentDuration == -1)
            return -1; // Undetermined length

        ret = qMax(ret, currentDuration);
    }

    return ret;
}

void QParallelAnimationGroup2::updateCurrentTime(int /*currentTime*/)
{
    if (!firstChild())
        return;

    if (m_currentLoop > m_previousLoop) {
        // simulate completion of the loop
        int dura = duration();
        if (dura > 0) {
            for (QAbstractAnimation2 *animation = firstChild(); animation; animation = animation->nextSibling()) {
                if (!animation->isStopped())
                    animation->setCurrentTime(dura);   // will stop
            }
        }
    } else if (m_currentLoop < m_previousLoop) {
        // simulate completion of the loop seeking backwards
        for (QAbstractAnimation2 *animation = firstChild(); animation; animation = animation->nextSibling()) {
            //we need to make sure the animation is in the right state
            //and then rewind it
            applyGroupState(animation);
            animation->setCurrentTime(0);
            animation->stop();
        }
    }

    // finally move into the actual time of the current loop
    for (QAbstractAnimation2 *animation = firstChild(); animation; animation = animation->nextSibling()) {
        const int dura = animation->totalDuration();
        //if the loopcount is bigger we should always start all animations
        if (m_currentLoop > m_previousLoop
            //if we're at the end of the animation, we need to start it if it wasn't already started in this loop
            //this happens in Backward direction where not all animations are started at the same time
            || shouldAnimationStart(animation, m_previousCurrentTime > dura /*startIfAtEnd*/)) {
            applyGroupState(animation);
        }

        if (animation->state() == state()) {
            animation->setCurrentTime(m_currentTime);
            if (dura > 0 && m_currentTime > dura)
                animation->stop();
        }
    }
    m_previousLoop = m_currentLoop;
    m_previousCurrentTime = m_currentTime;
}

void QParallelAnimationGroup2::updateState(QAbstractAnimation2::State newState,
                                          QAbstractAnimation2::State oldState)
{
    QAnimationGroup2::updateState(newState, oldState);

    switch (newState) {
    case Stopped:
        for (QAbstractAnimation2 *animation = firstChild(); animation; animation = animation->nextSibling())
            animation->stop();
        break;
    case Paused:
        for (QAbstractAnimation2 *animation = firstChild(); animation; animation = animation->nextSibling())
            if (animation->isRunning())
                animation->pause();
        break;
    case Running:
        resetUncontrolledAnimationsFinishTime();
        for (QAbstractAnimation2 *animation = firstChild(); animation; animation = animation->nextSibling()) {
            if (oldState == Stopped)
                animation->stop();
            animation->setDirection(m_direction);
            if (shouldAnimationStart(animation, oldState == Stopped))
                animation->start();
        }
        break;
    }
}

bool QParallelAnimationGroup2::shouldAnimationStart(QAbstractAnimation2 *animation, bool startIfAtEnd) const
{
    const int dura = animation->totalDuration();

    if (dura == -1)
        return uncontrolledAnimationFinishTime(animation) == -1;

    if (startIfAtEnd)
        return m_currentTime <= dura;
    if (m_direction == Forward)
        return m_currentTime < dura;
    else //direction == Backward
        return m_currentTime && m_currentTime <= dura;
}

void QParallelAnimationGroup2::applyGroupState(QAbstractAnimation2 *animation)
{
    switch (m_state)
    {
    case Running:
        animation->start();
        break;
    case Paused:
        animation->pause();
        break;
    case Stopped:
    default:
        break;
    }
}

void QParallelAnimationGroup2::updateDirection(QAbstractAnimation2::Direction direction)
{
    //we need to update the direction of the current animation
    if (!isStopped()) {
        for (QAbstractAnimation2 *animation = firstChild(); animation; animation = animation->nextSibling()) {
            animation->setDirection(direction);
        }
    } else {
        if (direction == Forward) {
            m_previousLoop = 0;
            m_previousCurrentTime = 0;
        } else {
            // Looping backwards with loopCount == -1 does not really work well...
            m_previousLoop = (m_loopCount == -1 ? 0 : m_loopCount - 1);
            m_previousCurrentTime = duration();
        }
    }
}

void QParallelAnimationGroup2::uncontrolledAnimationFinished(QAbstractAnimation2 *animation)
{
    Q_ASSERT(animation && animation->duration() == -1 || animation->loopCount() < 0);
    int uncontrolledRunningCount = 0;

    for (QAbstractAnimation2 *child = firstChild(); child; child = child->nextSibling()) {
        if (child == animation) {
            setUncontrolledAnimationFinishTime(animation, animation->currentTime());
        } else if (child->duration() == -1 || child->loopCount() < 0) {
            if (uncontrolledAnimationFinishTime(child) == -1)
                ++uncontrolledRunningCount;
        }
    }

    if (uncontrolledRunningCount > 0)
        return;

    int maxDuration = 0;
    for (QAbstractAnimation2 *animation = firstChild(); animation; animation = animation->nextSibling())
        maxDuration = qMax(maxDuration, animation->totalDuration());

    if (m_currentTime >= maxDuration)
        stop();
}

QT_END_NAMESPACE

