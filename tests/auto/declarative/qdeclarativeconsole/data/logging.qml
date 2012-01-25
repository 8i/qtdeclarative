/****************************************************************************
**
** Copyright (C) 2011 Nokia Corporation and/or its subsidiary(-ies).
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

QtObject {
    id:root

    function consoleCount() {
        console.count("console.count", "Ignore additonal argument");
        console.count();
    }

    Component.onCompleted: {
        console.debug("console.debug");
        console.log("console.log");
        console.info("console.info");
        console.warn("console.warn");
        console.error("console.error");

        consoleCount();
        consoleCount();

        var a = [1, 2];
        var b = {a: "hello", d: 1 };
        var c
        var d = 12;
        var e = function() { return 5;};
        var f = true;
        var g = {toString: function() { throw new Error('toString'); }};

        console.log(a);
        console.log(b);
        console.log(c);
        console.log(d);
        console.log(e);
        console.log(f);
        console.log(root);
        console.log(g);
        console.log(1, "pong!", new Object);
        console.log(1, ["ping","pong"], new Object, 2);

        try {
            console.log(exception);
        } catch (e) {
            return;
        }

        throw ("console.log(exception) should have raised an exception");
    }
}
