#pragma once

#include "common.h"

class SDialog : public wxDialog
{
public:
	SDialog(
		wxWindow*       parent,
		const wxString& title,
		string_view     id,
		int             width  = -1,
		int             height = -1,
		int             x      = -1,
		int             y      = -1);
	virtual ~SDialog();

	void setSavedSize(int def_width = -1, int def_height = -1);

private:
	string id_;

	void onSize(wxSizeEvent& e);
	void onMove(wxMoveEvent& e);
	void onShow(wxShowEvent& e);
};
