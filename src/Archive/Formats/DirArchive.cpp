
// -----------------------------------------------------------------------------
// SLADE - It's a Doom Editor
// Copyright(C) 2008 - 2017 Simon Judd
//
// Email:       sirjuddington@gmail.com
// Web:         http://slade.mancubus.net
// Filename:    DirArchive.cpp
// Description: DirArchive, archive class that opens a directory and treats it
//              as an archive. All entry data is still stored in memory and only
//              written to the file system when saving the 'archive'
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
#include "DirArchive.h"
#include "General/UI.h"
#include "TextEditor/TextStyle.h"
#include "Utility/FileUtils.h"
#include "WadArchive.h"


// -----------------------------------------------------------------------------
//
// External Variables
//
// -----------------------------------------------------------------------------
EXTERN_CVAR(Bool, archive_load_data)


// -----------------------------------------------------------------------------
//
// DirArchive Class Functions
//
// -----------------------------------------------------------------------------


// -----------------------------------------------------------------------------
// DirArchive class constructor
// -----------------------------------------------------------------------------
DirArchive::DirArchive() : Archive("folder")
{
	// Setup separator character
#ifdef WIN32
	separator_ = '\\';
#else
	separator_ = '/';
#endif

	rootDir()->allowDuplicateNames(false);
}

// -----------------------------------------------------------------------------
// Reads files from the directory [filename] into the archive
// Returns true if successful, false otherwise
// -----------------------------------------------------------------------------
bool DirArchive::open(string_view filename)
{
	UI::setSplashProgressMessage("Reading directory structure");
	UI::setSplashProgress(0);
	vector<string>      files, dirs;
	DirArchiveTraverser traverser(files, dirs);
	wxDir(filename.data()).Traverse(traverser, "", wxDIR_FILES | wxDIR_DIRS);

	// Stop announcements (don't want to be announcing modification due to entries being added etc)
	setMuted(true);

	UI::setSplashProgressMessage("Reading files");
	for (unsigned a = 0; a < files.size(); a++)
	{
		UI::setSplashProgress((float)a / (float)files.size());

		// Cut off directory to get entry name + relative path
		string name = files[a];
		name.erase(0, filename.size());
		if (name[0] == separator_)
			name.erase(0, 1);

		// Create entry
		ArchiveEntry* new_entry = new ArchiveEntry(StrUtil::Path::fileNameOf(name));

		// Setup entry info
		new_entry->setLoaded(false);
		new_entry->exProp("filePath") = files[a];

		// Add entry and directory to directory tree
		auto             path = StrUtil::Path::pathOf(name);
		ArchiveTreeNode* ndir = createDir(path);
		ndir->addEntry(new_entry);
		ndir->dirEntry()->exProp("filePath") = fmt::format("{}{}", filename, path);

		// Read entry data
		new_entry->importFile(files[a]);
		new_entry->setLoaded(true);

		file_modification_times_[new_entry] = FileUtil::fileModificationTime(files[a]);

		// Detect entry type
		EntryType::detectEntryType(new_entry);

		// Unload data if needed
		if (!archive_load_data)
			new_entry->unloadData();
	}

	// Add empty directories
	for (const auto& dir : dirs)
	{
		string name = dir;
		name.erase(0, filename.size());
		if (name[0] == separator_)
			name.erase(0, 1);
		std::replace(name.begin(), name.end(), '\\', '/');
		ArchiveTreeNode* ndir                = createDir(name);
		ndir->dirEntry()->exProp("filePath") = dir;
	}

	// Set all entries/directories to unmodified
	vector<ArchiveEntry*> entry_list;
	getEntryTreeAsList(entry_list);
	for (auto& entry : entry_list)
		entry->setState(0);

	// Enable announcements
	setMuted(false);

	// Setup variables
	this->filename_ = filename.data();
	setModified(false);
	on_disk_ = true;

	UI::setSplashProgressMessage("");

	return true;
}

// -----------------------------------------------------------------------------
// Reads an archive from an ArchiveEntry (not implemented)
// -----------------------------------------------------------------------------
bool DirArchive::open(ArchiveEntry* entry)
{
	Global::error = "Cannot open Folder Archive from entry";
	return false;
}

// -----------------------------------------------------------------------------
// Reads data from a MemChunk (not implemented)
// -----------------------------------------------------------------------------
bool DirArchive::open(MemChunk& mc)
{
	Global::error = "Cannot open Folder Archive from memory";
	return false;
}

// -----------------------------------------------------------------------------
// Writes the archive to a MemChunk (not implemented)
// -----------------------------------------------------------------------------
bool DirArchive::write(MemChunk& mc, bool update)
{
	Global::error = "Cannot write Folder Archive to memory";
	return false;
}

// -----------------------------------------------------------------------------
// Writes the archive to a file (not implemented)
// -----------------------------------------------------------------------------
bool DirArchive::write(string_view filename, bool update)
{
	return true;
}

// -----------------------------------------------------------------------------
// Saves any changes to the directory to the file system
// -----------------------------------------------------------------------------
bool DirArchive::save(string_view filename)
{
	// Get flat entry list
	vector<ArchiveEntry*> entries;
	getEntryTreeAsList(entries);

	// Get entry path list
	vector<string> entry_paths;
	for (auto& entry : entries)
	{
		entry_paths.push_back(this->filename_ + entry->path(true));
		if (separator_ != '/')
			std::replace(entry_paths.back().begin(), entry_paths.back().end(), '/', separator_);
	}

	// Get current directory structure
	long                time = App::runTimer();
	vector<string>      files, dirs;
	DirArchiveTraverser traverser(files, dirs);
	wxDir               dir(this->filename_);
	dir.Traverse(traverser, "", wxDIR_FILES | wxDIR_DIRS);
	Log::info(2, fmt::format("GetAllFiles took {}ums", App::runTimer() - time));

	// Check for any files to remove
	time = App::runTimer();
	for (const auto& removed_file : removed_files_)
	{
		if (FileUtil::fileExists(removed_file))
		{
			Log::info(2, fmt::format("Removing file {}", removed_file));
			FileUtil::removeFile(removed_file);
		}
	}

	// Check for any directories to remove
	for (int a = dirs.size() - 1; a >= 0; a--)
	{
		// Check if dir path matches an existing dir
		bool found = false;
		for (const auto& entry_path : entry_paths)
		{
			if (dirs[a] == entry_path)
			{
				found = true;
				break;
			}
		}

		// Dir on disk isn't part of the archive in memory
		// (Note that this will fail if there are any untracked files in the directory)
		if (!found && wxRmdir(dirs[a]))
			Log::info(2, fmt::format("Removing directory {}", dirs[a]));
	}
	Log::info(2, fmt::format("Remove check took {}ms", App::runTimer() - time));

	// Go through entries
	vector<string> files_written;
	for (unsigned a = 0; a < entries.size(); a++)
	{
		// Check for folder
		string path = entry_paths[a];
		if (entries[a]->type() == EntryType::folderType())
		{
			// Create if needed
			if (!FileUtil::createDir(path))
			{
				Global::error = fmt::format("Unable to create directory \"{}\"", path);
				return false;
			}

			// Set unmodified
			entries[a]->exProp("filePath") = path;
			entries[a]->setState(0);

			continue;
		}

		// Check if entry needs to be (re)written
		if (entries[a]->state() == 0 && path == entries[a]->exProp("filePath").stringValueRef())
			continue;

		// Write entry to file
		if (!entries[a]->exportFile(path))
		{
			Log::warning(fmt::format("Unable to save entry {}: {}", entries[a]->name(), Global::error));
		}
		else
			files_written.push_back(path);

		// Set unmodified
		entries[a]->setState(0);
		entries[a]->exProp("filePath")       = path;
		file_modification_times_[entries[a]] = FileUtil::fileModificationTime(path);
	}

	removed_files_.clear();
	setModified(false);

	return true;
}

// -----------------------------------------------------------------------------
// Loads an entry's data from the saved copy of the archive if any
// -----------------------------------------------------------------------------
bool DirArchive::loadEntryData(ArchiveEntry* entry)
{
	if (entry->importFile(entry->exProp("filePath").stringValueRef()))
	{
		file_modification_times_[entry] = FileUtil::fileModificationTime(entry->exProp("filePath").stringValueRef());
		return true;
	}

	return false;
}

// -----------------------------------------------------------------------------
// Deletes the directory matching [path], starting from [base]. If [base] is
// NULL, the root directory is used.
// Returns false if the directory does not exist, true otherwise
//
// For DirArchive also adds all subdirs and entries to the removed files list,
// so they are ignored when checking for changes on disk
// -----------------------------------------------------------------------------
bool DirArchive::removeDir(string_view path, ArchiveTreeNode* base)
{
	// Abort if read only
	if (read_only_)
		return false;

	// Get the dir to remove
	ArchiveTreeNode* dir = getDir(path, base);

	// Check it exists (and that it isn't the root dir)
	if (!dir || dir == rootDir())
		return false;

	// Get all entries in the directory (and subdirectories)
	vector<ArchiveEntry*> entries;
	getEntryTreeAsList(entries, dir);

	// Add to removed files list
	for (auto& entry : entries)
	{
		Log::info(2, entry->exProp("filePath").stringValueRef());
		removed_files_.push_back(entry->exProp("filePath").stringValueRef());
	}

	// Do normal dir remove
	return Archive::removeDir(path, base);
}

// -----------------------------------------------------------------------------
// Renames [dir] to [new_name]. Returns false if [dir] isn't part of the
// archive, true otherwise
// -----------------------------------------------------------------------------
bool DirArchive::renameDir(ArchiveTreeNode* dir, string_view new_name)
{
	string path = dir->parent()->path();
	if (separator_ != '/')
		std::replace(path.begin(), path.end(), '/', separator_);
	renamed_dirs_.emplace_back(StrUtil::join(path, dir->name()), StrUtil::join(path, new_name));
	Log::info(2, fmt::format("RENAME {} to {}", renamed_dirs_.back().first, renamed_dirs_.back().second));

	return Archive::renameDir(dir, new_name);
}

// -----------------------------------------------------------------------------
// Adds [entry] to the end of the namespace matching [add_namespace]. If [copy]
// is true a copy of the entry is added.
// Returns the added entry or NULL if the entry is invalid
//
// Namespaces in a folder are treated the same way as a zip archive
// -----------------------------------------------------------------------------
ArchiveEntry* DirArchive::addEntry(ArchiveEntry* entry, string_view add_namespace, bool copy)
{
	// Check namespace
	if (add_namespace.empty() || add_namespace == "global")
		return Archive::addEntry(entry, 0xFFFFFFFF, nullptr, copy);

	// Get/Create namespace dir
	ArchiveTreeNode* dir = createDir(StrUtil::lower(add_namespace));

	// Add the entry to the dir
	return Archive::addEntry(entry, 0xFFFFFFFF, dir, copy);
}

// -----------------------------------------------------------------------------
// Removes [entry] from the archive. If [delete_entry] is true, the entry will
// also be deleted.
// Returns true if the removal succeeded
// -----------------------------------------------------------------------------
bool DirArchive::removeEntry(ArchiveEntry* entry)
{
	const auto& old_name = entry->exProp("filePath").stringValueRef();
	bool        success  = Archive::removeEntry(entry);
	if (success)
		removed_files_.push_back(old_name);
	return success;
}

// -----------------------------------------------------------------------------
// Renames [entry]. Returns true if the rename succeeded
// -----------------------------------------------------------------------------
bool DirArchive::renameEntry(ArchiveEntry* entry, string_view name)
{
	// Check rename won't result in duplicated name
	if (entry->parentDir()->entry(name.data()))
	{
		Global::error = fmt::format("An entry named {} already exists", name);
		return false;
	}

	const auto& old_name = entry->exProp("filePath").stringValueRef();
	bool        success  = Archive::renameEntry(entry, name);
	if (success)
		removed_files_.push_back(old_name);
	return success;
}

// -----------------------------------------------------------------------------
// Returns the mapdesc_t information about the map at [entry], if [entry] is
// actually a valid map (ie. a wad archive in the maps folder)
// -----------------------------------------------------------------------------
Archive::MapDesc DirArchive::getMapInfo(ArchiveEntry* entry)
{
	MapDesc map;

	// Check entry
	if (!checkEntry(entry))
		return map;

	// Check entry type
	if (entry->type()->formatId() != "archive_wad")
		return map;

	// Check entry directory
	if (entry->parentDir()->parent() != rootDir() || entry->parentDir()->name() != "maps")
		return map;

	// Setup map info
	map.archive = true;
	map.head    = entry;
	map.end     = entry;
	map.name    = StrUtil::upper(entry->nameNoExt());

	return map;
}

// -----------------------------------------------------------------------------
// Detects all the maps in the archive and returns a vector of information about
// them.
// -----------------------------------------------------------------------------
vector<Archive::MapDesc> DirArchive::detectMaps()
{
	vector<MapDesc> ret;

	// Get the maps directory
	ArchiveTreeNode* mapdir = getDir("maps");
	if (!mapdir)
		return ret;

	// Go through entries in map dir
	for (unsigned a = 0; a < mapdir->numEntries(); a++)
	{
		ArchiveEntry* entry = mapdir->entryAt(a);

		// Maps can only be wad archives
		if (entry->type()->formatId() != "archive_wad")
			continue;

		// Detect map format (probably kinda slow but whatever, no better way to do it really)
		int      format  = MAP_UNKNOWN;
		Archive* tempwad = new WadArchive();
		tempwad->open(entry);
		vector<MapDesc> emaps = tempwad->detectMaps();
		if (!emaps.empty())
			format = emaps[0].format;
		delete tempwad;

		// Add map description
		MapDesc md;
		md.head    = entry;
		md.end     = entry;
		md.archive = true;
		md.name    = StrUtil::upper(entry->nameNoExt());
		md.format  = format;
		ret.push_back(md);
	}

	return ret;
}

// -----------------------------------------------------------------------------
// Returns the first entry matching the search criteria in [options], or NULL if
// no matching entry was found
// -----------------------------------------------------------------------------
ArchiveEntry* DirArchive::findFirst(SearchOptions& options)
{
	// Init search variables
	ArchiveTreeNode* dir = rootDir();

	// Check for search directory (overrides namespace)
	if (options.dir)
	{
		dir = options.dir;
	}
	// Check for namespace
	else if (!options.match_namespace.empty())
	{
		dir = getDir(options.match_namespace);

		// If the requested namespace doesn't exist, return nothing
		if (!dir)
			return nullptr;
		else
			options.search_subdirs = true; // Namespace search always includes namespace subdirs
	}

	// Do default search
	SearchOptions opt   = options;
	opt.dir             = dir;
	opt.match_namespace = "";
	return Archive::findFirst(opt);
}

// -----------------------------------------------------------------------------
// Returns the last entry matching the search criteria in [options], or NULL if
// no matching entry was found
// -----------------------------------------------------------------------------
ArchiveEntry* DirArchive::findLast(SearchOptions& options)
{
	// Init search variables
	ArchiveTreeNode* dir = rootDir();

	// Check for search directory (overrides namespace)
	if (options.dir)
	{
		dir = options.dir;
	}
	// Check for namespace
	else if (!options.match_namespace.empty())
	{
		dir = getDir(options.match_namespace);

		// If the requested namespace doesn't exist, return nothing
		if (!dir)
			return nullptr;
		else
			options.search_subdirs = true; // Namespace search always includes namespace subdirs
	}

	// Do default search
	SearchOptions opt   = options;
	opt.dir             = dir;
	opt.match_namespace = "";
	return Archive::findLast(opt);
}

// -----------------------------------------------------------------------------
// Returns all entries matching the search criteria in [options]
// -----------------------------------------------------------------------------
vector<ArchiveEntry*> DirArchive::findAll(SearchOptions& options)
{
	// Init search variables
	ArchiveTreeNode*      dir = rootDir();
	vector<ArchiveEntry*> ret;

	// Check for search directory (overrides namespace)
	if (options.dir)
	{
		dir = options.dir;
	}
	// Check for namespace
	else if (!options.match_namespace.empty())
	{
		dir = getDir(options.match_namespace);

		// If the requested namespace doesn't exist, return nothing
		if (!dir)
			return ret;
		else
			options.search_subdirs = true; // Namespace search always includes namespace subdirs
	}

	// Do default search
	SearchOptions opt   = options;
	opt.dir             = dir;
	opt.match_namespace = "";
	return Archive::findAll(opt);
}

// -----------------------------------------------------------------------------
// Remember to ignore the given files until they change again
// -----------------------------------------------------------------------------
void DirArchive::ignoreChangedEntries(vector<DirEntryChange>& changes)
{
	for (auto& change : changes)
		ignored_file_changes_[change.file_path] = change;
}

// -----------------------------------------------------------------------------
// Updates entries/directories based on [changes] list
// -----------------------------------------------------------------------------
void DirArchive::updateChangedEntries(vector<DirEntryChange>& changes)
{
	bool was_modified = isModified();

	for (auto& change : changes)
	{
		ignored_file_changes_.erase(change.file_path);

		// Modified Entries
		if (change.action == DirEntryChange::UPDATED)
		{
			ArchiveEntry* entry = entryAtPath(change.entry_path);
			entry->importFile(change.file_path);
			EntryType::detectEntryType(entry);
			file_modification_times_[entry] = FileUtil::fileModificationTime(change.file_path);
		}

		// Deleted Entries
		else if (change.action == DirEntryChange::DELETED_FILE)
		{
			ArchiveEntry* entry = entryAtPath(change.entry_path);
			// If the parent directory was already removed, this entry no longer exists
			if (entry)
				removeEntry(entry);
		}

		// Deleted Directories
		else if (change.action == DirEntryChange::DELETED_DIR)
			removeDir(change.entry_path);

		// New Directory
		else if (change.action == DirEntryChange::ADDED_DIR)
		{
			string name = change.file_path;
			name.erase(0, filename_.size());
			if (name[0] == separator_)
				name.erase(0, 1);
			std::replace(name.begin(), name.end(), '\\', '/');

			ArchiveTreeNode* ndir = createDir(name);
			ndir->dirEntry()->setState(0);
			ndir->dirEntry()->exProp("filePath") = change.file_path;
		}

		// New Entry
		else if (change.action == DirEntryChange::ADDED_FILE)
		{
			string name = change.file_path;
			name.erase(0, filename_.size());
			if (name[0] == separator_)
				name.erase(0, 1);
			std::replace(name.begin(), name.end(), '\\', '/');

			// Create entry
			ArchiveEntry* new_entry = new ArchiveEntry(StrUtil::Path::fileNameOf(name));

			// Setup entry info
			new_entry->setLoaded(false);
			new_entry->exProp("filePath") = change.file_path;

			// Add entry and directory to directory tree
			ArchiveTreeNode* ndir = createDir(StrUtil::Path::pathOf(name));
			ndir->addEntry(new_entry);

			// Read entry data
			new_entry->importFile(change.file_path);
			new_entry->setLoaded(true);

			file_modification_times_[new_entry] = FileUtil::fileModificationTime(change.file_path);

			// Detect entry type
			EntryType::detectEntryType(new_entry);

			// Unload data if needed
			if (!archive_load_data)
				new_entry->unloadData();

			// Set entry not modified
			new_entry->setState(0);
		}
	}

	// Preserve old modified state
	setModified(was_modified);
}

// -----------------------------------------------------------------------------
// Returns true iff the user has previously indicated no interest in this change
// -----------------------------------------------------------------------------
bool DirArchive::shouldIgnoreEntryChange(const DirEntryChange& change)
{
	auto it = ignored_file_changes_.find(change.file_path);

	// If we've never seen this file before, definitely don't ignore the change
	if (it == ignored_file_changes_.end())
		return false;

	auto old_change = it->second;
	bool was_deleted =
		(old_change.action == DirEntryChange::DELETED_FILE || old_change.action == DirEntryChange::DELETED_DIR);
	bool is_deleted = (change.action == DirEntryChange::DELETED_FILE || change.action == DirEntryChange::DELETED_DIR);

	// Was deleted, is still deleted, nothing's changed
	if (was_deleted && is_deleted)
		return true;

	// Went from deleted to not, or vice versa; interesting
	if (was_deleted != is_deleted)
		return false;

	// Otherwise, it was modified both times, which is only interesting if the
	// mtime is different.  (You might think it's interesting if the mtime is
	// /greater/, but this is more robust against changes to the system clock,
	// and an unmodified file will never change mtime.)
	return (old_change.mtime == change.mtime);
}
