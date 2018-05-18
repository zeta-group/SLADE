#pragma once

#include "Archive/Archive.h"
#include "Utility/Structs.h"

class CTexture;
class MapVertex;
class MapSide;
class MapLine;
class MapSector;
class SLADEMap;
class MapThing;

class ClipboardItem
{
public:
	enum class Type
	{
		EntryTree,
		CompositeTexture,
		Patch,
		MapArch,
		MapThings,

		Unknown,
	};

	ClipboardItem(Type type = Type::Unknown) : type_{ type } {}
	~ClipboardItem() = default;

	Type type() const { return type_; }

	typedef std::unique_ptr<ClipboardItem> UPtr;

private:
	Type type_;
};

class EntryTreeClipboardItem : public ClipboardItem
{
public:
	EntryTreeClipboardItem(vector<ArchiveEntry*>& entries, vector<ArchiveTreeNode*>& dirs);
	~EntryTreeClipboardItem() = default;

	ArchiveTreeNode* tree() const { return tree_.get(); }

private:
	ArchiveTreeNode::UPtr tree_;
};


class TextureClipboardItem : public ClipboardItem
{
public:
	TextureClipboardItem(CTexture* texture, Archive* parent);
	~TextureClipboardItem() = default;

	CTexture*     texture() const { return texture_.get(); }
	ArchiveEntry* patchEntry(string_view patch);

private:
	std::unique_ptr<CTexture>  texture_;
	vector<ArchiveEntry::UPtr> patch_entries_;
};


class MapArchClipboardItem : public ClipboardItem
{
public:
	MapArchClipboardItem();
	~MapArchClipboardItem();

	void               addLines(vector<MapLine*> lines);
	string             info() const;
	vector<MapVertex*> pasteToMap(SLADEMap* map, fpoint2_t position);
	void               getLines(vector<MapLine*>& list);
	fpoint2_t          midpoint() const { return midpoint_; }

private:
	vector<std::unique_ptr<MapVertex>> vertices_;
	vector<std::unique_ptr<MapSide>>   sides_;
	vector<std::unique_ptr<MapLine>>   lines_;
	vector<std::unique_ptr<MapSector>> sectors_;
	fpoint2_t                          midpoint_;
};

class MapThingsClipboardItem : public ClipboardItem
{
public:
	MapThingsClipboardItem();
	~MapThingsClipboardItem();

	void      addThings(vector<MapThing*>& things);
	string    info() const;
	void      pasteToMap(SLADEMap* map, fpoint2_t position);
	void      getThings(vector<MapThing*>& list);
	fpoint2_t midpoint() const { return midpoint_; }

private:
	vector<std::unique_ptr<MapThing>> things_;
	fpoint2_t                         midpoint_;
};

class Clipboard
{
public:
	Clipboard()  = default;
	~Clipboard() = default;

	// Singleton implementation
	static Clipboard* getInstance()
	{
		if (!instance_)
			instance_ = new Clipboard();

		return instance_;
	}

	uint32_t       nItems() const { return items_.size(); }
	ClipboardItem* getItem(uint32_t index) { return index >= items_.size() ? nullptr : items_[index].get(); }
	bool           putItem(ClipboardItem* item);
	bool           putItems(const vector<ClipboardItem*>& items);

	void clear() { items_.clear(); }

private:
	vector<ClipboardItem::UPtr> items_;
	static Clipboard*           instance_;
};

// Define for less cumbersome ClipBoard::getInstance()
#define theClipboard Clipboard::getInstance()
