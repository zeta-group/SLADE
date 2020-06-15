#pragma once

#include "General/Sigslot.h"
#include <wx/dataview.h>

namespace slade
{
class Archive;
class ArchiveEntry;
class ArchiveDir;

namespace ui
{
	class ArchiveViewModel : public wxDataViewModel
	{
	public:
		ArchiveViewModel() = default;

		void openArchive(shared_ptr<Archive> archive);

	private:
		weak_ptr<Archive>    archive_;
		ScopedConnectionList connections_;

		// wxDataViewModel
		unsigned int   GetColumnCount() const override;
		wxString       GetColumnType(unsigned int col) const override;
		void           GetValue(wxVariant& variant, const wxDataViewItem& item, unsigned int col) const override;
		bool           GetAttr(const wxDataViewItem& item, unsigned int col, wxDataViewItemAttr& attr) const override;
		bool           SetValue(const wxVariant& variant, const wxDataViewItem& item, unsigned int col) override;
		wxDataViewItem GetParent(const wxDataViewItem& item) const override;
		bool           IsContainer(const wxDataViewItem& item) const override;
		unsigned int   GetChildren(const wxDataViewItem& item, wxDataViewItemArray& children) const override;
		bool           IsListModel() const override;
		bool           HasDefaultCompare() const override { return true; }
		int Compare(const wxDataViewItem& item1, const wxDataViewItem& item2, unsigned int column, bool ascending)
			const override;

		wxDataViewItem createItemForDirectory(const ArchiveDir* dir);
	};

	class ArchiveEntryTree : public wxDataViewCtrl
	{
	public:
		ArchiveEntryTree(wxWindow* parent, shared_ptr<Archive> archive);

		ArchiveEntry* entryForItem(const wxDataViewItem& item) const
		{
			return static_cast<ArchiveEntry*>(item.GetID());
		}
		ArchiveDir* dirForDirItem(const wxDataViewItem& item) const;

		vector<ArchiveEntry*> selectedEntries(bool include_dirs = false) const;
		ArchiveEntry*         firstSelectedEntry(bool include_dirs = false) const;
		ArchiveEntry*         lastSelectedEntry(bool include_dirs = false) const;
		vector<ArchiveDir*>   selectedDirectories() const;
		ArchiveDir*           firstSelectedDirectory() const;
		ArchiveDir*           lastSelectedDirectory() const;
		wxDataViewItem        firstSelectedItem() const;
		wxDataViewItem        lastSelectedItem() const;
		ArchiveDir*           currentSelectedDir() const;
		ArchiveDir*           selectedEntriesDir() const;

	private:
		weak_ptr<Archive> archive_;
		ArchiveViewModel* model_    = nullptr;
		wxDataViewColumn* col_name_ = nullptr;
		wxDataViewColumn* col_size_ = nullptr;
		wxDataViewColumn* col_type_ = nullptr;
	};

} // namespace ui
} // namespace slade
