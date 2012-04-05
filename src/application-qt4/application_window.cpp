
/*
    Copyright 2010-2012, Romain Behar <romainbehar@users.sourceforge.net>

    This file is part of Shrimp 2.

    Shrimp 2 is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    Shrimp 2 is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with Shrimp 2.  If not, see <http://www.gnu.org/licenses/>.
*/


#include "application_window.h"
#include "ui_application_window.h"
#include "block_code.h"
#include "block_info.h"
#include "block_input_output.h"
#include "block_name.h"
#include "code_preview.h"
#include "options.h"
#include "rib_block.h"
#include "shader_properties.h"

#include "src/application-qt4/system_functions.h"
#include "src/miscellaneous/logging.h"
#include "src/shading/rib_root_block.h"

#include <QFileDialog>
#include <QMessageBox>
#include <QContextMenuEvent>
#include <QSignalMapper>

#include <memory>

application_window::application_window (QWidget* parent) :
    QMainWindow (parent),
    ui (new Ui::application_window)
{
    ui->setupUi (this);

    // initialize logging
    log_level_t level = ASPECT;
    std::auto_ptr<std::streambuf> filter_level (new filter_by_level_buf (level, log()));

    /*
            log() << error << "LOG = ERROR" << std::endl;
            log() << warning << "LOG = WARNING" << std::endl;
            log() << info << "LOG = INFO" << std::endl;
            log() << debug << "LOG = DEBUG" << std::endl;
            log() << aspect << "LOG = ASPECT" << std::endl;
    */

    log() << aspect << "Starting Shrimp" << std::endl;

    // initialize system functions
    systemFunctions = new system_functions();

    // load preferences
    std::string dataPath = findDataDirectory("data");
    preferences.initialize(systemFunctions, dataPath);

    // create service
    std::string blockPath = findDataDirectory("blocks");
    shrimp_services = new services(systemFunctions, preferences, blockPath);

    // initialize renderer
    renderers = preferences.get_renderer_list();
    scenes = preferences.get_scene_list();

    setupRendererCombo ("Aqsis");
    setupSceneCombo();

    // add the QGLWidget scene_view to the main window
    ui_scene_view = new scene_view(shrimp_services);
    ui->verticalLayout->addWidget(ui_scene_view);

    // build block popup menu
    menuNode menuRootNode;
    blockPopupMenuTree["Root"] = menuNode();
    const block_tree_node_t rootNode = shrimp_services->get_block_hierarchy();
    buildBlockSubmenu(rootNode, "Root");

    // initiliaze variables
    activeBlock = 0;

    // connect events
    connect (ui->actionNew, SIGNAL(triggered()), this, SLOT(newScene()));
    connect (ui->actionOpen, SIGNAL(triggered()), this, SLOT(openSceneFile()));
    connect (ui->actionSave, SIGNAL(triggered()), this, SLOT(saveScene()));
    connect (ui->actionSaveAs, SIGNAL(triggered()), this, SLOT(saveSceneAsFile()));
    connect (ui->actionShaderProperties, SIGNAL(triggered()), this, SLOT(shaderPropertiesDialog()));
    connect (ui->actionCodePreview, SIGNAL(triggered()), this, SLOT(codePreviewDialog()));
    connect (ui->actionExportScene, SIGNAL(triggered()), this, SLOT(exportScene()));
    connect (ui->actionOptions, SIGNAL(triggered()), this, SLOT(optionsDialog()));
    connect (ui->actionQuit, SIGNAL(triggered()), this, SLOT(close()));

    connect (ui->actionCopy, SIGNAL(triggered()), this, SLOT(copy()));
    connect (ui->actionPaste, SIGNAL(triggered()), this, SLOT(paste()));
    connect (ui->actionCut, SIGNAL(triggered()), this, SLOT(cut()));
    connect (ui->actionGroup, SIGNAL(triggered()), this, SLOT(group()));
    connect (ui->actionUngroup, SIGNAL(triggered()), this, SLOT(ungroup()));
    connect (ui->actionEditBlock, SIGNAL(triggered()), this, SLOT(editBlock()));
    connect (ui->actionDeleteBlocks, SIGNAL(triggered()), this, SLOT(deleteBlocks()));

    connect (ui->actionShowGrid, SIGNAL(toggled(bool)), this, SLOT(toggleGrid(bool)));
    connect (ui->actionSnapBlocks, SIGNAL(toggled(bool)), this, SLOT(toggleGridSnap(bool)));

    connect (ui->addBlockButton, SIGNAL(clicked()), this, SLOT(newBlockPopup()));
    connect (ui->rendererCombo, SIGNAL(activated(QString)), this, SLOT(changeRenderer(QString)));
    connect (ui->displayCombo, SIGNAL(activated(QString)), SLOT(changeDisplay(QString)));
    connect (ui->sceneCombo, SIGNAL(activated(QString)), SLOT(changeRenderScene(QString)));
    connect (ui->renderButton, SIGNAL(clicked()), this, SLOT(renderScene()));

    connect (ui->zoomSlider, SIGNAL(valueChanged(int)), this, SLOT(changeZoom(int)));
    connect (ui->fitSceneButton, SIGNAL(clicked()), this, SLOT(fitScene()));

    connect (ui_scene_view, SIGNAL(setSceneZoom(const double)), this, SLOT(updateSceneZoom(const double)));
    connect (ui_scene_view, SIGNAL(shaderBlockRightClick(const std::string)), this, SLOT(blockRightClick(const std::string)));
}


application_window::~application_window()
{
    delete ui;
}


std::string application_window::findDataDirectory(const std::string &directoryName)
{
    // find the given directory on system: in current path
    QString currentPath = QDir().currentPath();
    std::string dataPath = systemFunctions->combine_paths (currentPath.toStdString(), directoryName);
    if (systemFunctions->is_directory (dataPath))
    {
        log() << info << directoryName << " found in: " << dataPath << std::endl;
        return dataPath;
    }

    // in parent path
    dataPath = systemFunctions->combine_paths (currentPath.toStdString(), "..");
    dataPath = systemFunctions->combine_paths (dataPath, directoryName);
    if (systemFunctions->is_directory (dataPath))
    {
        log() << info << directoryName << " found in: " << dataPath << std::endl;
        return dataPath;
    }

    // not found
    log() << error << "Shrimp '" << directoryName << "' path not found" << std::endl;
    return "";
}


void application_window::buildBlockSubmenu(const block_tree_node_t& treeNode, const std::string& menuNodeName)
{
    // check whether the directory has children
    for (block_tree_node_list_t::const_iterator subNode = treeNode.child_nodes.begin();
         subNode != treeNode.child_nodes.end(); ++subNode)
    {
        std::string newDir = subNode->node_name;
        blockPopupMenuTree[menuNodeName].subDirList.insert(newDir);

        blockPopupMenuTree[newDir] = menuNode();

        buildBlockSubmenu(*subNode, newDir);
    }

    for (default_block_list_t::const_iterator block = treeNode.blocks.begin();
         block != treeNode.blocks.end(); ++block)
    {
        // add menu item
        QAction* blockAction = new QAction(QString::fromStdString(block->name), this);

        menuBlock blockInfo;
        blockInfo.action = blockAction;
        blockInfo.block = *block;
        blockPopupMenuTree[menuNodeName].actionList.push_back(blockInfo);
    }
}


void application_window::setupRendererCombo(const std::string& rendererName)
{
    unsigned long currentMenuNumber = 0;
    unsigned long rendererIndex = 0;

    for (general_options::renderers_t::iterator renderer = renderers.begin(); renderer != renderers.end(); ++renderer, ++currentMenuNumber)
    {
        ui->rendererCombo->addItem(QString::fromStdString(renderer->second.name));

        if (renderer->second.name == rendererName)
        {
            rendererIndex = currentMenuNumber;
        }
    }

    // select renderer and corresponding display list
    ui->rendererCombo->setCurrentIndex(rendererIndex);
    setupDisplayCombo(rendererName);
}


void application_window::setupDisplayCombo(const std::string& renderer)
{
    for (general_options::renderers_t::iterator r = renderers.begin(); r != renderers.end(); ++r)
    {
        if (r->second.name == renderer)
        {
            ui->displayCombo->clear();
            for (general_options::displays_t::const_iterator currentDisplay = r->second.displays.begin(); currentDisplay != r->second.displays.end(); ++currentDisplay)
            {
                ui->displayCombo->addItem(QString::fromStdString(*currentDisplay));
            }
        }
    }
}


void application_window::setupSceneCombo()
{
    ui->sceneCombo->clear();

    for (general_options::scenes_t::iterator scene = scenes.begin(); scene != scenes.end(); ++scene)
    {
        ui->sceneCombo->addItem(QString::fromStdString(scene->name));
    }
}


void application_window::newScene()
{
    shrimp_services->reset_scene();
    ui_scene_view->fit_scene();
}


void application_window::openSceneFile()
{
    QString path = QFileDialog::getOpenFileName(this, "Find scene file", QString(), QString());
    log() << INFO << "opening file: " << path.toStdString() << std::endl;

    std::string std_path = path.toStdString();

    bool success = shrimp_services->load(std_path);
    if (!success)
    {
        std::string message = "Could not open scene file '" + std_path + "'";
        log() << ERROR << message << std::endl;

        QMessageBox msgBox;
        msgBox.setText("Error");
        msgBox.setInformativeText(QString::fromStdString(message));
        msgBox.setStandardButtons(QMessageBox::Ok);
        msgBox.exec();

        return;
    }

    ui_scene_view->fit_scene();

    // Set scene name as window's title
    setWindowTitle(QString::fromStdString(shrimp_services->get_scene_name()));
}


void application_window::saveScene()
{
    if (!shrimp_services->save())
    {
        saveSceneAsFile();
    }
}


void application_window::saveSceneAsFile()
{
    QString path = QFileDialog::getSaveFileName(this, "Save scene as...", QString(), "Shrimp file (*.xml)");
    log() << INFO << "File save: " << path.toStdString() << std::endl;

    if (path.isEmpty())
    {
        // No file selected
        return;
    }

    // automatically add file extension
    if (!path.endsWith(".xml"))
    {
        path.append(".xml");
    }

    // check whether file already exists
    QFile saveFile(path);
    if (saveFile.exists())
    {
        QMessageBox msgBox;
        msgBox.setText("The file already exists.");
        msgBox.setInformativeText("Do you want to overwrite it?");
        msgBox.setStandardButtons(QMessageBox::Yes | QMessageBox::Cancel);
        msgBox.setDefaultButton(QMessageBox::Cancel);
        int answer = msgBox.exec();

        if (answer == QMessageBox::Cancel)
        {
            // File save cancelled
            return;
        }
    }

    shrimp_services->save_as(path.toStdString());
}


void application_window::shaderPropertiesDialog()
{
    shader_properties properties(this, shrimp_services);
    properties.exec();

    setWindowTitle(QString::fromStdString(shrimp_services->get_scene_name()));
}


void application_window::codePreviewDialog()
{
    code_preview preview (this, shrimp_services);
    preview.exec();
}


void application_window::exportScene()
{
    QString path = QFileDialog::getExistingDirectory(this, "Export scene to...", QString());
    if (path.isEmpty())
    {
        // No directory selected
        return;
    }

    // export
    log() << INFO << "Exporting file to: " << path.toStdString() << std::endl;
    shrimp_services->export_scene (path.toStdString());
}


void application_window::optionsDialog()
{
    options editPreferences (this, shrimp_services, preferences);
    editPreferences.exec();
}


void application_window::copy()
{
    shrimp_services->copy_selected_blocks(ui_scene_view->get_active_block());
}


void application_window::paste()
{
    shrimp_services->paste(ui_scene_view->get_active_block());
    ui_scene_view->redraw();
}


void application_window::cut()
{
    if (shrimp_services->selection_size() >= 1)
    {
        shrimp_services->cut_selection(ui_scene_view->get_active_block());
        ui_scene_view->redraw();
    }
}


void application_window::group()
{
    if (shrimp_services->selection_size() >= 1)
    {
        shrimp_services->group_selection();
        ui_scene_view->redraw();
    }
}


void application_window::ungroup()
{
    const int selectedGroup = ui_scene_view->get_selected_group();
    if (selectedGroup)
    {
        shrimp_services->ungroup(selectedGroup);
        ui_scene_view->redraw();
    }
    else if (shrimp_services->group_selection_size() > 0)
    {
        shrimp::group_set_t selectedGroups = shrimp_services->get_selected_groups();
        for (shrimp::group_set_t::const_iterator g = selectedGroups.begin(); g != selectedGroups.end(); ++g)
        {
            shrimp_services->ungroup (*g);
        }

        ui_scene_view->redraw();
    }
}


void application_window::editBlock()
{
    if (shrimp_services->selection_size() == 1)
    {
        shader_block* block = ui_scene_view->get_active_block();
        if (block)
        {
            block_code editCode (this, shrimp_services);
            editCode.exec();

            // toggle block selection
            shrimp_services->clear_selection();
            shrimp_services->set_block_selection(block, !shrimp_services->is_selected(block));
        }
    }
}


void application_window::deleteBlocks()
{
    shrimp_services->delete_selection();
    ui_scene_view->redraw();
}


void application_window::toggleGrid(bool checked)
{
    ui_scene_view->set_grid_state(checked);
    ui_scene_view->redraw();
}


void application_window::toggleGridSnap(bool checked)
{
    ui_scene_view->set_snap_to_grid_state(checked);
    ui_scene_view->redraw();
}


void application_window::newBlockPopup()
{
    QMenu menu(this);

    buildContextMenuFromBlock(menu, std::string("Root"));

    menu.setStyleSheet(QString("QMenu::item:selected { background-color: orange }"));

    QPoint localPos = ui->addBlockButton->pos();
    menu.exec(ui->addBlockButton->mapToGlobal(localPos));
}


void application_window::changeRenderer(const QString& rendererName)
{
    log() << aspect << "Change renderer to " << rendererName.toStdString() << std::endl;

    setupDisplayCombo(rendererName.toStdString());

    // save new renderer to preferences
    preferences.set_renderer_name (rendererName.toStdString());
    preferences.save();
}


void application_window::changeDisplay(const QString& displayName)
{
    log() << aspect << "Change display to " << displayName.toStdString() << std::endl;

    // save new display to preferences
    preferences.set_display_name (displayName.toStdString());
    preferences.save();
}


void application_window::changeRenderScene(const QString& sceneName)
{

}


void application_window::renderScene()
{
    std::string tempDir = systemFunctions->get_temp_directory();
    shrimp_services->show_preview(tempDir);
}


void application_window::changeZoom(int zoom)
{
    double value = static_cast<double>(zoom) / 20.0d;

    ui_scene_view->set_size(value);
    ui_scene_view->redraw();
}


void application_window::fitScene()
{
    ui_scene_view->fit_scene();
}


void application_window::blockPopupMenu(QString blockName)
{
    log() << aspect << "Adding new block: " << blockName.toStdString() << std::endl;

    shader_block* newBlock = shrimp_services->add_predefined_block(blockName.toStdString());
    ui_scene_view->move_block_to_view_center(newBlock);
    ui_scene_view->redraw();
}


/*void application_window::contextMenuEvent(QContextMenuEvent *event)
{
    QMenu menu(this);

    buildContextMenuFromBlock(menu, std::string("Root"));

    menu.exec(event->globalPos());
}*/


void application_window::buildContextMenuFromBlock(QMenu& menu, const std::string menuName)
{
    menuNode& node = blockPopupMenuTree[menuName];

    for (std::set<std::string>::iterator dirName = node.subDirList.begin();
         dirName != node.subDirList.end(); ++dirName)
    {
        const std::string subDirName = *dirName;
        QMenu* newSubMenu = menu.addMenu(QString::fromStdString(subDirName));
        buildContextMenuFromBlock(*newSubMenu, subDirName);
    }

    QSignalMapper* signalMapper = new QSignalMapper(this);
    for (std::vector<menuBlock>::iterator entry = node.actionList.begin();
        entry != node.actionList.end(); ++entry)
    {
        QAction* newAction = menu.addAction(QString::fromStdString(entry->block.name));
        connect(newAction, SIGNAL(triggered()), signalMapper, SLOT(map()));
        signalMapper->setMapping(newAction, QString::fromStdString(entry->block.name));
    }

    connect(signalMapper, SIGNAL(mapped(QString)), this, SLOT(blockPopupMenu(QString)));
}


void application_window::updateSceneZoom(const double newZoom)
{
    ui->zoomSlider->setValue(static_cast<int>(newZoom * 20.0d));
}


void application_window::blockRightClick (const std::string& blockName)
{
    activeBlock = shrimp_services->get_block (blockName);
    if (!activeBlock)
    {
        return;
    }

    const bool isRoot = activeBlock->m_root_block;

    QMenu menu (this);
    menu.setStyleSheet ("QMenu::item:selected { background-color: orange }");
    menu.setTitle ("Block");

    if (!isRoot)
    {
        if (shrimp_services->is_selected (activeBlock))
        {
            QAction* deselectMenuItem = menu.addAction ("Deselect");
            connect (deselectMenuItem, SIGNAL(triggered()), this, SLOT(deselectActiveBlock()));
        }
        else
        {
            QAction* selectMenuItem = menu.addAction ("Select");
            connect (selectMenuItem, SIGNAL(triggered()), this, SLOT(selectActiveBlock()));
        }

        if (shrimp_services->selection_size() > 1)
        {
            QAction* groupSelectionItem = menu.addAction ("Group selection");
            connect (groupSelectionItem, SIGNAL(triggered()), this, SLOT(groupSelection()));
        }

        if (shrimp_services->is_rolled (activeBlock))
        {
            QAction* unrollItem = menu.addAction ("Unroll");
            connect (unrollItem, SIGNAL(triggered()), this, SLOT(unrollActiveBlock()));
        }
        else
        {
            QAction* rollItem = menu.addAction ("Roll");
            connect (rollItem, SIGNAL(triggered()), this, SLOT(rollActiveBlock()));
        }
    }

    QAction* infoItem = menu.addAction ("Info");
    connect (infoItem, SIGNAL(triggered()), this, SLOT(activeBlockInfo()));

    QAction* renameItem = menu.addAction ("Rename");
    connect (renameItem, SIGNAL(triggered()), this, SLOT(renameActiveBlock()));

    if (!isRoot)
    {
        QAction* addInputItem = menu.addAction ("Add input");
        connect (addInputItem, SIGNAL(triggered()), this, SLOT(addInput()));
        QAction* addOutputItem = menu.addAction ("Add output");
        connect (addOutputItem, SIGNAL(triggered()), this, SLOT(addOutput()));
        QAction* editCodeItem = menu.addAction ("Edit code");
        connect (editCodeItem, SIGNAL(triggered()), this, SLOT(editCode()));
        QAction* deleteItem = menu.addAction ("Delete");
        connect (deleteItem, SIGNAL(triggered()), this, SLOT(deleteActiveBlock()));
    }
    else
    {
        // RIB root block functions
        rib_root_block* rib = dynamic_cast<rib_root_block*> (activeBlock);
        if (rib)
        {
            QAction* editRibItem = menu.addAction ("Edit RIB");
            connect (editRibItem, SIGNAL(triggered()), this, SLOT(editRibBlock()));
        }
    }

    menu.exec(QCursor::pos());
}


void application_window::selectActiveBlock()
{
    shrimp_services->set_block_selection (activeBlock, true);
    ui_scene_view->redraw();
}


void application_window::deselectActiveBlock()
{
    shrimp_services->set_block_selection (activeBlock, false);
    ui_scene_view->redraw();
}


void application_window::groupSelection()
{
    shrimp_services->group_selection();
    ui_scene_view->redraw();
}


void application_window::rollActiveBlock()
{
    shrimp_services->set_block_rolled_state (activeBlock, true);
    ui_scene_view->redraw();
}


void application_window::unrollActiveBlock()
{
    shrimp_services->set_block_rolled_state (activeBlock, false);
    ui_scene_view->redraw();
}


void application_window::activeBlockInfo()
{
    log() << aspect << "Edit block info" << std::endl;

    block_info editInfo (this, shrimp_services);
    editInfo.exec();
}


void application_window::renameActiveBlock()
{
    log() << aspect << "Edit block name" << std::endl;

    block_name editName (this, shrimp_services);
    editName.exec();
}


void application_window::addInput()
{
    log() << aspect << "Edit block input" << std::endl;

    block_input_output editInput (this, shrimp_services);
    editInput.exec();
}


void application_window::addOutput()
{
    log() << aspect << "Edit block output" << std::endl;

    block_input_output editOutput (this, shrimp_services);
    editOutput.exec();
}


void application_window::editCode()
{
    log() << aspect << "Edit block code" << std::endl;

    block_code editCode (this, shrimp_services);
    editCode.exec();
}


void application_window::deleteActiveBlock()
{
    QMessageBox msgBox (this);
    msgBox.setInformativeText (QString::fromStdString ("Do you really want to delete '" + activeBlock->name() + "' block?"));
    msgBox.setStandardButtons (QMessageBox::Yes | QMessageBox::Cancel);
    msgBox.setDefaultButton (QMessageBox::Cancel);

    int answer = msgBox.exec();

    if (answer == QMessageBox::Cancel)
    {
        // Delete cancelled
        return;
    }

    shrimp_services->delete_block (activeBlock->name());
}


void application_window::editRibBlock()
{
    log() << aspect << "Edit RIB block" << std::endl;

    rib_block editRIB (this, shrimp_services);
    editRIB.exec();
}

