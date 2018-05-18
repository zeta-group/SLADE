#pragma once

#include "common.h"
#include "UI/WxBasicControls.h"

class ExtMessageDialog : public wxDialog
{
public:
	ExtMessageDialog(wxWindow* parent, string caption);
	~ExtMessageDialog() = default;

	void setMessage(string_view message);
	void setExt(string_view text);

private:
	wxStaticText* label_message_;
	wxTextCtrl*   text_ext_;

	// Events
	void onSize(wxSizeEvent& e);
};
