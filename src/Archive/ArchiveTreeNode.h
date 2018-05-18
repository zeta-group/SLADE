#pragma once

#include "Utility/Tree.h"
#include "ArchiveEntry.h"

class ArchiveTreeNode : public STreeNode
{
	friend class Archive;

public:
	ArchiveTreeNode(ArchiveTreeNode* parent = nullptr, Archive* archive = nullptr);
	~ArchiveTreeNode() = default;

	// Accessors
	Archive*                          archive() const;
	const vector<ArchiveEntry::SPtr>& entries() const { return entries_; }
	ArchiveEntry*                     dirEntry() const { return dir_entry_.get(); }

	// STreeNode
	string_view name() const override;
	void        addChild(STreeNode* child) override;
	void        setName(string_view name) override { S_SET_VIEW(dir_entry_->name_, name); }

	// Entry Access
	ArchiveEntry*      entryAt(unsigned index);
	ArchiveEntry::SPtr sharedEntryAt(unsigned index);
	ArchiveEntry*      entry(string_view name, bool cut_ext = false);
	ArchiveEntry::SPtr sharedEntry(string_view name, bool cut_ext = false);
	ArchiveEntry::SPtr sharedEntry(ArchiveEntry* entry);
	unsigned           numEntries(bool inc_subdirs = false);
	int                entryIndex(ArchiveEntry* entry, size_t startfrom = 0);

	vector<ArchiveEntry::SPtr> allEntries();

	// Entry Operations
	void linkEntries(ArchiveEntry* first, ArchiveEntry* second) const;
	bool addEntry(ArchiveEntry* entry, unsigned index = 0xFFFFFFFF);
	bool addEntry(ArchiveEntry::SPtr& entry, unsigned index = 0xFFFFFFFF);
	bool removeEntry(unsigned index);
	bool swapEntries(unsigned index1, unsigned index2);

	// Other
	void             clear();
	ArchiveTreeNode* clone();
	bool             merge(ArchiveTreeNode* node, unsigned position = 0xFFFFFFFF, int state = 2);
	bool             exportTo(string_view path);
	void             allowDuplicateNames(bool allow) { allow_duplicate_names_ = allow; }

	typedef std::unique_ptr<ArchiveTreeNode> UPtr;

protected:
	STreeNode* createChild(string_view name) override
	{
		auto node = new ArchiveTreeNode();
		node->setName(name);
		node->archive_               = archive_;
		node->allow_duplicate_names_ = allow_duplicate_names_;
		return node;
	}

private:
	Archive*                   archive_;
	ArchiveEntry::SPtr         dir_entry_;
	vector<ArchiveEntry::SPtr> entries_;
	bool                       allow_duplicate_names_ = true;

	void ensureUniqueName(ArchiveEntry* entry);
};
