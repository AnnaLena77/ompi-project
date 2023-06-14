import QtQuick 2.0
import QtQuick.Controls 2.0
import QtQuick.Window 2.0

Window {
    id: window
    width: 640
    height: 480
    visible: true
    property alias actualScreen: actualScreen
    title: qsTr("Hello World")

    Sidebar {
        id: sidebar
    }

    Loader{
        id: actualScreen

        focus: true
        anchors {
            left: sidebar.right
            right: parent.right
            top: parent.top
            bottom: parent.bottom
        }

        function reload(){
            actualScreen.source = "";
            QmlEngine.clearCache();
        }

        //anchors.fill: parent
        //source: "Cores2D.qml"
    }

}
