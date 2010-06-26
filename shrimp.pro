######################################################################
# Automatically generated by qmake (2.01a) dim. juin 20 22:59:11 2010
######################################################################

TEMPLATE = app
TARGET = 
DEPENDPATH += .
INCLUDEPATH += .

QT += opengl

MOC_DIR = .qmake_temp
OBJECTS_DIR = .qmake_temp


# Input
HEADERS += src/application-qt4/scene_view.h src/application-qt4/application_window.h src/application-qt4/system_functions.h
SOURCES += src/application-qt4/scene_view.cpp src/application-qt4/shrimp.cpp src/application-qt4/application_window.cpp src/application-qt4/system_functions.cpp

HEADERS += src/miscellaneous/tinyxml/tinystr.h src/miscellaneous/tinyxml/tinyxml.h
SOURCES += src/miscellaneous/tinyxml/tinystr.cpp src/miscellaneous/tinyxml/tinyxml.cpp src/miscellaneous/tinyxml/tinyxmlerror.cpp src/miscellaneous/tinyxml/tinyxmlparser.cpp

HEADERS += src/miscellaneous/logging.h src/miscellaneous/misc_xml.h
SOURCES += src/miscellaneous/logging.cpp src/miscellaneous/misc_xml.cpp

HEADERS += src/shading/preferences.h src/shading/rib_root_block.h src/shading/rib_root_block_parsing.h src/shading/scene.h src/shading/shader_block.h src/shading/shrimp_public_structures.h
SOURCES += src/shading/preferences.cpp src/shading/rib_root_block.cpp src/shading/rib_root_block_parsing.cpp src/shading/scene_blocks.cpp src/shading/scene.cpp src/shading/scene_grouping.cpp src/shading/scene_serialization.cpp src/shading/shader_block.cpp

HEADERS += src/opengl_view/opengl_view.h
SOURCES += src/opengl_view/opengl_view.cpp src/opengl_view/draw.cpp

HEADERS += src/services.h
SOURCES += src/services.cpp

