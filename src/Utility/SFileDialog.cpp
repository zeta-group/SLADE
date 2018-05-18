
// -----------------------------------------------------------------------------
// SLADE - It's a Doom Editor
// Copyright(C) 2008 - 2017 Simon Judd
//
// Email:       sirjuddington@gmail.com
// Web:         http://slade.mancubus.net
// Filename:    SFileDialog.cpp
// Description: Various file dialog related functions, to keep things consistent
//              where file open/save dialogs are used, and so that the last used
//              directory is saved correctly
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
#include "App.h"
#include "SFileDialog.h"
#include "StringUtils.h"
#include "UI/WxUtils.h"


// -----------------------------------------------------------------------------
//
// External Variables
//
// -----------------------------------------------------------------------------
EXTERN_CVAR(String, dir_last)


// -----------------------------------------------------------------------------
//
// SFileDialog Namespace Functions
//
// -----------------------------------------------------------------------------


// -----------------------------------------------------------------------------
// Shows a dialog to open a single file.
// Returns true and sets [info] if the user clicked ok, false otherwise
// -----------------------------------------------------------------------------
bool SFileDialog::openFile(
	FileInfo&   info,
	string_view caption,
	string_view extensions,
	wxWindow*   parent,
	string_view fn_default,
	int         ext_default)
{
	// Create file dialog
	wxFileDialog fd(
		parent,
		WxUtils::stringFromView(caption),
		dir_last.value,
		WxUtils::stringFromView(fn_default),
		WxUtils::stringFromView(extensions),
		wxFD_OPEN | wxFD_FILE_MUST_EXIST);

	// Select default extension
	fd.SetFilterIndex(ext_default);

	// Run the dialog
	if (fd.ShowModal() == wxID_OK)
	{
		// Set file dialog info
		StrUtil::Path fn(WxUtils::stringToView(fd.GetPath()));
		info.filenames.push_back(fn.fullPath());
		S_SET_VIEW(info.extension, fn.extension());
		S_SET_VIEW(info.path, fn.path(true));
		info.ext_index = fd.GetFilterIndex();

		// Set last dir
		dir_last = info.path;

		return true;
	}
	else
		return false;
}

// -----------------------------------------------------------------------------
// Shows a dialog to open multiple files.
// Returns true and sets [info] if the user clicked ok, false otherwise
// -----------------------------------------------------------------------------
bool SFileDialog::openFiles(
	FileInfo&   info,
	string_view caption,
	string_view extensions,
	wxWindow*   parent,
	string_view fn_default,
	int         ext_default)
{
	// Create file dialog
	wxFileDialog fd(
		parent,
		WxUtils::stringFromView(caption),
		dir_last.value,
		WxUtils::stringFromView(fn_default),
		WxUtils::stringFromView(extensions),
		wxFD_OPEN | wxFD_FILE_MUST_EXIST | wxFD_MULTIPLE);

	// Select default extension
	fd.SetFilterIndex(ext_default);

	// Run the dialog
	if (fd.ShowModal() == wxID_OK)
	{
		// Set file dialog info
		wxArrayString paths;
		fd.GetPaths(paths);
		info.filenames = WxUtils::vectorString(paths);
		wxFileName fn(paths[0]);
		info.extension = fn.GetExt();
		info.ext_index = fd.GetFilterIndex();
		info.path      = fn.GetPath(true);

		// Set last dir
		dir_last = info.path;

		return true;
	}
	else
		return false;
}

// -----------------------------------------------------------------------------
// Shows a dialog to save a single file.
// Returns true and sets [info] if the user clicked ok, false otherwise
// -----------------------------------------------------------------------------
bool SFileDialog::saveFile(
	FileInfo&   info,
	string_view caption,
	string_view extensions,
	wxWindow*   parent,
	string_view fn_default,
	int         ext_default)
{
	// Create file dialog
	wxFileDialog fd(
		parent,
		WxUtils::stringFromView(caption),
		dir_last.value,
		WxUtils::stringFromView(fn_default),
		WxUtils::stringFromView(extensions),
		wxFD_SAVE | wxFD_OVERWRITE_PROMPT);

	// Select default extension
	fd.SetFilterIndex(ext_default);

	// Run the dialog
	if (fd.ShowModal() == wxID_OK)
	{
		// Set file dialog info
		wxFileName fn(fd.GetPath());
		info.filenames.push_back(fn.GetFullPath().ToStdString());
		info.extension = fn.GetExt();
		info.ext_index = fd.GetFilterIndex();
		info.path      = fn.GetPath(true);

		// Set last dir
		dir_last = info.path;

		return true;
	}
	else
		return false;
}

// -----------------------------------------------------------------------------
// Shows a dialog to save multiple files.
// Returns true and sets [info] if the user clicked ok, false otherwise.
// This is used to replace wxDirDialog, which sucks
// -----------------------------------------------------------------------------
bool SFileDialog::saveFiles(
	FileInfo&   info,
	string_view caption,
	string_view extensions,
	wxWindow*   parent,
	int         ext_default)
{
	// Create file dialog
	wxFileDialog fd(
		parent,
		WxUtils::stringFromView(caption),
		dir_last.value,
		"ignored",
		WxUtils::stringFromView(extensions),
		wxFD_SAVE | wxFD_OVERWRITE_PROMPT);

	// Select default extension
	fd.SetFilterIndex(ext_default);

	// Run the dialog
	if (fd.ShowModal() == wxID_OK)
	{
		// Set file dialog info
		info.filenames.clear();
		info.extension = fd.GetWildcard().AfterLast('.');
		info.ext_index = fd.GetFilterIndex();
		info.path      = fd.GetDirectory();

		// Set last dir
		dir_last = info.path;

		return true;
	}
	else
		return false;
}

// -----------------------------------------------------------------------------
// Returns the wx file extension string for executable files
// (*.exe in Windows, no extension everywhere else)
// -----------------------------------------------------------------------------
string SFileDialog::executableExtensionString()
{
	if (App::platform() == App::Platform::Windows)
		return "Executable Files (*.exe)|*.exe";
	else
		return "Executable Files|*.*";
}

// -----------------------------------------------------------------------------
// Returns a platform-specific executable filename for [exe_name]
// -----------------------------------------------------------------------------
string SFileDialog::executableFileName(const string& exe_name)
{
	if (App::platform() == App::Platform::Windows)
		return exe_name + ".exe";
	else
		return exe_name;
}
