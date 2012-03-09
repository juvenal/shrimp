/*
    Copyright 2010, Romain Behar <romainbehar@users.sourceforge.net>

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


#include <QtGui>
#include <QtOpenGL>

#include <math.h>

#include "src/miscellaneous/logging.h"

#include "scene_view.h"


scene_view::scene_view (services* services_instance) :
	//QGLWidget (parent),
        opengl_view (services_instance),
    currentSize(500, 500),
    currentFont()
{
}

scene_view::~scene_view()
{
}


QSize scene_view::sizeHint() const
{
	return QSize (500, 500);
}

void scene_view::redraw()
{
    updateGL();
}


double scene_view::fit_scene()
{
    const double size = opengl_view::fit_scene(currentSize.rwidth(), currentSize.rheight());
    redraw();

    return size;
}


void scene_view::initializeGL()
{
}


void scene_view::paintGL()
{
        draw_scene (false, currentSize.rwidth(), currentSize.rheight());
}


void scene_view::resizeGL (int width, int height)
{
    currentSize = QSize(width, height);
    draw_scene(false, width, height);
}


void scene_view::drawGlText(const std::string Text, const double X, const double Y, const double Z, const double Size, const bool Bold)
{
    currentFont.setPointSizeF(Size);
    renderText(X, Y, Z, QString::fromStdString(Text), currentFont);
}


void scene_view::mousePressEvent (QMouseEvent * event)
{
    log() << aspect << "mousePressEvent" << std::endl;

    if ((event->buttons() & Qt::LeftButton) || (event->buttons() & Qt::RightButton))
    {
        mouse_any_button_down(event->x(), event->y());

        if (event->buttons() & Qt::LeftButton)
        {
            mouse_left_button_down(event->x(), event->y(), event->modifiers() & Qt::SHIFT, event->modifiers() & Qt::CTRL);
        }
        if (event->buttons() & Qt::RightButton)
        {
            mouse_right_button_down();
        }

        redraw();
    }
}


void scene_view::mouseMoveEvent (QMouseEvent * event)
{
    log() << aspect << "mouseMoveEvent" << std::endl;

    mouse_any_button_drag(event->x(), event->y());

    if (event->buttons() & Qt::LeftButton)
    {
        mouse_left_button_drag(currentSize.width(), currentSize.height(), event->modifiers() & Qt::ALT);
    }

    redraw();
}


void scene_view::mouseReleaseEvent(QMouseEvent * event)
{
    log() << aspect << "mouseReleaseEvent" << std::endl;

    if (event->buttons() & Qt::LeftButton)
    {
        mouse_left_button_release(event->x(), event->y(), event->modifiers() & Qt::SHIFT, event->modifiers() & Qt::CTRL, event->modifiers() & Qt::ALT);
    }

    redraw();
}


void scene_view::wheelEvent(QWheelEvent * event)
{
    int wheel_move = event->delta();
    mouse_wheel_update(static_cast<double>(wheel_move) / 100);
    redraw();
}

