#pragma once

#include "General/ListenerAnnouncer.h"
#include "UI/Browser/BrowserWindow.h"

class Archive;
class TextureXList;
class PatchTable;

class PatchBrowserItem : public BrowserItem
{
public:
	PatchBrowserItem(
		string_view name,
		Archive*    archive = nullptr,
		uint8_t     type    = 0,
		string_view nspace  = "",
		unsigned    index   = 0) :
		BrowserItem{ name, index, "patch" },
		archive_{ archive },
		type_{ type },
		nspace_{ nspace }
	{
	}

	~PatchBrowserItem();

	bool   loadImage() override;
	string itemInfo() override;

private:
	Archive* archive_;
	uint8_t  type_; // 0=patch, 1=ctexture
	string   nspace_;
};

class PatchBrowser : public BrowserWindow, Listener
{
public:
	PatchBrowser(wxWindow* parent);
	~PatchBrowser() = default;

	bool openPatchTable(PatchTable* table);
	bool openArchive(Archive* archive);
	bool openTextureXList(TextureXList* texturex, Archive* parent);
	int  getSelectedPatch();
	void selectPatch(int pt_index);
	void selectPatch(string_view name);

private:
	PatchTable* patch_table_;

	// Events
	void onAnnouncement(Announcer* announcer, string_view event_name, MemChunk& event_data) override;
};
