
// -----------------------------------------------------------------------------
// SLADE - It's a Doom Editor
// Copyright(C) 2008 - 2017 Simon Judd
//
// Email:       sirjuddington@gmail.com
// Web:         http://slade.mancubus.net
// Filename:    ExternalEditManager.cpp
// Description: ExternalEditManager class, keeps track of all entries currently
//              being edited externally for a single ArchivePanel.
//              Also contains some FileMonitor subclasses for handling export /
//              import of various entry types (conversions, etc.)
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


// -----------------------------------------------------------------------------
//
// Includes
//
// -----------------------------------------------------------------------------
#include "Main.h"
#include "App.h"
#include "Archive/Archive.h"
#include "Conversions.h"
#include "EntryOperations.h"
#include "ExternalEditManager.h"
#include "General/Executables.h"
#include "General/Misc.h"
#include "Graphics/SImage/SIFormat.h"
#include "Graphics/SImage/SImage.h"
#include "MainEditor.h"
#include "Utility/FileMonitor.h"
#include "Utility/StringUtils.h"


// -----------------------------------------------------------------------------
// ExternalEditFileMonitor Class
//
// FileMonitor subclass to handle exporting, monitoring and re-importing an
// entry
// -----------------------------------------------------------------------------
class ExternalEditFileMonitor : public FileMonitor, Listener
{
public:
	ExternalEditFileMonitor(ArchiveEntry* entry, ExternalEditManager* manager) :
		FileMonitor("", false),
		entry_(entry),
		manager_(manager)
	{
		// Listen to entry parent archive
		listenTo(entry->parent());
	}

	virtual ~ExternalEditFileMonitor() { manager_->monitorStopped(this); }

	ArchiveEntry* getEntry() const { return entry_; }
	void          fileModified() override { updateEntry(); }

	virtual void updateEntry() { entry_->importFile(filename_); }

	virtual bool exportEntry()
	{
		// Determine export filename/path
		StrUtil::Path fn(App::path(entry_->name(), App::Dir::Temp));
		fn.setExtension(entry_->type()->extension());

		// Export entry and start monitoring
		bool ok = entry_->exportFile(fn.fullPath());
		if (ok)
		{
			filename_      = fn.fullPath();
			file_modified_ = wxFileModificationTime(filename_);
			Start(1000);
		}
		else
			Global::error = "Failed to export entry";

		return ok;
	}

	void onAnnouncement(Announcer* announcer, string_view event_name, MemChunk& event_data) override
	{
		if (announcer != entry_->parent())
			return;

		bool finished = false;

		// Entry removed
		if (event_name == "entry_removed")
		{
			int       index;
			wxUIntPtr ptr;
			event_data.read(&index, sizeof(int));
			event_data.read(&ptr, sizeof(wxUIntPtr));
			if (wxUIntToPtr(ptr) == entry_)
				finished = true;
		}

		if (finished)
			delete this;
	}

protected:
	ArchiveEntry*        entry_;
	ExternalEditManager* manager_;
	string               gfx_format_;
};


// -----------------------------------------------------------------------------
// GfxExternalFileMonitor Class
//
// ExternalEditFileMonitor subclass to handle gfx entries
// -----------------------------------------------------------------------------
class GfxExternalFileMonitor : public ExternalEditFileMonitor
{
public:
	GfxExternalFileMonitor(ArchiveEntry* entry, ExternalEditManager* manager) : ExternalEditFileMonitor(entry, manager)
	{
	}
	virtual ~GfxExternalFileMonitor() = default;

	void updateEntry() override
	{
		// Read file
		MemChunk data;
		data.importFile(filename_);

		// Read image
		SImage image;
		image.open(data, 0, "png");
		image.convertPaletted(&palette_);

		// Convert image to entry gfx format
		SIFormat* format = SIFormat::getFormat(gfx_format_);
		if (format)
		{
			MemChunk conv_data;
			if (format->saveImage(image, conv_data, &palette_))
			{
				// Update entry data
				entry_->importMemChunk(conv_data);
				EntryOperations::setGfxOffsets(entry_, offsets_.x, offsets_.y);
			}
			else
			{
				Log::error(fmt::sprintf("Unable to convert external png to %s", format->name()));
			}
		}
	}

	bool exportEntry() override
	{
		StrUtil::Path fn(App::path(entry_->name(), App::Dir::Temp));

		fn.setExtension("png");

		// Create image from entry
		SImage image;
		if (!Misc::loadImageFromEntry(&image, entry_))
		{
			Global::error = "Could not read graphic";
			return false;
		}

		// Set export info
		gfx_format_ = image.format()->id();
		offsets_    = image.offset();
		palette_.copyPalette(MainEditor::currentPalette(entry_));

		// Write png data
		MemChunk  png;
		SIFormat* fmt_png = SIFormat::getFormat("png");
		if (!fmt_png->saveImage(image, png, &palette_))
		{
			Global::error = "Error converting to png";
			return false;
		}

		// Export file and start monitoring if successful
		filename_ = fn.fullPath();
		if (png.exportFile(filename_))
		{
			file_modified_ = wxFileModificationTime(filename_);
			Start(1000);
			return true;
		}

		return false;
	}

private:
	string   gfx_format_;
	point2_t offsets_;
	Palette  palette_;
};


// -----------------------------------------------------------------------------
// MIDIExternalFileMonitor Class
//
// ExternalEditFileMonitor subclass to handle MIDI entries
// -----------------------------------------------------------------------------
class MIDIExternalFileMonitor : public ExternalEditFileMonitor
{
public:
	MIDIExternalFileMonitor(ArchiveEntry* entry, ExternalEditManager* manager) : ExternalEditFileMonitor(entry, manager)
	{
	}
	virtual ~MIDIExternalFileMonitor() = default;

	void updateEntry() override
	{
		// Can't convert back, just import the MIDI
		entry_->importFile(filename_);
	}

	bool exportEntry() override
	{
		StrUtil::Path fn(App::path(entry_->name(), App::Dir::Temp));
		fn.setExtension("mid");

		// Convert to MIDI data
		MemChunk convdata;

		// MUS
		if (entry_->type()->formatId() == "midi_mus")
			Conversions::musToMidi(entry_->data(), convdata);

		// HMI/HMP/XMI
		else if (
			entry_->type()->formatId() == "midi_xmi" || entry_->type()->formatId() == "midi_hmi"
			|| entry_->type()->formatId() == "midi_hmp")
			Conversions::zmusToMidi(entry_->data(), convdata, 0);

		// GMID
		else if (entry_->type()->formatId() == "midi_gmid")
			Conversions::gmidToMidi(entry_->data(), convdata);

		else
		{
			Global::error = fmt::sprintf("Type %s can not be converted to MIDI", entry_->type()->name());
			return false;
		}

		// Export file and start monitoring if successful
		filename_ = fn.fullPath();
		if (convdata.exportFile(filename_))
		{
			file_modified_ = wxFileModificationTime(filename_);
			Start(1000);
			return true;
		}

		return false;
	}

	static bool canHandleEntry(ArchiveEntry* entry)
	{
		return entry->type()->formatId() == "midi" || entry->type()->formatId() == "midi_mus"
			   || entry->type()->formatId() == "midi_xmi" || entry->type()->formatId() == "midi_hmi"
			   || entry->type()->formatId() == "midi_hmp" || entry->type()->formatId() == "midi_gmid";
	}
};


// -----------------------------------------------------------------------------
// SfxExternalFileMonitor Class
//
// ExternalEditFileMonitor subclass to handle sfx entries
// -----------------------------------------------------------------------------
class SfxExternalFileMonitor : public ExternalEditFileMonitor
{
public:
	SfxExternalFileMonitor(ArchiveEntry* entry, ExternalEditManager* manager) :
		ExternalEditFileMonitor(entry, manager),
		doom_sound(true)
	{
	}
	virtual ~SfxExternalFileMonitor() = default;

	void updateEntry() override
	{
		// Convert back to doom sound if it was originally
		if (doom_sound)
		{
			MemChunk in, out;
			in.importFile(filename_);
			if (Conversions::wavToDoomSnd(in, out))
			{
				// Import converted data to entry if successful
				entry_->importMemChunk(out);
				return;
			}
		}

		// Just import wav to entry if conversion to doom sound
		// failed or the entry was not a convertable type
		entry_->importFile(filename_);
	}

	bool exportEntry() override
	{
		StrUtil::Path fn(App::path(entry_->name(), App::Dir::Temp));
		fn.setExtension("mid");

		// Convert to WAV data
		MemChunk convdata;

		// Doom Sound
		if (entry_->type()->formatId() == "snd_doom" || entry_->type()->formatId() == "snd_doom_mac")
			Conversions::doomSndToWav(entry_->data(), convdata);

		// Doom PC Speaker Sound
		else if (entry_->type()->formatId() == "snd_speaker")
			Conversions::spkSndToWav(entry_->data(), convdata);

		// AudioT PC Speaker Sound
		else if (entry_->type()->formatId() == "snd_audiot")
			Conversions::spkSndToWav(entry_->data(), convdata, true);

		// Wolfenstein 3D Sound
		else if (entry_->type()->formatId() == "snd_wolf")
			Conversions::wolfSndToWav(entry_->data(), convdata);

		// Creative Voice File
		else if (entry_->type()->formatId() == "snd_voc")
			Conversions::vocToWav(entry_->data(), convdata);

		// Jaguar Doom Sound
		else if (entry_->type()->formatId() == "snd_jaguar")
			Conversions::jagSndToWav(entry_->data(), convdata);

		// Blood Sound
		else if (entry_->type()->formatId() == "snd_bloodsfx")
			Conversions::bloodToWav(entry_, convdata);

		else
		{
			Global::error = fmt::sprintf("Type %s can not be converted to WAV", entry_->type()->name());
			return false;
		}

		// Export file and start monitoring if successful
		filename_ = fn.fullPath();
		if (convdata.exportFile(filename_))
		{
			file_modified_ = wxFileModificationTime(filename_);
			Start(1000);
			return true;
		}

		return false;
	}

	static bool canHandleEntry(ArchiveEntry* entry)
	{
		return entry->type()->formatId() == "snd_doom" || entry->type()->formatId() == "snd_doom_mac"
			   || entry->type()->formatId() == "snd_speaker" || entry->type()->formatId() == "snd_audiot"
			   || entry->type()->formatId() == "snd_wolf" || entry->type()->formatId() == "snd_voc"
			   || entry->type()->formatId() == "snd_jaguar" || entry->type()->formatId() == "snd_bloodsfx";
	}

private:
	bool doom_sound;
};


// -----------------------------------------------------------------------------
//
// ExternalEditManager Class Functions
//
// -----------------------------------------------------------------------------


ExternalEditManager::ExternalEditManager()
{
	// Needs to be defined to allow unique_ptrs for file_monitors_
}

ExternalEditManager::~ExternalEditManager()
{
	// Needs to be defined to allow unique_ptrs for file_monitors_
}

// -----------------------------------------------------------------------------
// Opens [entry] for external editing with [editor] for [category]
// -----------------------------------------------------------------------------
bool ExternalEditManager::openEntryExternal(ArchiveEntry* entry, string_view editor, string_view category)
{
	// Check the entry isn't already opened externally
	for (auto& file_monitor : file_monitors_)
		if (file_monitor->getEntry() == entry)
		{
			Log::warning(fmt::sprintf("Entry %s is already open in an external editor", entry->name()));
			return true;
		}

	// Setup file monitor depending on entry type
	std::unique_ptr<ExternalEditFileMonitor> monitor;

	// Gfx entry
	if (entry->type()->editor() == "gfx" && entry->type()->id() != "png")
		monitor = std::make_unique<GfxExternalFileMonitor>(entry, this);
	// MIDI entry
	else if (MIDIExternalFileMonitor::canHandleEntry(entry))
		monitor = std::make_unique<MIDIExternalFileMonitor>(entry, this);
	// Sfx entry
	else if (SfxExternalFileMonitor::canHandleEntry(entry))
		monitor = std::make_unique<SfxExternalFileMonitor>(entry, this);
	// Other entry
	else
		monitor = std::make_unique<ExternalEditFileMonitor>(entry, this);

	// Export entry to temp file and start monitoring if successful
	if (!monitor->exportEntry())
		return false;

	// Get external editor path
	string exe_path = Executables::externalExe(editor, category).path;
#ifdef WIN32
	if (exe_path.empty() || !wxFileExists(exe_path))
#else
	if (exe_path.empty())
#endif
	{
		Global::error = fmt::sprintf("External editor %s has invalid path", editor);
		return false;
	}

	// Run external editor
	string command = fmt::sprintf("\"%s\" \"%s\"", exe_path, monitor->getFilename());
	long   success = wxExecute(command, wxEXEC_ASYNC, monitor->getProcess());
	if (success == 0)
	{
		Global::error = fmt::sprintf("Failed to launch %s", editor);
		return false;
	}

	// Add to list of file monitors for tracking
	file_monitors_.push_back(std::move(monitor));

	return true;
}

// -----------------------------------------------------------------------------
// Called when a FileMonitor is stopped/deleted
// -----------------------------------------------------------------------------
void ExternalEditManager::monitorStopped(ExternalEditFileMonitor* monitor)
{
	auto i = std::find_if(
		file_monitors_.begin(), file_monitors_.end(), [&monitor](const std::unique_ptr<ExternalEditFileMonitor>& ptr) {
			return ptr.get() == monitor;
		});

	if (i != file_monitors_.end())
		file_monitors_.erase(i);
}
