#pragma once

#include "Graphics/Palette/Palette.h"

class ArchiveEntry;
class ExternalEditFileMonitor;

class ExternalEditManager
{
	friend class ExternalEditFileMonitor;

public:
	ExternalEditManager();
	~ExternalEditManager();

	bool openEntryExternal(ArchiveEntry* entry, string_view editor, string_view category);

	typedef std::unique_ptr<ExternalEditManager> UPtr;

private:
	vector<std::unique_ptr<ExternalEditFileMonitor>> file_monitors_;

	void monitorStopped(ExternalEditFileMonitor* monitor);
};
