
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


#ifndef _i_system_functions_h_
#define _i_system_functions_h_

#include <vector>
#include <string>

class i_system_functions
{
public:
	// return user's data directory (home)
	virtual std::string get_user_directory() { return std::string(); }

	// return temporary directory
	virtual std::string get_temp_directory() { return std::string(); }

	// return absolute system path
	virtual std::string get_absolute_path (const std::string& Path) { return std::string(); }

	// return the content of a directory (directories and files)
	virtual std::vector<std::string> list_directory (const std::string& directory) { return std::vector<std::string>(); }

	// return true when the path is a directory
	virtual bool is_directory (const std::string& path) { return false; }

	// combine paths and files (add '/' or '\')
	virtual std::string combine_paths (const std::string& path1, const std::string& path2) { return std::string(); }

	// return file extension
	virtual std::string get_file_extension (const std::string& file) { return std::string(); }

	// execute a system command
	virtual bool execute_command (const std::string& Command) { return false; }
};


#endif // _i_system_functions_h_

