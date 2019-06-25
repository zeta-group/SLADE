
// -----------------------------------------------------------------------------
// SLADE - It's a Doom Editor
// Copyright(C) 2008 - 2019 Simon Judd
//
// Email:       sirjuddington@gmail.com
// Web:         http://slade.mancubus.net
// Filename:    Database.cpp
// Description: Functions for working with the SLADE program database.
//              The Context class keeps connections open to a database, since
//              opening a new connection is expensive. It can also keep cached
//              sql queries (for frequent reuse)
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
#include "Database.h"
#include "App.h"
#include "Archive/ArchiveManager.h"
#include "Utility/FileUtils.h"


// -----------------------------------------------------------------------------
//
// Variables
//
// -----------------------------------------------------------------------------
namespace Database
{
Context db_global;
} // namespace Database


// -----------------------------------------------------------------------------
//
// Database::Context Class Functions
//
// -----------------------------------------------------------------------------


// -----------------------------------------------------------------------------
// Context class constructor
// -----------------------------------------------------------------------------
Database::Context::Context(string_view file_path)
{
	if (!file_path.empty())
		open(file_path);
}

// -----------------------------------------------------------------------------
// Opens connections to the database file at [file_path].
// Returns false if any existing connections couldn't be closed, true otherwise
// -----------------------------------------------------------------------------
bool Database::Context::open(string_view file_path)
{
	if (!close())
		return false;

	file_path_     = file_path;
	connection_ro_ = std::make_unique<SQLite::Database>(file_path_, SQLite::OPEN_READONLY);
	connection_rw_ = std::make_unique<SQLite::Database>(file_path_, SQLite::OPEN_READWRITE);

	return true;
}

// -----------------------------------------------------------------------------
// Closes the context's connections to its database
// -----------------------------------------------------------------------------
bool Database::Context::close()
{
	if (!connection_ro_)
		return true;

	try
	{
		cached_queries_.clear();
		connection_ro_ = nullptr;
		connection_rw_ = nullptr;
	}
	catch (std::exception& ex)
	{
		Log::error("Error closing connections for database {}: {}", file_path_, ex.what());
		return false;
	}

	file_path_.clear();

	return true;
}

// -----------------------------------------------------------------------------
// Returns the cached query [id]
// -----------------------------------------------------------------------------
SQLite::Statement* Database::Context::cachedQuery(string_view id)
{
	auto i = cached_queries_.find(id);
	return i == cached_queries_.end() ? nullptr : i->second.get();
}

// -----------------------------------------------------------------------------
// Returns the cached query at [id] if it exists, otherwise creates a new cached
// query from the given [sql] string and returns it.
// If [writes] is true, the created query will use the read+write connection.
// -----------------------------------------------------------------------------
SQLite::Statement* Database::Context::getOrCreateCachedQuery(string_view id, const char* sql, bool writes)
{
	// Check for existing cached query [id]
	auto i = cached_queries_.find(id);
	if (i != cached_queries_.end())
		return i->second.get();

	// Check connection
	if (!connection_ro_)
		return nullptr;

	// Create & add cached query
	auto& db            = writes ? *connection_rw_ : *connection_ro_;
	auto  statement     = std::make_unique<SQLite::Statement>(db, sql);
	auto  ptr           = statement.get();
	cached_queries_.emplace(id, std::move(statement));

	return ptr;
}

// -----------------------------------------------------------------------------
// Executes an sql [query] on the database.
// Returns the number of rows modified/created by the query, or 0 if the context
// is not connected
// -----------------------------------------------------------------------------
int Database::Context::exec(const string& query) const
{
	return connection_rw_ ? connection_rw_->exec(query) : 0;
}
int Database::Context::exec(const char* query) const
{
	return connection_rw_ ? connection_rw_->exec(query) : 0;
}


// -----------------------------------------------------------------------------
//
// Database Namespace Functions
//
// -----------------------------------------------------------------------------

namespace Database
{
// -----------------------------------------------------------------------------
// Creates a new program database file using the create_db.sql script in
// slade.pk3
// -----------------------------------------------------------------------------
bool createDatabase(const string& file_path)
{
	SQLite::Database db(file_path, SQLite::OPEN_READWRITE | SQLite::OPEN_CREATE);

	// Get script entry as string
	auto   entry = App::archiveManager().programResourceArchive()->entryAtPath("database/create_db.sql");
	string create_query;
	create_query.assign((const char*)entry->rawData(), entry->size());

	db.exec(create_query);

	return true;
}
} // namespace Database

// -----------------------------------------------------------------------------
// Returns the 'global' database connection context.
// This can only be used from the main thread
// -----------------------------------------------------------------------------
Database::Context& Database::global()
{
	// Check we are on the main thread
	if (std::this_thread::get_id() != App::mainThreadId())
		Log::warning("A non-main thread is requesting the global database connection context");

	return db_global;
}

// -----------------------------------------------------------------------------
// Returns the 'global' read-only database connection, or nullptr if the
// context isn't connected or this isn't called from the main thread
// -----------------------------------------------------------------------------
SQLite::Database* Database::connectionRO()
{
	// Check we are on the main thread
	if (std::this_thread::get_id() != App::mainThreadId())
	{
		Log::error("Can't get global database connection from non-main thread, use newConnection instead");
		return nullptr;
	}

	return db_global.connectionRO();
}

// -----------------------------------------------------------------------------
// Returns the 'global' read+write database connection, or nullptr if the
// context isn't connected or this isn't called from the main thread
// -----------------------------------------------------------------------------
SQLite::Database* Database::connectionRW()
{
	// Check we are on the main thread
	if (std::this_thread::get_id() != App::mainThreadId())
	{
		Log::error("Can't get global database connection from non-main thread, use newConnection instead");
		return nullptr;
	}

	return db_global.connectionRW();
}

// -----------------------------------------------------------------------------
// Executes an sql [query] on the database using the given [connection].
// If [connection] is null, the global read+write connection is used.
// Returns the number of rows modified/created by the query, or 0 if the global
// connection context is not connected
// -----------------------------------------------------------------------------
int Database::exec(const string& query, SQLite::Database* connection)
{
	if (!connection)
		connection = connectionRW();
	if (!connection)
		return 0;

	return connection->exec(query);
}
int Database::exec(const char* query, SQLite::Database* connection)
{
	if (!connection)
		connection = connectionRW();
	if (!connection)
		return 0;

	return connection->exec(query);
}

// -----------------------------------------------------------------------------
// Returns true if the program database file exists
// -----------------------------------------------------------------------------
bool Database::fileExists()
{
	return FileUtil::fileExists(App::path("slade.sqlite", App::Dir::User));
}

// -----------------------------------------------------------------------------
// Initialises the program database, creating it if it doesn't exist and opening
// the 'global' connection context.
// Returns false if the database couldn't be created or the global context
// failed to open, true otherwise
// -----------------------------------------------------------------------------
bool Database::init()
{
	auto db_path = App::path("slade.sqlite", App::Dir::User);

	// Create database if needed
	if (!FileUtil::fileExists(db_path))
		if (!createDatabase(db_path))
			return false;

	// Open global connections to database (for main thread usage only)
	if (!db_global.open(db_path))
		return false;

	return true;
}

// -----------------------------------------------------------------------------
// Closes the global connection context to the database
// -----------------------------------------------------------------------------
void Database::close()
{
	db_global.close();
}
