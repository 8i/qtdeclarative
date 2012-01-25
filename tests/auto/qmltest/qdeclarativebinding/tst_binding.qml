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
import QtTest 1.0

Rectangle {
    id: screen
    width: 320; height: 240
    property string text
    property bool changeColor: false

    Text { id: s1; text: "Hello" }
    Rectangle { id: r1; width: 1; height: 1; color: "yellow" }
    Rectangle { id: r2; width: 1; height: 1; color: "red" }

    Binding { target: screen; property: "text"; value: s1.text; id: binding1 }
    Binding { target: screen; property: "color"; value: r1.color }
    Binding { target: screen; property: "color"; when: screen.changeColor == true; value: r2.color; id: binding3 }

    TestCase {
        name: "Binding"

        function test_binding() {
            compare(screen.color, "#ffff00")    // Yellow
            compare(screen.text, "Hello")
            verify(!binding3.when)

            screen.changeColor = true
            compare(screen.color, "#ff0000")    // Red

            verify(binding1.target == screen)
            compare(binding1.property, "text")
            compare(binding1.value, "Hello")
        }
    }
}
