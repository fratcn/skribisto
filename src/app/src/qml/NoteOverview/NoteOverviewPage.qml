import QtQuick 2.15
import QtQml 2.15
import QtQuick.Controls 2.15
import QtQuick.Layouts 1.15
import Qt.labs.settings 1.1
import "../Commons"
import ".."


NoteOverviewPageForm {
    id: root
    property string pageType: "noteOverview"

    clip: true

    Component.onCompleted: {

    }



    //-------------------------------------------------------------
    //-------Left Dock------------------------------------------
    //-------------------------------------------------------------


    leftDockMenuGroup.visible: !Globals.compactMode && leftDockMenuButton.checked
    leftDockMenuButton.visible: !Globals.compactMode


    leftDockShowButton.onClicked: leftDrawer.isVisible ? leftDrawer.isVisible = false : leftDrawer.isVisible = true

    leftDockShowButton.icon {
        source: leftDrawer.isVisible ? "qrc:///icons/backup/go-previous.svg" : "qrc:///icons/backup/go-next.svg"
        height: 50
        width: 50
    }

    leftDockMenuButton.icon {
        source: "qrc:///icons/backup/overflow-menu.svg"
        height: 50
        width: 50
    }

    leftDockResizeButton.icon {
        source: "qrc:///icons/backup/resizecol.svg"
        height: 50
        width: 50
    }


    // compact mode :
    compactLeftDockShowButton.visible: Globals.compactMode

    compactLeftDockShowButton.onClicked: leftDrawer.open()
    compactLeftDockShowButton.icon {
        source: "qrc:///icons/backup/go-next.svg"
        height: 50
        width: 50
    }

    // resizing with leftDockResizeButton:

    property int leftDockResizeButtonFirstPressX: 0
    leftDockResizeButton.onReleased: {
        leftDockResizeButtonFirstPressX = 0
        rootSwipeView.interactive = SkrSettings.accessibilitySettings.allowSwipeBetweenTabs
    }

    leftDockResizeButton.onPressXChanged: {

        if(leftDockResizeButtonFirstPressX === 0){
            leftDockResizeButtonFirstPressX = root.mapFromItem(leftDockResizeButton, leftDockResizeButton.pressX, 0).x
        }

        var pressX = root.mapFromItem(leftDockResizeButton, leftDockResizeButton.pressX, 0).x
        var displacement = leftDockResizeButtonFirstPressX - pressX
        leftDrawerFixedWidth = leftDrawerFixedWidth - displacement
        leftDockResizeButtonFirstPressX = pressX

        if(leftDrawerFixedWidth < 300){
            leftDrawerFixedWidth = 300
        }
        if(leftDrawerFixedWidth > 600){
            leftDrawerFixedWidth = 600
        }



    }

    leftDockResizeButton.onPressed: {

        rootSwipeView.interactive = false

    }

    leftDockResizeButton.onCanceled: {

        rootSwipeView.interactive = SkrSettings.accessibilitySettings.allowSwipeBetweenTabs
        leftDockResizeButtonFirstPressX = 0

    }
    //---------------------------------------------------------



    property alias leftDock: leftDock
    property int leftDrawerFixedWidth: 300
    SKRDrawer {
        id: leftDrawer
        enabled: base.enabled
        parent: base
        widthInDockMode: leftDrawerFixedWidth
        widthInDrawerMode: 400
        height: base.height
        interactive: Globals.compactMode
        dockModeEnabled: !Globals.compactMode
        settingsCategory: "noteOverviewLeftDrawer"
        edge: Qt.LeftEdge


        LeftDock {
            id: leftDock
            anchors.fill: parent
        }

    }


    //------------------------------------------------------------
    //--------menus--------------------------------------------
    //------------------------------------------------------------


    QtObject{
        id: privateMenuObject
        property string dockUniqueId: ""
    }

    function addMenus(){
        //create dockUniqueId:
        privateMenuObject.dockUniqueId = Qt.formatDateTime(new Date(), "yyyyMMddhhmmsszzz")


        // add new menus
        var k
        for(k = 0 ; k < leftDock.menuComponents.length ; k++){

            var newMenu = leftDock.menuComponents[k].createObject(mainMenu)
            newMenu.objectName = newMenu.objectName + "-" + privateMenuObject.dockUniqueId
            mainMenu.addMenu(newMenu)

        }

        var menuCount = mainMenu.count


        // move Help menu
        menuCount = mainMenu.count
        var m;
        for(m = 0 ; m < menuCount ; m++){
            var menu = mainMenu.menuAt(m)
            if(menu.objectName === "helpMenu"){
                mainMenu.moveItem(m, mainMenu.count -1)
            }
        }

        // move bottomMenuItem
        var bottomMenuItem
        var p;
        for(p = 0 ; p < menuCount ; p++){
            var p_menu = mainMenu.itemAt(p)
            if(p_menu.objectName === "bottomMenuItem"){
                mainMenu.moveItem(p, mainMenu.count - 1)
            }
        }

    }

    function removeMenus(){

        if(mainMenu === null){
            return
        }

        var menuCount = mainMenu.count

        menuCount = mainMenu.count
        var i;
        for(i = menuCount - 1 ; i >= 0  ; i--){
            var menu1 = mainMenu.menuAt(i)

            if(!menu1){
                continue
            }

            if(menu1.objectName === "navigationDockMenu-" + privateMenuObject.dockUniqueId
                    || menu1.objectName === "toolDockMenu-" + privateMenuObject.dockUniqueId){

                mainMenu.removeMenu(menu1)

            }
        }



        privateMenuObject.dockUniqueId = ""
    }

    onEnabledChanged: {

        if(root.enabled){
            addMenus()
        }
        else{
            removeMenus()
        }
    }


    //------------------------------------------------------------
    // fullscreen :
    //------------------------------------------------------------

    property bool fullscreen_left_drawer_visible: false

    Connections {
        target: Globals
        function onFullScreenCalled(value) {
            if(value){
                //save previous conf
                fullscreen_left_drawer_visible = leftDrawer.isVisible

                leftDrawer.isVisible = false

            }
            else{
                leftDrawer.isVisible = fullscreen_left_drawer_visible

            }

        }
    }


}
