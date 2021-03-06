
/*
    Copyright 2009-2010, Romain Behar <romainbehar@users.sourceforge.net>

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


#include "../miscellaneous/logging.h"
#include "../miscellaneous/misc_string_functions.h"

#include "opengl_view.h"


#if defined(__APPLE__) && defined (__MACH__)
    #include <OpenGL/glu.h>
#else
    #include <GL/glu.h>
#endif

#include <iostream>
#include <vector>
#include <cmath>
#include <limits>



void opengl_view::draw_grid()
{
	glShadeModel (GL_FLAT);
	glDisable (GL_LINE_SMOOTH);
	glDisable (GL_BLEND);
	glLineWidth (1.0);

	glColor3f (0.3, 0.3, 0.3);

	// grid's horizontal lines
	glBegin (GL_LINES);
		for (double i = -100; i < 100; ++i) {

			glVertex3d (-100, i, 0);
			glVertex3d (100, i, 0);
		}
	glEnd();

	// grid's vertical lines
	glBegin (GL_LINES);
		for (double i = -100; i < 100; ++i) {

			glVertex3d (i, -100, 0);
			glVertex3d (i, 100, 0);
		}
	glEnd();
}


void opengl_view::draw_scene (bool valid, int window_width, int window_height)
{
	// draw scene
	glMatrixMode (GL_PROJECTION);

	if (!valid) {

		update_projection(window_width, window_height);
	}

	glLoadIdentity();
	glViewport (0, 0, m_view_width, m_view_height);

	glOrtho (m_projection_left, m_projection_right, m_projection_bottom, m_projection_top, m_projection_near, m_projection_far);

	glEnable (GL_BLEND);
	glBlendFunc (GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

	// background color
	glClearColor( 0.2f, 0.2f, 0.2f, 1);

	glClear (GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

	glPushMatrix();
		transform_scene();

		if (m_grid) {
			draw_grid();
		}

		draw_shader(window_width, window_height);
		if (m_box_selection)
		{
			box_selection(window_width, window_height);
		}

	glPopMatrix();
}


void opengl_view::draw_shader(int window_width, int window_height)
{
	glShadeModel (GL_FLAT);
	glEnable (GL_LINE_SMOOTH);
	glEnable (GL_BLEND);
	glHint (GL_LINE_SMOOTH_HINT, GL_NICEST);
	glLineWidth (1.5);

	// compute group positions
	m_group_positions.clear();

	shrimp::group_set_t group_list = m_services->group_list();
	for (shrimp::group_set_t::const_iterator group = group_list.begin(); group != group_list.end(); ++group)
	{
		position average (0.0, 0.0);

		shrimp::shader_blocks_t blocks = m_services->get_group_blocks (*group);
		for (shrimp::shader_blocks_t::const_iterator block = blocks.begin(); block != blocks.end(); ++block)
		{
			average.position_x += (*block)->m_position_x;
			average.position_y += (*block)->m_position_y;
		}

		// compute and store group's average position
		average.position_x /= static_cast<double> (blocks.size());
		average.position_y /= static_cast<double> (blocks.size());

		m_group_positions.insert (std::make_pair (*group, average));
	}

	// draw blocks
	positions_t property_positions;
	shrimp::shader_blocks_t block_list = m_services->get_scene_blocks();
	for (shrimp::shader_blocks_t::const_iterator block_i = block_list.begin(); block_i != block_list.end(); ++block_i) {

		shader_block* block = *block_i;

		// draw blocks that don't belong to any group
		int group = m_services->get_block_group (block);
		if (!group) {
			if (m_snap_to_grid && (block == m_under_mouse_block))
			{
				double x = block->m_position_x;
				double y = block->m_position_y;

				snap_position (x, y);

				draw_block (block, x, y, property_positions);
			} else {

				draw_block (block, block->m_position_x, block->m_position_y, property_positions);
			}
		}
	}

	// draw connections
	glColor3f (0.8, 0.4, 0.4);
		shrimp::dag_t scene_dag = m_services->get_scene_dag();
		for (shrimp::dag_t::const_iterator connection = scene_dag.begin(); connection != scene_dag.end(); ++connection) {

			const shrimp::io_t to = connection->first;
			const shrimp::io_t from = connection->second;

			double to_x = 0;
			double to_y = 0;
			double from_x = 0;
			double from_y = 0;

			const positions_t::const_iterator to_property = property_positions.find (to);
			if (property_positions.end() == to_property) {

				// if the property has a parent, get its position
				const shader_block* block = m_services->get_block (to.first);
				const std::string parent = block->get_input_parent (to.second);
				if (!parent.empty()) {

					// get the parent property's position
					const positions_t::const_iterator parent_property = property_positions.find (shrimp::io_t (to.first, parent));
					if (property_positions.end() == parent_property) {

						log() << error << "parent property '" << parent << "' not found in block '" << to.first << "'" << std::endl;
					} else {
						to_x = parent_property->second.position_x;
						to_y = parent_property->second.position_y;
					}

				} else {

					// property not found, may be part of a group or rolled block
					const int block_group = m_services->get_block_group (block);
					const bool rolled_block = m_services->is_rolled (block);

					if (block_group) {

						// set group's position
						group_position_t::const_iterator p = m_group_positions.find (block_group);
						if (p == m_group_positions.end()) {
							log() << error << "group '" << block_group << "' not found." << std::endl;
							continue;
						}

						to_x = p->second.position_x;
						to_y = p->second.position_y;

					} else if (rolled_block) {

						to_x = block->m_position_x + block->m_width / 2;
						to_y = block->m_position_y - block->m_width / 2;

					} else {

						log() << error << "start property '" << to.first << "-" << to.second << "' not found." << std::endl;
						continue;
					}

				}
			}
			else {
				to_x = to_property->second.position_x;
				to_y = to_property->second.position_y;
			}

			const positions_t::const_iterator from_property = property_positions.find (from);
			if (property_positions.end() == from_property) {

				const shader_block* block = m_services->get_block (from.first);

				// property not found, may be part of a group or rolled block
				const int block_group = m_services->get_block_group (block);
				const bool rolled_block = m_services->is_rolled (block);

				if (block_group) {

					// set group's position
					group_position_t::const_iterator p = m_group_positions.find (block_group);
					if (p == m_group_positions.end()) {
						log() << error << "group '" << block_group << "' not found." << std::endl;
						continue;
					}

					from_x = p->second.position_x;
					from_y = p->second.position_y;

				} else if (rolled_block) {

					from_x = block->m_position_x + block->m_width / 2;
					from_y = block->m_position_y - block->m_width / 2;

				} else {

					log() << error << "end property '" << from.first << "-" << from.second << "' not found." << std::endl;
					continue;
				}
			}
			else
			{
				from_x = from_property->second.position_x;
				from_y = from_property->second.position_y;
			}

//			// draw a line between property centers
//			glVertex3d (to_x, to_y, 0);
//			glVertex3d (from_x, from_y, 0);
			// draw a spline between property centers
			GLfloat ctrlpoints[6][3] = {
				{ from_x+0.1,from_y,0.0}, {from_x+0.25,from_y,0.0},{from_x+1.0,from_y,0.0},
				{ to_x-1.1,to_y,0.0}, {to_x-0.35,to_y,0.0}, {to_x-0.1,to_y,0.0}

			};
			glMap1f(GL_MAP1_VERTEX_3, 0.0, 1.0, 3, 6, &ctrlpoints[0][0]);
			glEnable(GL_MAP1_VERTEX_3);
			glBegin(GL_LINE_STRIP);
			for (int i = 0; i <= 30; i ++) {
				glEvalCoord1f((GLfloat) i/30.0);
			}
			glEnd();
			glDisable(GL_MAP1_VERTEX_3);

			// draw arrow heads
			glBegin(GL_TRIANGLES);
			glVertex3f( to_x-0.1, to_y, 0.0);
			glVertex3f( to_x-0.18, to_y+0.08, 0.0);
			glVertex3f( to_x-0.18, to_y-0.08, 0.0);
			glVertex3f( to_x-0.1, to_y, 0.0);
			glEnd();
		}

	// draw groups
	draw_groups();

	// draw connection in progress
	if (m_connection_start.first != "")
	{
		const positions_t::const_iterator start_property = property_positions.find (m_connection_start);
		if (property_positions.end() == start_property) {

			log() << error << "connection start property '" << m_connection_start.first << "-" << m_connection_start.second << "' not found." << std::endl;
		}
		else {
			// draw a line between the connection start and mouse pointer
			glColor3f (0.8, 0.0, 0.0);
			glLineStipple(3, 0xAAAA);
			glEnable(GL_LINE_STIPPLE);
			glPushMatrix();
			glMatrixMode (GL_PROJECTION);
			glLoadIdentity();
			gluOrtho2D (0, static_cast<float> (window_width), 0, static_cast<float> (window_height));

			// draw a spline between property centers
			double to_x = static_cast<float> (m_current_mouse_x);
			double to_y = static_cast<float> (window_height) - static_cast<float> (m_current_mouse_y);
			double from_x = static_cast<float> (m_connection_start_x);
			double from_y = static_cast<float> (window_height) - static_cast<float> (m_connection_start_y);


			GLfloat ctrlpoints[6][3] = {
				{ from_x,from_y,0.0}, {from_x+25,from_y,0.0},{from_x+100,from_y,0.0},
				{ to_x-110,to_y,0.0}, {to_x-35,to_y,0.0}, {to_x,to_y,0.0}
			};
			glMap1f(GL_MAP1_VERTEX_3, 0.0, 1.0, 3, 6, &ctrlpoints[0][0]);
			glEnable(GL_MAP1_VERTEX_3);
			glBegin(GL_LINE_STRIP);

			for (int i = 0; i <= 30; i ++) {
				glEvalCoord1f((GLfloat) i/30.0);
			}

			glEnd();

			glPopMatrix();
			glDisable(GL_MAP1_VERTEX_3);
			glDisable(GL_LINE_STIPPLE);
		}
	}
}


void opengl_view::draw_block (shader_block* Block, const double X, const double Y, positions_t& PropertyPositions)
{
	if (!m_services->is_rolled (Block))
	{
		draw_block_body (Block, X, Y);
		draw_block_name (Block, X, Y);
		draw_block_properties (Block, X, Y, PropertyPositions);
	}
	else
	{
		draw_rolled_block_body (Block, X, Y);
		draw_block_name (Block, X, Y);
	}
}


void opengl_view::draw_block_body (shader_block* Block, const double X, const double Y)
{
	const double width = Block->m_width;
	const unsigned long max_properties = std::max (Block->input_count(), static_cast<unsigned long>( Block->m_outputs.size() ));

	// set minimal block height
	const double height1 = m_services->is_rolled (Block) ? width : (width * (1.0 / 3.7) * static_cast<double> (max_properties));
	const double height = (height1 < m_min_block_height) ? m_min_block_height : height1;

	const double alpha = 0.5;

	// check whether the block's selected
	const bool is_selected = m_services->is_selected (Block);
	// block color
	if (is_selected)
		// selected blocks are "hover orange"
		glColor4f (1.0, 0.55, 0.0, alpha);
	else if (!Block->m_inputs.size())
		// inputs are green
		glColor4f (0.0, 1.0, 0.0, alpha);
	else
		// other ones are blue
		glColor4f (0.33, 0.47, 0.69, alpha);

	const float radius = 0.1;
	const float step = 0.3;

	float xcoord, ycoord;

	// build points array
	std::vector<float> points;

	// top left
	xcoord = X + radius;
	ycoord = Y - radius;

	for (float angle = M_PI; angle >= M_PI_2; angle -= step)
	{
		points.push_back( radius * cos(angle) + xcoord );
		points.push_back( radius * sin(angle) + ycoord );
		points.push_back( 0 );
	}

	// top right
	xcoord = X + width - radius;
	ycoord = Y - radius;

	for (float angle = M_PI_2; angle >= 0; angle -= step)
	{
		points.push_back( radius * cos(angle) + xcoord );
		points.push_back( radius * sin(angle) + ycoord );
		points.push_back( 0 );
	}

	// bottom right
	xcoord = X + width - radius;
	ycoord = Y - height + radius;

	for (float angle = 0; angle >= -M_PI_2; angle -= step)
	{
		points.push_back( radius * cos(angle) + xcoord );
		points.push_back( radius * sin(angle) + ycoord );
		points.push_back( 0 );
	}

	// bottom left
	xcoord = X + radius;
	ycoord = Y - height + radius;

	for (float angle = -M_PI_2; angle >= -M_PI; angle -= step)
	{
		points.push_back( radius * cos(angle) + xcoord );
		points.push_back( radius * sin(angle) + ycoord );
		points.push_back( 0 );
	}

	// join up for line
	points.push_back( X ); points.push_back( Y - radius ); points.push_back( 0 );

	unsigned indices = ( points.size() / 3 );

	glEnableClientState( GL_VERTEX_ARRAY );
	glVertexPointer( 3, GL_FLOAT, 0, &points[0] ); // const GLvoid *

	// body
	glDrawArrays( GL_POLYGON, 0, indices );

	// block outline (and external block text)
	if (Block == m_under_mouse_block)
	{ // mouse is over block, draw the block border as selected
		glColor3f (1.0, 0.55, 0.0);
	} else {
		glColor3f (1.0, 1.0, 1.0);
	}

	// corners
	glDrawArrays( GL_LINE_STRIP, 0, indices );

	glDisableClientState( GL_VERTEX_ARRAY );
}

void opengl_view::draw_rolled_block_body (shader_block* Block, const double X, const double Y) {

	const double width = Block->m_width;
	const unsigned long max_properties = std::max (Block->input_count(), static_cast<unsigned long>( Block->m_outputs.size() ));

	// set minimal block height
	const double height1 = m_services->is_rolled (Block) ? width : (width * (1.0 /3.7) * static_cast<double> (max_properties));
	const double height = (height1 < m_min_block_height) ? m_min_block_height : height1;

	const double alpha = 0.5;

	// check whether the block's selected
	const bool is_selected = m_services->is_selected (Block);
	// block color
	if (is_selected)
		// selected blocks are "hover orange"
		glColor4f (1.0, 0.55, 0.0, alpha);
	else if (!Block->m_inputs.size())
		// inputs are green
		glColor4f (0.0, 1.0, 0.0, alpha);
	else
		// other ones are blue
		glColor4f (0.2, 0.3, 1.0, alpha); // for rolled up blocks

	glBegin (GL_QUADS);
		glVertex3d ((2*X + width) / 2.0, Y, 0);
		glVertex3d (X + width, (2*Y - height) / 2.0, 0);
		glVertex3d ((2*X + width) / 2.0, Y - height, 0);
		glVertex3d (X, (2*Y - height) / 2.0, 0);
	glEnd();

	glColor3f (1.0, 1.0, 1.0);
	glBegin (GL_LINES);
		glVertex3d ((2*X + width) / 2.0, Y, 0);
		glVertex3d (X + width, (2*Y - height) / 2.0, 0);
		glVertex3d (X + width, (2*Y - height) / 2.0, 0);
		glVertex3d ((2*X + width) / 2.0, Y - height, 0);
		glVertex3d ((2*X + width) / 2.0, Y - height, 0);
		glVertex3d (X, (2*Y - height) / 2.0, 0);
		glVertex3d (X, (2*Y - height) / 2.0, 0);
		glVertex3d ((2*X + width) / 2.0, Y, 0);

		glVertex3d (X, (2*Y - height) / 2.0, 0);
		glVertex3d (X + width, (2*Y - height) / 2.0, 0);
	glEnd();
}


void opengl_view::draw_block_name (const shader_block* Block, const double X, const double Y)
{
	// show block name
	drawGlText (Block->name(), (float)X, (float) (Y + .05), 0, (m_font_size+0.5) * m_size * 0.75, true);
}


void opengl_view::draw_block_properties (const shader_block* Block, const double X, const double Y, positions_t& PropertyPositions, const bool Selection) {

	const double width = Block->m_width;

	// draw properties
	const double property_size = 1.0/5.0;

	// input properties
	double start_x = X - property_size / 2;
	double start_y = Y - property_size / 2;

	for (shader_block::properties_t::const_iterator input = Block->m_inputs.begin(); input != Block->m_inputs.end(); ++input) {

		if (!input->m_multi_operator_parent_name.empty()) {

			// skip it
			continue;
		}

		if (Selection) {
			glLoadName (m_property_index);
			m_property_indices.insert (std::make_pair (m_property_index, std::make_pair (Block, input->m_name)));
			++m_property_index;
		}

		std::string type = Block->input_type (input->m_name);
		bool shader_param = (Block->is_shader_parameter (input->m_name) && !Block->m_root_block);

		// show property name
		glColor4f (0.65, 0.77, 0.97, 1.0); // #a5c5f7 light blue
		drawGlText (input->m_name, start_x + property_size * 1.25, start_y - 0.135, 0, m_font_size * m_size * 0.75, false);

		if (std::make_pair (Block->name(), input->m_name) == m_active_property)
			type = "selected";

		if (input->m_multi_operator_parent_name.empty())
			draw_property (input->m_name, type, shader_param, start_x, start_y, property_size, input->is_multi_operator());

		PropertyPositions.insert (std::make_pair (shrimp::io_t (Block->name(), input->m_name), position (start_x + property_size / 2, start_y - property_size / 2)));

		start_y -= property_size * (3.0/2.0);
	}

	// output properties
	start_x = X + width - property_size / 2;
	start_y = Y - property_size / 2;
	for (shader_block::properties_t::const_iterator output = Block->m_outputs.begin(); output != Block->m_outputs.end(); ++output) {

		if (Selection) {
			glLoadName (m_property_index);
			m_property_indices.insert (std::make_pair (m_property_index, std::make_pair (Block, output->m_name)));
			++m_property_index;
		}

		std::string type = Block->output_type (output->m_name);
		bool shader_param = false;

		if (std::make_pair (Block->name(), output->m_name) == m_active_property)
			type = "selected";

		draw_property (output->m_name, type, shader_param, start_x, start_y, property_size);
		PropertyPositions.insert (std::make_pair (shrimp::io_t (Block->name(), output->m_name), position (start_x + property_size / 2, start_y - property_size / 2)));

		start_y -= property_size * (3.0/2.0);
	}

}

void opengl_view::draw_property (const std::string& Name, const std::string& Type, bool shader_param, const double X, const double Y, const double Size, const bool Multi)
{
	const double third = Size / 3.0;
	const double small = Size / 6.0;


	if ("selected" == Type) {

		// orange hover color
		glColor3f (1.0, 0.55, 0.0 );
		glBegin (GL_QUADS);
			glVertex3d (X, Y, 0);
			glVertex3d (X + Size, Y, 0);
			glVertex3d (X + Size, Y - Size, 0);
			glVertex3d (X, Y - Size, 0);
		glEnd();

		// show property name
		glColor4f (1.0, 0.55, 0.0, 1.0); // orange hover color
		drawGlText (Name, X + Size * 1.25, Y - 0.135, 0, m_font_size * m_size * 0.75, false);
	}
	else if ("colour" == Type || "color" == Type) {

		// R G B

		if (shader_param){
		// Over orange square for shader parameter
		glColor3f (1.0, 0.55, 0.0 );
		glBegin (GL_LINES);
			glVertex3d (X - small, Y + small, 0);
			glVertex3d (X + Size + small, Y + small, 0);

			glVertex3d (X + Size + small, Y + small, 0);
			glVertex3d (X + Size + small, Y - Size - small, 0);

			glVertex3d (X + Size + small, Y - Size - small, 0);
			glVertex3d (X - small, Y - Size - small, 0);

			glVertex3d (X - small, Y - Size - small, 0);
			glVertex3d (X - small, Y + small, 0);
		glEnd();
		}
		glColor3f (0.8, 0.8, 0.8);
		glBegin (GL_QUADS);
			glVertex3d (X, Y, 0);
			glVertex3d (X + Size, Y, 0);
			glVertex3d (X + Size, Y - Size, 0);
			glVertex3d (X, Y - Size, 0);
		glEnd();


		glBegin (GL_QUADS);
			glColor3f (1.0, 0.0, 0.0);
			glVertex3d (X + small , Y - small, 0);
			glVertex3d (X + Size - small, Y - small, 0);
			glVertex3d (X + Size - small, Y - third * 1.2, 0);
			glVertex3d (X + small, Y - third * 1.2, 0);

			glColor3f (0.0, 1.0, 0.0);
			glVertex3d (X + small, Y - third * 1.2, 0);
			glVertex3d (X + Size - small, Y - third * 1.2, 0);
			glVertex3d (X + Size - small, Y - 1.8 * third, 0);
			glVertex3d (X + small, Y - 1.8 * third, 0);

			glColor3f (0.0, 0.0, 1.0);
			glVertex3d (X + small, Y - 1.8 * third, 0);
			glVertex3d (X + Size - small, Y - 1.8 * third, 0);
			glVertex3d (X + Size - small, Y - 3*third + small, 0);
			glVertex3d (X + small, Y - 3*third + small, 0);
		glEnd();
	}
	else if ("point" == Type) {

		// circle

		if (shader_param){
		// Over orange square for shader parameter
		glColor3f (1.0, 0.55, 0.0 );
		glBegin (GL_LINES);
			glVertex3d (X - small, Y + small, 0);
			glVertex3d (X + Size + small, Y + small, 0);

			glVertex3d (X + Size + small, Y + small, 0);
			glVertex3d (X + Size + small, Y - Size - small, 0);

			glVertex3d (X + Size + small, Y - Size - small, 0);
			glVertex3d (X - small, Y - Size - small, 0);

			glVertex3d (X - small, Y - Size - small, 0);
			glVertex3d (X - small, Y + small, 0);
		glEnd();
		}

		glColor3f (0.45, 0.83, 0.97);
		glBegin (GL_QUADS);
			glVertex3d (X, Y, 0);
			glVertex3d (X + Size, Y, 0);
			glVertex3d (X + Size, Y - Size, 0);
			glVertex3d (X, Y - Size, 0);
		glEnd();


		glColor3f (0.03, 0.14, 0.17);
		const int sections = 16;
		const double radius = Size / 4.0;
		// draw a filled circle
		glBegin (GL_TRIANGLE_FAN);
//			glVertex3d (X, Y, 0);
			for (int i = 0; i <= sections; ++i) {
				const double angle = static_cast<double> (i) * 2 * M_PI / static_cast<double> (sections);
				glVertex3d ((X+Size/2.0) + radius * cos (angle), (Y-Size/2.0) + radius * sin (angle), 0);
			}
		glEnd();
	}
	else if ("vector" == Type)
	{
		// arrow

		if (shader_param){
		// Over orange square for shader parameter
		glColor3f (1.0, 0.55, 0.0 );
		glBegin (GL_LINES);
			glVertex3d (X - small, Y + small, 0);
			glVertex3d (X + Size + small, Y + small, 0);

			glVertex3d (X + Size + small, Y + small, 0);
			glVertex3d (X + Size + small, Y - Size - small, 0);

			glVertex3d (X + Size + small, Y - Size - small, 0);
			glVertex3d (X - small, Y - Size - small, 0);

			glVertex3d (X - small, Y - Size - small, 0);
			glVertex3d (X - small, Y + small, 0);
		glEnd();
		}

		glColor3f (0.80, 0.91, 0.31);
		glBegin (GL_QUADS);
			glVertex3d (X, Y, 0);
			glVertex3d (X + Size, Y, 0);
			glVertex3d (X + Size, Y - Size, 0);
			glVertex3d (X, Y - Size, 0);
		glEnd();


		glColor3f (0.11, 0.12, 0.09);
		glBegin (GL_LINES);
			glVertex3d (X + small, Y - Size + small, 0);
			glVertex3d (X + Size - small, Y - small, 0);

			glVertex3d (X + Size - small, Y - small, 0);
			glVertex3d (X + Size - small - third, Y - small, 0);

			glVertex3d (X + Size - small, Y - small, 0);
			glVertex3d (X + Size - small, Y - small - third, 0);
		glEnd();
	}
	else if ("normal" == Type) {

		// arrow

		if (shader_param){
		// Over orange square for shader parameter
		glColor3f (1.0, 0.55, 0.0 );
		glBegin (GL_LINES);
			glVertex3d (X - small, Y + small, 0);
			glVertex3d (X + Size + small, Y + small, 0);

			glVertex3d (X + Size + small, Y + small, 0);
			glVertex3d (X + Size + small, Y - Size - small, 0);

			glVertex3d (X + Size + small, Y - Size - small, 0);
			glVertex3d (X - small, Y - Size - small, 0);

			glVertex3d (X - small, Y - Size - small, 0);
			glVertex3d (X - small, Y + small, 0);
		glEnd();
		}
		glColor3f (0.97, 0.74, 0.28);
		glBegin (GL_QUADS);
			glVertex3d (X, Y, 0);
			glVertex3d (X + Size, Y, 0);
			glVertex3d (X + Size, Y - Size, 0);
			glVertex3d (X, Y - Size, 0);
		glEnd();


		glColor3f (0.14, 0.11, 0.04);
		glBegin (GL_LINES);

			glVertex3d ( X + Size/2, Y - Size + small, 0);
			glVertex3d ( X + Size/2, Y - small, 0 );

			glVertex3d ( X + Size/2, Y - small, 0 );
			glVertex3d ( X + Size - small, Y - small - third, 0 );

			glVertex3d ( X + Size/2, Y - small, 0 );
			glVertex3d ( X + small, Y - small - third, 0 );

			glVertex3d ( X + small, Y - Size + small, 0 );
			glVertex3d ( X + Size - small, Y - Size + small, 0 );

		glEnd();
	}
	else if ("string" == Type) {

		// two horizontal lines

		if (shader_param){
		// Over orange square for shader parameter
		glColor3f (1.0, 0.55, 0.0 );
		glBegin (GL_LINES);
			glVertex3d (X - small, Y + small, 0);
			glVertex3d (X + Size + small, Y + small, 0);

			glVertex3d (X + Size + small, Y + small, 0);
			glVertex3d (X + Size + small, Y - Size - small, 0);

			glVertex3d (X + Size + small, Y - Size - small, 0);
			glVertex3d (X - small, Y - Size - small, 0);

			glVertex3d (X - small, Y - Size - small, 0);
			glVertex3d (X - small, Y + small, 0);
		glEnd();
		}

		glColor3f (0.6, 0.6, 0.6);
		glBegin (GL_QUADS);
			glVertex3d (X, Y, 0);
			glVertex3d (X + Size, Y, 0);
			glVertex3d (X + Size, Y - Size, 0);
			glVertex3d (X, Y - Size, 0);
		glEnd();


		glColor3f (0.0, 0.0, 0.0);
		glBegin (GL_LINES);
			glVertex3d (X + small, Y - small, 0);
			glVertex3d (X + Size - small, Y - small, 0);

			glVertex3d (X + small, Y - 2*small, 0);
			glVertex3d (X + Size - small, Y - 2*small, 0);
		glEnd();
	}
	else if ("matrix" == Type) {

		// two horizontal lines

		if (shader_param){
		// Over orange square for shader parameter
		glColor3f (1.0, 0.55, 0.0 );
		glBegin (GL_LINES);
			glVertex3d (X - small, Y + small, 0);
			glVertex3d (X + Size + small, Y + small, 0);

			glVertex3d (X + Size + small, Y + small, 0);
			glVertex3d (X + Size + small, Y - Size - small, 0);

			glVertex3d (X + Size + small, Y - Size - small, 0);
			glVertex3d (X - small, Y - Size - small, 0);

			glVertex3d (X - small, Y - Size - small, 0);
			glVertex3d (X - small, Y + small, 0);
		glEnd();
		}

		glColor3f (0.79, 0.66, 0.89);
		glBegin (GL_QUADS);
			glVertex3d (X, Y, 0);
			glVertex3d (X + Size, Y, 0);
			glVertex3d (X + Size, Y - Size, 0);
			glVertex3d (X, Y - Size, 0);
		glEnd();


		glColor3f (0.0, 0.0, 0.0);
		glBegin (GL_LINES);
			glVertex3d (X + small, Y - third, 0);
			glVertex3d (X + small, Y - 2*third, 0);

			glVertex3d (X + Size - small, Y - third, 0);
			glVertex3d (X + Size - small, Y - 2*third, 0);
		glEnd();
	}
	else if ("array" == Type) {

		// Grey square

		if (shader_param){
		// Over orange square for shader parameter
		glColor3f (1.0, 0.55, 0.0 );
		glBegin (GL_LINES);
			glVertex3d (X - small, Y + small, 0);
			glVertex3d (X + Size + small, Y + small, 0);

			glVertex3d (X + Size + small, Y + small, 0);
			glVertex3d (X + Size + small, Y - Size - small, 0);

			glVertex3d (X + Size + small, Y - Size - small, 0);
			glVertex3d (X - small, Y - Size - small, 0);

			glVertex3d (X - small, Y - Size - small, 0);
			glVertex3d (X - small, Y + small, 0);
		glEnd();
		}

		glColor3f (0.8, 0.8, 0.8);
		glBegin (GL_QUADS);
			glVertex3d (X, Y, 0);
			glVertex3d (X + Size, Y, 0);
			glVertex3d (X + Size, Y - Size, 0);
			glVertex3d (X, Y - Size, 0);
		glEnd();


		glBegin (GL_QUADS);
			glColor3f (0.00, 0.00, 0.00);
			glVertex3d (X + small, Y - small, 0);
			glVertex3d (X + Size/2 - small, Y - small, 0);
			glVertex3d (X + Size/2 - small, Y - Size/2 + small, 0);
			glVertex3d (X + small, Y - Size/2 + small, 0);

			glVertex3d (X + Size/2 + small, Y - small, 0);
			glVertex3d (X + Size - small, Y - small, 0);
			glVertex3d (X + Size - small, Y - Size/2 + small, 0);
			glVertex3d (X + Size/2 + small, Y - Size/2 + small, 0);

			glVertex3d (X + small, Y - Size/2 - small, 0);
			glVertex3d (X + Size/2 - small, Y - Size/2 - small, 0);
			glVertex3d (X + Size/2 - small, Y - Size + small, 0);
			glVertex3d (X + small, Y - Size + small, 0);

			glVertex3d (X + Size/2 + small, Y - Size/2 - small, 0);
			glVertex3d (X + Size - small, Y - Size/2 - small, 0);
			glVertex3d (X + Size - small, Y - Size + small, 0);
			glVertex3d (X + Size/2 + small, Y - Size + small, 0);
		glEnd();
	}
	else { // float
		// blank

		if (shader_param){
		// Over orange square for shader parameter
		glColor3f (1.0, 0.55, 0.0 );
		glBegin (GL_LINES);
			glVertex3d (X - small, Y + small, 0);
			glVertex3d (X + Size + small, Y + small, 0);

			glVertex3d (X + Size + small, Y + small, 0);
			glVertex3d (X + Size + small, Y - Size - small, 0);

			glVertex3d (X + Size + small, Y - Size - small, 0);
			glVertex3d (X - small, Y - Size - small, 0);

			glVertex3d (X - small, Y - Size - small, 0);
			glVertex3d (X - small, Y + small, 0);
		glEnd();
		}

		glColor3f (0.8, 0.8, 0.8);
		glBegin (GL_QUADS);
			glVertex3d (X, Y, 0);
			glVertex3d (X + Size, Y, 0);
			glVertex3d (X + Size, Y - Size, 0);
			glVertex3d (X, Y - Size, 0);
		glEnd();

	}

	if (Multi) {
		glColor3f (1.0, 1.0, 0.0);
		glBegin (GL_LINES);
			glVertex3d (X - small, Y + small, 0);
			glVertex3d (X + Size + small, Y + small, 0);

			glVertex3d (X + Size + small, Y + small, 0);
			glVertex3d (X + Size + small, Y - Size - small, 0);

			glVertex3d (X + Size + small, Y - Size - small, 0);
			glVertex3d (X - small, Y - Size - small, 0);

			glVertex3d (X - small, Y - Size - small, 0);
			glVertex3d (X - small, Y + small, 0);
		glEnd();
	}

}

void opengl_view::draw_groups()
{
	const double alpha = 0.5;

	for (group_position_t::const_iterator p = m_group_positions.begin(); p != m_group_positions.end(); ++p)
	{
		const int group = p->first;
		const double x = p->second.position_x;
		const double y = p->second.position_y;


		draw_group_body (x, y,group);

		// check whether the group's selected
		// show group name
		if (group == m_under_mouse_group)
		{
			// selected group are "hover orange"
			glColor4f (1.0, 0.55, 0.0, alpha);
		}
		else
		{
			glColor3f (1, 1, 1);
		}


		std::string name = m_services->get_group_name (group);
		// position with hexagon/group radius taken into account
		drawGlText (name, x-0.3f, y+0.4f, 0, (m_font_size+0.5) * m_size * 0.75, false);
	}
}

void opengl_view::draw_group_body (int Group)
{
	group_position_t::const_iterator g = m_group_positions.find (Group);
	if (g == m_group_positions.end()) {
		log() << error << "Unknown group" << std::endl;
		return;
	}

	draw_group_body (g->second.position_x, g->second.position_y,Group);
}

void opengl_view::draw_group_body (const double X, const double Y, const int current_group)
{
	const int sections = 6;
	const double radius = 0.4;
	const double alpha = 1;

	// check whether the group's selected
	const bool is_selected = m_services->is_group_selected (current_group);
	// block color
	if (is_selected){
		// selected blocks are "hover orange"
		glColor4f (1.0, 0.55, 0.0, alpha);
	}
	else {
		// draw a filled hexagon, "hover blue"
		glColor3f (0.23, 0.37, 0.80);
	}
	glBegin (GL_TRIANGLE_FAN);
		for (int i = 0; i <= sections; ++i) {
			const double angle = static_cast<double> (i) * 2 * M_PI / static_cast<double> (sections);
			glVertex3d (X + radius * cos (angle), Y + radius * sin (angle), 0);
		}
	glEnd();

	// draw an hexagon around
	if (current_group == m_under_mouse_group)
	{
		// mouse is over a group, draw the group border as selected (orange)
		glColor4f (1.0, 0.55, 0.0, alpha);
	}
	else
	{
		// white
		glColor3f (1, 1, 1);
	}

	glBegin (GL_LINES);
		double prev_x = X + radius * cos (0);
		double prev_y = Y + radius * sin (0);
		for (int i = 1; i <= sections; ++i) {
			const double angle = static_cast<double> (i) * 2 * M_PI / static_cast<double> (sections);
			double new_x = X + radius * cos (angle);
			double new_y = Y + radius * sin (angle);
			glVertex3d (prev_x, prev_y, 0);
			glVertex3d (new_x, new_y, 0);
			prev_x = new_x;
			prev_y = new_y;
		}
	glEnd();
}



