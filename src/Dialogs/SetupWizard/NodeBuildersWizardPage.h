#pragma once

#include "WizardPageBase.h"

class NodesPrefsPanel;
class NodeBuildersWizardPage : public WizardPageBase
{
public:
	NodeBuildersWizardPage(wxWindow* parent);
	~NodeBuildersWizardPage() = default;

	bool   canGoNext() override { return true; }
	string getTitle() override { return "Node Builders"; }
	string getDescription() override;

private:
	NodesPrefsPanel* panel_nodes_;
};
