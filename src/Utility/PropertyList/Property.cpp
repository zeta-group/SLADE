
// -----------------------------------------------------------------------------
// SLADE - It's a Doom Editor
// Copyright(C) 2008 - 2017 Simon Judd
//
// Email:       sirjuddington@gmail.com
// Web:         http://slade.mancubus.net
// Filename:    Property.cpp
// Description: The Property class. Basically acts as a 'dynamic' variable type,
//              for use in the PropertyList class. Can contain a boolean,
//              integer, floating point (double) or std::string value.
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
#include "Property.h"
#include "Utility/StringUtils.h"

using namespace nonstd::variants;


// -----------------------------------------------------------------------------
//
// Property Class Functions
//
// -----------------------------------------------------------------------------


// -----------------------------------------------------------------------------
// Property class default constructor
// -----------------------------------------------------------------------------
Property::Property(Type type) : type_{ type }, has_value_{ false }
{
	// Set default value depending on type
	switch (type)
	{
	case Type::Bool: value_ = false; break;
	case Type::Int: value_ = 0; break;
	case Type::Float: value_ = 0.; break;
	case Type::Flag: value_ = true; break;
	case Type::UInt: value_ = 0u; break;
	default: break;
	}
}

// -----------------------------------------------------------------------------
// Returns the property value as a bool.
// If [warn_wrong_type] is true, a warning message is written to the log if the
// property is not of boolean type
// -----------------------------------------------------------------------------
bool Property::boolValue(bool warn_wrong_type) const
{
	// If this is a flag, just return boolean 'true' (or equivalent)
	if (type_ == Type::Flag)
		return true;

	// If the value is undefined, default to false
	if (!has_value_)
		return false;

	// Write warning to log if needed
	if (warn_wrong_type && type_ != Type::Bool)
		Log::warning(fmt::sprintf("Warning: Requested Boolean value of a %s Property", typeString()));

	// Return value (convert if needed)
	if (type_ == Type::Bool)
		return get<bool>(value_);
	else if (type_ == Type::Int)
		return !!get<int>(value_);
	else if (type_ == Type::UInt)
		return !!get<unsigned>(value_);
	else if (type_ == Type::Float)
		return !!((int)get<double>(value_));
	else if (type_ == Type::String)
	{
		static const char* zero      = "0";
		static const char* no        = "no";
		static const char* false_str = "false";

		// Anything except "0", "no" or "false" is considered true
		const auto& val_string = get<string>(value_);
		if (val_string == zero || StrUtil::equalCI(val_string, no) || StrUtil::equalCI(val_string, false_str))
			return false;
		else
			return true;
	}

	// Return default boolean value
	return true;
}

// -----------------------------------------------------------------------------
// Returns the property value as an int.
// If [warn_wrong_type] is true, a warning message is written to the log if the
// property is not of integer type
// -----------------------------------------------------------------------------
int Property::intValue(bool warn_wrong_type) const
{
	// If this is a flag, just return boolean 'true' (or equivalent)
	if (type_ == Type::Flag)
		return 1;

	// If the value is undefined, default to 0
	if (!has_value_)
		return 0;

	// Write warning to log if needed
	if (warn_wrong_type && type_ != Type::Int)
		Log::warning(fmt::sprintf("Warning: Requested Integer value of a %s Property", typeString()));

	// Return value (convert if needed)
	if (type_ == Type::Int)
		return get<int>(value_);
	else if (type_ == Type::UInt)
		return (int)get<unsigned>(value_);
	else if (type_ == Type::Bool)
		return (int)get<bool>(value_);
	else if (type_ == Type::Float)
		return (int)get<double>(value_);
	else if (type_ == Type::String)
		return StrUtil::toInt(get<string>(value_));

	// Return default integer value
	return 0;
}

// -----------------------------------------------------------------------------
// Returns the property value as a double.
// If [warn_wrong_type] is true, a warning message is written to the log if the
// property is not of floating point type
// -----------------------------------------------------------------------------
double Property::floatValue(bool warn_wrong_type) const
{
	// If this is a flag, just return boolean 'true' (or equivalent)
	if (type_ == Type::Flag)
		return 1;

	// If the value is undefined, default to 0
	if (!has_value_)
		return 0;

	// Write warning to log if needed
	if (warn_wrong_type && type_ != Type::Float)
		Log::warning(fmt::sprintf("Warning: Requested Float value of a %s Property", typeString()));

	// Return value (convert if needed)
	if (type_ == Type::Float)
		return get<double>(value_);
	else if (type_ == Type::Bool)
		return (double)get<bool>(value_);
	else if (type_ == Type::Int)
		return (double)get<int>(value_);
	else if (type_ == Type::UInt)
		return (double)get<unsigned>(value_);
	else if (type_ == Type::String)
		return StrUtil::toDouble(get<string>(value_));

	// Return default float value
	return 0.0f;
}

// -----------------------------------------------------------------------------
// Returns the property value as a string.
// If [warn_wrong_type] is true, a warning message is written to the log if the
// property is not of string type
// -----------------------------------------------------------------------------
string Property::stringValue(bool warn_wrong_type) const
{
	// If this is a flag, just return boolean 'true' (or equivalent)
	if (type_ == Type::Flag)
		return "1";

	// If the value is undefined, default to null
	if (!has_value_)
		return "";

	// Write warning to log if needed
	if (warn_wrong_type && type_ != Type::String)
		Log::warning(fmt::sprintf("Warning: Requested String value of a %s Property", typeString()));

	// Return value (convert if needed)
	if (type_ == Type::String)
		return get<string>(value_);
	else if (type_ == Type::Int)
		return fmt::sprintf("%d", get<int>(value_));
	else if (type_ == Type::UInt)
		return fmt::sprintf("%d", get<unsigned>(value_));
	else if (type_ == Type::Bool)
	{
		if (get<bool>(value_))
			return "true";
		else
			return "false";
	}
	else if (type_ == Type::Float)
		return fmt::sprintf("%f", get<double>(value_));

	// Return default string value
	return {};
}

// -----------------------------------------------------------------------------
// Returns the property value as a (const) string reference.
// If the property type is not a string this will return either
// StrUtil::BOOL_TRUE/FALSE (for bool/flag types) or StrUtil::EMPTY
// -----------------------------------------------------------------------------
const string& Property::stringValueRef() const
{
	switch (type_)
	{
	case Type::Bool: return get<bool>(value_) ? StrUtil::BOOL_TRUE : StrUtil::BOOL_FALSE;
	case Type::String: return get<string>(value_);
	case Type::Flag: return StrUtil::BOOL_TRUE;
	default: return StrUtil::EMPTY;
	}
}

// -----------------------------------------------------------------------------
// Returns the property value as an unsigned int.
// If [warn_wrong_type] is true, a warning message is written to the log if the
// property is not of integer type
// -----------------------------------------------------------------------------
unsigned Property::unsignedValue(bool warn_wrong_type) const
{
	// If this is a flag, just return boolean 'true' (or equivalent)
	if (type_ == Type::Flag)
		return 1;

	// If the value is undefined, default to 0
	if (!has_value_)
		return 0;

	// Write warning to log if needed
	if (warn_wrong_type && type_ != Type::Int)
		Log::warning(fmt::sprintf("Warning: Requested Integer value of a %s Property", typeString()));

	// Return value (convert if needed)
	if (type_ == Type::Int)
		return get<int>(value_);
	else if (type_ == Type::Bool)
		return (int)get<bool>(value_);
	else if (type_ == Type::Float)
		return (int)get<double>(value_);
	else if (type_ == Type::String)
		return StrUtil::toInt(get<string>(value_));
	else if (type_ == Type::UInt)
		return get<unsigned>(value_);

	// Return default integer value
	return 0;
}

// -----------------------------------------------------------------------------
// Sets the property to [val], and changes its type to boolean if necessary
// -----------------------------------------------------------------------------
void Property::setValue(bool val)
{
	// Change type if necessary
	if (type_ != Type::Bool)
		changeType(Type::Bool);

	// Set value
	value_     = val;
	has_value_ = true;
}

// -----------------------------------------------------------------------------
// Sets the property to [val], and changes its type to integer if necessary
// -----------------------------------------------------------------------------
void Property::setValue(int val)
{
	// Change type if necessary
	if (type_ != Type::Int)
		changeType(Type::Int);

	// Set value
	value_     = val;
	has_value_ = true;
}

// -----------------------------------------------------------------------------
// Sets the property to [val], and changes its type to floating point if
// necessary
// -----------------------------------------------------------------------------
void Property::setValue(double val)
{
	// Change type if necessary
	if (type_ != Type::Float)
		changeType(Type::Float);

	// Set value
	value_     = val;
	has_value_ = true;
}

// -----------------------------------------------------------------------------
// Sets the property to [val], and changes its type to string if necessary
// -----------------------------------------------------------------------------
void Property::setValue(const string& val)
{
	// Change type if necessary
	if (type_ != Type::String)
		changeType(Type::String);

	// Set value
	value_     = val;
	has_value_ = true;
}

// -----------------------------------------------------------------------------
// Sets the property to [val], and changes its type to unsigned int if necessary
// -----------------------------------------------------------------------------
void Property::setValue(unsigned val)
{
	// Change type if necessary
	if (type_ != Type::UInt)
		changeType(Type::UInt);

	// Set value
	value_     = val;
	has_value_ = true;
}

// -----------------------------------------------------------------------------
// Changes the property's value type and gives it a default value
// -----------------------------------------------------------------------------
void Property::changeType(Type newtype)
{
	// Do nothing if changing to same type
	if (type_ == newtype)
		return;

	// Update type
	type_ = newtype;

	// Update value
	if (type_ == Type::Bool)
		value_ = true;
	else if (type_ == Type::Int)
		value_ = 0;
	else if (type_ == Type::Float)
		value_ = 0.;
	else if (type_ == Type::String)
		value_ = StrUtil::EMPTY;
	else if (type_ == Type::Flag)
		value_ = true;
	else if (type_ == Type::UInt)
		value_ = 0u;
}

// -----------------------------------------------------------------------------
// Returns a string representing the property's value type
// -----------------------------------------------------------------------------
string Property::typeString() const
{
	switch (type_)
	{
	case Type::Bool: return "Boolean";
	case Type::Int: return "Integer";
	case Type::Float: return "Float";
	case Type::String: return "String";
	case Type::Flag: return "Flag";
	case Type::UInt: return "Unsigned";
	default: return "Unknown";
	}
}
