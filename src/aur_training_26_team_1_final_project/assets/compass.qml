pragma ComponentBehavior: Bound
import QtQuick
import QtQuick.Shapes

Rectangle{
    // required property var data
    property int angle: 0

    id: root
    width: 100
    height: width
    color: '#b3ee3e'

    Rectangle{
        id: compass
        width: (root.height > root.width ? root.width : root.height)
        height: width
        radius: width / 2
        border.width: 6
        border.color: '#aaa'
        color: '#3eeeee'
        anchors.centerIn: root

        Repeater{
            model: 60

            delegate:Item{
                id: tickContainer

                required property int index

                anchors.bottom: compass.verticalCenter
                anchors.topMargin: 16
                anchors.top: compass.top
                anchors.horizontalCenter: parent.horizontalCenter

                rotation: index * 6
                transformOrigin: Item.Bottom

                Rectangle{
                    width: (parent.index%5 ? 2 : 5)
                    height: (parent.index%5 ? 10 : 12)
                    color: 'black'
                    anchors.top: parent.top
                    anchors.horizontalCenter: parent.horizontalCenter
                }
            }

        }

        Shape{
            id: arrow
            anchors.topMargin: parent.height * 0.05
            anchors.top: parent.top
            anchors.bottom: parent.verticalCenter
            anchors.horizontalCenter: parent.horizontalCenter
            rotation: root.angle
            transformOrigin: Item.Bottom

            ShapePath{
                strokeColor: 'red'
                strokeWidth: 1.2
                fillColor: 'red'

                startX: 0
                startY: arrow.height
                
                PathLine{x: 0; y: 0}
            }
        }
    }
}