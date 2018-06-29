
// -----------------------------------------------------------------------------
// SLADE - It's a Doom Editor
// Copyright(C) 2008 - 2017 Simon Judd
//
// Email:       sirjuddington@gmail.com
// Web:         http://slade.mancubus.net
// Filename:    RffArchive.cpp
// Description: RffArchive, archive class to handle Blood's encrypted RFF
//              archives
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


/*
** Part of this file have been taken or adapted from ZDoom's rff_file.cpp.
**
**---------------------------------------------------------------------------
** Copyright 1998-2009 Randy Heit
** Copyright 2005-2009 Christoph Oelckers
** All rights reserved.
**
** Redistribution and use in source and binary forms, with or without
** modification, are permitted provided that the following conditions
** are met:
**
** 1. Redistributions of source code must retain the above copyright
**    notice, this list of conditions and the following disclaimer.
** 2. Redistributions in binary form must reproduce the above copyright
**    notice, this list of conditions and the following disclaimer in the
**    documentation and/or other materials provided with the distribution.
** 3. The name of the author may not be used to endorse or promote products
**    derived from this software without specific prior written permission.
**
** THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR
** IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
** OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
** IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT,
** INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT
** NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
** DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
** THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
** (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
** THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
**---------------------------------------------------------------------------
**
**
*/


// -----------------------------------------------------------------------------
//
// Includes
//
// -----------------------------------------------------------------------------
#include "Main.h"
#include "General/UI.h"
#include "RffArchive.h"
#include "Utility/FileUtils.h"
#include "Utility/StringUtils.h"


// -----------------------------------------------------------------------------
//
// External Variables
//
// -----------------------------------------------------------------------------
EXTERN_CVAR(Bool, wad_force_uppercase)
EXTERN_CVAR(Bool, archive_load_data)


// -----------------------------------------------------------------------------
//
// Helper Functions & Structs
//
// -----------------------------------------------------------------------------


// -----------------------------------------------------------------------------
// From ZDoom: decrypts RFF data
// -----------------------------------------------------------------------------
void BloodCrypt(void* data, int key, int len)
{
	int p = (uint8_t)key, i;

	for (i = 0; i < len; ++i)
	{
		((uint8_t*)data)[i] ^= (unsigned char)(p + (i >> 1));
	}
}


// -----------------------------------------------------------------------------
//
// RffArchive Class Functions
//
// -----------------------------------------------------------------------------


// -----------------------------------------------------------------------------
// Returns the file byte offset for [entry]
// -----------------------------------------------------------------------------
uint32_t RffArchive::getEntryOffset(ArchiveEntry* entry)
{
	// Check entry
	if (!checkEntry(entry))
		return 0;

	return (uint32_t)(int)entry->exProp("Offset");
}

// -----------------------------------------------------------------------------
// Sets the file byte offset for [entry]
// -----------------------------------------------------------------------------
void RffArchive::setEntryOffset(ArchiveEntry* entry, uint32_t offset)
{
	// Check entry
	if (!checkEntry(entry))
		return;

	entry->exProp("Offset") = (int)offset;
}

// -----------------------------------------------------------------------------
// Reads grp format data from a MemChunk
// Returns true if successful, false otherwise
// -----------------------------------------------------------------------------
bool RffArchive::open(MemChunk& mc)
{
	// Check data was given
	if (!mc.hasData())
		return false;

	// Read grp header
	uint8_t  magic[4];
	uint32_t version, dir_offset, num_lumps;

	mc.seek(0, SEEK_SET);
	mc.read(magic, 4);       // Should be "RFF\x18"
	mc.read(&version, 4);    // 0x01 0x03 \x00 \x00
	mc.read(&dir_offset, 4); // Offset to directory
	mc.read(&num_lumps, 4);  // No. of lumps in rff

	// Byteswap values for big endian if needed
	dir_offset = wxINT32_SWAP_ON_BE(dir_offset);
	num_lumps  = wxINT32_SWAP_ON_BE(num_lumps);
	version    = wxINT32_SWAP_ON_BE(version);

	// Check the header
	if (magic[0] != 'R' || magic[1] != 'F' || magic[2] != 'F' || magic[3] != 0x1A || version != 0x301)
	{
		Log::error(fmt::format("RffArchive::openFile: File {} has invalid header", filename_));
		Global::error = "Invalid rff header";
		return false;
	}

	// Stop announcements (don't want to be announcing modification due to entries being added etc)
	setMuted(true);

	// Read the directory
	Lump* lumps = new Lump[num_lumps];
	mc.seek(dir_offset, SEEK_SET);
	UI::setSplashProgressMessage("Reading rff archive data");
	mc.read(lumps, num_lumps * sizeof(Lump));
	BloodCrypt(lumps, dir_offset, num_lumps * sizeof(Lump));
	for (uint32_t d = 0; d < num_lumps; d++)
	{
		// Update splash window progress
		UI::setSplashProgress(((float)d / (float)num_lumps));

		// Read lump info
		char     name[13] = { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 };
		uint32_t offset   = wxINT32_SWAP_ON_BE(lumps[d].FilePos);
		uint32_t size     = wxINT32_SWAP_ON_BE(lumps[d].Size);

		// Reconstruct name
		int i, j = 0;
		for (i = 0; i < 8; ++i)
		{
			if (lumps[d].Name[i] == 0)
				break;
			name[i] = lumps[d].Name[i];
		}
		for (name[i++] = '.'; j < 3; ++j)
			name[i + j] = lumps[d].Extension[j];

		// If the lump data goes past the end of the file,
		// the rfffile is invalid
		if (offset + size > mc.size())
		{
			Log::error(1, "RffArchive::open: rff archive is invalid or corrupt");
			Global::error = "Archive is invalid and/or corrupt";
			setMuted(false);
			return false;
		}

		// Create & setup lump
		ArchiveEntry* nlump = new ArchiveEntry(name, size);
		nlump->setLoaded(false);
		nlump->exProp("Offset") = (int)offset;
		nlump->setState(0);

		// Is the entry encrypted?
		if (lumps[d].Flags & 0x10)
			nlump->setEncryption(ENC_BLOOD);

		// Add to entry list
		rootDir()->addEntry(nlump);
	}
	delete[] lumps;

	// Detect all entry types
	MemChunk edata;
	UI::setSplashProgressMessage("Detecting entry types");
	for (size_t a = 0; a < numEntries(); a++)
	{
		// Update splash window progress
		UI::setSplashProgress((((float)a / (float)num_lumps)));

		// Get entry
		ArchiveEntry* entry = getEntry(a);

		// Read entry data if it isn't zero-sized
		if (entry->size() > 0)
		{
			// Read the entry data
			mc.exportMemChunk(edata, getEntryOffset(entry), entry->size());

			// If the entry is encrypted, decrypt it
			if (entry->isEncrypted())
			{
				uint8_t* cdata = new uint8_t[entry->size()];
				memcpy(cdata, edata.data(), entry->size());
				int cryptlen = entry->size() < 256 ? entry->size() : 256;
				BloodCrypt(cdata, 0, cryptlen);
				edata.importMem(cdata, entry->size());
				delete[] cdata;
			}

			// Import data
			entry->importMemChunk(edata);
		}

		// Detect entry type
		EntryType::detectEntryType(entry);

		// Unload entry data if needed
		if (!archive_load_data)
			entry->unloadData();

		// Set entry to unchanged
		entry->setState(0);
	}

	// Detect maps (will detect map entry types)
	// UI::setSplashProgressMessage("Detecting maps");
	// detectMaps();

	// Setup variables
	setMuted(false);
	setModified(false);
	announce("opened");

	UI::setSplashProgressMessage("");

	return true;
}

// -----------------------------------------------------------------------------
// Writes the rff archive to a MemChunk
// Not implemented because of encrypted directory and unknown stuff.
// -----------------------------------------------------------------------------
bool RffArchive::write(MemChunk& mc, bool update)
{
	Log::warning(1, "Saving RFF files is not implemented because the format is not entirely known.");
	return false;
}

// -----------------------------------------------------------------------------
// Loads an entry's data from the grpfile
// Returns true if successful, false otherwise
// -----------------------------------------------------------------------------
bool RffArchive::loadEntryData(ArchiveEntry* entry)
{
	return loadEntryDataAtOffset(entry, getEntryOffset(entry));
}

// -----------------------------------------------------------------------------
// Override of Archive::addEntry to force entry addition to the root directory
// -----------------------------------------------------------------------------
ArchiveEntry* RffArchive::addEntry(ArchiveEntry* entry, unsigned position, ArchiveTreeNode* dir, bool copy)
{
	// Check entry
	if (!entry)
		return nullptr;

	// Check if read-only
	if (isReadOnly())
		return nullptr;

	// Copy if necessary
	if (copy)
		entry = new ArchiveEntry(*entry);

	// Do default entry addition (to root directory)
	Archive::addEntry(entry, position);

	return entry;
}

// -----------------------------------------------------------------------------
// Since RFF files have no namespaces, just call the other function.
// -----------------------------------------------------------------------------
ArchiveEntry* RffArchive::addEntry(ArchiveEntry* entry, string_view add_namespace, bool copy)
{
	return addEntry(entry, 0xFFFFFFFF, nullptr, copy);
}

// -----------------------------------------------------------------------------
// Checks if the given data is a valid Duke Nukem 3D grp archive
// -----------------------------------------------------------------------------
bool RffArchive::isRffArchive(MemChunk& mc)
{
	// Check size
	if (mc.size() < 12)
		return false;

	// Read grp header
	uint8_t  magic[4];
	uint32_t version, dir_offset, num_lumps;

	mc.seek(0, SEEK_SET);
	mc.read(magic, 4);       // Should be "RFF\x18"
	mc.read(&version, 4);    // 0x01 0x03 \x00 \x00
	mc.read(&dir_offset, 4); // Offset to directory
	mc.read(&num_lumps, 4);  // No. of lumps in rff

	// Byteswap values for big endian if needed
	dir_offset = wxINT32_SWAP_ON_BE(dir_offset);
	num_lumps  = wxINT32_SWAP_ON_BE(num_lumps);
	version    = wxINT32_SWAP_ON_BE(version);

	// Check the header
	if (magic[0] != 'R' || magic[1] != 'F' || magic[2] != 'F' || magic[3] != 0x1A || version != 0x301)
		return false;


	// Compute total size
	Lump* lumps = new Lump[num_lumps];
	mc.seek(dir_offset, SEEK_SET);
	UI::setSplashProgressMessage("Reading rff archive data");
	mc.read(lumps, num_lumps * sizeof(Lump));
	BloodCrypt(lumps, dir_offset, num_lumps * sizeof(Lump));
	uint32_t totalsize = 12 + num_lumps * sizeof(Lump);
	for (uint32_t a = 0; a < num_lumps; ++a)
		totalsize += lumps[a].Size;

	// Check if total size is correct
	if (totalsize > mc.size())
		return false;

	// If it's passed to here it's probably a grp file
	return true;
}

// -----------------------------------------------------------------------------
// Checks if the file at [filename] is a valid DN3D grp archive
// -----------------------------------------------------------------------------
bool RffArchive::isRffArchive(string_view filename)
{
	// Open file for reading
	SFile file(filename);

	// Check it opened ok
	if (!file.isOpen())
		return false;

	// Check size
	if (file.size() < 12)
		return false;

	// Read grp header
	char magic[4];
	file.read(magic, 4);                    // Should be "RFF\x18"
	auto version    = file.get<uint32_t>(); // 0x01 0x03 \x00 \x00
	auto dir_offset = file.get<uint32_t>(); // Offset to directory
	auto num_lumps  = file.get<uint32_t>(); // No. of lumps in rff

	// Byteswap values for big endian if needed
	dir_offset = wxINT32_SWAP_ON_BE(dir_offset);
	num_lumps  = wxINT32_SWAP_ON_BE(num_lumps);
	version    = wxINT32_SWAP_ON_BE(version);

	// Check the header
	if (magic[0] != 'R' || magic[1] != 'F' || magic[2] != 'F' || magic[3] != 0x1A || version != 0x301)
		return false;


	// Compute total size
	Lump* lumps = new Lump[num_lumps];
	file.seekFromStart(dir_offset);
	file.read(lumps, num_lumps * sizeof(Lump));
	BloodCrypt(lumps, dir_offset, num_lumps * sizeof(Lump));
	uint32_t totalsize = 12 + num_lumps * sizeof(Lump);
	for (uint32_t a = 0; a < num_lumps; ++a)
		totalsize += lumps[a].Size;
	delete[] lumps;

	// Check if total size is correct
	if (totalsize > file.size())
		return false;

	// If it's passed to here it's probably a grp file
	return true;
}
