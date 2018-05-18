#pragma once

#include "common.h"

class FileMonitor : public wxTimer
{
public:
	FileMonitor(string_view filename, bool start = true);
	virtual ~FileMonitor() = default;

	wxProcess*    getProcess() const { return process_.get(); }
	const string& getFilename() const { return filename_; }

	virtual void fileModified() {}
	virtual void processTerminated() {}

	void Notify() override;
	void onEndProcess(wxProcessEvent& e);

protected:
	string filename_;
	time_t file_modified_;

private:
	std::unique_ptr<wxProcess> process_;
};

class Archive;
class DB2MapFileMonitor : public FileMonitor
{
public:
	DB2MapFileMonitor(string_view filename, Archive* archive, string_view map_name);
	~DB2MapFileMonitor() = default;

	void fileModified() override;
	void processTerminated() override;

private:
	Archive* archive_;
	string   map_name_;
};
