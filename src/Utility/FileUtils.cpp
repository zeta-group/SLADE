
// -----------------------------------------------------------------------------
// SLADE - It's a Doom Editor
// Copyright(C) 2008 - 2017 Simon Judd
//
// Email:       sirjuddington@gmail.com
// Web:         http://slade.mancubus.net
// Filename:    FileUtils.cpp
// Description: Various filesystem utility functions. Also includes SFile, a
//              simple safe-ish wrapper around a c-style FILE with various
//              convenience functions
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

// -----------------------------------------------------------------------------
// Copies the file at [filename] to [target_filename], overwriting it if the
// target file already exists
// -----------------------------------------------------------------------------
bool FileUtil::copyFile(string_view filename, string_view target_filename)
{
	// TODO: Remove wx
	return wxCopyFile(WxUtils::stringFromView(filename), WxUtils::stringFromView(target_filename));
}

// -----------------------------------------------------------------------------
// Writes [str] to a file at path [filename]. Will overwrite the file if it
// already exists
// -----------------------------------------------------------------------------
bool FileUtil::writeStrToFile(string_view str, string_view filename)
{
	SFile file(filename, SFile::Mode::Write);
	return file.writeStr(str);
}

// -----------------------------------------------------------------------------
// Returns the modification time of the file at [path], or 0 if the file doesn't
// exist or can't be acessed
// -----------------------------------------------------------------------------
time_t FileUtil::fileModificationTime(const string& path)
{
	auto result = stat(path.c_str(), &stat_buffer);
	if (result == 0 && !(stat_buffer.st_mode & S_IFDIR))
		return stat_buffer.st_mtime;

	return 0;
}


// -----------------------------------------------------------------------------
//
// SFile Class Functions
//
// -----------------------------------------------------------------------------


// -----------------------------------------------------------------------------
// SFile class constructor
// -----------------------------------------------------------------------------
SFile::SFile(const string& path, Mode mode)
{
	open(path, mode);
}

// -----------------------------------------------------------------------------
// Returns the current read/write position in the file
// -----------------------------------------------------------------------------
unsigned SFile::currentPos() const
{
	return handle_ ? ftell(handle_) : 0;
}

// -----------------------------------------------------------------------------
// Opens the file at [path] in [mode] (read/write/etc.)
// -----------------------------------------------------------------------------
bool SFile::open(const string& path, Mode mode)
{
	// Needs to be closed first if already open
	if (handle_)
		return false;

	switch (mode)
	{
	case Mode::ReadOnly: handle_ = fopen(path.c_str(), "rb"); break;
	case Mode::Write: handle_ = fopen(path.c_str(), "wb"); break;
	case Mode::ReadWite: handle_ = fopen(path.c_str(), "r+b"); break;
	case Mode::Append: handle_ = fopen(path.c_str(), "ab"); break;
	}

	if (handle_)
		stat(path.c_str(), &stat_);

	return handle_ != nullptr;
}

// -----------------------------------------------------------------------------
// Closes the file
// -----------------------------------------------------------------------------
void SFile::close()
{
	if (handle_)
	{
		fclose(handle_);
		handle_ = nullptr;
	}
}

// -----------------------------------------------------------------------------
// Seeks ahead by [offset] bytes from the current position
// -----------------------------------------------------------------------------
bool SFile::seek(unsigned offset) const
{
	return handle_ ? fseek(handle_, offset, SEEK_CUR) == 0 : false;
}

// -----------------------------------------------------------------------------
// Seeks to [offset] bytes from the beginning of the file
// -----------------------------------------------------------------------------
bool SFile::seekFromStart(unsigned offset) const
{
	return handle_ ? fseek(handle_, offset, SEEK_SET) == 0 : false;
}

// -----------------------------------------------------------------------------
// Seeks to [offset] bytes back from the end of the file
// -----------------------------------------------------------------------------
bool SFile::seekFromEnd(unsigned offset) const
{
	return handle_ ? fseek(handle_, offset, SEEK_END) == 0 : false;
}

// -----------------------------------------------------------------------------
// Reads [count] bytes from the file into [buffer]
// -----------------------------------------------------------------------------
bool SFile::read(void* buffer, unsigned count) const
{
	if (handle_)
		return fread(buffer, count, 1, handle_) > 0;

	return false;
}

// -----------------------------------------------------------------------------
// Reads [count] bytes from the file into a MemChunk [mc]
// (replaces the existing contents of the MemChunk)
// -----------------------------------------------------------------------------
bool SFile::read(MemChunk& mc, unsigned count) const
{
	return mc.importFileStream(*this, count);
}

// -----------------------------------------------------------------------------
// Reads [count] characters from the file into a string [str]
// (replaces the existing contents of the string)
// -----------------------------------------------------------------------------
bool SFile::read(string& str, unsigned count) const
{
	if (handle_)
	{
		str.resize(count);
		auto c = fread(&str[0], 1, count, handle_);
		str.push_back('\0');
		return c > 0;
	}

	return false;
}

// -----------------------------------------------------------------------------
// Writes [count] bytes from [buffer] to the file
// -----------------------------------------------------------------------------
bool SFile::write(const void* buffer, unsigned count) const
{
	if (handle_)
		return fwrite(buffer, count, 1, handle_) > 0;

	return false;
}

// -----------------------------------------------------------------------------
// Writes [str] to the file
// -----------------------------------------------------------------------------
bool SFile::writeStr(string_view str) const
{
	if (handle_)
		return fwrite(str.data(), 1, str.size(), handle_);

	return false;
}
