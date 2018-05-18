#pragma once

#include "Archive/Archive.h"

class TarArchive : public Archive
{
public:
	TarArchive() : Archive("tar") {}
	~TarArchive() = default;

	// Opening/writing
	bool open(MemChunk& mc) override;                      // Open from MemChunk
	bool write(MemChunk& mc, bool update = true) override; // Write to MemChunk

	// Misc
	bool loadEntryData(ArchiveEntry* entry) override;

	// Static functions
	static bool isTarArchive(MemChunk& mc);
	static bool isTarArchive(string_view filename);

private:
#pragma pack(1)
	struct TarHeader
	{
		/* byte offset */
		char name[100];     /*   0 */
		char mode[8];       /* 100 */
		char uid[8];        /* 108 */
		char gid[8];        /* 116 */
		char size[12];      /* 124 */
		char mtime[12];     /* 136 */
		char chksum[8];     /* 148 */
		char typeflag;      /* 156 */
		char linkname[100]; /* 157 */
		char magic[5];      /* 257 */
		char version[3];    /* 262 */
		char uname[32];     /* 265 */
		char gname[32];     /* 297 */
		char devmajor[8];   /* 329 */
		char devminor[8];   /* 337 */
		char prefix[155];   /* 345 */
		char padding[12];   /* 500 */
	};
#pragma pack()

	static constexpr const char* TMAGIC = "ustar"; /* ustar */
	static constexpr const char* GMAGIC = "  ";    /* two spaces */
	enum TypeFlags
	{
		AREGTYPE = 0,   /* regular file */
		REGTYPE  = '0', /* regular file */
		LNKTYPE  = '1', /* link */
		SYMTYPE  = '2', /* reserved */
		CHRTYPE  = '3', /* character special */
		BLKTYPE  = '4', /* block special */
		DIRTYPE  = '5', /* directory */
		FIFOTYPE = '6', /* FIFO special */
		CONTTYPE = '7', /* reserved */
	};

	static int    tarSum(const char* field, int size);
	static bool   writeOctal(size_t sum, char* field, int size);
	static bool   checksum(TarHeader* header);
	static size_t makeChecksum(TarHeader* header);
	static void   defaultHeader(TarHeader* header);
};
