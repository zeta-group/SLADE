#pragma once

#include "common.h"

class WizardPageBase : public wxPanel
{
public:
	WizardPageBase(wxWindow* parent) : wxPanel(parent, -1) {}
	~WizardPageBase() = default;

	virtual bool   canGoNext() { return true; }
	virtual void   applyChanges() {}
	virtual string getTitle() { return "Page Title"; }
	virtual string getDescription() { return ""; }
};
