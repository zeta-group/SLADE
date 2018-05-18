
// -----------------------------------------------------------------------------
// SLADE - It's a Doom Editor
// Copyright(C) 2008 - 2017 Simon Judd
//
// Email:       sirjuddington@gmail.com
// Web:         http://slade.mancubus.net
// Filename:    MapSector.cpp
// Description: MapSector class, represents a sector object in a map
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
#include "Game/Configuration.h"
#include "MapLine.h"
#include "MapSector.h"
#include "MapSide.h"
#include "MapVertex.h"
#include "SLADEMap.h"
#include "Utility/MathStuff.h"


// -----------------------------------------------------------------------------
//
// Variables
//
// -----------------------------------------------------------------------------
const string MapSector::PROP_TEX_FLOOR              = "texturefloor";
const string MapSector::PROP_TEX_CEILING            = "textureceiling";
const string MapSector::PROP_HEIGHT_FLOOR           = "heightfloor";
const string MapSector::PROP_HEIGHT_CEILING         = "heightceiling";
const string MapSector::PROP_LIGHT                  = "lightlevel";
const string MapSector::PROP_SPECIAL                = "special";
const string MapSector::PROP_ID                     = "id";
const string MapSector::PROP_LIGHT_FLOOR            = "lightfloor";
const string MapSector::PROP_LIGHT_CEILING          = "lightceiling";
const string MapSector::PROP_LIGHT_FLOOR_ABSOLUTE   = "lightfloorabsolute";
const string MapSector::PROP_LIGHT_CEILING_ABSOLUTE = "lightceilingabsolute";
const string MapSector::PROP_LIGHT_COLOR            = "lightcolor";
const string MapSector::PROP_FADE_COLOR             = "fadecolor";
const string MapSector::PROP_PAN_X_FLOOR            = "xpanningfloor";
const string MapSector::PROP_PAN_Y_FLOOR            = "ypanningfloor";
const string MapSector::PROP_SCALE_X_FLOOR          = "xscalefloor";
const string MapSector::PROP_SCALE_Y_FLOOR          = "yscalefloor";
const string MapSector::PROP_SCALE_X_CEILING        = "xscaleceiling";
const string MapSector::PROP_SCALE_Y_CEILING        = "yscaleceiling";
const string MapSector::PROP_ROTATION_FLOOR         = "rotationfloor";
const string MapSector::PROP_ROTATION_CEILING       = "rotationceiling";


// -----------------------------------------------------------------------------
//
// MapSector Class Functions
//
// -----------------------------------------------------------------------------


// -----------------------------------------------------------------------------
// MapSector class constructor
// -----------------------------------------------------------------------------
MapSector::MapSector(SLADEMap* parent) : MapObject{ Type::Sector, parent }, geometry_updated_{ App::runTimer() } {}

// -----------------------------------------------------------------------------
// MapSector class constructor
// -----------------------------------------------------------------------------
MapSector::MapSector(string_view f_tex, string_view c_tex, SLADEMap* parent) :
	MapObject{ Type::Sector, parent },
	geometry_updated_{ App::runTimer() }
{
	// Init variables
	S_SET_VIEW(floor_.texture, f_tex);
	S_SET_VIEW(ceiling_.texture, c_tex);
}

// -----------------------------------------------------------------------------
// Copies another map object [s]
// -----------------------------------------------------------------------------
void MapSector::copy(MapObject* s)
{
	// Don't copy a non-sector
	if (s->objType() != Type::Sector)
		return;

	setModified();

	// Update texture counts (decrement previous)
	if (parent_map_)
	{
		parent_map_->updateFlatUsage(floor_.texture, -1);
		parent_map_->updateFlatUsage(ceiling_.texture, -1);
	}

	// Basic variables
	MapSector* sector = (MapSector*)s;
	floor_.texture    = sector->floor_.texture;
	ceiling_.texture  = sector->ceiling_.texture;
	floor_.height     = sector->floor_.height;
	ceiling_.height   = sector->ceiling_.height;
	light_            = sector->light_;
	special_          = sector->special_;
	id_               = sector->id_;
	floor_.plane.set(0, 0, 1, sector->floor_.height);
	ceiling_.plane.set(0, 0, 1, sector->ceiling_.height);

	// Update texture counts (increment new)
	if (parent_map_)
	{
		parent_map_->updateFlatUsage(floor_.texture, 1);
		parent_map_->updateFlatUsage(ceiling_.texture, 1);
	}

	// Other properties
	MapObject::copy(s);
}

// -----------------------------------------------------------------------------
// Update the last time the sector geometry changed
// -----------------------------------------------------------------------------
void MapSector::setGeometryUpdated()
{
	geometry_updated_ = App::runTimer();
}

// -----------------------------------------------------------------------------
// Finds the 'text point' for the sector. This is a point within the sector that
// is reasonably close to the middle of the sector bbox while still being within
// the sector itself
// -----------------------------------------------------------------------------
void MapSector::findTextPoint()
{
	// Check if actual sector midpoint can be used
	text_point_ = point(Point::Mid);
	if (isWithin(text_point_))
		return;

	if (connected_sides_.empty())
		return;

	// Find nearest line to sector midpoint (that is also part of the sector)
	double   min_dist = 9999999999.0;
	MapSide* mid_side = connected_sides_[0];
	for (auto& side : connected_sides_)
	{
		double dist = MathStuff::distanceToLineFast(text_point_, side->parentLine()->seg());

		if (dist < min_dist)
		{
			min_dist = dist;
			mid_side = side;
		}
	}

	// Calculate ray
	fpoint2_t r_o = mid_side->parentLine()->point(Point::Mid);
	fpoint2_t r_d = mid_side->parentLine()->frontVector();
	if (mid_side == mid_side->parentLine()->s1())
		r_d.set(-r_d.x, -r_d.y);

	// Find nearest intersecting line
	min_dist = 9999999999.0;
	for (auto& side : connected_sides_)
	{
		if (side == mid_side)
			continue;

		double dist =
			MathStuff::distanceRayLine(r_o, r_o + r_d, side->parentLine()->point1(), side->parentLine()->point2());

		if (dist > 0 && dist < min_dist)
			min_dist = dist;
	}

	// Set text point to halfway between the two lines
	text_point_.set(r_o.x + (r_d.x * min_dist * 0.5), r_o.y + (r_d.y * min_dist * 0.5));
}

// -----------------------------------------------------------------------------
// Returns the value of the string property matching [key]
// -----------------------------------------------------------------------------
string MapSector::stringProperty(const string& key)
{
	if (key == PROP_TEX_FLOOR)
		return floor_.texture;
	else if (key == PROP_TEX_CEILING)
		return ceiling_.texture;
	else
		return MapObject::stringProperty(key);
}

// -----------------------------------------------------------------------------
// Returns the value of the integer property matching [key]
// -----------------------------------------------------------------------------
int MapSector::intProperty(const string& key)
{
	if (key == PROP_HEIGHT_FLOOR)
		return floor_.height;
	else if (key == PROP_HEIGHT_CEILING)
		return ceiling_.height;
	else if (key == PROP_LIGHT)
		return light_;
	else if (key == PROP_SPECIAL)
		return special_;
	else if (key == PROP_ID)
		return id_;
	else
		return MapObject::intProperty(key);
}

// -----------------------------------------------------------------------------
// Sets the string value of the property [key] to [value]
// -----------------------------------------------------------------------------
void MapSector::setStringProperty(const string& key, const string& value)
{
	// Update modified time
	setModified();

	if (key == PROP_TEX_FLOOR)
	{
		if (parent_map_)
			parent_map_->updateFlatUsage(floor_.texture, -1);
		floor_.texture = value;
		if (parent_map_)
			parent_map_->updateFlatUsage(floor_.texture, 1);
	}
	else if (key == PROP_TEX_CEILING)
	{
		if (parent_map_)
			parent_map_->updateFlatUsage(ceiling_.texture, -1);
		ceiling_.texture = value;
		if (parent_map_)
			parent_map_->updateFlatUsage(ceiling_.texture, 1);
	}
	else
		return MapObject::setStringProperty(key, value);
}

// -----------------------------------------------------------------------------
// Sets the float value of the property [key] to [value]
// -----------------------------------------------------------------------------
void MapSector::setFloatProperty(const string& key, double value)
{
	using Game::UDMFFeature;

	// Check if flat offset/scale/rotation is changing (if UDMF)
	if (parent_map_->currentFormat() == MAP_UDMF)
	{
		if ((Game::configuration().featureSupported(UDMFFeature::FlatPanning)
			 && (key == PROP_PAN_X_FLOOR || key == PROP_PAN_Y_FLOOR))
			|| (Game::configuration().featureSupported(UDMFFeature::FlatScaling)
				&& (key == PROP_SCALE_X_FLOOR || key == PROP_SCALE_Y_FLOOR || key == PROP_SCALE_X_CEILING
					|| key == PROP_SCALE_Y_CEILING))
			|| (Game::configuration().featureSupported(UDMFFeature::FlatRotation)
				&& (key == PROP_ROTATION_FLOOR || key == PROP_ROTATION_CEILING)))
			polygon_.setTexture(nullptr); // Clear texture to force update
	}

	MapObject::setFloatProperty(key, value);
}

// -----------------------------------------------------------------------------
// Sets the integer value of the property [key] to [value]
// -----------------------------------------------------------------------------
void MapSector::setIntProperty(const string& key, int value)
{
	// Update modified time
	setModified();

	if (key == PROP_HEIGHT_FLOOR)
		setFloorHeight(value);
	else if (key == PROP_HEIGHT_CEILING)
		setCeilingHeight(value);
	else if (key == PROP_LIGHT)
		light_ = value;
	else if (key == PROP_SPECIAL)
		special_ = value;
	else if (key == PROP_ID)
		id_ = value;
	else
		MapObject::setIntProperty(key, value);
}

// -----------------------------------------------------------------------------
// Sets the floor texture to [tex]
// -----------------------------------------------------------------------------
void MapSector::setFloorTexture(string_view tex)
{
	setModified();
	S_SET_VIEW(floor_.texture, tex);
}

// -----------------------------------------------------------------------------
// Sets the ceiling texture to [tex]
// -----------------------------------------------------------------------------
void MapSector::setCeilingTexture(string_view tex)
{
	setModified();
	S_SET_VIEW(floor_.texture, tex);
}

// -----------------------------------------------------------------------------
// Sets the floor height to [height]
// -----------------------------------------------------------------------------
void MapSector::setFloorHeight(short height)
{
	setModified();
	floor_.height = height;
	setFloorPlane(Plane::flat(height));
}

// -----------------------------------------------------------------------------
// Sets the ceiling height to [height]
// -----------------------------------------------------------------------------
void MapSector::setCeilingHeight(short height)
{
	setModified();
	ceiling_.height = height;
	setCeilingPlane(Plane::flat(height));
}

// -----------------------------------------------------------------------------
// Sets the floor plane to [p]
// -----------------------------------------------------------------------------
void MapSector::setFloorPlane(const Plane& p)
{
	if (floor_.plane != p)
		setGeometryUpdated();
	floor_.plane = p;
}

// -----------------------------------------------------------------------------
// Sets the ceiling plane to [p]
// -----------------------------------------------------------------------------
void MapSector::setCeilingPlane(const Plane& p)
{
	if (ceiling_.plane != p)
		setGeometryUpdated();
	ceiling_.plane = p;
}

// -----------------------------------------------------------------------------
// Returns the object point [point]:
// Point::Mid = the absolute mid point of the sector,
// Point::Within/Text = a calculated point that is within the actual sector
// -----------------------------------------------------------------------------
fpoint2_t MapSector::point(Point point)
{
	if (point == Point::Mid)
	{
		BBox bbox = boundingBox();
		return { bbox.min.x + ((bbox.max.x - bbox.min.x) * 0.5), bbox.min.y + ((bbox.max.y - bbox.min.y) * 0.5) };
	}
	else
	{
		if (text_point_.x == 0 && text_point_.y == 0 && parent_map_)
			findTextPoint();
		return text_point_;
	}
}

// -----------------------------------------------------------------------------
// Calculates the sector's bounding box
// -----------------------------------------------------------------------------
void MapSector::updateBBox()
{
	// Reset bounding box
	bbox_.reset();

	for (auto& connected_side : connected_sides_)
	{
		MapLine* line = connected_side->parentLine();
		if (!line)
			continue;
		bbox_.extend(line->v1()->xPos(), line->v1()->yPos());
		bbox_.extend(line->v2()->xPos(), line->v2()->yPos());
	}

	text_point_.set(0, 0);
	setGeometryUpdated();
}

// -----------------------------------------------------------------------------
// Returns the sector bounding box
// -----------------------------------------------------------------------------
const BBox& MapSector::boundingBox()
{
	// Update bbox if needed
	if (!bbox_.isValid())
		updateBBox();

	return bbox_;
}

// -----------------------------------------------------------------------------
// Returns the sector polygon, updating it if necessary
// -----------------------------------------------------------------------------
Polygon2D* MapSector::polygon()
{
	if (poly_needsupdate_)
	{
		polygon_.openSector(this);
		poly_needsupdate_ = false;
	}

	return &polygon_;
}

// -----------------------------------------------------------------------------
// Returns true if the point is inside the sector
// -----------------------------------------------------------------------------
bool MapSector::isWithin(fpoint2_t point)
{
	// Check with bbox first
	if (!boundingBox().contains(point))
		return false;

	// Find nearest line in the sector
	double   dist;
	double   min_dist = 999999;
	MapLine* nline    = nullptr;
	for (auto& connected_side : connected_sides_)
	{
		// Calculate distance to line
		dist = connected_side->parentLine()->distanceTo(point);

		// Check distance
		if (dist < min_dist)
		{
			nline    = connected_side->parentLine();
			min_dist = dist;
		}
	}

	// No nearest (shouldn't happen)
	if (!nline)
		return false;

	// Check the side of the nearest line
	double side = MathStuff::lineSide(point, nline->seg());
	if (side >= 0 && nline->frontSector() == this)
		return true;
	else if (side < 0 && nline->backSector() == this)
		return true;
	else
		return false;
}

// -----------------------------------------------------------------------------
// Returns the minimum distance from the point to the closest line in the sector
// -----------------------------------------------------------------------------
double MapSector::distanceTo(fpoint2_t point, double maxdist)
{
	// Init
	if (maxdist < 0)
		maxdist = 9999999;

	// Check bounding box first
	if (!bbox_.isValid())
		updateBBox();
	double min_dist = 9999999;
	double dist     = MathStuff::distanceToLine(point, bbox_.leftSide());
	if (dist < min_dist)
		min_dist = dist;
	dist = MathStuff::distanceToLine(point, bbox_.topSide());
	if (dist < min_dist)
		min_dist = dist;
	dist = MathStuff::distanceToLine(point, bbox_.rightSide());
	if (dist < min_dist)
		min_dist = dist;
	dist = MathStuff::distanceToLine(point, bbox_.bottomSide());
	if (dist < min_dist)
		min_dist = dist;

	if (min_dist > maxdist && !bbox_.contains(point))
		return -1;

	// Go through connected sides
	for (auto& connected_side : connected_sides_)
	{
		// Get side parent line
		MapLine* line = connected_side->parentLine();
		if (!line)
			continue;

		// Check distance
		dist = line->distanceTo(point);
		if (dist < min_dist)
			min_dist = dist;
	}

	return min_dist;
}

// -----------------------------------------------------------------------------
// Adds all lines that are part of the sector to [list]
// -----------------------------------------------------------------------------
bool MapSector::getLines(vector<MapLine*>& list)
{
	// Go through connected sides
	for (auto& connected_side : connected_sides_)
	{
		// Add the side's parent line to the list if it doesn't already exist
		if (std::find(list.begin(), list.end(), connected_side->parentLine()) == list.end())
			list.push_back(connected_side->parentLine());
	}

	return true;
}

// -----------------------------------------------------------------------------
// Adds all vertices that are part of the sector to [list]
// -----------------------------------------------------------------------------
bool MapSector::getVertices(vector<MapVertex*>& list)
{
	// Go through connected sides
	MapLine* line;
	for (auto& connected_side : connected_sides_)
	{
		line = connected_side->parentLine();

		// Add the side's parent line's vertices to the list if they doesn't already exist
		if (line->v1() && std::find(list.begin(), list.end(), line->v1()) == list.end())
			list.push_back(line->v1());
		if (line->v2() && std::find(list.begin(), list.end(), line->v2()) == list.end())
			list.push_back(line->v2());
	}

	return true;
}

// -----------------------------------------------------------------------------
// Adds all vertices that are part of the sector to [list]
// -----------------------------------------------------------------------------
bool MapSector::getVertices(vector<MapObject*>& list)
{
	// Go through connected sides
	MapLine* line;
	for (auto& connected_side : connected_sides_)
	{
		line = connected_side->parentLine();

		// Add the side's parent line's vertices to the list if they doesn't already exist
		if (line->v1() && std::find(list.begin(), list.end(), line->v1()) == list.end())
			list.push_back(line->v1());
		if (line->v2() && std::find(list.begin(), list.end(), line->v2()) == list.end())
			list.push_back(line->v2());
	}

	return true;
}

// -----------------------------------------------------------------------------
// Returns the light level of the sector at [where]
// -----------------------------------------------------------------------------
uint8_t MapSector::lightAt(int where)
{
	// Check for UDMF + flat lighting
	if (parent_map_->currentFormat() == MAP_UDMF
		&& Game::configuration().featureSupported(Game::UDMFFeature::FlatLighting))
	{
		// Get general light level
		int l = light_;

		// Get specific light level
		if (where == Floor)
		{
			// Floor
			int fl = intProperty(PROP_LIGHT_FLOOR);
			if (boolProperty(PROP_LIGHT_FLOOR_ABSOLUTE))
				l = fl;
			else
				l += fl;
		}
		else if (where == Ceiling)
		{
			// Ceiling
			int cl = intProperty(PROP_LIGHT_CEILING);
			if (boolProperty(PROP_LIGHT_CEILING_ABSOLUTE))
				l = cl;
			else
				l += cl;
		}

		// Clamp light level
		if (l > 255)
			l = 255;
		if (l < 0)
			l = 0;

		return l;
	}
	else
	{
		// Clamp light level
		int l = light_;
		if (l > 255)
			l = 255;
		if (l < 0)
			l = 0;

		return l;
	}
}

// -----------------------------------------------------------------------------
// Changes the sector light level by [amount]
// -----------------------------------------------------------------------------
void MapSector::changeLight(int amount, int where)
{
	// Get current light level
	int ll = lightAt(where);

	// Clamp amount
	if (ll + amount > 255)
		amount -= ((ll + amount) - 255);
	else if (ll + amount < 0)
		amount = -ll;

	// Check for UDMF + flat lighting independent from the sector
	bool separate = parent_map_->currentFormat() == MAP_UDMF
					&& Game::configuration().featureSupported(Game::UDMFFeature::FlatLighting);

	// Change light level by amount
	if (where == 1 && separate)
	{
		int cur = intProperty(PROP_LIGHT_FLOOR);
		setIntProperty(PROP_LIGHT_FLOOR, cur + amount);
	}
	else if (where == 2 && separate)
	{
		int cur = intProperty(PROP_LIGHT_CEILING);
		setIntProperty(PROP_LIGHT_CEILING, cur + amount);
	}
	else
	{
		setModified();
		light_ = ll + amount;
	}
}

// -----------------------------------------------------------------------------
// Returns the colour of the sector at [where]
// If [fullbright] is true, light level is ignored
// -----------------------------------------------------------------------------
ColRGBA MapSector::colourAt(int where, bool fullbright)
{
	using Game::UDMFFeature;

	// Check for sector colour set in open script
	// TODO: Test if this is correct behaviour
	if (parent_map_->mapSpecials()->tagColoursSet())
	{
		ColRGBA col;
		if (parent_map_->mapSpecials()->getTagColour(id_, &col))
		{
			if (fullbright)
				return col;

			// Get sector light level
			int ll = light_;

			// Clamp light level
			if (ll > 255)
				ll = 255;
			if (ll < 0)
				ll = 0;

			// Calculate and return the colour
			float lightmult = (float)ll / 255.0f;
			return col.ampf(lightmult, lightmult, lightmult, 1.0f);
		}
	}

	// Check for UDMF
	if (parent_map_->currentFormat() == MAP_UDMF
		&& (Game::configuration().featureSupported(UDMFFeature::SectorColor)
			|| Game::configuration().featureSupported(UDMFFeature::FlatLighting)))
	{
		// Get sector light colour
		wxColour wxcol;
		if (Game::configuration().featureSupported(UDMFFeature::SectorColor))
		{
			int intcol = MapObject::intProperty(PROP_LIGHT_COLOR);
			wxcol      = wxColour(intcol);
		}
		else
			wxcol = wxColour(255, 255, 255, 255);


		// Ignore light level if fullbright
		if (fullbright)
			return { wxcol.Blue(), wxcol.Green(), wxcol.Red(), 255 };

		// Get sector light level
		int ll = light_;

		if (Game::configuration().featureSupported(UDMFFeature::FlatLighting))
		{
			// Get specific light level
			if (where == Floor)
			{
				// Floor
				int fl = MapObject::intProperty(PROP_LIGHT_FLOOR);
				if (boolProperty(PROP_LIGHT_FLOOR_ABSOLUTE))
					ll = fl;
				else
					ll += fl;
			}
			else if (where == Ceiling)
			{
				// Ceiling
				int cl = MapObject::intProperty(PROP_LIGHT_CEILING);
				if (boolProperty(PROP_LIGHT_CEILING_ABSOLUTE))
					ll = cl;
				else
					ll += cl;
			}
		}

		// Clamp light level
		if (ll > 255)
			ll = 255;
		if (ll < 0)
			ll = 0;

		// Calculate and return the colour
		float lightmult = (float)ll / 255.0f;
		return {
			uint8_t(wxcol.Blue() * lightmult), uint8_t(wxcol.Green() * lightmult), uint8_t(wxcol.Red() * lightmult), 255
		};
	}

	// Other format, simply return the light level
	if (fullbright)
		return { 255, 255, 255, 255 };
	else
	{
		uint8_t l = light_;

		// Clamp light level
		if (light_ > 255)
			l = 255;
		if (light_ < 0)
			l = 0;

		return { l, l, l, 255 };
	}
}

// -----------------------------------------------------------------------------
// Returns the fog colour of the sector
// -----------------------------------------------------------------------------
ColRGBA MapSector::fogColour()
{
	ColRGBA color(0, 0, 0, 0);

	// map specials/scripts
	if (parent_map_->mapSpecials()->tagFadeColoursSet())
	{
		if (parent_map_->mapSpecials()->getTagFadeColour(id_, &color))
			return color;
	}

	// udmf
	if (parent_map_->currentFormat() == MAP_UDMF
		&& Game::configuration().featureSupported(Game::UDMFFeature::SectorFog))
	{
		int intcol = MapObject::intProperty(PROP_FADE_COLOR);

		wxColour wxcol(intcol);
		color = ColRGBA(wxcol.Blue(), wxcol.Green(), wxcol.Red(), 0);
	}
	return color;
}

// -----------------------------------------------------------------------------
// Adds [side] to the list of 'connected sides'
// (sides that are part of this sector)
// -----------------------------------------------------------------------------
void MapSector::connectSide(MapSide* side)
{
	setModified();
	connected_sides_.push_back(side);
	poly_needsupdate_ = true;
	bbox_.reset();
	setGeometryUpdated();
}

// -----------------------------------------------------------------------------
// Removes [side] from the list of connected sides
// -----------------------------------------------------------------------------
void MapSector::disconnectSide(MapSide* side)
{
	setModified();
	for (unsigned a = 0; a < connected_sides_.size(); a++)
	{
		if (connected_sides_[a] == side)
		{
			connected_sides_.erase(connected_sides_.begin() + a);
			break;
		}
	}

	poly_needsupdate_ = true;
	bbox_.reset();
	setGeometryUpdated();
}

// -----------------------------------------------------------------------------
// Write all sector info to a Backup struct
// -----------------------------------------------------------------------------
void MapSector::writeBackup(Backup* backup)
{
	backup->props_internal[PROP_TEX_FLOOR]      = floor_.texture;
	backup->props_internal[PROP_TEX_CEILING]    = ceiling_.texture;
	backup->props_internal[PROP_HEIGHT_FLOOR]   = floor_.height;
	backup->props_internal[PROP_HEIGHT_CEILING] = ceiling_.height;
	backup->props_internal[PROP_LIGHT]          = light_;
	backup->props_internal[PROP_SPECIAL]        = special_;
	backup->props_internal[PROP_ID]             = id_;
}

// -----------------------------------------------------------------------------
// Reads all sector info from a Backup struct
// -----------------------------------------------------------------------------
void MapSector::readBackup(Backup* backup)
{
	// Update texture counts (decrement previous)
	parent_map_->updateFlatUsage(floor_.texture, -1);
	parent_map_->updateFlatUsage(ceiling_.texture, -1);

	floor_.texture   = backup->props_internal[PROP_TEX_FLOOR].stringValueRef();
	ceiling_.texture = backup->props_internal[PROP_TEX_CEILING].stringValueRef();
	floor_.height    = backup->props_internal[PROP_HEIGHT_FLOOR].intValue();
	ceiling_.height  = backup->props_internal[PROP_HEIGHT_CEILING].intValue();
	floor_.plane.set(0, 0, 1, floor_.height);
	ceiling_.plane.set(0, 0, 1, ceiling_.height);
	light_   = backup->props_internal[PROP_LIGHT].intValue();
	special_ = backup->props_internal[PROP_SPECIAL].intValue();
	id_      = backup->props_internal[PROP_ID].intValue();

	// Update texture counts (increment new)
	parent_map_->updateFlatUsage(floor_.texture, 1);
	parent_map_->updateFlatUsage(ceiling_.texture, 1);

	// Update geometry info
	poly_needsupdate_ = true;
	bbox_.reset();
	setGeometryUpdated();
}
