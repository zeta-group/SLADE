#pragma once

#include "Archive/Archive.h"

class RffArchive : public TreelessArchive
{
public:
	RffArchive() : TreelessArchive("rff") {}
	~RffArchive() = default;

	// RFF specific
	uint32_t getEntryOffset(ArchiveEntry* entry);
	void     setEntryOffset(ArchiveEntry* entry, uint32_t offset);

	// Opening/writing
	bool open(MemChunk& mc) override;                      // Open from MemChunk
	bool write(MemChunk& mc, bool update = true) override; // Write to MemChunk

	// Misc
	bool loadEntryData(ArchiveEntry* entry) override;

	// Entry addition/removal
	ArchiveEntry* addEntry(
		ArchiveEntry*    entry,
		unsigned         position = 0xFFFFFFFF,
		ArchiveTreeNode* dir      = nullptr,
		bool             copy     = false) override;
	ArchiveEntry* addEntry(ArchiveEntry* entry, string_view add_namespace, bool copy = false) override;

	// Static functions
	static bool isRffArchive(MemChunk& mc);
	static bool isRffArchive(string_view filename);

private:
	// From ZDoom: store lump data. We need it because of the encryption
	struct Lump
	{
		uint32_t DontKnow1[4];
		uint32_t FilePos;
		uint32_t Size;
		uint32_t DontKnow2;
		uint32_t Time;
		uint8_t  Flags;
		char     Extension[3];
		char     Name[8];
		uint32_t IndexNum; // Used by .sfx, possibly others
	};
};
