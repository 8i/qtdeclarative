import QtQuick 2.0

Rectangle { width: 300; height: 300; color: "white"
    property string contextualProperty: "Hello"
    TextEdit {  text: "Hello world!"; id: textEditObject; objectName: "textEditObject"
        resources: [ Component { id:cursor; Item { id:cursorInstance; objectName: "cursorInstance"; property string localProperty: contextualProperty } } ]
        cursorDelegate: cursor
    }
}
