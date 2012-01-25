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
    id: textelementtest
    anchors.fill: parent
    property string testtext: ""

    Text {
        id: textelement
        property int pseudopointsize: 12
        anchors.centerIn: parent
        height: 200; width: parent.width *.8; wrapMode: Text.WordWrap; font.pointSize: 12
        text: "Hello, my name is Text"; horizontalAlignment: Text.AlignHCenter; textFormat: Text.PlainText
        Behavior on font.pointSize { NumberAnimation { duration: 1000 } }
        Behavior on color { ColorAnimation { duration: 1000 } }
    }

    SystemTestHelp { id: helpbubble; visible: statenum != 0
        anchors { top: parent.top; horizontalCenter: parent.horizontalCenter; topMargin: 50 }
    }
    BugPanel { id: bugpanel }

    states: [
        State { name: "start"; when: statenum == 1
            PropertyChanges { target: textelementtest
                testtext: "This is a Text element. At present it should be saying hello.\n"+
                "Next, it will animatedly increase the text to twice its size" }
        },
        State { name: "fontlarge"; when: statenum == 2
            PropertyChanges { target: textelement; font.pointSize: 24 }
            PropertyChanges { target: textelementtest
                testtext: "The font should have increased in size, and wrapped once too large for the screen width.\n"+
                "Next, let's change the color to blue." }
            PropertyChanges { target: bugpanel; bugnumber: "19848" }
        },
        State { name: "fontlargeblue"; when: statenum == 3
            PropertyChanges { target: textelement; font.pointSize: 24; color: "blue" }
            PropertyChanges { target: textelementtest
                testtext: "The font should now be blue.\n"+
                "Next, let's add some formatting." }
        },
        State { name: "fontlargeblueitalicsbold"; when: statenum == 4
            PropertyChanges { target: textelement; font.pointSize: 24; color: "blue"; font.bold: true; font.italic: true }
            PropertyChanges { target: textelementtest
                testtext: "The font should now be blue, bold and italic.\n"+
                "Next, let's use rich text." }
            PropertyChanges { target: bugpanel; bugnumber: "19848" }
        },
        State { name: "fontlargerichtext"; when: statenum == 5
            PropertyChanges {
                target: textelement; font.pointSize: 24; color: "blue"; font.bold: true; font.italic: true
                text: "<font color=\"green\">I</font> am <font color=\"green\">now</font> multicolored!"; textFormat: Text.RichText
            }
            PropertyChanges { target: textelementtest
                testtext: "The font should now be bold and italic, and alternating in color.\n"+
                "The test will now return to the start." }
        }
    ]

}
