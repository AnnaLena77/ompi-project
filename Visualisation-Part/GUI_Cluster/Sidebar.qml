import QtQuick 2.0
import QtQuick.Layouts 6.3
import QtQuick.Controls 6.3

Item {
        width: 200

        Rectangle {
            id: rectangle
            x: 0
            y: 0
            width: parent.width
            height: 480
            color: "#80b987"

            Text {
                id: text1
                x: 34
                y: 18
                text: qsTr("Das ist eine Test-Sidebar")
                font.pixelSize: 12
            }

            Button {
                id: button
                x: 60
                y: 61
                text: qsTr("2D-View")

                MouseArea{
                    anchors.fill: parent
                    onClicked: actualScreen.source = "Cores2D.qml"
                }
            }

            Button {
                id: button1
                x: 60
                y: 98
                text: qsTr("3D-View")

                MouseArea{
                    anchors.fill: parent
                    onClicked: actualScreen.source = "Cores3D.qml"
                }
            }
        }

    }
