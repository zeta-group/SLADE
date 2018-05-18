
// -----------------------------------------------------------------------------
// SLADE - It's a Doom Editor
// Copyright(C) 2008 - 2017 Simon Judd
//
// Email:       sirjuddington@gmail.com
// Web:         http://slade.mancubus.net
// Filename:    InputPrefsPanel.cpp
// Description: Panel containing controls to view/modify input key binds
//
// This program is free software; you can redistribute it and/or modify it
// under the terms of the GNU General Public License as published by the Free
// Software Foundation; either version 2 of the License, or (at your option)
// any later version.
//
// This program is distributed in the hope that it will be useful, but WITHOUT
// ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
// FITNESS FOR A PARTICULAR PURPOSE. See the GNU General Public License for
// more details.
//
// You should have received a copy of the GNU General Public License along with
// this program; if not, write to the Free Software Foundation, Inc.,
// 51 Franklin Street, Fifth Floor, Boston, MA  02110 - 1301, USA.
// -----------------------------------------------------------------------------


// -----------------------------------------------------------------------------
//
// Includes
//
// -----------------------------------------------------------------------------
#include "Main.h"
#include "InputPrefsPanel.h"
#include "MapEditor/MapEditor.h"
#include "MapEditor/UI/MapEditorWindow.h"
#include "General/UI.h"
#include "UI/WxUtils.h"


// -----------------------------------------------------------------------------
//
// BindListItemData Class
//
// -----------------------------------------------------------------------------
class BindListItemData : public wxClientData
{
public:
	KeyPress key;
	KeyBind*   bind;

	BindListItemData(KeyPress key, KeyBind* bind = nullptr) : key{ std::move(key) }, bind{ bind } {}
};


// -----------------------------------------------------------------------------
//
// InputKeyCtrl Class Functions
//
// -----------------------------------------------------------------------------


// -----------------------------------------------------------------------------
// InputKeyCtrl class constructor
// -----------------------------------------------------------------------------
InputKeyCtrl::InputKeyCtrl(wxWindow* parent, KeyPress init) :
	wxTextCtrl(parent, -1, wxEmptyString, wxDefaultPosition, wxDefaultSize, wxTE_PROCESS_TAB | wxTE_PROCESS_ENTER),
	key_{ init }
{
	// Set initial value
	wxTextCtrl::SetValue(init.toString());

	// Bind events
	Bind(wxEVT_KEY_DOWN, &InputKeyCtrl::onKeyDown, this);
	Bind(wxEVT_CHAR, &InputKeyCtrl::onKeyDown, this);
	Bind(wxEVT_MIDDLE_DOWN, &InputKeyCtrl::onMouseDown, this);
	Bind(wxEVT_AUX1_DOWN, &InputKeyCtrl::onMouseDown, this);
	Bind(wxEVT_AUX2_DOWN, &InputKeyCtrl::onMouseDown, this);
	Bind(wxEVT_MOUSEWHEEL, &InputKeyCtrl::onMouseDown, this);
	Bind(wxEVT_TEXT_ENTER, &InputKeyCtrl::onEnter, this);
}

// -----------------------------------------------------------------------------
// Called when a key is pressed in the control
// -----------------------------------------------------------------------------
void InputKeyCtrl::onKeyDown(wxKeyEvent& e)
{
	// Ignore if modifier key
	if (e.GetKeyCode() == WXK_SHIFT || e.GetKeyCode() == WXK_ALT || e.GetKeyCode() == WXK_CONTROL
		|| e.GetKeyCode() == WXK_COMMAND)
	{
		e.Skip();
		return;
	}

	key_ = KeyBind::asKeyPress(e.GetKeyCode(), e.GetModifiers());
	SetValue(key_.toString());
}

// -----------------------------------------------------------------------------
// Called when a mouse button is clicked in the control
// -----------------------------------------------------------------------------
void InputKeyCtrl::onMouseDown(wxMouseEvent& e)
{
	// Middle button
	if (e.GetEventType() == wxEVT_MIDDLE_DOWN)
		key_.key = "mouse3";

	// Button 4
	else if (e.GetEventType() == wxEVT_AUX1_DOWN)
		key_.key = "mouse4";

	// Button 5
	else if (e.GetEventType() == wxEVT_AUX2_DOWN)
		key_.key = "mouse5";

	// Mouse wheel
	else if (e.GetEventType() == wxEVT_MOUSEWHEEL)
	{
		if (e.GetWheelRotation() > 0)
			key_.key = "mwheelup";
		else if (e.GetWheelRotation() < 0)
			key_.key = "mwheeldown";
	}

	key_.alt   = e.AltDown();
	key_.ctrl  = e.CmdDown();
	key_.shift = e.ShiftDown();
	SetValue(key_.toString());
}

// -----------------------------------------------------------------------------
// Called when the enter key is preessed in the control
// -----------------------------------------------------------------------------
void InputKeyCtrl::onEnter(wxCommandEvent& e)
{
	key_.key = "return";
	SetValue(key_.toString());
}


// -----------------------------------------------------------------------------
//
// InputPrefsPanel Class Functions
//
// -----------------------------------------------------------------------------


// -----------------------------------------------------------------------------
// InputPrefsPanel class constructor
// -----------------------------------------------------------------------------
InputPrefsPanel::InputPrefsPanel(wxWindow* parent) : PrefsPanelBase(parent)
{
	// Create sizer
	wxBoxSizer* sizer = new wxBoxSizer(wxVERTICAL);
	SetSizer(sizer);

	// Keybinds list
	list_binds_ = new wxTreeListCtrl(this, -1);
	sizer->Add(list_binds_, 1, wxEXPAND | wxBOTTOM, UI::pad());

	// Buttons
	WxUtils::layoutHorizontally(
		sizer,
		{ btn_change_   = new wxButton(this, -1, "Set Key"),
		  btn_add_      = new wxButton(this, -1, "Add Key"),
		  btn_remove_   = new wxButton(this, -1, "Remove Key"),
		  btn_defaults_ = new wxButton(this, -1, "Reset to Default") },
		wxSizerFlags(0).Expand());

	// Disable buttons initially
	btn_change_->Enable(false);
	btn_add_->Enable(false);
	btn_remove_->Enable(false);
	btn_defaults_->Enable(false);

	// Bind events
	list_binds_->Bind(wxEVT_TREELIST_SELECTION_CHANGED, &InputPrefsPanel::onListSelectionChanged, this);
	list_binds_->Bind(wxEVT_TREELIST_ITEM_ACTIVATED, &InputPrefsPanel::onListItemActivated, this);
	btn_change_->Bind(wxEVT_BUTTON, &InputPrefsPanel::onBtnChangeKey, this);
	btn_add_->Bind(wxEVT_BUTTON, &InputPrefsPanel::onBtnAddKey, this);
	btn_remove_->Bind(wxEVT_BUTTON, &InputPrefsPanel::onBtnRemoveKey, this);
	btn_defaults_->Bind(wxEVT_BUTTON, &InputPrefsPanel::onBtnDefaults, this);

	Layout();
	initBindsList();
	updateBindsList();
}

// -----------------------------------------------------------------------------
// Initialises panel controls
// -----------------------------------------------------------------------------
void InputPrefsPanel::init()
{
	updateBindsList();

	// Update list column sizes
	int width = list_binds_->GetSize().x / 3;
	list_binds_->SetColumnWidth(1, width);
	list_binds_->SetColumnWidth(0, width * 1.8);
}

// -----------------------------------------------------------------------------
// Returns the wxTreeListItem for keybind group [group]
// -----------------------------------------------------------------------------
wxTreeListItem InputPrefsPanel::getListGroupItem(string group)
{
	// Go through items
	wxTreeListItem item = list_binds_->GetFirstChild(list_binds_->GetRootItem());
	while (item.IsOk())
	{
		if (list_binds_->GetItemText(item) == group)
			return item;

		item = list_binds_->GetNextSibling(item);
	}

	// Not found, create group
	return list_binds_->AppendItem(list_binds_->GetRootItem(), group);
}

// -----------------------------------------------------------------------------
// Populates the keybinds tree list
// -----------------------------------------------------------------------------
void InputPrefsPanel::initBindsList()
{
	// Get list of all keybinds
	vector<KeyBind*> binds;
	KeyBind::allKeyBinds(binds);

	// Create columns
	int width = list_binds_->GetSize().x / 3;
	list_binds_->AppendColumn("Control", width * 1.8);
	list_binds_->AppendColumn("Bound Keys", width);

	// Add binds to list
	for (auto& bind : binds)
	{
		// Skip if not in a group
		if (bind->group().empty())
			continue;

		// Add to list
		wxTreeListItem group = getListGroupItem(bind->group());
		wxTreeListItem item =
			list_binds_->AppendItem(group, bind->description(), -1, -1, new BindListItemData(bind->firstKey(), bind));

		// Add any extra key binds
		for (auto i = bind->keys().begin() + 1; i != bind->keys().end(); ++i)
			list_binds_->AppendItem(item, "", -1, -1, new BindListItemData(*i, bind));
	}

	// Update list column sizes
	list_binds_->SetColumnWidth(1, width);
	list_binds_->SetColumnWidth(0, width * 1.8);
}

// -----------------------------------------------------------------------------
// Updates all keybind list items
// -----------------------------------------------------------------------------
void InputPrefsPanel::updateBindsList()
{
	// Get first list item
	wxTreeListItem item = list_binds_->GetFirstItem();

	// Go through all list items
	while (item.IsOk())
	{
		// Get item data
		BindListItemData* bind = ((BindListItemData*)list_binds_->GetItemData(item));

		// Set item text if key data exists
		if (bind)
			list_binds_->SetItemText(item, 1, bind->key.toString());

		// Next item
		item = list_binds_->GetNextItem(item);
	}
}

// -----------------------------------------------------------------------------
// Changes the key for the keybind at [item]
// -----------------------------------------------------------------------------
void InputPrefsPanel::changeKey(wxTreeListItem item)
{
	// Get item keybind info
	BindListItemData* bind = (BindListItemData*)list_binds_->GetItemData(item);

	// Do nothing if item is a group
	if (!bind)
		return;

	// Create a dialog
	wxDialog    dlg(this, -1, "Set Key");
	wxBoxSizer* sizer = new wxBoxSizer(wxVERTICAL);
	dlg.SetSizer(sizer);

	// Add key input box
	InputKeyCtrl* key_ctrl = new InputKeyCtrl(&dlg, bind->key);
	sizer->Add(key_ctrl, 0, wxEXPAND | wxALL, UI::pad());

	// Add buttons
	wxSizer* btnsizer = dlg.CreateButtonSizer(wxOK | wxCANCEL);
	sizer->Add(btnsizer, 0, wxEXPAND | wxALL, UI::pad());

	// Init dialog
	dlg.SetInitialSize(wxSize(-1, -1));
	dlg.Layout();
	dlg.Fit();
	dlg.SetMinSize(GetBestSize());
	dlg.CenterOnParent();
	key_ctrl->SetFocus();

	// Show dialog
	if (dlg.ShowModal() == wxID_OK)
	{
		// Set keybind if not cancelled
		bind->key = key_ctrl->key();
		list_binds_->SetItemText(item, 1, bind->key.toString());
	}
}

// -----------------------------------------------------------------------------
// Adds a new key to the currently selected keybind
// -----------------------------------------------------------------------------
void InputPrefsPanel::addKey()
{
	// Get selected item
	wxTreeListItem    item = list_binds_->GetSelection();
	BindListItemData* bind = ((BindListItemData*)list_binds_->GetItemData(item));

	// Do nothing if item is a group
	if (!bind)
		return;

	// Get parent-level keybind item
	if (!bind->bind)
		item = list_binds_->GetItemParent(item);

	// Add new keybind item
	wxTreeListItem n = list_binds_->AppendItem(item, "", -1, -1, new BindListItemData(KeyPress()));
	changeKey(n);

	// Delete item if no key was chosen (or dialog cancelled)
	bind = ((BindListItemData*)list_binds_->GetItemData(n));
	if (bind->key.key.empty())
		list_binds_->DeleteItem(n);
	else
	{
		// Otherwise update the new keybind text
		list_binds_->SetItemText(n, 1, bind->key.toString());
	}
}

// -----------------------------------------------------------------------------
// Removes the keybind key at [item]
// -----------------------------------------------------------------------------
void InputPrefsPanel::removeKey(wxTreeListItem item)
{
	// Get item keybind info
	BindListItemData* bind = ((BindListItemData*)list_binds_->GetItemData(item));

	// Do nothing if item is a group
	if (!bind)
		return;

	// Remove the item if it's not a primary bind
	if (!(bind->bind))
	{
		list_binds_->DeleteItem(item);
		return;
	}

	// Clear the primary bind and move all subsequent keys back 1
	bind->key.key        = "";
	wxTreeListItem child = list_binds_->GetFirstChild(item);
	wxTreeListItem last_child;
	while (child.IsOk())
	{
		last_child = child;
		bind->key  = ((BindListItemData*)list_binds_->GetItemData(child))->key;
		child      = list_binds_->GetNextSibling(child);
	}

	// Remove last key if any existed
	if (last_child.IsOk())
		list_binds_->DeleteItem(last_child);

	// Refresh list
	updateBindsList();
}

// -----------------------------------------------------------------------------
// Applies keybind values from the control
// -----------------------------------------------------------------------------
void InputPrefsPanel::applyPreferences()
{
	// Go through all list items
	wxTreeListItem item = list_binds_->GetFirstItem();
	while (item.IsOk())
	{
		// Get bind info
		BindListItemData* bind  = ((BindListItemData*)list_binds_->GetItemData(item));
		KeyBind*          kbind = nullptr;
		if (bind)
			kbind = bind->bind;

		// Check if it's a primary key
		if (kbind)
		{
			// Clear the keybind
			kbind->clear();

			// Set primary key if any
			if (!bind->key.key.empty())
				kbind->addKey(bind->key.key, bind->key.alt, bind->key.ctrl, bind->key.shift);

			// Add any secondary keys
			wxTreeListItem child = list_binds_->GetFirstChild(item);
			while (child.IsOk())
			{
				// Add key
				bind = ((BindListItemData*)list_binds_->GetItemData(child));
				kbind->addKey(bind->key.key, bind->key.alt, bind->key.ctrl, bind->key.shift);

				// Next child
				child = list_binds_->GetNextSibling(child);
			}
		}

		// Next item
		item = list_binds_->GetNextItem(item);
	}

	// Update sorted keybinds list
	KeyBind::updateSortedBindsList();

	// Update map editor menus
	MapEditor::window()->setupMenu();
}


// -----------------------------------------------------------------------------
//
// InputPrefsPanel Class Events
//
// -----------------------------------------------------------------------------


// -----------------------------------------------------------------------------
// Called when the panel is resized
// -----------------------------------------------------------------------------
void InputPrefsPanel::onSize(wxSizeEvent& e)
{
	// Update list column sizes
	int width = list_binds_->GetSize().x / 3;
	list_binds_->SetColumnWidth(1, width);

	e.Skip();
}

// -----------------------------------------------------------------------------
// Called when the selection on the keybinds list is changed
// -----------------------------------------------------------------------------
void InputPrefsPanel::onListSelectionChanged(wxTreeListEvent& e)
{
	// Get selected item
	wxTreeListItem item = e.GetItem();

	// Check it has a keybind attached
	bool              kb   = false;
	bool              kbp  = false;
	BindListItemData* bind = ((BindListItemData*)list_binds_->GetItemData(item));
	if (bind)
	{
		kb = true;
		if (bind->bind)
			kbp = true;
	}

	// Check if it's parent has a keybind attached
	if (!kb && list_binds_->GetItemData(list_binds_->GetItemParent(item)))
		kb = true;

	// Enable/disable controls accordingly
	btn_add_->Enable(kb);
	btn_remove_->Enable(kb);
	btn_change_->Enable(kb);
	btn_defaults_->Enable(kbp);
}

// -----------------------------------------------------------------------------
// Called when a keybind list item is activated
// -----------------------------------------------------------------------------
void InputPrefsPanel::onListItemActivated(wxTreeListEvent& e)
{
	// Get selected item
	wxTreeListItem item = list_binds_->GetSelection();

	// Change keybind if the item isn't a group
	if (list_binds_->GetItemData(item))
		changeKey(item);
}

// -----------------------------------------------------------------------------
// Called when the 'Set Key' button is clicked
// -----------------------------------------------------------------------------
void InputPrefsPanel::onBtnChangeKey(wxCommandEvent& e)
{
	// Get selected item
	wxTreeListItem item = list_binds_->GetSelection();

	// Change keybind if the item isn't a group
	if (list_binds_->GetItemData(item))
		changeKey(item);
}

// -----------------------------------------------------------------------------
// Called when the 'Add Key' button is clicked
// -----------------------------------------------------------------------------
void InputPrefsPanel::onBtnAddKey(wxCommandEvent& e)
{
	addKey();
}

// -----------------------------------------------------------------------------
// Called when the 'Remove Key' button is clicked
// -----------------------------------------------------------------------------
void InputPrefsPanel::onBtnRemoveKey(wxCommandEvent& e)
{
	removeKey(list_binds_->GetSelection());
}

// -----------------------------------------------------------------------------
// Called when the 'Reset to Default' button is clicked
// -----------------------------------------------------------------------------
void InputPrefsPanel::onBtnDefaults(wxCommandEvent& e)
{
	// Get selected item
	wxTreeListItem    item = list_binds_->GetSelection();
	BindListItemData* item_data = ((BindListItemData*)list_binds_->GetItemData(item));

	// Do nothing if it's not a primary keybind
	if (!item_data || !(item_data->bind))
		return;

	// Remove all child items
	wxTreeListItem child = list_binds_->GetFirstChild(item);
	while (child.IsOk())
	{
		list_binds_->DeleteItem(child);
		child = list_binds_->GetFirstChild(item);
	}

	// Reset primary key to default
	item_data->key.key = "";
	if (!item_data->bind->defaults().empty())
		item_data->key = item_data->bind->firstDefault();

	// Go through default keys for the bind
	const auto& defaults = item_data->bind->defaults();
	for (auto i = defaults.begin() + 1; i != defaults.end(); ++i)
		list_binds_->AppendItem(item, "", -1, -1, new BindListItemData(*i));

	// Refresh list
	updateBindsList();
}

// -----------------------------------------------------------------------------
// Called when a key is pressed in the keybind list
// -----------------------------------------------------------------------------
void InputPrefsPanel::onListKeyDown(wxKeyEvent& e)
{
	if (e.GetKeyCode() == WXK_DELETE)
		removeKey(list_binds_->GetSelection());
	else if (e.GetKeyCode() == WXK_INSERT)
		addKey();
	else
		e.Skip();
}
