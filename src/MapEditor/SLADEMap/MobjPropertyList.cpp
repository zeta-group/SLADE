
// -----------------------------------------------------------------------------
// SLADE - It's a Doom Editor
// Copyright(C) 2008 - 2017 Simon Judd
//
// Email:       sirjuddington@gmail.com
// Web:         http://slade.mancubus.net
// Filename:    MobjPropertyList.cpp
// Description: A special version of the PropertyList class that uses a vector
//              rather than a map to store properties
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
#include "MobjPropertyList.h"
#include "Utility/StringUtils.h"


// -----------------------------------------------------------------------------
//
// MobjPropertyList Class Functions
//
// -----------------------------------------------------------------------------


// -----------------------------------------------------------------------------
// Returns true if a property with the given name exists, false otherwise
// -----------------------------------------------------------------------------
bool MobjPropertyList::propertyExists(string_view key) const
{
	for (auto& entry : properties_)
		if (entry.name == key)
			return true;

	return false;
}

// -----------------------------------------------------------------------------
// Removes a property value, returns true if [key] was removed or false if [key]
// didn't exist
// -----------------------------------------------------------------------------
bool MobjPropertyList::removeProperty(string_view key)
{
	for (unsigned a = 0; a < properties_.size(); ++a)
	{
		if (properties_[a].name == key)
		{
			properties_[a] = properties_.back();
			properties_.pop_back();
			return true;
		}
	}

	return false;
}

// -----------------------------------------------------------------------------
// Copies all properties to [list]
// -----------------------------------------------------------------------------
void MobjPropertyList::copyTo(MobjPropertyList& list) const
{
	// Clear given list
	list.clear();

	for (auto& entry : properties_)
		list.properties_.emplace_back(entry.name, entry.value);
}

// -----------------------------------------------------------------------------
// Returns a string representation of the property list
// -----------------------------------------------------------------------------
string MobjPropertyList::toString(bool condensed) const
{
	// Init return string
	string ret;

	for (auto& entry : properties_)
	{
		// Skip if no value
		if (!entry.value.hasValue())
			continue;

		// Add "key = value;\n" to the return string
		string val = entry.value.stringValue();
		if (entry.value.type() == Property::Type::String)
		{
			val = StrUtil::escapedString(val);
			val.insert(0, 1, '\"');
			val.push_back('\"');
		}

		ret += entry.name;
		ret += condensed ? "=" : " = ";
		ret += val;
		ret += ";\n";
	}

	return ret;
}
