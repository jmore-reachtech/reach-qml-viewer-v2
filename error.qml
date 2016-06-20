import QtQuick 2.0

Rectangle {
    id: root
    objectName: "root"
    color: "gray"
    width: screen.getScreenWidth()
    height: screen.getScreenHeight()

    Text{
        id: txtError
        objectName: "txtError"
        anchors.centerIn: parent
        text: connection.getStartUpError()
        font.pixelSize: 22
        font.family: "DejaVu Sans"
        color: "red"
    }
}
