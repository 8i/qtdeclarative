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
    id: rowelementtest
    anchors.fill: parent
    property string testtext: ""

    Row {
        id: rowelement
        height: 50; width: 250; spacing: 5
        anchors.centerIn: parent
        Rectangle { id: gr; color: "green"; height: 50; width: 50; border.color: "gray"; border.width: 3; opacity: .9; radius: 5; clip: true }
        Rectangle { id: re; color: "red"; height: 50; width: 50; border.color: "gray"; border.width: 3; opacity: .9; radius: 5; clip: true }
        Rectangle {
            id: bl
            color: "blue"; height: 50; width: 50; border.color: "gray"; border.width: 3; radius: 5; clip: true
            opacity: 0; visible: opacity != 0
        }
        Rectangle { id: bk; color: "black"; height: 50; width: 50; border.color: "gray"; border.width: 3; opacity: .9; radius: 5; clip: true }

        move: Transition { NumberAnimation { properties: "x"; duration: 500; easing.type: Easing.OutBounce } }
        add: Transition { NumberAnimation { properties: "x"; duration: 1000; easing.type: Easing.OutBounce } }

    }

    SystemTestHelp { id: helpbubble; visible: statenum != 0
        anchors { top: parent.top; horizontalCenter: parent.horizontalCenter; topMargin: 50 }
    }
    BugPanel { id: bugpanel }

    states: [
        State { name: "start"; when: statenum == 1
            PropertyChanges { target: rowelementtest
                testtext: "This is a Row element. At present it should be showing three rectangles - green, red and black.\n"+
                "Next, let's add a rectangle to the Row - it should slide in from the left and the black rectangle should move to give it space" }
        },
        State { name: "back"; when: statenum == 2
            PropertyChanges { target: bl; opacity: .9 }
            PropertyChanges { target: rowelementtest
                testtext: "Row should now be showing four rectangles - green, red, blue and black.\n"+
                "Advance to restart the test." }
        }
    ]

}
