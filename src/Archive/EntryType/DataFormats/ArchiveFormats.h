#pragma once

#include "Archive/Formats/All.h"

class WadDataFormat : public EntryDataFormat
{
public:
	WadDataFormat() : EntryDataFormat("archive_wad"){};
	~WadDataFormat() = default;

	int isThisFormat(MemChunk& mc) override { return WadArchive::isWadArchive(mc) ? EDF_TRUE : EDF_FALSE; }
};

class ZipDataFormat : public EntryDataFormat
{
public:
	ZipDataFormat() : EntryDataFormat("archive_zip"){};
	~ZipDataFormat() = default;

	int isThisFormat(MemChunk& mc) override { return ZipArchive::isZipArchive(mc) ? EDF_TRUE : EDF_FALSE; }
};

class LibDataFormat : public EntryDataFormat
{
public:
	LibDataFormat() : EntryDataFormat("archive_lib"){};
	~LibDataFormat() = default;

	int isThisFormat(MemChunk& mc) override { return LibArchive::isLibArchive(mc) ? EDF_TRUE : EDF_FALSE; }
};

class DatDataFormat : public EntryDataFormat
{
public:
	DatDataFormat() : EntryDataFormat("archive_dat"){};
	~DatDataFormat() = default;

	int isThisFormat(MemChunk& mc) override { return DatArchive::isDatArchive(mc) ? EDF_TRUE : EDF_FALSE; }
};

class ResDataFormat : public EntryDataFormat
{
public:
	ResDataFormat() : EntryDataFormat("archive_res"){};
	~ResDataFormat() = default;

	int isThisFormat(MemChunk& mc) override { return ResArchive::isResArchive(mc) ? EDF_TRUE : EDF_FALSE; }
};

class PakDataFormat : public EntryDataFormat
{
public:
	PakDataFormat() : EntryDataFormat("archive_pak"){};
	~PakDataFormat() = default;

	int isThisFormat(MemChunk& mc) override { return PakArchive::isPakArchive(mc) ? EDF_TRUE : EDF_FALSE; }
};

class BSPDataFormat : public EntryDataFormat
{
public:
	BSPDataFormat() : EntryDataFormat("archive_bsp"){};
	~BSPDataFormat() = default;

	int isThisFormat(MemChunk& mc) override { return BSPArchive::isBSPArchive(mc) ? EDF_TRUE : EDF_FALSE; }
};

class Wad2DataFormat : public EntryDataFormat
{
public:
	Wad2DataFormat() : EntryDataFormat("archive_wad2") {}
	~Wad2DataFormat() = default;

	int isThisFormat(MemChunk& mc) override { return Wad2Archive::isWad2Archive(mc) ? EDF_TRUE : EDF_FALSE; }
};

class WadJDataFormat : public EntryDataFormat
{
public:
	WadJDataFormat() : EntryDataFormat("archive_wadj") {}
	~WadJDataFormat() = default;

	int isThisFormat(MemChunk& mc) override { return WadJArchive::isWadJArchive(mc) ? EDF_TRUE : EDF_FALSE; }
};

class GrpDataFormat : public EntryDataFormat
{
public:
	GrpDataFormat() : EntryDataFormat("archive_grp"){};
	~GrpDataFormat() = default;

	int isThisFormat(MemChunk& mc) override { return GrpArchive::isGrpArchive(mc) ? EDF_TRUE : EDF_FALSE; }
};

class RffDataFormat : public EntryDataFormat
{
public:
	RffDataFormat() : EntryDataFormat("archive_rff"){};
	~RffDataFormat() = default;

	int isThisFormat(MemChunk& mc) override { return RffArchive::isRffArchive(mc) ? EDF_TRUE : EDF_FALSE; }
};

class GobDataFormat : public EntryDataFormat
{
public:
	GobDataFormat() : EntryDataFormat("archive_gob"){};
	~GobDataFormat() = default;

	int isThisFormat(MemChunk& mc) override { return GobArchive::isGobArchive(mc) ? EDF_TRUE : EDF_FALSE; }
};

class LfdDataFormat : public EntryDataFormat
{
public:
	LfdDataFormat() : EntryDataFormat("archive_lfd"){};
	~LfdDataFormat() = default;

	int isThisFormat(MemChunk& mc) override { return LfdArchive::isLfdArchive(mc) ? EDF_TRUE : EDF_FALSE; }
};

class ADatDataFormat : public EntryDataFormat
{
public:
	ADatDataFormat() : EntryDataFormat("archive_adat"){};
	~ADatDataFormat() = default;

	int isThisFormat(MemChunk& mc) override { return ADatArchive::isADatArchive(mc) ? EDF_TRUE : EDF_FALSE; }
};

class HogDataFormat : public EntryDataFormat
{
public:
	HogDataFormat() : EntryDataFormat("archive_hog"){};
	~HogDataFormat() = default;

	int isThisFormat(MemChunk& mc) override { return HogArchive::isHogArchive(mc) ? EDF_TRUE : EDF_FALSE; }
};

class WolfDataFormat : public EntryDataFormat
{
public:
	WolfDataFormat() : EntryDataFormat("archive_wolf"){};
	~WolfDataFormat() = default;

	int isThisFormat(MemChunk& mc) override { return WolfArchive::isWolfArchive(mc) ? EDF_TRUE : EDF_FALSE; }
};

class GZipDataFormat : public EntryDataFormat
{
public:
	GZipDataFormat() : EntryDataFormat("archive_gzip"){};
	~GZipDataFormat() = default;

	int isThisFormat(MemChunk& mc) override { return GZipArchive::isGZipArchive(mc) ? EDF_TRUE : EDF_FALSE; }
};

class BZip2DataFormat : public EntryDataFormat
{
public:
	BZip2DataFormat() : EntryDataFormat("archive_bz2"){};
	~BZip2DataFormat() = default;

	int isThisFormat(MemChunk& mc) override { return BZip2Archive::isBZip2Archive(mc) ? EDF_TRUE : EDF_FALSE; }
};

class TarDataFormat : public EntryDataFormat
{
public:
	TarDataFormat() : EntryDataFormat("archive_tar"){};
	~TarDataFormat() = default;

	int isThisFormat(MemChunk& mc) override { return TarArchive::isTarArchive(mc) ? EDF_TRUE : EDF_FALSE; }
};

class DiskDataFormat : public EntryDataFormat
{
public:
	DiskDataFormat() : EntryDataFormat("archive_disk"){};
	~DiskDataFormat() = default;

	int isThisFormat(MemChunk& mc) override { return PakArchive::isPakArchive(mc) ? EDF_TRUE : EDF_FALSE; }
};

class PodArchiveDataFormat : public EntryDataFormat
{
public:
	PodArchiveDataFormat() : EntryDataFormat("archive_pod"){};
	~PodArchiveDataFormat() = default;

	int isThisFormat(MemChunk& mc) override { return PodArchive::isPodArchive(mc) ? EDF_PROBABLY : EDF_FALSE; }
};

class ChasmBinArchiveDataFormat : public EntryDataFormat
{
public:
	ChasmBinArchiveDataFormat() : EntryDataFormat("archive_chasm_bin"){};

	int isThisFormat(MemChunk& mc) override { return ChasmBinArchive::isChasmBinArchive(mc) ? EDF_TRUE : EDF_FALSE; }
};

class SinArchiveDataFormat : public EntryDataFormat
{
public:
	SinArchiveDataFormat() : EntryDataFormat("archive_sin"){};

	int isThisFormat(MemChunk& mc) override { return SiNArchive::isSiNArchive(mc) ? EDF_TRUE : EDF_FALSE; }
};
