
// -----------------------------------------------------------------------------
// SLADE - It's a Doom Editor
// Copyright(C) 2008 - 2017 Simon Judd
//
// Email:       sirjuddington@gmail.com
// Web:         http://slade.mancubus.net
// Filename:    FileUtils.cpp
// Description: Various filesystem utility functions
//
// This program is free software; you can redistribute it and/or modify it
// under the terms of the GNU General Public License as published by the Free
// Software Foundation; either version 2 of the License, or (at your option)
// any later version.
//
// This program is distributed in the hope that it will be useful, but WITHOUT
// ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
// FITNESS FOR A PARTICULAR PURPOSE. See the GNU General Public License for
// more details.
//
// You should have received a copy of the GNU General Public License along with
// this program; if not, write to the Free Software Foundation, Inc.,
// 51 Franklin Street, Fifth Floor, Boston, MA  02110 - 1301, USA.
// -----------------------------------------------------------------------------


// -----------------------------------------------------------------------------
//
// Includes
//
// -----------------------------------------------------------------------------
#include "Main.h"
#include "FileUtils.h"
#include "UI/WxUtils.h"


// -----------------------------------------------------------------------------
//
// Variables
//
// -----------------------------------------------------------------------------
namespace
{
struct stat stat_buffer;
}


// -----------------------------------------------------------------------------
//
// FileUtil Namespace Functions
//
// -----------------------------------------------------------------------------


// -----------------------------------------------------------------------------
// Returns true if a file at [path] exists
// -----------------------------------------------------------------------------
bool FileUtil::fileExists(const string& path)
{
	auto result = stat(path.c_str(), &stat_buffer);
	return result == 0 && !(stat_buffer.st_mode & S_IFDIR);
}

// -----------------------------------------------------------------------------
// Removes the file at [path], returns true if successful
// -----------------------------------------------------------------------------
bool FileUtil::removeFile(const string& path)
{
	return std::remove(path.c_str()) == 0;
}

// -----------------------------------------------------------------------------
// Returns true if a directory at [path] exists
// -----------------------------------------------------------------------------
bool FileUtil::dirExists(const string& path)
{
	auto result = stat(path.c_str(), &stat_buffer);
	return result == 0 && stat_buffer.st_mode & S_IFDIR;
}

// -----------------------------------------------------------------------------
// Creates a new directory at [path] if it doesn't aleady exist.
// Returns false if the directory doesn't exist and could not be created
// -----------------------------------------------------------------------------
bool FileUtil::createDir(string_view path)
{
	// TODO: Remove wx
	auto wxpath = WxUtils::stringFromView(path);
	if (!wxDirExists(wxpath))
		return wxMkDir(wxpath) > 0; // TODO: true on success

	return true;
}

// -----------------------------------------------------------------------------
// Returns a list of all files in the directory at [path].
// If [include_subdirs] is true, it will also include all files in
// subdirectories (recursively)
// -----------------------------------------------------------------------------
vector<string> FileUtil::allFilesInDir(string_view path, bool include_subdirs)
{
	// TODO: Remove wx
	wxArrayString files;
	wxDir::GetAllFiles(
		WxUtils::stringFromView(path), &files, wxEmptyString, include_subdirs ? wxDIR_DEFAULT : wxDIR_FILES);
	return WxUtils::vectorString(files);
}
