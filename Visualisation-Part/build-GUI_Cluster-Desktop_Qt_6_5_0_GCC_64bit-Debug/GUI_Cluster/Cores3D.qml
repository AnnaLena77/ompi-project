import Qt3D.Render

import QtQuick 2.15
import QtQuick.Controls 2.15
import QtQuick3D.Effects
import QtQuick3D.Helpers
import QtQuick3D




Rectangle {
    width: 440
    height: parent.height
    color: "#5bc2c6"



    View3D {
        id: viewport
        anchors.fill: parent
        SplitView.fillHeight: true
        SplitView.fillWidth: true
        SplitView.minimumWidth: splitView.width * 0.5
        environment: SceneEnvironment {
            /*lightProbe: Texture {
                source: "maps/OpenfootageNET_garage-1024.hdr"
            }*/
            backgroundMode: SceneEnvironment.Color
            clearColor: "white"
            property bool enableEffects: false


        }
        Node {
            id: originNode
            PerspectiveCamera {
                id: cameraNode
                z: 600
                clipNear: 1
                clipFar: 10000
            }
        }

        OrbitCameraController {
            origin: originNode
            camera: cameraNode
        }
        MouseArea {
            id: pickController
            anchors.fill: parent
            onClicked: function(mouse) {
                let pickResult = viewport.pick(mouse.x, mouse.y);
                if (pickResult.objectHit) {
                    let pickedObject = pickResult.objectHit;
                    // Move the camera orbit origin to be the clicked object
                    originNode.position = pickedObject.position
                }
            }
        }
        Model {
            id: cube
            source: "#Cube"
            scale: Qt.vector3d(2,2,2)
            materials: [
                CustomMaterial {
                    id: glassMaterial
                    shadingMode: CustomMaterial.Shaded
                    //vertexShader: "qrc:/shaders/simple.vert"
                    fragmentShader: "qrc:/shaders/glass.frag"
                }
            ]
            Model {
                id: innerCube
                source: "#Cube"
                scale: Qt.vector3d(0.5, 0.5, 0.5)
                materials: [
                    CustomMaterial{
                        lineWidth: 1
                    }
                ]
            }

        }
        DirectionalLight {
            eulerRotation.x: 250
            eulerRotation.y: -30
            brightness: 1.0
            ambientColor: "#7f7f7f"
        }
        /*DirectionalLight{
            color: "white"
            eulerRotation.x: 90
            brightness: 1
            castsShadow: false
        }
        DirectionalLight{
            color: "white"
            eulerRotation.z: 90
            brightness: 1
            castsShadow: false
        }
        DirectionalLight{
            color: "white"
            eulerRotation.y: 90
            brightness: 1
            castsShadow: false
        }*/
    }
}

