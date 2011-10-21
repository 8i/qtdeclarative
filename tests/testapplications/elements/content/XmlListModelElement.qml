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

Item {
    id: xmllistmodelelementtest
    anchors.fill: parent
    property string testtext: ""
    property string modelxml: "<?xml version=\"1.0\" encoding=\"UTF-8\"?>"+
    "<cookbook><recipe id=\"MushroomSoup\">"+
    "<title>Hot chocolate</title>"+
    "<ingredient name=\"Milk\" quantity=\"1\" unit=\"cup\"/>"+
    "<time quantity=\"2\" unit=\"minutes\"/>"+
    "<method><step>1. Place the cup of milk in the microwave for 1 minute.</step>"+
    "<step>2. Stir in 2 teaspoons of drinking chocolate.</step></method></recipe></cookbook>"

    XmlListModel { id: xmllistmodelelement
        source: "cookbook.xml"
        query: "/cookbook/recipe"
        XmlRole { name: "title"; query: "title/string()" }
        XmlRole { name: "xmlid"; query: "@id/string()" }
        XmlRole { name: "method"; query: "method/string()" }
        XmlRole { name: "time"; query: "time/@quantity/number()" }
    }

    ListView {
        id: recipeview
        model: xmllistmodelelement; height: 300; width: 300; clip: true
        anchors.centerIn: parent
        delegate: Component {
            Rectangle { id: delbox; height: 50; width: 296; color: "orange"; border.color: "black"; state: "closed"; clip: true; radius: 5
                anchors.horizontalCenter: parent.horizontalCenter
                Text {
                    id: recipetitle
                    text: model.title; font.pointSize: 12; font.bold: true
                    anchors.horizontalCenter: parent.horizontalCenter; y: 20;
                }

                Text {
                    id: recipetime
                    width: parent.width; height: 20; text: "<b>Time: </b>" +model.time + " minutes"; visible: opacity != 0
                    anchors.horizontalCenter: parent.horizontalCenter; anchors.top: recipetitle.bottom
                    Behavior on opacity { NumberAnimation { duration: 250 } }
                }

                // Sub XmlListModel
                XmlListModel { id: subxmllistmodelelement
                    source: "cookbook.xml"
                    query: "/cookbook/recipe[@id = '"+model.xmlid+"']/ingredient"
                    XmlRole { name: "ingredientname"; query: "@name/string()" }
                    XmlRole { name: "ingredientquantity"; query: "@quantity/string()" }
                    XmlRole { name: "ingredientunit"; query: "@unit/string()" }
                }

                ListView {
                    id: ingredientlist
                    model: subxmllistmodelelement
                    height: 20 * count; width: parent.width
                    visible: opacity != 0
                    Behavior on opacity { NumberAnimation { duration: 250 } }
                    anchors.horizontalCenter: parent.horizontalCenter; anchors.top: recipetime.bottom; anchors.topMargin: 10
                    header: Text { text: "<b>Ingredients:</b>" }
                    delegate: Text {
                        text: ingredientquantity + " " + ingredientunit + " of " + ingredientname; height: 20;
                    }
                }

                Text {
                    id: recipemethod
                    property string methodtext: ""
                    width: parent.width; wrapMode: Text.WordWrap; visible: opacity != 0; text: methodtext
                    anchors.horizontalCenter: parent.horizontalCenter; anchors.top: ingredientlist.bottom
                    Behavior on opacity { NumberAnimation { duration: 250 } }
                    Component.onCompleted: { console.log(model.method); methodtext = model.method; console.log(recipemethod.textFormat); }
                }
                MouseArea { anchors.fill: parent; onClicked: delbox.state = delbox.state == "open" ? "closed" : "open" }
                Behavior on height { NumberAnimation { duration: 250 } }
                states: [
                    State { name: "closed"
                        PropertyChanges { target: delbox; height: 50 }
                        PropertyChanges { target: recipemethod; opacity: 0 }
                        PropertyChanges { target: recipetime; opacity: 0 }
                        PropertyChanges { target: ingredientlist; opacity: 0 }
                    },
                    State { name: "open"
                        PropertyChanges { target: delbox; height: recipemethod.height+recipetime.height+ingredientlist.height+50 }
                        PropertyChanges { target: recipemethod; opacity: 1 }
                        StateChangeScript { script: { recipeview.positionViewAtIndex(model.index, ListView.Beginning); } }
                    }
                ]
            }
        }
    }

    SystemTestHelp { id: helpbubble; visible: statenum != 0
        anchors { top: parent.top; horizontalCenter: parent.horizontalCenter; topMargin: 50 }
    }
    BugPanel { id: bugpanel }

    states: [
        State { name: "start"; when: statenum == 1
            PropertyChanges { target: xmllistmodelelementtest
                testtext: "This is a ListView populated by an XmlListModel. Clicking on an item will show the recipe details.\n"+
                "Next we will change to source of the model data to a local string" }
        },
        State { name: "xmlstring"; when: statenum == 2
            PropertyChanges { target: xmllistmodelelement; source: "" }
            PropertyChanges { target: xmllistmodelelement; xml: modelxml }
            PropertyChanges { target: xmllistmodelelementtest
                testtext: "The list should now only contain a Hot chocolate recipe.\n"+
                "Advance to restart the test." }
        }
    ]

}
