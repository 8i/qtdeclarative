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

#ifndef PARTICLE_H
#define PARTICLE_H

#include <QObject>
#include <QDebug>
#include "qsgparticlesystem_p.h"

QT_BEGIN_HEADER

QT_BEGIN_NAMESPACE

QT_MODULE(Declarative)


class QSGParticlePainter : public QSGItem
{
    Q_OBJECT
    Q_PROPERTY(QSGParticleSystem* system READ system WRITE setSystem NOTIFY systemChanged)
    Q_PROPERTY(QStringList particles READ particles WRITE setParticles NOTIFY particlesChanged)

public:
    explicit QSGParticlePainter(QSGItem *parent = 0);
    virtual void load(QSGParticleData*);
    virtual void reload(QSGParticleData*);
    virtual void setCount(int c);
    virtual int count();
    QSGParticleSystem* system() const
    {
        return m_system;
    }


    QStringList particles() const
    {
        return m_particles;
    }

    int particleTypeIndex(QSGParticleData*);
signals:
    void countChanged();
    void systemChanged(QSGParticleSystem* arg);

    void particlesChanged(QStringList arg);

public slots:
void setSystem(QSGParticleSystem* arg);

void setParticles(QStringList arg)
{
    if (m_particles != arg) {
        m_particles = arg;
        emit particlesChanged(arg);
    }
}
private slots:
    void calcSystemOffset();
protected:
    virtual void reset();
    virtual void componentComplete();

//    virtual QSGNode *updatePaintNode(QSGNode *, UpdatePaintNodeData *){
//        qDebug() << "Shouldn't be here..." << this;
//        return 0;
//    }

    QSGParticleSystem* m_system;
    friend class QSGParticleSystem;
    int m_count;
    bool m_pleaseReset;
    QStringList m_particles;
    QHash<int,int> m_particleStarts;
    int m_lastStart;
    QPointF m_systemOffset;

    template <typename VertexStruct>
    void vertexCopy(VertexStruct &b, const ParticleVertex& a)
    {
        b.x = a.x - m_systemOffset.x();
        b.y = a.y - m_systemOffset.y();
        b.t = a.t;
        b.lifeSpan = a.lifeSpan;
        b.size = a.size;
        b.endSize = a.endSize;
        b.sx = a.sx;
        b.sy = a.sy;
        b.ax = a.ax;
        b.ay = a.ay;
    }

private:
};

QT_END_NAMESPACE
QT_END_HEADER
#endif // PARTICLE_H
