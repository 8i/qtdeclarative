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

#include "private/qdeclarativesmoothedanimation_p.h"
#include "private/qdeclarativesmoothedanimation_p_p.h"

#include "private/qdeclarativeanimation_p_p.h"

#include <qdeclarativeproperty.h>
#include "private/qdeclarativeproperty_p.h"

#include "private/qdeclarativeglobal_p.h"

#include <QtCore/qdebug.h>

#include <math.h>

#define DELAY_STOP_TIMER_INTERVAL 32

QT_BEGIN_NAMESPACE

QSmoothedAnimation::QSmoothedAnimation(QObject *parent)
    : QAbstractAnimation(parent), to(0), velocity(200), userDuration(-1), maximumEasingTime(-1),
      reversingMode(QDeclarativeSmoothedAnimation::Eased), initialVelocity(0),
      trackVelocity(0), initialValue(0), invert(false), finalDuration(-1), lastTime(0)
{
    delayedStopTimer.setInterval(DELAY_STOP_TIMER_INTERVAL);
    delayedStopTimer.setSingleShot(true);
    connect(&delayedStopTimer, SIGNAL(timeout()), this, SLOT(stop()));
}

void QSmoothedAnimation::restart()
{
    initialVelocity = trackVelocity;
    if (state() != QAbstractAnimation::Running)
        start();
    else
        init();
}

void QSmoothedAnimation::updateState(QAbstractAnimation::State newState, QAbstractAnimation::State /*oldState*/)
{
    if (newState == QAbstractAnimation::Running)
        init();
}

void QSmoothedAnimation::delayedStop()
{
    if (!delayedStopTimer.isActive())
        delayedStopTimer.start();
}

int QSmoothedAnimation::duration() const
{
    return -1;
}

bool QSmoothedAnimation::recalc()
{
    s = to - initialValue;
    vi = initialVelocity;

    s = (invert? -1.0: 1.0) * s;

    if (userDuration > 0 && velocity > 0) {
        tf = s / velocity;
        if (tf > (userDuration / 1000.)) tf = (userDuration / 1000.);
    } else if (userDuration > 0) {
        tf = userDuration / 1000.;
    } else if (velocity > 0) {
        tf = s / velocity;
    } else {
        return false;
    }

    finalDuration = ceil(tf * 1000.0);

    if (maximumEasingTime == 0) {
        a = 0;
        d = 0;
        tp = 0;
        td = tf;
        vp = velocity;
        sp = 0;
        sd = s;
    } else if (maximumEasingTime != -1 && tf > (maximumEasingTime / 1000.)) {
        qreal met = maximumEasingTime / 1000.;
        //       tp|       |td
        // vp_      _______
        //         /       \
        // vi_    /         \
        //                   \
        //                    \   _ 0
        //       |ta|      |ta|
        //
        qreal ta = met / 2.;
        a = (s - (vi * tf - 0.5 * vi * ta)) / (tf * ta - ta * ta);

        vp = vi + a * ta;
        d = vp / ta;
        tp = ta;
        sp = vi * ta + 0.5 * a * tp * tp;
        sd = sp + vp * (tf - 2 * ta);
        td = tf - ta;
    } else {
        qreal c1 = 0.25 * tf * tf;
        qreal c2 = 0.5 * vi * tf - s;
        qreal c3 = -0.25 * vi * vi;

        qreal a1 = (-c2 + sqrt(c2 * c2 - 4 * c1 * c3)) / (2. * c1);

        qreal tp1 = 0.5 * tf - 0.5 * vi / a1;
        qreal vp1 = a1 * tp1 + vi;

        qreal sp1 = 0.5 * a1 * tp1 * tp1 + vi * tp1;

        a = a1;
        d = a1;
        tp = tp1;
        td = tp1;
        vp = vp1;
        sp = sp1;
        sd = sp1;
    }
    return true;
}

qreal QSmoothedAnimation::easeFollow(qreal time_seconds)
{
    qreal value;
    if (time_seconds < tp) {
        trackVelocity = vi + time_seconds * a;
        value = 0.5 * a * time_seconds * time_seconds + vi * time_seconds;
    } else if (time_seconds < td) {
        time_seconds -= tp;
        trackVelocity = vp;
        value = sp + time_seconds * vp;
    } else if (time_seconds < tf) {
        time_seconds -= td;
        trackVelocity = vp - time_seconds * a;
        value = sd - 0.5 * d * time_seconds * time_seconds + vp * time_seconds;
    } else {
        trackVelocity = 0;
        value = s;
        delayedStop();
    }

    // to normalize 's' between [0..1], divide 'value' by 's'
    return value;
}

void QSmoothedAnimation::updateCurrentTime(int t)
{
    qreal time_seconds = qreal(t - lastTime) / 1000.;

    qreal value = easeFollow(time_seconds);
    value *= (invert? -1.0: 1.0);
    QDeclarativePropertyPrivate::write(target, initialValue + value,
                                       QDeclarativePropertyPrivate::BypassInterceptor
                                       | QDeclarativePropertyPrivate::DontRemoveBinding);
}

void QSmoothedAnimation::init()
{
    if (velocity == 0) {
        stop();
        return;
    }

    if (delayedStopTimer.isActive())
        delayedStopTimer.stop();

    initialValue = target.read().toReal();
    lastTime = this->currentTime();

    if (to == initialValue) {
        stop();
        return;
    }

    bool hasReversed = trackVelocity != 0. &&
                      ((!invert) == ((initialValue - to) > 0));

    if (hasReversed) {
        switch (reversingMode) {
            default:
            case QDeclarativeSmoothedAnimation::Eased:
                initialVelocity = -trackVelocity;
                break;
            case QDeclarativeSmoothedAnimation::Sync:
                QDeclarativePropertyPrivate::write(target, to,
                                                   QDeclarativePropertyPrivate::BypassInterceptor
                                                   | QDeclarativePropertyPrivate::DontRemoveBinding);
                trackVelocity = 0;
                stop();
                return;
            case QDeclarativeSmoothedAnimation::Immediate:
                initialVelocity = 0;
                break;
        }
    }

    trackVelocity = initialVelocity;

    invert = (to < initialValue);

    if (!recalc()) {
        QDeclarativePropertyPrivate::write(target, to,
                                           QDeclarativePropertyPrivate::BypassInterceptor
                                           | QDeclarativePropertyPrivate::DontRemoveBinding);
        stop();
        return;
    }
}

/*!
    \qmlclass SmoothedAnimation QDeclarativeSmoothedAnimation
    \ingroup qml-animation-transition
    \since 4.7
    \inherits NumberAnimation
    \brief The SmoothedAnimation element allows a property to smoothly track a value.

    A SmoothedAnimation animates a property's value to a set target value
    using an ease in/out quad easing curve.  When the target value changes,
    the easing curves used to animate between the old and new target values
    are smoothly spliced together to create a smooth movement to the new
    target value that maintains the current velocity.

    The follow example shows one \l Rectangle tracking the position of another
    using SmoothedAnimation. The green rectangle's \c x and \c y values are
    bound to those of the red rectangle. Whenever these values change, the
    green rectangle smoothly animates to its new position:

    \snippet doc/src/snippets/declarative/smoothedanimation.qml 0

    A SmoothedAnimation can be configured by setting the \l velocity at which the
    animation should occur, or the \l duration that the animation should take.
    If both the \l velocity and \l duration are specified, the one that results in
    the quickest animation is chosen for each change in the target value.

    For example, animating from 0 to 800 will take 4 seconds if a velocity
    of 200 is set, will take 8 seconds with a duration of 8000 set, and will
    take 4 seconds with both a velocity of 200 and a duration of 8000 set.
    Animating from 0 to 20000 will take 10 seconds if a velocity of 200 is set,
    will take 8 seconds with a duration of 8000 set, and will take 8 seconds
    with both a velocity of 200 and a duration of 8000 set.

    The default velocity of SmoothedAnimation is 200 units/second.  Note that if the range of the
    value being animated is small, then the velocity will need to be adjusted
    appropriately.  For example, the opacity of an item ranges from 0 - 1.0.
    To enable a smooth animation in this range the velocity will need to be
    set to a value such as 0.5 units/second.  Animating from 0 to 1.0 with a velocity
    of 0.5 will take 2000 ms to complete.

    Like any other animation element, a SmoothedAnimation can be applied in a
    number of ways, including transitions, behaviors and property value
    sources. The \l {QML Animation and Transitions} documentation shows a
    variety of methods for creating animations.

    \sa SpringAnimation, NumberAnimation, {QML Animation and Transitions}, {declarative/animation/basics}{Animation basics example}
*/

QDeclarativeSmoothedAnimation::QDeclarativeSmoothedAnimation(QObject *parent)
: QDeclarativeNumberAnimation(*(new QDeclarativeSmoothedAnimationPrivate), parent)
{
}

QDeclarativeSmoothedAnimation::~QDeclarativeSmoothedAnimation()
{
}

QDeclarativeSmoothedAnimationPrivate::QDeclarativeSmoothedAnimationPrivate()
    : wrapperGroup(new QParallelAnimationGroup), anim(new QSmoothedAnimation)
{
    Q_Q(QDeclarativeSmoothedAnimation);
    QDeclarative_setParent_noEvent(wrapperGroup, q);
    QDeclarative_setParent_noEvent(anim, q);
}

void QDeclarativeSmoothedAnimationPrivate::updateRunningAnimations()
{
    foreach(QSmoothedAnimation* ease, activeAnimations.values()){
        ease->maximumEasingTime = anim->maximumEasingTime;
        ease->reversingMode = anim->reversingMode;
        ease->velocity = anim->velocity;
        ease->userDuration = anim->userDuration;
        ease->init();
    }
}

QAbstractAnimation* QDeclarativeSmoothedAnimation::qtAnimation()
{
    Q_D(QDeclarativeSmoothedAnimation);
    return d->wrapperGroup;
}

void QDeclarativeSmoothedAnimation::transition(QDeclarativeStateActions &actions,
                                               QDeclarativeProperties &modified,
                                               TransitionDirection direction)
{
    Q_D(QDeclarativeSmoothedAnimation);
    QDeclarativeNumberAnimation::transition(actions, modified, direction);

    if (!d->actions)
        return;

    QSet<QAbstractAnimation*> anims;
    for (int i = 0; i < d->actions->size(); i++) {
        QSmoothedAnimation *ease;
        bool needsRestart;
        if (!d->activeAnimations.contains((*d->actions)[i].property)) {
            ease = new QSmoothedAnimation();
            d->wrapperGroup->addAnimation(ease);
            d->activeAnimations.insert((*d->actions)[i].property, ease);
            needsRestart = false;
        } else {
            ease = d->activeAnimations.value((*d->actions)[i].property);
            needsRestart = true;
        }
        ease->target = (*d->actions)[i].property;
        ease->to = (*d->actions)[i].toValue.toReal();

        // copying public members from main value holder animation
        ease->maximumEasingTime = d->anim->maximumEasingTime;
        ease->reversingMode = d->anim->reversingMode;
        ease->velocity = d->anim->velocity;
        ease->userDuration = d->anim->userDuration;

        ease->initialVelocity = ease->trackVelocity;

        if (needsRestart)
            ease->init();
        anims.insert(ease);
    }

    for (int i = d->wrapperGroup->animationCount() - 1; i >= 0 ; --i) {
        if (!anims.contains(d->wrapperGroup->animationAt(i))) {
            QSmoothedAnimation *ease = static_cast<QSmoothedAnimation*>(d->wrapperGroup->animationAt(i));
            d->activeAnimations.remove(ease->target);
            d->wrapperGroup->takeAnimation(i);
            delete ease;
        }
    }
}

/*!
    \qmlproperty enumeration SmoothedAnimation::reversingMode

    Sets how the SmoothedAnimation behaves if an animation direction is reversed.

    Possible values are:

    \list
    \o SmoothedAnimation.Eased (default) - the animation will smoothly decelerate, and then reverse direction
    \o SmoothedAnimation.Immediate - the animation will immediately begin accelerating in the reverse direction, beginning with a velocity of 0
    \o SmoothedAnimation.Sync - the property is immediately set to the target value
    \endlist
*/
QDeclarativeSmoothedAnimation::ReversingMode QDeclarativeSmoothedAnimation::reversingMode() const
{
    Q_D(const QDeclarativeSmoothedAnimation);
    return (QDeclarativeSmoothedAnimation::ReversingMode) d->anim->reversingMode;
}

void QDeclarativeSmoothedAnimation::setReversingMode(ReversingMode m)
{
    Q_D(QDeclarativeSmoothedAnimation);
    if (d->anim->reversingMode == m)
        return;

    d->anim->reversingMode = m;
    emit reversingModeChanged();
    d->updateRunningAnimations();
}

/*!
    \qmlproperty int SmoothedAnimation::duration

    This property holds the animation duration, in msecs, used when tracking the source.

    Setting this to -1 (the default) disables the duration value.

    If the velocity value and the duration value are both enabled, then the animation will
    use whichever gives the shorter duration.
*/
int QDeclarativeSmoothedAnimation::duration() const
{
    Q_D(const QDeclarativeSmoothedAnimation);
    return d->anim->userDuration;
}

void QDeclarativeSmoothedAnimation::setDuration(int duration)
{
    Q_D(QDeclarativeSmoothedAnimation);
    if (duration != -1)
        QDeclarativeNumberAnimation::setDuration(duration);
    if(duration == d->anim->userDuration)
        return;
    d->anim->userDuration = duration;
    d->updateRunningAnimations();
}

qreal QDeclarativeSmoothedAnimation::velocity() const
{
    Q_D(const QDeclarativeSmoothedAnimation);
    return d->anim->velocity;
}

/*!
    \qmlproperty real SmoothedAnimation::velocity

    This property holds the average velocity allowed when tracking the 'to' value.

    The default velocity of SmoothedAnimation is 200 units/second.

    Setting this to -1 disables the velocity value.

    If the velocity value and the duration value are both enabled, then the animation will
    use whichever gives the shorter duration.
*/
void QDeclarativeSmoothedAnimation::setVelocity(qreal v)
{
    Q_D(QDeclarativeSmoothedAnimation);
    if (d->anim->velocity == v)
        return;

    d->anim->velocity = v;
    emit velocityChanged();
    d->updateRunningAnimations();
}

/*!
    \qmlproperty int SmoothedAnimation::maximumEasingTime

    This property specifies the maximum time, in msecs, any "eases" during the follow should take.
    Setting this property causes the velocity to "level out" after at a time.  Setting
    a negative value reverts to the normal mode of easing over the entire animation
    duration.

    The default value is -1.
*/
int QDeclarativeSmoothedAnimation::maximumEasingTime() const
{
    Q_D(const QDeclarativeSmoothedAnimation);
    return d->anim->maximumEasingTime;
}

void QDeclarativeSmoothedAnimation::setMaximumEasingTime(int v)
{
    Q_D(QDeclarativeSmoothedAnimation);
    if(v == d->anim->maximumEasingTime)
        return;
    d->anim->maximumEasingTime = v;
    emit maximumEasingTimeChanged();
    d->updateRunningAnimations();
}

QT_END_NAMESPACE
