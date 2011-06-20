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

#ifndef TRAILSEMITTER_H
#define TRAILSEMITTER_H

#include <QtCore>
#include <QtGui>

#include "qsgparticleemitter_p.h"

QT_BEGIN_HEADER

QT_BEGIN_NAMESPACE

QT_MODULE(Declarative)


class QSGGeometryNode;

class QSGBasicEmitter : public QSGParticleEmitter
{
    Q_OBJECT

    Q_PROPERTY(qreal speedFromMovement READ speedFromMovement WRITE setSpeedFromMovement NOTIFY speedFromMovementChanged)

public:
    explicit QSGBasicEmitter(QSGItem* parent=0);
    virtual ~QSGBasicEmitter(){}
    virtual void emitWindow(int timeStamp);


    qreal speedFromMovement() const { return m_speed_from_movement; }
    void setSpeedFromMovement(qreal s);

    qreal renderOpacity() const { return m_render_opacity; }

signals:

    void speedFromMovementChanged();

public slots:
public:
    virtual void reset();
protected:

private:

    qreal m_speed_from_movement;

    // derived values...
    int m_particle_count;
    bool m_reset_last;
    qreal m_last_timestamp;
    qreal m_last_emission;

    QPointF m_last_emitter;
    QPointF m_last_last_emitter;
    QPointF m_last_last_last_emitter;

    qreal m_render_opacity;
};

QT_END_NAMESPACE
QT_END_HEADER
#endif // TRAILSEMITTER_H
