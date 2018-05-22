
// -----------------------------------------------------------------------------
// SLADE - It's a Doom Editor
// Copyright(C) 2008 - 2017 Simon Judd
//
// Email:       sirjuddington@gmail.com
// Web:         http://slade.mancubus.net
// Filename:    MapTextureBrowser.cpp
// Description: Specialisation of BrowserWindow to show and browse for map
//              textures/flats
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
#include "MapTextureBrowser.h"
#include "Dialogs/Preferences/InputPrefsPanel.h"
#include "Game/Configuration.h"
#include "MapEditor/MapEditor.h"
#include "MapEditor/MapTextureManager.h"
#include "MapEditor/SLADEMap/SLADEMap.h"


// -----------------------------------------------------------------------------
//
// Variables
//
// -----------------------------------------------------------------------------
CVAR(Int, map_tex_sort, 2, CVAR_SAVE)
CVAR(String, map_tex_treespec, "type,archive,category", CVAR_SAVE)


// -----------------------------------------------------------------------------
//
// Functions
//
// -----------------------------------------------------------------------------
namespace
{
// -----------------------------------------------------------------------------
// Builds and returns the tree item path for [info]
// -----------------------------------------------------------------------------
string determineTexturePath(Archive* archive, MapTextureManager::Category category, string_view type, string_view path)
{
	using TexCat     = MapTextureManager::Category;
	auto   tree_spec = StrUtil::splitToViews(map_tex_treespec, ',');
	string ret;
	for (const auto& spec : tree_spec)
	{
		if (spec == "archive")
			ret += archive->filename(false);
		else if (spec == "type")
			ret.append(type.data(), type.size());
		else if (spec == "category")
		{
			switch (category)
			{
			case TexCat::TEXTUREX: ret += "TEXTUREx"; break;
			case TexCat::TEXTURES: ret += "TEXTURES"; break;
			case TexCat::HIRES: ret += "HIRESTEX"; break;
			case TexCat::TX: ret += "Single (TX)"; break;
			default: continue;
			}
		}

		ret += "/";
	}

	return ret.append(path.data(), path.size());
}

// -----------------------------------------------------------------------------
// Returns true if [left] has higher usage count than [right].
// If both are equal it will go alphabetically by name
// -----------------------------------------------------------------------------
bool sortBIUsage(BrowserItem* left, BrowserItem* right)
{
	// Sort alphabetically if usage counts are equal
	if (((MapTexBrowserItem*)left)->usageCount() == ((MapTexBrowserItem*)right)->usageCount())
		return left->name() < right->name();
	else
		return ((MapTexBrowserItem*)left)->usageCount() > ((MapTexBrowserItem*)right)->usageCount();
}
} // namespace


// -----------------------------------------------------------------------------
//
// MapTexBrowserItem Class Functions
//
// -----------------------------------------------------------------------------


// -----------------------------------------------------------------------------
// MapTexBrowserItem class constructor
// -----------------------------------------------------------------------------
MapTexBrowserItem::MapTexBrowserItem(string_view name, int type, unsigned index) : BrowserItem{ name, index }
{
	if (type == 0)
		this->type_ = "texture";
	else if (type == 1)
		this->type_ = "flat";

	// Check for blank texture
	if (name == "-" && type == 0)
		blank_ = true;
}

// -----------------------------------------------------------------------------
// Loads the item image
// -----------------------------------------------------------------------------
bool MapTexBrowserItem::loadImage()
{
	GLTexture* tex = nullptr;

	// Get texture or flat depending on type
	if (type_ == "texture")
		tex = MapEditor::textureManager().texture(name_, false);
	else if (type_ == "flat")
		tex = MapEditor::textureManager().flat(name_, false);

	if (tex)
	{
		image_ = tex;
		return true;
	}
	else
		return false;
}

// -----------------------------------------------------------------------------
// Returns a string with extra information about the texture/flat
// -----------------------------------------------------------------------------
string MapTexBrowserItem::itemInfo()
{
	string info;

	// Check for blank texture
	if (name_ == "-")
		return "No Texture";

	// Add dimensions if known
	if (image_ || loadImage())
		info += fmt::sprintf("%dx%d", image_->getWidth(), image_->getHeight());
	else
		info += "Unknown size";

	// Add type
	if (type_ == "texture")
		info += ", Texture";
	else
		info += ", Flat";

	// Add scaling info
	if (image_->getScaleX() != 1.0 || image_->getScaleY() != 1.0)
		info += ", Scaled";

	// Add usage count
	info += fmt::sprintf(", Used %d times", usage_count_);

	return info;
}


// -----------------------------------------------------------------------------
//
// MapTextureBrowser Class Functions
//
// -----------------------------------------------------------------------------


// -----------------------------------------------------------------------------
// MapTextureBrowser class constructor
// -----------------------------------------------------------------------------
MapTextureBrowser::MapTextureBrowser(wxWindow* parent, int type, string_view texture, SLADEMap* map) :
	BrowserWindow(parent),
	type_{ type },
	map_{ map }
{
	// Init variables
	truncate_names_ = true;

	// Init sorting
	addSortType("Usage Count");
	setSortType(map_tex_sort);

	// Set window title
	wxDialog::SetTitle("Browse Map Textures");

	// Textures
	if (type == 0 || Game::configuration().featureSupported(Game::Feature::MixTexFlats))
	{
		addGlobalItem(new MapTexBrowserItem("-", 0, 0));

		auto& textures = MapEditor::textureManager().allTexturesInfo();
		for (auto& tex : textures)
		{
			// Add browser item
			addItem(
				new MapTexBrowserItem(tex.name, 0, tex.index),
				determineTexturePath(tex.archive, tex.category, "Textures", tex.path));
		}
	}

	// Flats
	if (type == 1 || Game::configuration().featureSupported(Game::Feature::MixTexFlats))
	{
		auto& flats = MapEditor::textureManager().allFlatsInfo();
		for (auto& flat : flats)
		{
			// Determine tree path
			string path = determineTexturePath(flat.archive, flat.category, "Flats", flat.path);

			// Add browser item
			if (flat.category == MapTextureManager::Category::TEXTURES)
				addItem(new MapTexBrowserItem(flat.name, 0, flat.index), path);
			else
				addItem(new MapTexBrowserItem(flat.name, 1, flat.index), path);
		}
	}

	populateItemTree(false);

	// Select initial texture (if any)
	selectItem(texture);
}

// -----------------------------------------------------------------------------
// Sort the current items depending on [sort_type]
// -----------------------------------------------------------------------------
void MapTextureBrowser::doSort(unsigned sort_type)
{
	map_tex_sort = sort_type;

	// Default sorts
	if (sort_type < 2)
		return BrowserWindow::doSort(sort_type);

	// Sort by usage
	else if (sort_type == 2)
	{
		updateUsage();
		vector<BrowserItem*>& items = canvas_->itemList();
		std::sort(items.begin(), items.end(), sortBIUsage);
	}
}

// -----------------------------------------------------------------------------
// Updates usage counts for all browser items
// -----------------------------------------------------------------------------
void MapTextureBrowser::updateUsage() const
{
	if (!map_)
		return;

	auto& items = canvas_->itemList();
	for (auto& i : items)
	{
		auto item = (MapTexBrowserItem*)i;
		if (type_ == 0)
			item->setUsage(map_->texUsageCount(item->name()));
		else
			item->setUsage(map_->flatUsageCount(item->name()));
	}
}
