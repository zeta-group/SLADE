#pragma once

#include "Archive/ArchiveEntry.h"
#include "General/ListenerAnnouncer.h"

class CTexture;

class PatchTable : public Announcer
{
public:
	struct Patch
	{
		string         name;
		vector<string> used_in;

		void removeTextureUsage(string_view texture)
		{
			for (unsigned a = 0; a < used_in.size(); a++)
			{
				if (texture == used_in[a])
				{
					used_in.erase(used_in.begin() + a);
					a--;
				}
			}
		}
	};

	PatchTable(Archive* parent = nullptr) : parent_{ parent } {}
	~PatchTable() = default;

	size_t   nPatches() const { return patches_.size(); }
	Archive* getParent() const { return parent_; }
	void     setParent(Archive* parent) { this->parent_ = parent; }

	Patch&        patch(size_t index);
	Patch&        patch(string_view name);
	const string& patchName(size_t index);
	ArchiveEntry* patchEntry(size_t index);
	ArchiveEntry* patchEntry(string_view name);
	int32_t       patchIndex(string_view name);
	int32_t       patchIndex(ArchiveEntry* entry);
	bool          removePatch(unsigned index);
	bool          replacePatch(unsigned index, string_view newname);
	bool          addPatch(string_view name, bool allow_dup = false);

	bool loadPNAMES(ArchiveEntry* pnames, Archive* parent = nullptr);
	bool writePNAMES(ArchiveEntry* pnames);

	void clearPatchUsage();
	void updatePatchUsage(CTexture* tex);

private:
	Archive*      parent_;
	vector<Patch> patches_;
};
