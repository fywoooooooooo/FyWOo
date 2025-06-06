import QtQuick
import QtQuick.Controls
import QtGraphs
import QtQuick3D 6.0

Item {
    id: root
    anchors.fill: parent

    // 数据模型：每一项包含 time、type、value
    ListModel {
        id: dataModel
        // 由C++端动态填充
    }

    property var rowLabels: []

    function updateData(data) {
        barsGraph.updateData(data)
    }

    Bars3D {
        id: barsGraph
        anchors.fill: parent
        Component.onCompleted: {
            barsGraph.cameraPreset = 7 // isometricLeft
        }

        Bar3DSeries {
            id: barSeries
            itemLabelFormat: "@colLabel, @rowLabel: @valueLabel"
            baseColor: "#4caf50"
            ItemModelBarDataProxy {
                itemModel: dataModel
                rowRole: "time"
                columnRole: "type"
                valueRole: "value"
            }
        }

        rowAxis: Category3DAxis {
            id: rowAxis
            title: "时间"
            titleVisible: true
            labels: root.rowLabels
        }

        columnAxis: Category3DAxis {
            id: columnAxis
            title: "数据类型"
            titleVisible: true
            labels: ["CPU", "GPU", "内存", "磁盘"]
        }

        valueAxis: Value3DAxis {
            title: "使用率/速率"
            titleVisible: true
            min: 0
            max: 100
            labelFormat: "%.1f"
        }

        // 相机控制
        function resetCamera() {
            barsGraph.cameraPreset = 1 // front
        }
        function setTopView() {
            barsGraph.cameraPreset = 5 // top
        }
        function setFrontView() {
            barsGraph.cameraPreset = 1 // front
        }
        function setSideView() {
            barsGraph.cameraPreset = 9 // leftSide
        }

        // 数据更新函数，C++端调用
        function updateData(data) {
            dataModel.clear()
            var labelSet = {}
            for (var i = 0; i < data.length; i++) {
                dataModel.append(data[i])
                labelSet[data[i].time] = true
            }
            root.rowLabels = Object.keys(labelSet)
        }
    }

    function resetCamera() { barsGraph.resetCamera() }
    function setTopView() { barsGraph.setTopView() }
    function setFrontView() { barsGraph.setFrontView() }
    function setSideView() { barsGraph.setSideView() }
}