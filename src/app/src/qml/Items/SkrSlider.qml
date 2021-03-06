import QtQuick.Controls 2.15
import QtQuick.Controls.Material 2.15
import ".."

Slider {
    id: control

    Material.accent: SkrTheme.accent


    property string tip
    hoverEnabled: true

    SkrToolTip {
        text: control.tip
        visible: control.hovered && tip.length !== 0
    }

    snapMode: Slider.SnapOnRelease



}
