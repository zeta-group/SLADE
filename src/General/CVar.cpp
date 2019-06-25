
// -----------------------------------------------------------------------------
// SLADE - It's a Doom Editor
// Copyright(C) 2008 - 2019 Simon Judd
//
// Email:       sirjuddington@gmail.com
// Web:         http://slade.mancubus.net
// Filename:    CVar.cpp
// Description: CVar system. 'Borrowed' from ZDoom, which is written by Randi
//              Heit
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
#include "General/Database.h"
#include "Utility/StringUtils.h"
#include "thirdparty/fmt/fmt/format.h"


// -----------------------------------------------------------------------------
//
// Variables
//
// -----------------------------------------------------------------------------
namespace
{
CVar**   cvars;
uint16_t n_cvars;
} // namespace


// -----------------------------------------------------------------------------
//
// Functions
//
// -----------------------------------------------------------------------------
namespace
{
// -----------------------------------------------------------------------------
// Adds a CVar to the CVar list
// -----------------------------------------------------------------------------
void addCVarList(CVar* cvar)
{
	cvars          = (CVar**)realloc(cvars, (n_cvars + 1) * sizeof(CVar*));
	cvars[n_cvars] = cvar;
	n_cvars++;
}

// -----------------------------------------------------------------------------
// Updates [cvar] in the database with the given [value]
// -----------------------------------------------------------------------------
template<typename T> void updateCVarDB(CVar* cvar, T value)
{
	try
	{
		auto sql_update = Database::global().getOrCreateCachedQuery(
			"cvar_update", "REPLACE INTO cvar(name, value) VALUES (?,?)", true);
		if (!sql_update)
			return;

		sql_update->clearBindings();
		sql_update->bind(1, cvar->name);
		sql_update->bind(2, value);
		sql_update->exec();
		sql_update->reset();
	}
	catch (SQLite::Exception& e)
	{
		Log::error("Unable to update cvar \"{}\" in the database: {}", cvar->name, e.getErrorStr());
	}
}
} // namespace


// -----------------------------------------------------------------------------
//
// CVar (and subclasses) Class Functions
//
// -----------------------------------------------------------------------------


// -----------------------------------------------------------------------------
// CIntCVar class constructor
// -----------------------------------------------------------------------------
CIntCVar::CIntCVar(const string& NAME, int defval, uint16_t FLAGS)
{
	name  = NAME;
	flags = FLAGS;
	value = defval;
	type  = Type::Integer;
	addCVarList(this);
}

// -----------------------------------------------------------------------------
// Updates the saved cvar value in the database
// -----------------------------------------------------------------------------
void CIntCVar::updateDB()
{
	updateCVarDB<int>(this, value);
}

// -----------------------------------------------------------------------------
// CBoolCVar class constructor
// -----------------------------------------------------------------------------
CBoolCVar::CBoolCVar(const string& NAME, bool defval, uint16_t FLAGS)
{
	name  = NAME;
	flags = FLAGS;
	value = defval;
	type  = Type::Boolean;
	addCVarList(this);
}

// -----------------------------------------------------------------------------
// Updates the saved cvar value in the database
// -----------------------------------------------------------------------------
void CBoolCVar::updateDB()
{
	updateCVarDB<bool>(this, value);
}

// -----------------------------------------------------------------------------
// CFloatCVar class constructor
// -----------------------------------------------------------------------------
CFloatCVar::CFloatCVar(const string& NAME, double defval, uint16_t FLAGS)
{
	name  = NAME;
	flags = FLAGS;
	value = defval;
	type  = Type::Float;
	addCVarList(this);
}

// -----------------------------------------------------------------------------
// Updates the saved cvar value in the database
// -----------------------------------------------------------------------------
void CFloatCVar::updateDB()
{
	updateCVarDB<double>(this, value);
}

// -----------------------------------------------------------------------------
// CStringCVar class constructor
// -----------------------------------------------------------------------------
CStringCVar::CStringCVar(const string& NAME, const string& defval, uint16_t FLAGS)
{
	name  = NAME;
	flags = FLAGS;
	value = defval;
	type  = Type::String;
	addCVarList(this);
}

// -----------------------------------------------------------------------------
// Updates the saved cvar value in the database
// -----------------------------------------------------------------------------
void CStringCVar::updateDB()
{
	updateCVarDB<string>(this, value);
}


// -----------------------------------------------------------------------------
//
// CVar Class Static Functions
//
// -----------------------------------------------------------------------------


// -----------------------------------------------------------------------------
// Finds a CVar by name
// -----------------------------------------------------------------------------
CVar* CVar::get(const string& name)
{
	for (unsigned i = 0; i < n_cvars; ++i)
	{
		if (cvars[i]->name == name)
			return cvars[i];
	}

	return nullptr;
}

// -----------------------------------------------------------------------------
// Adds all cvar names to a vector of strings
// -----------------------------------------------------------------------------
void CVar::putList(vector<string>& list)
{
	for (unsigned i = 0; i < n_cvars; ++i)
	{
		if (!(cvars[i]->flags & Flag::Secret))
			list.push_back(cvars[i]->name);
	}
}

// -----------------------------------------------------------------------------
// Reads all saved cvars from the database
// -----------------------------------------------------------------------------
void CVar::readFromDB()
{
	auto db = Database::connectionRO();
	if (!db)
	{
		Log::warning("Unable to open database connection, not loading CVars");
		return;
	}

	SQLite::Statement sql_cvars(*db, "SELECT * FROM cvar");
	while (sql_cvars.executeStep())
	{
		auto name = sql_cvars.getColumn("name").getString();
		auto cvar = get(name);

		if (!cvar)
			continue;

		auto col_value = sql_cvars.getColumn("value");

		switch (cvar->type)
		{
		case Type::Boolean: dynamic_cast<CBoolCVar*>(cvar)->value = col_value.getInt() != 0; break;
		case Type::Integer: dynamic_cast<CIntCVar*>(cvar)->value = col_value.getInt(); break;
		case Type::Float: dynamic_cast<CFloatCVar*>(cvar)->value = col_value.getDouble(); break;
		case Type::String: dynamic_cast<CStringCVar*>(cvar)->value = col_value.getString(); break;
		default: break;
		}
	}
}

// -----------------------------------------------------------------------------
// Reads [value] into the CVar with matching [name],
// or does nothing if no CVar [name] exists
// -----------------------------------------------------------------------------
void CVar::set(const string& name, const string& value)
{
	for (unsigned i = 0; i < n_cvars; ++i)
	{
		auto cvar = cvars[i];
		if (name == cvar->name)
		{
			if (cvar->type == Type::Integer)
				*dynamic_cast<CIntCVar*>(cvar) = StrUtil::asInt(value);

			if (cvar->type == Type::Boolean)
				*dynamic_cast<CBoolCVar*>(cvar) = StrUtil::asBoolean(value);

			if (cvar->type == Type::Float)
				*dynamic_cast<CFloatCVar*>(cvar) = StrUtil::asFloat(value);

			if (cvar->type == Type::String)
				*dynamic_cast<CStringCVar*>(cvar) = value;
		}
	}
}
