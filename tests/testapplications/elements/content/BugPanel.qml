/****************************************************************************
**
** Copyright (C) 2011 Nokia Corporation and/or its subsidiary(-ies).
** All rights reserved.
** Contact: Nokia Corporation (qt-info@nokia.com)
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

Rectangle {
    property string urltext
    urltext: "<a href=\"" + bugreports + bugnumber + "\">QTBUG-" + bugnumber + "</a>"
    property string bugnumber: ""
    property string bugreports: "http://bugreports.qt.nokia.com/browse/QTBUG-"
    visible: opacity != 0
    opacity: bugnumber == "" ? 0 : 1
    Behavior on opacity { NumberAnimation { duration: 1500 } }
    height: buglist.paintedHeight; width: 200; radius: 5; border.color: "lightgray"
    anchors { bottom: parent.bottom; left: parent.left; leftMargin: 15; bottomMargin: 15 }
    Text { id: buglist; text: urltext; textFormat: Text.RichText; visible: bugnumber != ""
        anchors.centerIn: parent; onLinkActivated: { Qt.openUrlExternally(link); }
    }
}
