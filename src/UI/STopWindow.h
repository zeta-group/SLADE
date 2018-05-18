#pragma once

#include "common.h"

class SAction;
class SToolBar;
class wxMenu;

class STopWindow : public wxFrame
{
public:
	STopWindow(const wxString& title, string_view id, int xpos = 0, int ypos = 0, int width = 1024, int height = 768);
	~STopWindow();

	// Custom menu
	void addCustomMenu(wxMenu* menu, const wxString& title);
	void removeCustomMenu(wxMenu* menu);
	void removeAllCustomMenus();

	// Toolbars
	void enableToolBar(string_view name, bool enable = true) const;
	void addCustomToolBar(string_view name, const vector<string>& actions) const;
	void removeCustomToolBar(string_view name) const;
	void removeAllCustomToolBars() const;
	void populateToolbarsMenu() const;

	// Events
	void onMenu(wxCommandEvent& e);

protected:
	vector<wxMenu*> custom_menus_;
	int             custom_menus_begin_ = 0;
	SToolBar*       toolbar_;
	string          id_;
	wxMenu*         toolbar_menu_;
	int             toolbar_menu_wx_id_;
	SAction*        action_toolbar_menu_;
};
