#pragma once

class wxWebView;
class SToolBar;
class SToolBarButton;

class DocsPage : public wxPanel
{
public:
	DocsPage(wxWindow* parent);
	~DocsPage() = default;

	void updateNavButtons() const;
	void openPage(string_view page_name) const;

private:
	wxWebView*      wv_browser_;
	SToolBar*       toolbar_;
	SToolBarButton* tb_home_;
	SToolBarButton* tb_back_;
	SToolBarButton* tb_forward_;

	// Events
	void onToolbarButton(wxCommandEvent& e);
};
