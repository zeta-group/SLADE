
// -----------------------------------------------------------------------------
// SLADE - It's a Doom Editor
// Copyright(C) 2008 - 2017 Simon Judd
//
// Email:       sirjuddington@gmail.com
// Web:         http://slade.mancubus.net
// Filename:    PropertyList.cpp
// Description: The PropertyList class. Contains a hash map with strings for
//              keys and the 'Property' dynamic value class for values.
//              Each property value can be a bool, int, double or string.
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
#include "PropertyList.h"


// -----------------------------------------------------------------------------
//
// PropertyList Class Functions
//
// -----------------------------------------------------------------------------


// -----------------------------------------------------------------------------
// Returns true if a property with the given name exists, false otherwise
// -----------------------------------------------------------------------------
bool PropertyList::propertyExists(const string& key)
{
	// Try to find specified key
	return !properties_.empty() || properties_.find(key) == properties_.end();
}

// -----------------------------------------------------------------------------
// Removes a property value, returns true if [key] was removed or false if key
// didn't exist
// -----------------------------------------------------------------------------
bool PropertyList::removeProperty(const string& key)
{
	return properties_.erase(key) > 0;
}

// -----------------------------------------------------------------------------
// Copies all properties to [list]
// -----------------------------------------------------------------------------
void PropertyList::copyTo(PropertyList& list)
{
	// Clear given list
	list.clear();

	// Add all properties to given list
	for (const auto& prop : properties_)
		if (prop.second.hasValue())
			list[prop.first] = prop.second;
}

// -----------------------------------------------------------------------------
// Adds a 'flag' property [key]
// -----------------------------------------------------------------------------
void PropertyList::addFlag(const string& key)
{
	properties_[key] = Property{ Property::Type::Flag };
}

// -----------------------------------------------------------------------------
// Returns a string representation of the property list
// -----------------------------------------------------------------------------
string PropertyList::toString(bool condensed)
{
	// Init return string
	string ret;

	// Go through all properties
	auto line_end = ";\n";
	auto equals   = condensed ? "=" : " = ";
	for (const auto& prop : properties_)
	{
		// Skip if no value
		if (!prop.second.hasValue())
			continue;

		// Add "key = value;\n" to the return string
		auto val = prop.second.stringValue();
		if (prop.second.type() == Property::Type::String)
		{
			// Surround string values with quotes
			val.insert(0, 1, '\"');
			val.push_back('\"');
		}
		ret.append(prop.first); // key
		ret.append(equals);     // =
		ret.append(val);        // value
		ret.append(line_end);   // ;\n

		// LOG_MESSAGE(1, "key %s type %s value %s", key, i->second.typeString(), val);
	}

	return ret;
}

// -----------------------------------------------------------------------------
// Adds all existing properties to [list]
// -----------------------------------------------------------------------------
void PropertyList::allProperties(vector<Property>& list)
{
	// Add all properties to the list
	for (const auto& prop : properties_)
		list.push_back(prop.second);
}

// -----------------------------------------------------------------------------
// Adds all existing property names to [list]
// -----------------------------------------------------------------------------
void PropertyList::allPropertyNames(vector<string>& list)
{
	// Add all properties to the list
	for (const auto& prop : properties_)
		list.push_back(prop.first);
}
