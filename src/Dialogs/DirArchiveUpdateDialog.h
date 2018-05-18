#pragma once

#include "Archive/Formats/DirArchive.h"
#include "UI/SDialog.h"

class wxDataViewListCtrl;
class DirArchiveUpdateDialog : public SDialog
{
public:
	DirArchiveUpdateDialog(wxWindow* parent, DirArchive* archive, vector<DirEntryChange>& changes);
	~DirArchiveUpdateDialog() = default;

	void populateChangeList();

private:
	DirArchive*            archive_;
	vector<DirEntryChange> changes_;
	wxDataViewListCtrl*    list_changes_;

	// Events
	void onBtnOKClicked(wxCommandEvent& e);
};
