#pragma once

#include <SQLiteCpp/SQLiteCpp.h>
#include <thread>

namespace Database
{
class Context
{
public:
	Context(string_view file_path = {});
	~Context() { close(); }

	const string&     filePath() const { return file_path_; }
	SQLite::Database* connectionRO() const { return connection_ro_.get(); }
	SQLite::Database* connectionRW() const { return connection_rw_.get(); }

	bool isOpen() const { return connection_ro_.get(); }

	bool open(string_view file_path);
	bool close();

	SQLite::Statement* cachedQuery(string_view id);
	SQLite::Statement* getOrCreateCachedQuery(string_view, const char* sql, bool writes = false);

	int exec(const string& query) const;
	int exec(const char* query) const;

private:
	string file_path_;

	unique_ptr<SQLite::Database> connection_ro_;
	unique_ptr<SQLite::Database> connection_rw_;

	std::map<string, unique_ptr<SQLite::Statement>, std::less<>> cached_queries_;
};

Context&          global();
SQLite::Database* connectionRO();
SQLite::Database* connectionRW();

template<typename T> bool rowExists(SQLite::Database& connection, string_view table_name, string_view col_name, T value)
{
	SQLite::Statement sql(connection, fmt::format("SELECT * FROM {} WHERE {} = ?", table_name, col_name));
	sql.bind(1, value);
	return sql.executeStep();
}

int exec(const string& query, SQLite::Database* connection = nullptr);
int exec(const char* query, SQLite::Database* connection = nullptr);

bool fileExists();
bool init();
void close();
} // namespace Database
