
// -----------------------------------------------------------------------------
// SLADE - It's a Doom Editor
// Copyright(C) 2008 - 2017 Simon Judd
//
// Email:       sirjuddington@gmail.com
// Web:         http://slade.mancubus.net
// Filename:    Clipboard.cpp
// Description: The SLADE Clipboard implementation
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
#include "Clipboard.h"
#include "App.h"
#include "Game/Configuration.h"
#include "Graphics/CTexture/CTexture.h"
#include "MapEditor/SLADEMap/SLADEMap.h"
#include "MapEditor/SectorBuilder.h"
#include "Utility/StringUtils.h"


// -----------------------------------------------------------------------------
//
// Variables
//
// -----------------------------------------------------------------------------
Clipboard* Clipboard::instance_ = nullptr;


// -----------------------------------------------------------------------------
//
// EntryTreeClipboardItem Class Functions
//
// -----------------------------------------------------------------------------


// -----------------------------------------------------------------------------
// EntryTreeClipboardItem class constructor
// -----------------------------------------------------------------------------
EntryTreeClipboardItem::EntryTreeClipboardItem(vector<ArchiveEntry*>& entries, vector<ArchiveTreeNode*>& dirs) :
	ClipboardItem(Type::EntryTree)
{
	// Create tree
	tree_ = std::make_unique<ArchiveTreeNode>();

	// Copy entries
	for (auto& entry : entries)
		tree_->addEntry(new ArchiveEntry(*entry));

	// Copy entries to system clipboard
	// (exports them as temp files and adds the paths to the clipboard)
	if (wxTheClipboard->Open())
	{
		wxTheClipboard->Clear();
		auto   file          = new wxFileDataObject();
		string tmp_directory = App::path("", App::Dir::Temp); // cache temp directory
		for (auto& entry : entries)
		{
			// Export to file
			string filename = StrUtil::join(tmp_directory, entry->nameNoExt(), '.', entry->type()->extension());
			entry->exportFile(filename);

			// Add to clipboard
			file->AddFile(filename);
		}
		wxTheClipboard->AddData(file);
		wxTheClipboard->Close();
	}

	// Copy dirs
	for (auto& dir : dirs)
		tree_->addChild(dir->clone());
}


// -----------------------------------------------------------------------------
//
// TextureClipboardItem Class Functions
//
// -----------------------------------------------------------------------------


// -----------------------------------------------------------------------------
// TextureClipboardItem class constructor
// -----------------------------------------------------------------------------
TextureClipboardItem::TextureClipboardItem(CTexture* texture, Archive* parent) : ClipboardItem(Type::CompositeTexture)
{
	// Create/copy texture
	texture_ = std::make_unique<CTexture>();
	texture_->copyTexture(texture);

	// Copy patch entries if possible
	for (unsigned a = 0; a < texture->nPatches(); a++)
	{
		ArchiveEntry* entry = texture->patch(a)->getPatchEntry(parent);

		// FIXME/TODO: Do something to handle patches that are defined in TEXTURES rather than a discrete entry!
		if (!entry)
			continue;

		// Don't copy patch if it has been already
		bool there = false;
		for (auto& patch_entry : patch_entries_)
		{
			if (patch_entry->name() == entry->name())
			{
				there = true;
				break;
			}
		}
		if (there)
			continue;

		// Copy patch entry
		if (entry)
			patch_entries_.emplace_back(new ArchiveEntry(*entry));
	}
}

// -----------------------------------------------------------------------------
// Returns the entry copy for the patch at [index] in the texture
// -----------------------------------------------------------------------------
ArchiveEntry* TextureClipboardItem::patchEntry(string_view patch)
{
	// Find copied patch entry with matching name
	for (auto& patch_entry : patch_entries_)
	{
		if (StrUtil::equalCI(patch_entry->nameNoExt(), patch))
			return patch_entry.get();
	}

	// Not found
	return nullptr;
}


// -----------------------------------------------------------------------------
//
// MapArchClipboardItem Class Functions
//
// -----------------------------------------------------------------------------


// -----------------------------------------------------------------------------
// MapArchClipboardItem class constructor
// -----------------------------------------------------------------------------
MapArchClipboardItem::MapArchClipboardItem() : ClipboardItem(Type::MapArch)
{
	// This needs to be defined here to avoid compilation errors
	// (because we're using unique_ptrs to forwarded types)
}

// -----------------------------------------------------------------------------
// MapArchClipboardItem class destructor
// -----------------------------------------------------------------------------
MapArchClipboardItem::~MapArchClipboardItem() = default; // See constructor comment

// -----------------------------------------------------------------------------
// Copies [lines] and all related map structures
// -----------------------------------------------------------------------------
void MapArchClipboardItem::addLines(vector<MapLine*> lines)
{
	// Get sectors and sides to copy
	vector<MapSector*> copy_sectors;
	vector<MapSide*>   copy_sides;
	for (auto& line : lines)
	{
		MapSide* s1 = line->s1();
		MapSide* s2 = line->s2();

		// Front side
		if (s1)
		{
			copy_sides.push_back(s1);
			if (std::find(copy_sectors.begin(), copy_sectors.end(), s1->sector()) == copy_sectors.end())
				copy_sectors.push_back(s1->sector());
		}

		// Back side
		if (s2)
		{
			copy_sides.push_back(s2);
			if (std::find(copy_sectors.begin(), copy_sectors.end(), s2->sector()) == copy_sectors.end())
				copy_sectors.push_back(s2->sector());
		}
	}

	// Copy sectors
	for (auto& copy_sector : copy_sectors)
	{
		auto copy = new MapSector(nullptr);
		copy->copy(copy_sector);
		sectors_.emplace_back(copy);
	}

	// Copy sides
	for (auto& copy_side : copy_sides)
	{
		auto copy = new MapSide();
		copy->copy(copy_side);

		// Set relative sector
		for (unsigned b = 0; b < copy_sectors.size(); b++)
		{
			if (copy_side->sector() == copy_sectors[b])
			{
				copy->setSector(sectors_[b].get());
				break;
			}
		}

		sides_.emplace_back(copy);
	}

	// Get vertices to copy (and determine midpoint)
	double             min_x = 9999999;
	double             max_x = -9999999;
	double             min_y = 9999999;
	double             max_y = -9999999;
	vector<MapVertex*> copy_verts;
	for (auto& line : lines)
	{
		auto v1 = line->v1();
		auto v2 = line->v2();

		// Add vertices to copy list
		if (std::find(copy_verts.begin(), copy_verts.end(), v1) == copy_verts.end())
			copy_verts.push_back(v1);
		if (std::find(copy_verts.begin(), copy_verts.end(), v2) == copy_verts.end())
			copy_verts.push_back(v2);

		// Update min/max
		if (v1->xPos() < min_x)
			min_x = v1->xPos();
		if (v1->xPos() > max_x)
			max_x = v1->xPos();
		if (v1->yPos() < min_y)
			min_y = v1->yPos();
		if (v1->yPos() > max_y)
			max_y = v1->yPos();
		if (v2->xPos() < min_x)
			min_x = v2->xPos();
		if (v2->xPos() > max_x)
			max_x = v2->xPos();
		if (v2->yPos() < min_y)
			min_y = v2->yPos();
		if (v2->yPos() > max_y)
			max_y = v2->yPos();
	}

	// Determine midpoint
	double mid_x = min_x + ((max_x - min_x) * 0.5);
	double mid_y = min_y + ((max_y - min_y) * 0.5);
	this->midpoint_.set(mid_x, mid_y);

	// Copy vertices
	for (auto& copy_vert : copy_verts)
	{
		auto copy = new MapVertex(copy_vert->xPos() - mid_x, copy_vert->yPos() - mid_y);
		copy->copy(copy_vert);
		vertices_.emplace_back(copy);
	}

	// Copy lines
	for (auto& line : lines)
	{
		// Get relative sides
		MapSide* s1       = nullptr;
		MapSide* s2       = nullptr;
		bool     s1_found = false;
		bool     s2_found = !(line->s2());
		for (unsigned b = 0; b < copy_sides.size(); b++)
		{
			if (line->s1() == copy_sides[b])
			{
				s1       = sides_[b].get();
				s1_found = true;
			}
			if (line->s2() == copy_sides[b])
			{
				s2       = sides_[b].get();
				s2_found = true;
			}

			if (s1_found && s2_found)
				break;
		}

		// Get relative vertices
		MapVertex* v1 = nullptr;
		MapVertex* v2 = nullptr;
		for (unsigned b = 0; b < copy_verts.size(); b++)
		{
			if (line->v1() == copy_verts[b])
				v1 = vertices_[b].get();
			if (line->v2() == copy_verts[b])
				v2 = vertices_[b].get();

			if (v1 && v2)
				break;
		}

		// Copy line
		auto copy = new MapLine(v1, v2, s1, s2);
		copy->copy(line);
		this->lines_.emplace_back(copy);
	}
}

// -----------------------------------------------------------------------------
// Returns a string with info on what items are copied
// -----------------------------------------------------------------------------
string MapArchClipboardItem::info() const
{
	return fmt::format(
		"{} Vertices, {} Lines, {} Sides and {} Sectors",
		vertices_.size(),
		lines_.size(),
		sides_.size(),
		sectors_.size());
}

// -----------------------------------------------------------------------------
// Pastes copied architecture to [map] at [position]
// -----------------------------------------------------------------------------
vector<MapVertex*> MapArchClipboardItem::pasteToMap(SLADEMap* map, fpoint2_t position)
{
	std::map<MapVertex*, MapVertex*> vert_map;
	std::map<MapSector*, MapSector*> sect_map;
	std::map<MapSide*, MapSide*>     side_map;

	// Add vertices
	vector<MapVertex*> new_verts;
	for (auto& vertex : vertices_)
	{
		new_verts.push_back(map->createVertex(position.x + vertex->xPos(), position.y + vertex->yPos()));
		new_verts.back()->copy(vertex.get());
		vert_map[vertex.get()] = new_verts.back();
	}

	// Add sectors
	for (auto& sector : sectors_)
	{
		MapSector* new_sector = map->createSector();
		new_sector->copy(sector.get());
		sect_map[sector.get()] = new_sector;
	}

	// Add sides
	for (auto& side : sides_)
	{
		// Get relative sector
		MapSector* sector = findInMap(sect_map, side->sector());

		MapSide* new_side = map->createSide(sector);
		new_side->copy(side.get());
		side_map[side.get()] = new_side;
	}

	// Add lines
	for (auto& line : lines_)
	{
		// Get relative vertices
		MapVertex* v1 = findInMap(vert_map, line->v1());
		MapVertex* v2 = findInMap(vert_map, line->v2());

		if (!v1)
		{
			Log::debug(1, "no v1");
			continue;
		}
		if (!v2)
		{
			Log::debug(1, "no v2");
		}

		MapLine* newline = map->createLine(v1, v2, true);
		newline->copy(line.get());

		// Set relative sides
		MapSide* newS1 = findInMap(side_map, line->s1());
		MapSide* newS2 = findInMap(side_map, line->s2());
		if (newS1)
			newline->setS1(newS1);
		if (newS2)
			newline->setS2(newS2);

		// Set important flags (needed when copying from Doom/Hexen format to UDMF)
		// Won't be needed when proper map format conversion stuff is implemented
		Game::configuration().setLineBasicFlag("twosided", newline, map->currentFormat(), (newS1 && newS2));
		Game::configuration().setLineBasicFlag("blocking", newline, map->currentFormat(), !newS2);
	}

	// TODO:
	// - Split lines
	// - Merge lines

	return new_verts;
}

// -----------------------------------------------------------------------------
// Adds all copied lines to [list]
// -----------------------------------------------------------------------------
void MapArchClipboardItem::getLines(vector<MapLine*>& list)
{
	for (auto& line : lines_)
		list.push_back(line.get());
}


// -----------------------------------------------------------------------------
//
// MapThingsClipboardItem Class Functions
//
// -----------------------------------------------------------------------------


// -----------------------------------------------------------------------------
// MapThingsClipboardItem class constructor
// -----------------------------------------------------------------------------
MapThingsClipboardItem::MapThingsClipboardItem() : ClipboardItem(Type::MapThings)
{
	// This needs to be defined here to avoid compilation errors
	// (because we're using unique_ptrs to forwarded types)
}

// -----------------------------------------------------------------------------
// MapThingsClipboardItem class destructor
// -----------------------------------------------------------------------------
MapThingsClipboardItem::~MapThingsClipboardItem() = default; // See constructor comment

// -----------------------------------------------------------------------------
// Copies [things]
// -----------------------------------------------------------------------------
void MapThingsClipboardItem::addThings(vector<MapThing*>& things)
{
	// Copy things
	double min_x = 99999999;
	double min_y = 99999999;
	double max_x = -99999999;
	double max_y = -99999999;
	for (auto& thing : things)
	{
		auto copy_thing = new MapThing();
		copy_thing->copy(thing);
		things_.emplace_back(copy_thing);

		if (thing->xPos() < min_x)
			min_x = thing->xPos();
		if (thing->yPos() < min_y)
			min_y = thing->yPos();
		if (thing->xPos() > max_x)
			max_x = thing->xPos();
		if (thing->yPos() > max_y)
			max_y = thing->yPos();
	}

	// Get midpoint
	double mid_x = min_x + ((max_x - min_x) * 0.5);
	double mid_y = min_y + ((max_y - min_y) * 0.5);
	midpoint_.set(mid_x, mid_y);

	// Adjust thing positions
	for (auto& thing : things_)
		thing->setPosition(thing->xPos() - mid_x, thing->yPos() - mid_y);
}

// -----------------------------------------------------------------------------
// Returns a string with info on what items are copied
// -----------------------------------------------------------------------------
string MapThingsClipboardItem::info() const
{
	return fmt::format("{} Things", things_.size());
}

// -----------------------------------------------------------------------------
// Pastes copied things to [map] at [position]
// -----------------------------------------------------------------------------
void MapThingsClipboardItem::pasteToMap(SLADEMap* map, fpoint2_t position)
{
	for (auto& thing : things_)
	{
		auto newthing = map->createThing(0, 0);
		newthing->copy(thing.get());
		newthing->setPosition(position.x + thing->xPos(), position.y + thing->yPos());
	}
}

// -----------------------------------------------------------------------------
// Adds all copied things to [list]
// -----------------------------------------------------------------------------
void MapThingsClipboardItem::getThings(vector<MapThing*>& list)
{
	for (auto& thing : things_)
		list.push_back(thing.get());
}


// -----------------------------------------------------------------------------
//
// Clipboard Class Functions
//
// -----------------------------------------------------------------------------


// -----------------------------------------------------------------------------
// Puts an item on the clipboard. Returns false if item is invalid
// -----------------------------------------------------------------------------
bool Clipboard::putItem(ClipboardItem* item)
{
	// Clear current clipboard contents
	clear();

	if (!item)
		return false;

	items_.emplace_back(item);
	return true;
}

// -----------------------------------------------------------------------------
// Puts multiple items on the clipboard
// -----------------------------------------------------------------------------
bool Clipboard::putItems(const vector<ClipboardItem*>& items)
{
	// Clear current clipboard contents
	clear();

	for (auto& item : items)
		if (item)
			items_.emplace_back(item);

	return true;
}
