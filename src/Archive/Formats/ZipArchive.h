#pragma once

#include "Archive/Archive.h"

class ZipArchive : public Archive
{
public:
	ZipArchive() : Archive("zip") {}
	~ZipArchive();

	// Opening
	bool open(string_view filename) override; // Open from File
	bool open(MemChunk& mc) override;         // Open from MemChunk

	// Writing/Saving
	bool write(MemChunk& mc, bool update = true) override;         // Write to MemChunk
	bool write(string_view filename, bool update = true) override; // Write to File

	// Misc
	bool loadEntryData(ArchiveEntry* entry) override;

	// Entry addition/removal
	ArchiveEntry* addEntry(ArchiveEntry* entry, string_view add_namespace, bool copy = false) override;

	// Detection
	MapDesc         getMapInfo(ArchiveEntry* maphead) override;
	vector<MapDesc> detectMaps() override;

	// Search
	ArchiveEntry*         findFirst(SearchOptions& options) override;
	ArchiveEntry*         findLast(SearchOptions& options) override;
	vector<ArchiveEntry*> findAll(SearchOptions& options) override;

	// Static functions
	static bool isZipArchive(MemChunk& mc);
	static bool isZipArchive(string_view filename);

private:
	string temp_file_;

	// Struct representing a zip file header
	struct ZipFileHeader
	{
		uint32_t sig;
		uint16_t version;
		uint16_t flag;
		uint16_t compression;
		uint16_t mod_time;
		uint16_t mod_date;
		uint32_t crc;
		uint32_t size_comp;
		uint32_t size_orig;
		uint16_t len_fn;
		uint16_t len_extra;
	};

	void generateTempFileName(string_view filename);
};
