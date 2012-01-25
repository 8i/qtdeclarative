/****************************************************************************
**
** Copyright (C) 2012 Nokia Corporation and/or its subsidiary(-ies).
** All rights reserved.
** Contact: http://www.qt-project.org/
**
** This file is part of the test suite of the Qt Toolkit.
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

Item {
    id: scaleelementtest
    anchors.fill: parent
    property string testtext: ""

    Rectangle {
        id: scaletarget
        color: "green"; height: 100; width: 100; border.color: "gray"; opacity: 0.7; radius: 5
        anchors.centerIn: parent
        transform: Scale {
            id: scaleelement
            property alias originx: scaleelement.origin.x
            property alias originy: scaleelement.origin.y
            origin.x: 50; origin.y: 50
            Behavior on xScale { NumberAnimation { duration: 500 } }
            Behavior on yScale { NumberAnimation { duration: 500 } }
            // QTBUG-20827 Behavior on origin.x { NumberAnimation { duration: 500 } }
            // QTBUG-20827 Behavior on origin.y { NumberAnimation { duration: 500 } }

        }
    }

    SystemTestHelp { id: helpbubble; visible: statenum != 0
        anchors { top: parent.top; horizontalCenter: parent.horizontalCenter; topMargin: 50 }
    }
    BugPanel { id: bugpanel }

    states: [
        State { name: "start"; when: statenum == 1
            PropertyChanges { target: scaleelementtest
                testtext: "This is a Rectangle that will be transformed using a Scale element.\n"+
                "Next, it will be scaled to 2x size." }
        },
        State { name: "scaleup"; when: statenum == 2
            PropertyChanges { target: scaleelement; xScale: 2; yScale: 2 }
            PropertyChanges { target: scaleelementtest
                testtext: "It should be scaled to 2x.\nNext, it will shift to the right." }
        },
        State { name: "shiftright"; when: statenum == 3
            PropertyChanges { target: scaleelement; xScale: 2; yScale: 2; origin.x: 0; origin.y: 50 }
            PropertyChanges { target: scaleelementtest
                testtext: "It should be on the right, still scaled to 2x.\nNext, it will shift to the left" }
        },
        State { name: "shiftleft"; when: statenum == 4
            PropertyChanges { target: scaleelement; xScale: 2; yScale: 2; origin.x: 100; origin.y: 50 }
            PropertyChanges { target: scaleelementtest
                testtext: "It should be on the left, still scaled to 2x.\nAdvance to restart the test." }
        }
    ]

}
