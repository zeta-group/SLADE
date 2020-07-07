#pragma once

namespace slade
{
class Archive;

namespace database
{
	class Context;
}

namespace library
{
	void           addOrUpdateArchive(string_view file_path, const Archive& archive, database::Context* db = nullptr);
	vector<string> recentFiles(unsigned count = 20, database::Context* db = nullptr);
	int            archiveFileId(const Archive& archive, database::Context* db = nullptr);

	sigslot::signal<>& signalUpdated();
} // namespace library
} // namespace slade
