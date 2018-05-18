#pragma once

#include "PrefsPanelBase.h"

class wxCheckListBox;
class NodesPrefsPanel : public PrefsPanelBase
{
public:
	NodesPrefsPanel(wxWindow* parent, bool frame = true);
	~NodesPrefsPanel() = default;

	void init() override;
	void populateOptions(string_view options) const;
	void applyPreferences() override;

	string pageTitle() override { return "Node Builders"; }

private:
	wxChoice*       choice_nodebuilder_;
	wxButton*       btn_browse_path_;
	wxTextCtrl*     text_path_;
	wxCheckListBox* clb_options_;

	// Events
	void onBtnBrowse(wxCommandEvent& e);
};
