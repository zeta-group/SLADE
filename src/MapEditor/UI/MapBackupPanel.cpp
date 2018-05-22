
// -----------------------------------------------------------------------------
// SLADE - It's a Doom Editor
// Copyright(C) 2008 - 2017 Simon Judd
//
// Email:       sirjuddington@gmail.com
// Web:         https://slade.mancubus.net
// Filename:    MapBackupPanel.cpp
// Description: User interface for selecting a map backup to restore
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
#include "Archive/Formats/WadArchive.h"
#include "Archive/Formats/ZipArchive.h"
#include "MapBackupPanel.h"
#include "UI/Canvas/MapPreviewCanvas.h"
#include "UI/Lists/ListView.h"
#include "UI/WxUtils.h"
#include "Utility/StringUtils.h"


// -----------------------------------------------------------------------------
//
// MapBackupPanel Class Functions
//
// -----------------------------------------------------------------------------


// -----------------------------------------------------------------------------
// MapBackupPanel class constructor
// -----------------------------------------------------------------------------
MapBackupPanel::MapBackupPanel(wxWindow* parent) : wxPanel{ parent, -1 }, archive_backups_{ new ZipArchive() }
{
	// Setup Sizer
	wxBoxSizer* sizer = new wxBoxSizer(wxHORIZONTAL);
	SetSizer(sizer);

	// Backups list
	sizer->Add(list_backups_ = new ListView(this, -1), 0, wxEXPAND | wxRIGHT, UI::pad());

	// Map preview
	sizer->Add(canvas_map_ = new MapPreviewCanvas(this), 1, wxEXPAND);

	// Bind events
	list_backups_->Bind(wxEVT_LIST_ITEM_SELECTED, [&](wxListEvent&) { updateMapPreview(); });

	wxPanel::Layout();
}

// -----------------------------------------------------------------------------
// Opens the map backup file for [map_name] in [archive_name] and populates the
// list
// -----------------------------------------------------------------------------
bool MapBackupPanel::loadBackups(string archive_name, string_view map_name)
{
	// Open backup file
	std::replace(archive_name.begin(), archive_name.end(), '.', '_');
	string backup_file = App::path("backups", App::Dir::User) + "/" + archive_name + "_backup.zip";
	if (!archive_backups_->open(backup_file))
		return false;

	// Get backup dir for map
	dir_current_ = archive_backups_->getDir(map_name);
	if (dir_current_ == archive_backups_->rootDir() || !dir_current_)
		return false;

	// Populate backups list
	list_backups_->ClearAll();
	list_backups_->AppendColumn("Backup Date");
	list_backups_->AppendColumn("Time");

	int            index = 0;
	vector<string> cols{ 2 };
	for (int a = dir_current_->nChildren() - 1; a >= 0; a--)
	{
		auto timestamp = dir_current_->childAt(a)->name();

		// Date
		cols[0] = StrUtil::beforeLast(timestamp, '_');

		// Time
		string time = StrUtil::afterLast(timestamp, '_');
		cols[1]     = fmt::sprintf("%s:%s:%s", time.substr(0, 2), time.substr(2, 2), time.substr(time.size() - 2, 2));

		// Add to list
		list_backups_->addItem(index++, cols);
	}

	if (list_backups_->GetItemCount() > 0)
		list_backups_->selectItem(0);

	return true;
}

// -----------------------------------------------------------------------------
// Updates the map preview with the currently selected backup
// -----------------------------------------------------------------------------
void MapBackupPanel::updateMapPreview()
{
	// Clear current preview
	canvas_map_->clearMap();

	// Check for selection
	if (list_backups_->selectedItems().empty())
		return;
	int selection = (list_backups_->GetItemCount() - 1) - list_backups_->selectedItems()[0];

	// Load map data to temporary wad
	archive_mapdata_ = std::make_unique<WadArchive>();
	auto dir         = (ArchiveTreeNode*)dir_current_->childAt(selection);
	for (unsigned a = 0; a < dir->numEntries(); a++)
		archive_mapdata_->addEntry(dir->entryAt(a), "", true);

	// Open map preview
	auto maps = archive_mapdata_->detectMaps();
	if (!maps.empty())
		canvas_map_->openMap(maps[0]);
}
