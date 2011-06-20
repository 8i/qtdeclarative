/****************************************************************************
**
** Copyright (C) 2011 Nokia Corporation and/or its subsidiary(-ies).
** All rights reserved.
** Contact: Nokia Corporation (qt-info@nokia.com)
**
** This file is part of the examples of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:BSD$
** You may use this file under the terms of the BSD license as follows:
**
** "Redistribution and use in source and binary forms, with or without
** modification, are permitted provided that the following conditions are
** met:
**   * Redistributions of source code must retain the above copyright
**     notice, this list of conditions and the following disclaimer.
**   * Redistributions in binary form must reproduce the above copyright
**     notice, this list of conditions and the following disclaimer in
**     the documentation and/or other materials provided with the
**     distribution.
**   * Neither the name of Nokia Corporation and its Subsidiary(-ies) nor
**     the names of its contributors may be used to endorse or promote
**     products derived from this software without specific prior written
**     permission.
**
** THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
** "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
** LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
** A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
** OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
** SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
** LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
** DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
** THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
** (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
** OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE."
** $QT_END_LICENSE$
**
****************************************************************************/

import QtQuick 2.0
import QtQuick.Particles 2.0

Rectangle{
    color: "goldenrod"
    width: 400
    height: 400
    ImageParticle{
        id: test
        particles: ["Test"]
        source: "content/particle.png"
        system: sys
        z: 2
        anchors.fill: parent
        color: "#336666CC"
        colorVariation: 0.0
    }
    ImageParticle{
        id: single
        particles: ["Face"]
        system: sys
        z: 2
        anchors.fill: parent
        sprites: Sprite{
            source: "content/squarefacesprite.png"
            frames: 6
            duration: 120
        }
    }
    MaskShape{
        id: mask
        source: "content/smileMask.png"
    }
    Emitter{
        system: sys
        particle: "Test"
        anchors.fill: parent
        id: particles2
        emitRate: 6000
        lifeSpan: 720
        emitting: true
        size: 10
        shape: mask
    }
    Emitter{
        system: sys
        particle: "Face"
        anchors.fill: parent
        id: particles
        emitRate: 60
        lifeSpan: 1440
        emitting: true
        speed: PointDirection{xVariation: 10; yVariation: 10;}
        size: 30
        sizeVariation: 10
        shape: mask
    }
    ParticleSystem{
        id: sys
        anchors.fill: parent
    }

}
