pragma ComponentBehavior: Bound
import QtQuick
import QtQuick.Shapes

Rectangle{
    required property var compassData
    property int angle: compassData.angle

    id: root
    width: 150
    height: width
    color: '#3e3e3e'

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
            model: 8

            delegate:Item{
                id: tickContainer

                required property int index

                anchors.bottom: compass.verticalCenter
                anchors.topMargin: 16
                anchors.top: compass.top
                anchors.horizontalCenter: parent.horizontalCenter

                rotation: index * 45
                transformOrigin: Item.Bottom

                Rectangle{
                    width: (parent.index%2 ? 2 : 5)
                    height: (parent.index%2 ? 10 : 12)
                    color: 'black'
                    anchors.top: parent.top
                    anchors.horizontalCenter: parent.horizontalCenter
                }
            }

        }

        Shape{
            id: arrow
            anchors.topMargin: parent.height * 0.1
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