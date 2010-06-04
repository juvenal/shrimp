
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


#include "system_functions.h"

#include "../miscellaneous/logging.h"

#include <cstdlib>
#include <iostream>

#if defined _WIN32
#else
# include <sys/stat.h>
#endif


std::string system_functions::get_user_directory()
{
	log() << aspect << "system_functions::get_user_directory()" << std::endl;

	// get user's home
	char home[1024];
	fltk::filename_absolute (home, 1024, "~");

	// check for .shrimp directory, create it if not there
	std::string shrimp = std::string (home) + ".shrimp";
	if (!fltk::filename_isdir (shrimp.c_str()))
	{
#if defined _WIN32
		mkdir (shrimp.c_str());
#else
		mkdir(shrimp.c_str(), 0777);
#endif
	}

	return shrimp;
}


std::string system_functions::get_temp_directory()
{
	// get .shrimp directory
	std::string shrimp = get_user_directory();

	// check for temp directory
	std::string temp = shrimp + "/temp";
	if (!fltk::filename_isdir(temp.c_str()))
	{
#if defined _WIN32
		mkdir(temp.c_str());
#else
		mkdir(temp.c_str(), 0777);
#endif
	}

	return temp;
}


std::string system_functions::get_absolute_path (const std::string& Path)
{
	// get user's home
	char home[1024];
	fltk::filename_absolute (home, 1024, Path.c_str());

	return std::string (home);
}


bool system_functions::execute_command (const std::string& Command)
{
	int status = system (Command.c_str());

	return status > 0;
}


