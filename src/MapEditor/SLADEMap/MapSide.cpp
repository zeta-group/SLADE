
// -----------------------------------------------------------------------------
// SLADE - It's a Doom Editor
// Copyright(C) 2008 - 2017 Simon Judd
//
// Email:       sirjuddington@gmail.com
// Web:         http://slade.mancubus.net
// Filename:    MapSide.cpp
// Description: MapSide class, represents a line side object in a map
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
#include "MapSide.h"
#include "Game/Configuration.h"
#include "MapSector.h"
#include "SLADEMap.h"


// -----------------------------------------------------------------------------
//
// Variables
//
// -----------------------------------------------------------------------------
const string MapSide::PROP_SECTOR         = "sector";
const string MapSide::PROP_OFFSET_X       = "offsetx";
const string MapSide::PROP_OFFSET_Y       = "offsety";
const string MapSide::PROP_TEX_UPPER      = "texturetop";
const string MapSide::PROP_TEX_MIDDLE     = "texturemiddle";
const string MapSide::PROP_TEX_LOWER      = "texturebottom";
const string MapSide::PROP_LIGHT          = "light";
const string MapSide::PROP_LIGHT_ABSOLUTE = "lightabsolute";


// -----------------------------------------------------------------------------
//
// MapSide Class Functions
//
// -----------------------------------------------------------------------------


// -----------------------------------------------------------------------------
// MapSide class constructor
// -----------------------------------------------------------------------------
MapSide::MapSide(MapSector* sector, SLADEMap* parent) : MapObject{ Type::Side, parent }, sector_{ sector }
{
	// Add to parent sector
	if (sector)
		sector->connectSide(this);
}

// -----------------------------------------------------------------------------
// Copies another MapSide object [c]
// -----------------------------------------------------------------------------
void MapSide::copy(MapObject* c)
{
	if (!isSameType(c))
		return;

	// Update texture counts (decrement previous)
	if (parent_map_)
	{
		parent_map_->updateTexUsage(tex_lower_, -1);
		parent_map_->updateTexUsage(tex_middle_, -1);
		parent_map_->updateTexUsage(tex_upper_, -1);
	}

	// Copy properties
	auto side   = (MapSide*)c;
	tex_lower_  = side->tex_lower_;
	tex_middle_ = side->tex_middle_;
	tex_upper_  = side->tex_upper_;
	offset_.x   = side->offset_.x;
	offset_.y   = side->offset_.y;

	// Update texture counts (increment new)
	if (parent_map_)
	{
		parent_map_->updateTexUsage(tex_lower_, 1);
		parent_map_->updateTexUsage(tex_middle_, 1);
		parent_map_->updateTexUsage(tex_upper_, 1);
	}

	MapObject::copy(c);
}

// -----------------------------------------------------------------------------
// Returns the light level of the given side
// -----------------------------------------------------------------------------
uint8_t MapSide::light()
{
	int  light          = 0;
	bool include_sector = true;

	if (parent_map_->currentFormat() == MAP_UDMF
		&& Game::configuration().featureSupported(Game::UDMFFeature::SideLighting))
	{
		light += intProperty(PROP_LIGHT);
		if (boolProperty(PROP_LIGHT_ABSOLUTE))
			include_sector = false;
	}

	if (include_sector && sector_)
		light += sector_->lightAt(MapSector::Floor);

	// Clamp range
	if (light > 255)
		return 255;
	if (light < 0)
		return 0;
	return light;
}

// -----------------------------------------------------------------------------
// Change the light level of a side, if supported
// -----------------------------------------------------------------------------
void MapSide::changeLight(int amount)
{
	if (parent_map_->currentFormat() == MAP_UDMF
		&& Game::configuration().featureSupported(Game::UDMFFeature::SideLighting))
		setIntProperty(PROP_LIGHT, intProperty(PROP_LIGHT) + amount);
}

// -----------------------------------------------------------------------------
// Sets the side's sector to [sector]
// -----------------------------------------------------------------------------
void MapSide::setSector(MapSector* sector)
{
	if (!sector)
		return;

	// Remove side from current sector, if any
	if (this->sector_)
		this->sector_->disconnectSide(this);

	// Update modified time
	setModified();

	// Add side to new sector
	this->sector_ = sector;
	sector->connectSide(this);
}

// -----------------------------------------------------------------------------
// Returns the value of the integer property matching [key]
// -----------------------------------------------------------------------------
int MapSide::intProperty(const string& key)
{
	if (key == PROP_SECTOR)
	{
		if (sector_)
			return sector_->index();
		else
			return -1;
	}
	else if (key == PROP_OFFSET_X)
		return offset_.x;
	else if (key == PROP_OFFSET_Y)
		return offset_.y;
	else
		return MapObject::intProperty(key);
}

// -----------------------------------------------------------------------------
// Sets the integer value of the property [key] to [value]
// -----------------------------------------------------------------------------
void MapSide::setIntProperty(const string& key, int value)
{
	// Update modified time
	setModified();

	if (key == PROP_SECTOR && parent_map_)
		setSector(parent_map_->sector(value));
	else if (key == PROP_OFFSET_X)
		offset_.x = value;
	else if (key == PROP_OFFSET_Y)
		offset_.y = value;
	else
		MapObject::setIntProperty(key, value);
}

// -----------------------------------------------------------------------------
// Returns the value of the string property matching [key]
// -----------------------------------------------------------------------------
string MapSide::stringProperty(const string& key)
{
	if (key == PROP_TEX_UPPER)
		return tex_upper_;
	else if (key == PROP_TEX_MIDDLE)
		return tex_middle_;
	else if (key == PROP_TEX_LOWER)
		return tex_lower_;
	else
		return MapObject::stringProperty(key);
}

// -----------------------------------------------------------------------------
// Sets the string value of the property [key] to [value]
// -----------------------------------------------------------------------------
void MapSide::setStringProperty(const string& key, const string& value)
{
	// Update modified time
	setModified();

	if (key == PROP_TEX_UPPER)
	{
		if (parent_map_)
			parent_map_->updateTexUsage(tex_upper_, -1);
		tex_upper_ = value;
		if (parent_map_)
			parent_map_->updateTexUsage(tex_upper_, 1);
	}
	else if (key == PROP_TEX_MIDDLE)
	{
		if (parent_map_)
			parent_map_->updateTexUsage(tex_middle_, -1);
		tex_middle_ = value;
		if (parent_map_)
			parent_map_->updateTexUsage(tex_middle_, 1);
	}
	else if (key == PROP_TEX_LOWER)
	{
		if (parent_map_)
			parent_map_->updateTexUsage(tex_lower_, -1);
		tex_lower_ = value;
		if (parent_map_)
			parent_map_->updateTexUsage(tex_lower_, 1);
	}
	else
		MapObject::setStringProperty(key, value);
}

// -----------------------------------------------------------------------------
// Returns true if the property [key] can be modified via script
// -----------------------------------------------------------------------------
bool MapSide::scriptCanModifyProp(const string& key)
{
	return key != PROP_SECTOR;
}

// -----------------------------------------------------------------------------
// Write all side info to a Backup struct
// -----------------------------------------------------------------------------
void MapSide::writeBackup(Backup* backup)
{
	// Sector
	if (sector_)
		backup->props_internal[PROP_SECTOR] = sector_->objId();
	else
		backup->props_internal[PROP_SECTOR] = 0;

	// Textures
	backup->props_internal[PROP_TEX_UPPER]  = tex_upper_;
	backup->props_internal[PROP_TEX_MIDDLE] = tex_middle_;
	backup->props_internal[PROP_TEX_LOWER]  = tex_lower_;

	// Offsets
	backup->props_internal[PROP_OFFSET_X] = offset_.x;
	backup->props_internal[PROP_OFFSET_Y] = offset_.y;

	// Log::info(1, "Side %d backup sector #%d", id, sector->getIndex());
}

// -----------------------------------------------------------------------------
// Reads all side info from a Backup struct
// -----------------------------------------------------------------------------
void MapSide::readBackup(Backup* backup)
{
	// Sector
	MapObject* s = parent_map_->getObjectById(backup->props_internal[PROP_SECTOR]);
	if (s)
	{
		sector_->disconnectSide(this);
		sector_ = (MapSector*)s;
		sector_->connectSide(this);
		// Log::info(1, "Side %d load backup sector #%d", id, s->getIndex());
	}
	else
	{
		if (sector_)
			sector_->disconnectSide(this);
		sector_ = nullptr;
	}

	// Update texture counts (decrement previous)
	parent_map_->updateTexUsage(tex_upper_, -1);
	parent_map_->updateTexUsage(tex_middle_, -1);
	parent_map_->updateTexUsage(tex_lower_, -1);

	// Textures
	tex_upper_  = backup->props_internal[PROP_TEX_UPPER].stringValueRef();
	tex_middle_ = backup->props_internal[PROP_TEX_MIDDLE].stringValueRef();
	tex_lower_  = backup->props_internal[PROP_TEX_LOWER].stringValueRef();

	// Update texture counts (increment new)
	parent_map_->updateTexUsage(tex_upper_, 1);
	parent_map_->updateTexUsage(tex_middle_, 1);
	parent_map_->updateTexUsage(tex_lower_, 1);

	// Offsets
	offset_.x = backup->props_internal[PROP_OFFSET_X].intValue();
	offset_.y = backup->props_internal[PROP_OFFSET_Y].intValue();
}
