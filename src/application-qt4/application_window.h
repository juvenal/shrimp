#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include "src/application-qt4/scene_view.h"
#include "src/services.h"
#include "src/shading/preferences.h"

namespace Ui {
    class application_window;
}


// context menu pop tree structure
struct block_tree_node_t;
typedef std::vector<block_tree_node_t> block_tree_node_list_t;

struct menuBlock
{
    QAction* action;
    default_block_t block;
};

struct menuNode
{
    std::set<std::string> subDirList;
    std::vector<menuBlock> actionList;
};


class application_window : public QMainWindow
{
    Q_OBJECT

public:
    explicit application_window(QWidget *parent = 0);
    ~application_window();

public slots:
    // File menu
    void newScene();
    void openSceneFile();
    void saveScene();
    void saveSceneAsFile();

    // Edit menu
    void copy();
    void paste();
    void cut();
    void group();
    void ungroup();
    void editBlock();
    void deleteBlocks();

    // View menu
    void toggleGrid(bool);
    void toggleGridSnap(bool);

    void newBlockPopup();
    void changeZoom(int zoom);
    void fitScene();
    void updateSceneZoom(const double);
    void blockPopupMenu(QString);

protected:
    //void contextMenuEvent(QContextMenuEvent *event);
    void buildContextMenuFromBlock(QMenu&, const std::string);

private:
    Ui::application_window *ui;
    scene_view* ui_scene_view;
    services* shrimp_services;

    general_options preferences;
    general_options::renderers_t renderers;
    general_options::scenes_t scenes;

    void buildBlockSubmenu(const block_tree_node_t&, const std::string& menuNodeName);
    typedef std::map<std::string, menuNode> blockPopupMenuTree_t;
    blockPopupMenuTree_t blockPopupMenuTree;

    void setupRendererCombo();
};

#endif // MAINWINDOW_H
