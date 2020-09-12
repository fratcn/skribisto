import QtQuick 2.15
import QtQuick.Controls 2.15
import QtQuick.Window 2.3
import QtQml 2.15
//import QtQuick.Dialogs 1.3
import Qt.labs.settings 1.1
import Qt.labs.platform 1.1 as LabPlatform
import eu.skribisto.plmerror 1.0
import eu.skribisto.projecthub 1.0
import "Commons"

ApplicationWindow {

    id: rootWindow
    objectName: "rootWindow"
    //visible: true
    minimumHeight: 500
    minimumWidth: 600

    onHeightChanged: Globals.height = height
    onWidthChanged: Globals.width = width

    x: settings.x
    y: settings.y
    height: settings.height
    width: settings.width

    visibility: settings.visibility
    Settings {
        id: settings
        category: "window"
        property int x: 0
        property int y: 0
        property int height: Screen.height
        property int width: Screen.width
        property int visibility: Window.Maximized
    }

    //------------------------------------------------------------------
    //---------Fullscreen---------
    //------------------------------------------------------------------


    Action {

        id: fullscreenAction
        text: qsTr("Fullscreen")
        icon {
            name: "view-fullscreen"
            height: 50
            width: 50
        }

        shortcut: StandardKey.FullScreen
        checkable: true
        onCheckedChanged: {
            Globals.fullScreenCalled(fullscreenAction.checked)
        }
    }


    //------------------------------------------------------------------
    //---------New project---------
    //------------------------------------------------------------------

    Action {

        id: newProjectAction
        text: qsTr("&New Project")
        icon {
            name: "document-new"
            height: 50
            width: 50
        }

        shortcut: StandardKey.New
        onTriggered: {
            console.log("New Project")
            Globals.showWelcomePage()
            Globals.showProjectPage()
            Globals.showNewProjectWizard()
        }



    }
    //------------------------------------------------------------------
    //---------Open project---------
    //------------------------------------------------------------------

    Action {

        id: openProjectAction
        text: qsTr("&Open Project")
        icon {
            name: "document-open"
            height: 50
            width: 50
        }

        shortcut: StandardKey.Open
        onTriggered: {
            console.log("Open Project")
            Globals.showOpenProjectDialog()

        }



    }

    //------------------------------------------------------------------
    //---------Save---------
    //------------------------------------------------------------------

    Action {

        id: saveAction
        text: qsTr("Save")
        icon {
            name: "document-save"
            height: 50
            width: 50
        }

        shortcut: StandardKey.Save
        onTriggered: {
            var projectId = plmData.projectHub().getDefaultProject()
            var error = plmData.projectHub().saveProject(projectId)

            if (error.getErrorCode() === "E_PROJECT_no_path"){
                saveAsFileDialog.open()
            }



        }
    }


    Connections {
        target: plmData.projectHub()
        function onDefaultProjectChanged(projectId){
            if (!plmData.projectHub().isProjectNotModifiedOnce(projectId)){
                saveAction.enabled = true
            }
            else{
                saveAction.enabled = false
            }
        }

    }

    Connections {
        target: plmData.projectHub()
        function onProjectNotSavedAnymore(projectId){
            if (projectId === plmData.projectHub().getDefaultProject()
                    && !plmData.projectHub().isProjectNotModifiedOnce(projectId)){
                saveAction.enabled = true
            }
        }

    }

    Connections {
        target: plmData.projectHub()
        function onProjectSaved(projectId){
            if (projectId === plmData.projectHub().getDefaultProject()){
                saveAction.enabled = false
            }
        }

    }



    //------------------------------------------------------------------
    //---------Save All---------
    //------------------------------------------------------------------

    Action {

        id: saveAllAction
        text: qsTr("Save All")
        icon {
            name: "document-save-all"
            height: 50
            width: 50
        }

        shortcut: "Ctrl+Shift+S"
        onTriggered: {
            var projectIdList = plmData.projectHub().getProjectIdList()
            var projectCount = plmData.projectHub().getProjectCount()

            var i;
            for (i = 0; i < projectCount ; i++ ){
                var projectId = projectIdList[i]
                var error = plmData.projectHub().saveProject(projectId)

                if (error.getErrorCode() === "E_PROJECT_no_path"){
                    var errorProjectId = error.getDataList()[0];
                    saveAsFileDialog.projectId = errorProjectId
                    saveAsFileDialog.open()
                }
            }
        }
    }

    Connections {
        target: plmData.projectHub()
        function onIsThereAnyLoadedProjectChanged(value){
            saveAction.enabled = value
            saveAsAction.enabled = value
            saveAllAction.enabled = value

        }

    }


    //------------------------------------------------------------------
    //---------Save As---------
    //------------------------------------------------------------------

    Action {

        id: saveAsAction
        text: qsTr("Save as...")
        icon {
            name: "document-save-as"
            height: 50
            width: 50
        }

        shortcut: StandardKey.SaveAs
        onTriggered: {
            var projectId = plmData.projectHub().getDefaultProject()
            saveACopyFileDialog.projectId = projectId
            saveACopyFileDialog.projectName = plmData.projectHub().getProjectName(projectId)
            saveAsFileDialog.open()








        }
    }
    LabPlatform.FileDialog{
        property int projectId: -2
        property string projectName: ""

        id: saveAsFileDialog
        title: qsTr("Save the \"%1\" project as ...").arg(projectName)
        modality: Qt.ApplicationModal
        folder: LabPlatform.StandardPaths.writableLocation(LabPlatform.StandardPaths.DocumentsLocation)
        fileMode: LabPlatform.FileDialog.SaveFile
        selectedNameFilter.index: 0
        nameFilters: ["Skribisto file (*.skrib)"]
        onAccepted: {

            var file = saveAsFileDialog.file.toString()
            file = file.replace(/^(file:\/{2})/,"");

            if(file.indexOf(".skrib") === -1){ // not found
                file = file + ".skrib"
            }
            if(projectId == -2){
                projectId = plmData.projectHub().getDefaultProject()
            }
            console.log("FileDialog :" , projectId)

            if(projectName == ""){
                projectName = plmData.projectHub().getProjectName(plmData.projectHub().getDefaultProject())
            }

            var error = plmData.projectHub().saveProjectAs(projectId, "skrib", file)

            if (error.getErrorCode() === "E_PROJECT_path_is_readonly"){
                // Dialog:
                pathIsReadOnlydialog.open()

            }

        }
        onRejected: {

        }
    }
    SimpleDialog {
        id: pathIsReadOnlydialog
        title: "Error"
        text: qsTr("This path is read-only, please choose another path.")
        onAccepted: saveAsFileDialog.open()
    }


    //------------------------------------------------------------------
    //---------Save A Copy---------
    //------------------------------------------------------------------


    Action {

        id: saveACopyAction
        text: qsTr("Save a Copy")
        icon {
            name: "document-save-as-template"
            height: 50
            width: 50
        }

        //shortcut: StandardKey.SaveAs
        onTriggered: {
            var projectId = plmData.projectHub().getDefaultProject()
            saveACopyFileDialog.projectId = projectId
            saveACopyFileDialog.projectName = plmData.projectHub().getProjectName(projectId)
            saveACopyFileDialog.open()








        }
    }

    LabPlatform.FileDialog{
        property int projectId: -2
        property string projectName: ""

        id: saveACopyFileDialog
        title: qsTr("Save a copy of the \"%1\" project as ...").arg(projectName)
        modality: Qt.ApplicationModal
        folder: LabPlatform.StandardPaths.writableLocation(LabPlatform.StandardPaths.DocumentsLocation)
        fileMode: LabPlatform.FileDialog.SaveFile
        selectedNameFilter.index: 0
        nameFilters: ["Skribisto file (*.skrib)"]
        onAccepted: {

            var file = saveACopyFileDialog.file.toString()
            file = file.replace(/^(file:\/{2})/,"");

            if(file.indexOf(".skrib") === -1){ // not found
                file = file + ".skrib"
            }
            if(projectId == -2){
                projectId = plmData.projectHub().getDefaultProject()
            }
            console.log("FileDialog :" , projectId)

            if(projectName == ""){
                projectName = plmData.projectHub().getProjectName(plmData.projectHub().getDefaultProject())
            }

            var error = plmData.projectHub().saveAProjectCopy(projectId, "skrib", file)

            if (error.getErrorCode() === "E_PROJECT_path_is_readonly"){
                // Dialog:
                saveACopyFileDialog.open()

            }

        }
        onRejected: {

        }
    }
    SimpleDialog {
        id: pathIsReadOnlySaveACopydialog
        title: "Error"
        text: qsTr("This path is read-only, please choose another path.")
        onAccepted: saveACopyFileDialog.open()
    }
    //    Shortcut {
    //        sequence: StandardKey.Save
    //        context: Qt.ApplicationShortcut
    //        onActivated: saveAction.trigger()

    //    }

    Shortcut {
        sequence: StandardKey.Quit
        context: Qt.ApplicationShortcut
        onActivated: Qt.quit()

    }

    // style :
    //palette.window: "white"

    // Splash screen
    //    Window {
    //        id: splash
    //        color: "transparent"
    //        title: "Splash Window"
    //        modality: Qt.ApplicationModal
    //        flags: Qt.SplashScreen
    //        property int timeoutInterval: 1000
    //        signal timeout
    //        x: (Screen.width - splashImage.width) / 2
    //        y: (Screen.height - splashImage.height) / 2
    //        width: splashImage.width
    //        height: splashImage.height

    //        Image {
    //            id: splashImage
    //            source: "qrc:/pics/skribisto.svg"
    //        }
    //        Timer {
    //            interval: splash.timeoutInterval; running: true; repeat: false
    //            onTriggered: {
    //                splash.visible = false
    //                splash.timeout()
    //            }
    //        }
    //        Component.onCompleted: splash.visible = true
    //    }

    //    Loader {
    //        id: loader
    //        asynchronous: true
    //        sourceComponent: rootPage
    ////        onLoaded: splash.visible = false
    //    }

    //    Component{
    //        id: rootPage
    RootPage {
        //window: rootWindow
        anchors.fill: parent
    }



    Connections {
        target: Globals
        function onFullScreenCalled(value) {
            console.log("fullscreen")
            if(value){
                visibility = Window.FullScreen
            }
            else {
                visibility = Window.AutomaticVisibility
            }

        }
    }



    //------------------------------------------------------------
    //------------Back up-----------------------------------
    //------------------------------------------------------------

    Action {

        id: backUpAction
        text: qsTr("Back up")
        icon {
            name: "tools-media-optical-burn-image"
            height: 50
            width: 50
        }

        //shortcut: StandardKey.SaveAs
        onTriggered: {


            var backupPaths = SkrSettings.backupSettings.paths
            var backupPathList = backupPaths.split(";")



            var projectIdList = plmData.projectHub().getProjectIdList()
            var projectCobackUpButtonunt = plmData.projectHub().getProjectCount()
            console.log("z")

            // all projects :
            var i;
            for (i = 0; i < projectCount ; i++ ){
                var projectId = projectIdList[i]

                //no backup path set
                if (backupPaths === ""){
                    //TODO: send notification, backup not configured

                    break
                }
                //no backup path set
                if (plmData.projectHub().getPath(projectId) === ""){
                    //TODO: send notification, project not yet saved once

                    break
                }

                // in all backup paths :
                var j;
                for (j = 0; j < backupPathList.length ; j++ ){
                    var path = backupPathList[j]
                    console.log("b")

                    if (path === ""){
                        //TODO: send notification
                        continue
                    }
                    console.log("c")


                    var error = plmData.projectHub().backupAProject(projectId, "skrib", path)

                    if (error.getErrorCode() === "E_PROJECT_path_is_readonly"){

                    }

                }
            }

        }
    }

    //------------------------------------------------------------
    //------------Print project-----------------------------------
    //------------------------------------------------------------
    Action {
        id: printAction
        text: qsTr("&Print")
        icon {
            name: "document-print"
            height: 50
            width: 50
        }

        shortcut: StandardKey.Print
        onTriggered: {
            Globals.showWelcomePage()
            Globals.showProjectPage()
            Globals.showPrintWizard()

        }
    }
    //------------------------------------------------------------
    //------------Import project-----------------------------------
    //------------------------------------------------------------
    Action {
        id: importAction
        text: qsTr("&Import")
        icon {
            name: "document-import"
            height: 50
            width: 50
        }

        //shortcut: StandardKey
        onTriggered: {
            Globals.showWelcomePage()
            Globals.showProjectPage()
            Globals.showImportWizard()

        }
    }
    //------------------------------------------------------------
    //------------Export project-----------------------------------
    //------------------------------------------------------------
    Action {
        id: exportAction
        text: qsTr("&Export")
        icon {
            name: "document-export"
            height: 50
            width: 50
        }

        //shortcut: StandardKey.New
        onTriggered: {
            Globals.showWelcomePage()
            Globals.showProjectPage()
            Globals.showExportWizard()

        }
    }

    //------------------------------------------------------------
    //------------Close current project-----------------------------------
    //------------------------------------------------------------

    property string defaultProjectName: ""
    Action {
        id: closeCurrentProjectAction
        text: qsTr("&Close \"%1\" project").arg(defaultProjectName)
        icon {
            name: "document-close"
            height: 50
            width: 50
        }

        shortcut: StandardKey.New
        onTriggered: {
            console.log("Close Project")
            var defaultProjectId = plmData.projectHub().getDefaultProject()
            var savedBool = plmData.projectHub().isProjectSaved(defaultProjectId)
            if(savedBool || plmData.projectHub().isProjectNotModifiedOnce(projectId)){
                plmData.projectHub().closeProject(defaultProjectId)
            }
            else{
                saveOrNotBeforeClosingProjectDialog.projectId = defaultProjectId
                saveOrNotBeforeClosingProjectDialog.projectName = plmData.projectHub().getProjectName(defaultProjectId)
                saveOrNotBeforeClosingProjectDialog.open()
            }
        }

    }

    SimpleDialog {
        property int projectId: -2
        property string projectName: ""

        id: saveOrNotBeforeClosingProjectDialog
        title: "Warning"
        text: qsTr("The project %1 is not saved. Do you want to save it before quiting ?").arg(projectName)
        standardButtons: Dialog.Save  | Dialog.Discard | Dialog.Cancel

        onRejected: {
            saveOrNotBeforeClosingProjectDialog.close()

        }

        onDiscarded: {
            plmData.projectHub().closeProject(projectId)
            saveOrNotBeforeClosingProjectDialog.close()

        }

        onAccepted: {


            var error = plmData.projectHub().saveProject(projectId)
            if (error.getErrorCode() === "E_PROJECT_no_path"){
                var errorProjectId = error.getDataList()[0];
                saveAsBeforeClosingProjectFileDialog.projectId = errorProjectId
                saveAsBeforeClosingProjectFileDialog.projectName = plmData.projectHub().getProjectName(projectId)
                saveAsBeforeClosingProjectFileDialog.open()
            }
            else {
                plmData.projectHub().closeProject(projectId)
            }
            saveOrNotBeforeClosingProjectDialog.close()





        }




    }

    LabPlatform.FileDialog{
        property int projectId: -2
        property string projectName: ""

        id: saveAsBeforeClosingProjectFileDialog
        title: qsTr("Save the %1 project as ...").arg(projectName)
        modality: Qt.ApplicationModal
        folder: LabPlatform.StandardPaths.writableLocation(LabPlatform.StandardPaths.DocumentsLocation)
        fileMode: LabPlatform.FileDialog.SaveFile
        selectedNameFilter.index: 0
        nameFilters: ["Skribisto file (*.skrib)"]
        onAccepted: {

            var file = saveAsFileDialog.file.toString()
            file = file.replace(/^(file:\/{2})/,"");

            if(file.indexOf(".skrib") === -1){ // not found
                file = file + ".skrib"
            }
            if(projectId == -2){
                projectId = plmData.projectHub().getDefaultProject()
            }
            console.log("FileDialog :" , projectId)

            if(projectName == ""){
                projectName = plmData.projectHub().getProjectName(plmData.projectHub().getDefaultProject())
            }

            var error = plmData.projectHub().saveProjectAs(projectId, "skrib", file)

            if (error.getErrorCode() === "E_PROJECT_path_is_readonly"){
                // Dialog:
                pathIsReadOnlydialog.open()

            }
            else{
                plmData.projectHub().closeProject(projectId)
                saveOrNotBeforeClosingProjectDialog.close()
            }
        }
        onRejected: {
            plmData.projectHub().closeProject(projectId)
            saveOrNotBeforeClosingProjectDialog.close()
        }
    }



    Connections{
        target: plmData.projectHub()
        function onDefaultProjectChanged(){
            defaultProjectName = plmData.projectHub().getProjectName(plmData.projectHub().getDefaultProject())
        }
    }
    Connections{
        target: plmData.projectHub()
        function onProjectNameChanged(){
            defaultProjectName = plmData.projectHub().getProjectName(plmData.projectHub().getDefaultProject())
        }
    }

    //------------------------------------------------------------
    //------------Close logic-----------------------------------
    //------------------------------------------------------------

    Action {
        id: quitAction
        text: qsTr("&Quit")
        icon {
            name: "window-close"
            height: 50
            width: 50
        }

        shortcut: StandardKey.Quit
        onTriggered: {
            console.log("Quit")

        }
    }

    onClosing: {
        console.log("quiting")


        // determine if all projects are saved


        var projectsNotSavedList = plmData.projectHub().projectsNotSaved()
        var i;
        for (i = 0; i < projectsNotSavedList.length ; i++ ){
            var projectId = projectsNotSavedList[i]

            if(plmData.projectHub().isProjectNotModifiedOnce(projectId)){
                continue
            }
            else {
                saveOrNotBeforeClosingDialog.projectId = projectId
                saveOrNotBeforeClosingDialog.projectName = plmData.projectHub().getProjectName(projectId)
                saveOrNotBeforeClosingDialog.open()
                close.accepted = false
            }

        }
        if(projectsNotSavedList.length === 0){



            // geometry
            settings.x = rootWindow.x
            settings.y = rootWindow.y
            settings.width = rootWindow.width
            settings.height = rootWindow.height
            settings.visibility = rootWindow.visibility


            close.accepted = true
        }

    }


    SimpleDialog {
        property int projectId: -2
        property string projectName: ""

        id: saveOrNotBeforeClosingDialog
        title: "Warning"
        text: qsTr("The project %1 is not saved. Do you want to save it before quiting ?").arg(projectName)
        standardButtons: Dialog.Save  | Dialog.Discard | Dialog.Cancel

        onRejected: {

        }

        onDiscarded: {
            plmData.projectHub().closeProject(projectId)

            rootWindow.close()
        }

        onAccepted: {


            var error = plmData.projectHub().saveProject(projectId)
            if (error.getErrorCode() === "E_PROJECT_no_path"){
                var errorProjectId = error.getDataList()[0];
                saveAsBeforeQuitingFileDialog.projectId = errorProjectId
                saveAsBeforeQuitingFileDialog.projectName = plmData.projectHub().getProjectName(projectId)
                saveAsBeforeQuitingFileDialog.open()
            }
            else {
                rootWindow.close()
            }




        }




    }

    LabPlatform.FileDialog{
        property int projectId: -2
        property string projectName: ""

        id: saveAsBeforeQuitingFileDialog
        title: qsTr("Save the %1 project as ...").arg(projectName)
        modality: Qt.ApplicationModal
        folder: LabPlatform.StandardPaths.writableLocation(LabPlatform.StandardPaths.DocumentsLocation)
        fileMode: LabPlatform.FileDialog.SaveFile
        selectedNameFilter.index: 0
        nameFilters: ["Skribisto file (*.skrib)"]
        onAccepted: {

            var file = saveAsFileDialog.file.toString()
            file = file.replace(/^(file:\/{2})/,"");

            if(file.indexOf(".skrib") === -1){ // not found
                file = file + ".skrib"
            }
            if(projectId == -2){
                projectId = plmData.projectHub().getDefaultProject()
            }
            console.log("FileDialog :" , projectId)

            if(projectName == ""){
                projectName = plmData.projectHub().getProjectName(plmData.projectHub().getDefaultProject())
            }

            var error = plmData.projectHub().saveProjectAs(projectId, "skrib", file)

            if (error.getErrorCode() === "E_PROJECT_path_is_readonly"){
                // Dialog:
                pathIsReadOnlydialog.open()

            }
            else{
                rootWindow.close()
            }
        }
        onRejected: {
            rootWindow.close()
        }
    }




    //------------------------------------------------------------
    //----------------------------------------------
    //------------------------------------------------------------
    property var lastFocusedItem: undefined
    onActiveFocusItemChanged: {
        if(!activeFocusItem){
            return
        }
        var item = activeFocusItem

        if(skrEditMenuSignalHub.isSubscribed(activeFocusItem.objectName)){
            console.log("activeFocusItem", activeFocusItem.objectName)
            cutAction.enabled = true
            copyAction.enabled = true
            pasteAction.enabled = true
            lastFocusedItem = item
            return
        }


        // determine if menuBar is an ancestor
        if(item.parent){
            if(item.parent.objectName === "menuBar" || item.parent.objectName === "editMenu" ){
                //console.log("menuBar is ancestor")
                return
            }
            if(item.parent.parent){
                if(item.parent.parent.objectName === "menuBar" ||item.parent.parent.objectName === "editMenu"){
                    //console.log("menuBar is ancestor")
                    return

                }
                if(item.parent.parent.parent){
                    if(item.parent.parent.parent.objectName === "menuBar" ||item.parent.parent.parent.objectName === "editMenu"){
                        //console.log("menuBar is ancestor")
                        return

                    }
                    if(item.parent.parent.parent.parent){
                        if(item.parent.parent.parent.parent.objectName === "menuBar" || item.parent.parent.parent.parent.objectName === "editMenu"){
                            //console.log("menuBar is ancestor")
                            return

                        }
                        if(item.parent.parent.parent.parent.parent){
                            if(item.parent.parent.parent.parent.parent.objectName === "menuBar" || item.parent.parent.parent.parent.parent.objectName === "editMenu"){
                                //console.log("menuBar is ancestor")
                                return

                            }

                        }
                    }

                }


            }


        }
        var itemString = item.toString()
        if(itemString.slice(0, 15) === "QQuickPopupItem"){
            //console.log("item is QQuickPopupItem")
            return

        }




        //        console.log("item", activeFocusItem)
        //        console.log("objectName", activeFocusItem.objectName)
        if(!lastFocusedItem){
            lastFocusedItem = item
        }
        if(skrEditMenuSignalHub.isSubscribed(lastFocusedItem.objectName)){
            //console.log("lastFocusedItem", lastFocusedItem.objectName)
            cutAction.enabled = true
            copyAction.enabled = true
            pasteAction.enabled = true
        }
        else {
            cutAction.enabled = false
            copyAction.enabled = false
            pasteAction.enabled = false

        }
        lastFocusedItem = item


    }

    Action {
        id: cutAction
        text: qsTr("Cut")
        shortcut: StandardKey.Cut
        icon {
            name: "edit-cut"
        }

        onTriggered: {
            skrEditMenuSignalHub.cutActionTriggered()
        }
    }
    Action {
        id: copyAction
        text: qsTr("Copy")
        shortcut: StandardKey.Copy
        icon {
            name: "edit-copy"
        }

        onTriggered: {
            skrEditMenuSignalHub.copyActionTriggered()
        }
    }
    Action {
        id: pasteAction
        text: qsTr("Paste")
        shortcut: StandardKey.Paste
        icon {
            name: "edit-paste"
        }

        onTriggered: {
            skrEditMenuSignalHub.pasteActionTriggered()
        }
    }


    //    Keys.onReleased: {
    //        if(event.key === Qt.Key_Alt){
    //            console.log("alt")
    //            Globals.showMenuBarCalled()
    //            event.accepted = true
    //        }
    //    }
    //    Shortcut{
    //        enabled: true
    //        sequence: "Alt+X"
    //        onActivated: {console.log("alt x")}

    //    }
    //    Keys.onShortcutOverride: event.accepted = ((event.modifiers & Qt.AltModifier) && event.key === Qt.Key_F)

    //    Keys.onPressed: {
    //        if ((event.modifiers & Qt.AltModifier) && event.key === Qt.Key_F){
    //            menuBar.visible = true

    //        }
    //        i

    //    }

    //    Keys.onReleased: {
    //        if ((event.modifiers & Qt.AltModifier) && event.key === Qt.Key_F){
    //            menuBar.visible = true

    //        }

    //        //        if(event.key === Qt.Key_Alt){
    //        //            console.log("alt")
    //        //            Globals.showMenuBarCalled()

    //        //            event.accepted = true
    //        //        }
    //    }




    menuBar: MenuBar {
        id: menuBar
        visible: SkrSettings.accessibilitySettings.showMenuBar

        objectName: "menuBar"
        Component.onCompleted:{
            skrEditMenuSignalHub.subscribe(menuBar.objectName)
        }

        Menu {
            id: fileMenu
            title: qsTr("&File")
            MenuItem{
                action: newProjectAction
            }
            MenuItem{
                action: openProjectAction
            }
            MenuSeparator { }
            MenuItem{
                action: printAction
            }
            MenuItem{
                action: importAction
            }
            MenuItem{
                action: exportAction
            }

            MenuSeparator { }
            MenuItem{
                action: saveAction
            }
            MenuItem{
                action: saveAsAction
            }
            MenuItem{
                action: saveACopyAction
            }
            MenuItem{
                action: saveAllAction
            }

            MenuSeparator { }
            MenuItem{
                action: closeCurrentProjectAction
            }
            MenuItem{
                action: quitAction
            }
        }
        Menu {
            id: editMenu
            objectName: "editMenu"
            title: qsTr("&Edit")


            MenuItem{
                id: cutItem
                objectName: "cutItem"
                action: cutAction
            }
            MenuItem{
                id: copyItem
                objectName: "copyItem"
                action: copyAction
            }
            MenuItem{
                id: pasteItem
                objectName: "pasteItem"
                action: pasteAction
            }

            Component.onCompleted:{
                skrEditMenuSignalHub.subscribe(editMenu.objectName)
                skrEditMenuSignalHub.subscribe(cutItem.objectName)
                skrEditMenuSignalHub.subscribe(copyItem.objectName)
                skrEditMenuSignalHub.subscribe(pasteItem.objectName)
            }

        }
        Menu {
            title: qsTr("&Help")
            Action { text: qsTr("&About") }
        }


    }



} //}

/*##^## Designer {
    D{i:0;autoSize:true;height:480;width:640}
}
 ##^##*/

