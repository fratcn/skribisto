import QtQuick 2.15
import QtQuick.Layouts 1.15
import QtQuick.Controls 2.15
import Qt.labs.settings 1.1
import eu.skribisto.notelistproxymodel 1.0
import eu.skribisto.searchnotelistproxymodel 1.0
import eu.skribisto.skrusersettings 1.0
import ".."

LeftDockForm {


    SkrUserSettings {
        id: skrUserSettings
    }

    splitView.handle: Item {
        implicitHeight: 8
        RowLayout {
            anchors.fill: parent
            Rectangle {
                Layout.preferredWidth: 20
                Layout.preferredHeight: 5
                Layout.alignment: Qt.AlignHCenter | Qt.AlignVCenter
                color: "lightgrey"
            }
        }
    }



    //-----------------------------------------------------------



    Shortcut {
        id: navigationMenuShortcut
        enabled: root.enabled

    }


    //-----------------------------------------------------------




    //Menu :
    property list<Component> menuComponents:  [
        Component{
            id:  navigationDockMenuComponent
            Menu {
                id: navigationDockMenu
                objectName: "navigationDockMenu"
                title: qsTr("&Navigation dock")

                Component.onCompleted: {

                    navigationMenuShortcut.sequence = skrQMLTools.mnemonic(title)
                    navigationMenuShortcut.activated.connect(function() {
                        Globals.openSubMenuCalled(navigationDockMenu)
                    })
                }


                MenuItem {
                    text: qsTr("&Navigation")
                    onTriggered: {
                        if(Globals.compactSize){
                            leftDrawer.open()
                        }
                        navigationFrame.folded = false
                        navigation.forceActiveFocus()
                    }
                }

                MenuItem {
                    text: qsTr("&Documents")
                    onTriggered: {
                        if(Globals.compactSize){
                            leftDrawer.open()
                        }
                        documentFrame.folded = false
                        documentView.forceActiveFocus()
                    }
                }
            }
        }
    ]


    //Navigation List :
    //-----------------------------------------------------------

    PLMNoteListProxyModel {
        id: proxyModel
    }

    navigation.treeListViewProxyModel: proxyModel

    SKRSearchNoteListProxyModel {
        id: deletedNoteProxyModel
        showDeletedFilter: true
        showNotDeletedFilter: false
    }
    navigation.deletedListViewProxyModel: deletedNoteProxyModel


    //    Connections {
    //        target: Globals
    //        onOpenSheetCalled: function (projectId, paperId) {

    //           //proxyModel.setCurrentPaperId(projectId, paperId)


    //        }
    //    }








    //-----------------------------------------------------------

    //-----------------------------------------------------------


    //-----------------------------------------------------------

    //-----------------------------------------------------------
    transitions: [
        Transition {

            PropertyAnimation {
                properties: "implicitWidth"
                easing.type: Easing.InOutQuad
                duration: 500
            }
        }
    ]

    property alias settings: settings

    Settings {
        id: settings
        category: "noteOverviewLeftDock"
        property var dockSplitView
        property bool navigationFrameFolded: navigationFrame.folded
        property bool documentFrameFolded: documentFrame.folded
    }


    function setCurrentPaperId(projectId, paperId) {
        proxyModel.setCurrentPaperId(projectId, paperId)
    }


        PropertyAnimation {
            target: navigationFrame
            property: "SplitView.preferredHeight"
            duration: 500
            easing.type: Easing.InOutQuad
        }

    Connections {
        target: navigation
        function onOpenDocument(openedProjectId, openedPaperId, _projectId, _paperId) {
            Globals.openNoteInNewTabCalled(_projectId, _paperId)
        }
        }


    function loadConf(){

        navigationFrame.folded = settings.navigationFrameFolded
        documentFrame.folded = settings.documentFrameFolded
        splitView.restoreState(settings.dockSplitView)
    }

    function resetConf(){
        navigationFrame.folded = false
        documentFrame.folded = false
        splitView.restoreState("")

    }

    Component.onCompleted: {
            loadConf()
        navigation.onOpenDocumentInNewTab.connect(Globals.openNoteInNewTabCalled)
        navigation.openDocumentInNewWindow.connect(Globals.openNoteInNewWindowCalled)
        Globals.resetDockConfCalled.connect(resetConf)
    }

    Component.onDestruction: {
            settings.dockSplitView = splitView.saveState()
    }

    onEnabledChanged: {
        if(enabled){
        }

    }
}
