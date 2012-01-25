/****************************************************************************
**
** Copyright (C) 2012 Nokia Corporation and/or its subsidiary(-ies).
** All rights reserved.
** Contact: http://www.qt-project.org/
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

import QtQuick 2.0
import QtQuick.Particles 2.0

Item {
    id: block
    property bool dying: false
    property bool spawned: false
    property int type: 0
    property ParticleSystem particleSystem

    Behavior on x {
        enabled: spawned;
        SpringAnimation{ spring: 2; damping: 0.2 }
    }
    Behavior on y {
        SpringAnimation{ spring: 2; damping: 0.2 }
    }

    Image {
        id: img
        source: {
            if(type == 0){
                "pics/redStone.png";
            } else if(type == 1) {
                "pics/blueStone.png";
            } else {
                "pics/greenStone.png";
            }
        }
        opacity: 0
        Behavior on opacity { NumberAnimation { duration: 200 } }
        anchors.fill: parent
    }
    Emitter {
        id: particles
        system: particleSystem
        group: {
            if(type == 0){
                "red";
            } else if (type == 1) {
                "blue";
            } else {
                "green";
            }
        }
        anchors.fill: parent

        speed: TargetDirection{targetX: block.width/2; targetY: block.height/2; magnitude: -60; magnitudeVariation: 60}
        shape: EllipseShape{fill:true}
        enabled: false;
        lifeSpan: 700; lifeSpanVariation: 100
        emitRate: 1000
        maximumEmitted: 100 //only fires 0.1s bursts (still 2x old number)
        size: 28
        endSize: 14
    }

    states: [
        State {
            name: "AliveState"; when: spawned == true && dying == false
            PropertyChanges { target: img; opacity: 1 }
        },

        State {
            name: "DeathState"; when: dying == true
            StateChangeScript { script: particles.pulse(0.1); }
            PropertyChanges { target: img; opacity: 0 }
            StateChangeScript { script: block.destroy(1000); }
        }
    ]
}
