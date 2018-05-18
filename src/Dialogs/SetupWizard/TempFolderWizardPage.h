#pragma once

#include "WizardPageBase.h"
#include "UI/WxBasicControls.h"

class TempFolderWizardPage : public WizardPageBase
{
public:
	TempFolderWizardPage(wxWindow* parent);
	~TempFolderWizardPage() = default;

	bool   canGoNext() override;
	void   applyChanges() override;
	string getTitle() override { return "SLADE Temp Folder"; }
	string getDescription() override;

private:
	wxRadioButton* rb_use_system_;
	wxRadioButton* rb_use_slade_dir_;
	wxRadioButton* rb_use_custom_dir_;
	wxTextCtrl*    text_custom_dir_;
	wxButton*      btn_browse_dir_;

	// Events
	void onRadioButtonChanged(wxCommandEvent& e);
	void onBtnBrowse(wxCommandEvent& e);
};
