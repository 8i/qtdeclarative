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

#ifndef PARTICLESYSTEM_H
#define PARTICLESYSTEM_H

#include <QSGItem>
#include <QElapsedTimer>
#include <QVector>
#include <QHash>
#include <QPointer>

QT_BEGIN_HEADER

QT_BEGIN_NAMESPACE

QT_MODULE(Declarative)


class QSGParticleAffector;
class QSGParticleEmitter;
class QSGParticlePainter;
class QSGParticleData;


struct GroupData{
    int size;
    int start;
    int nextIdx;
    QList<QSGParticlePainter*> types;
};

class QSGParticleSystem : public QSGItem
{
    Q_OBJECT
    Q_PROPERTY(bool running READ isRunning WRITE setRunning NOTIFY runningChanged)
    Q_PROPERTY(int startTime READ startTime WRITE setStartTime NOTIFY startTimeChanged)
    Q_PROPERTY(bool overwrite READ overwrite WRITE setOverwrite NOTIFY overwriteChanged)//XXX: Should just be an implementation detail, but I can't decide which way
    /* The problem is that it ought to be false (usually) for stasis effects like model particles,
       but it ought to be true (usually) for burst effects where you want it to burst, and forget the old stuff
       Ideally burst never overflows? But that leads to crappy behaviour from crappy users...
    */

public:
    explicit QSGParticleSystem(QSGItem *parent = 0);

bool isRunning() const
{
    return m_running;
}

int startTime() const
{
    return m_startTime;
}

int count(){ return m_particle_count; }

signals:

void systemInitialized();
void runningChanged(bool arg);

void startTimeChanged(int arg);


void overwriteChanged(bool arg);

public slots:
void reset();
void setRunning(bool arg);


void setStartTime(int arg)
{
    m_startTime = arg;
}

void setOverwrite(bool arg)
{
    if (m_overwrite != arg) {
    m_overwrite = arg;
emit overwriteChanged(arg);
}
}

void fastForward(int ms)
{
    m_startTime += ms;
}

protected:
    void componentComplete();

private slots:
    void countChanged();
public://but only really for related class usage. Perhaps we should all be friends?
    void emitParticle(QSGParticleData* p);
    QSGParticleData* newDatum(int groupId);
    qint64 systemSync(QSGParticlePainter* p);
    QElapsedTimer m_timestamp;
    QVector<QSGParticleData*> m_data;
    QSet<QSGParticleData*> m_needsReset;
    QHash<QString, int> m_groupIds;
    QHash<int, GroupData*> m_groupData;//id, size, start
    qint64 m_timeInt;
    bool m_initialized;

    void registerParticleType(QSGParticlePainter* p);
    void registerParticleEmitter(QSGParticleEmitter* e);
    void registerParticleAffector(QSGParticleAffector* a);
    bool overwrite() const
    {
        return m_overwrite;
    }

    int m_particle_count;
private:
    void initializeSystem();
    bool m_running;
    QList<QPointer<QSGParticleEmitter> > m_emitters;
    QList<QPointer<QSGParticleAffector> > m_affectors;
    QList<QPointer<QSGParticlePainter> > m_particles;
    QList<QPointer<QSGParticlePainter> > m_syncList;
    qint64 m_startTime;
    int m_nextGroupId;
    bool m_overwrite;
    bool m_componentComplete;
};

//TODO: Clean up all this into ParticleData

struct ParticleVertex {
    float x;
    float y;
    float t;
    float lifeSpan;
    float size;
    float endSize;
    float sx;
    float sy;
    float ax;
    float ay;
    //TODO: Need opacity over life control. More variable size over life?
};

class QSGParticleData{
public:
    QSGParticleData();

    ParticleVertex pv;

    //Convenience functions for working backwards, because parameters are from the start of particle life
    //If setting multiple parameters at once, doing the conversion yourself will be faster.

    //sets the x accleration without affecting the instantaneous x velocity or position
    void setInstantaneousAX(qreal ax);
    //sets the x velocity without affecting the instantaneous x postion
    void setInstantaneousSX(qreal vx);
    //sets the instantaneous x postion
    void setInstantaneousX(qreal x);
    //sets the y accleration without affecting the instantaneous y velocity or position
    void setInstantaneousAY(qreal ay);
    //sets the y velocity without affecting the instantaneous y postion
    void setInstantaneousSY(qreal vy);
    //sets the instantaneous Y postion
    void setInstantaneousY(qreal y);

    //TODO: Slight caching?
    qreal curX() const;
    qreal curSX() const;
    qreal curY() const;
    qreal curSY() const;

    int group;
    QSGParticleEmitter* e;
    QSGParticleSystem* system;
    int particleIndex;
    int systemIndex;

    void debugDump();
    bool stillAlive();
};

QT_END_NAMESPACE

QT_END_HEADER

#endif // PARTICLESYSTEM_H


