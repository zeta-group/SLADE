#pragma once

#include "Archive/ArchiveEntry.h"

class MapBackupManager
{
public:
	MapBackupManager()  = default;
	~MapBackupManager() = default;

	bool     writeBackup(vector<ArchiveEntry::UPtr>& map_data, string_view archive_name, string_view map_name) const;
	Archive* openBackup(string_view archive_name, string_view map_name) const;
};
