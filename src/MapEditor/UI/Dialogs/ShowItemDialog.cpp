
// -----------------------------------------------------------------------------
// SLADE - It's a Doom Editor
// Copyright(C) 2008 - 2017 Simon Judd
//
// Email:       sirjuddington@gmail.com
// Web:         https://slade.mancubus.net
// Filename:    ShowItemDialog.cpp
// Description: A dialog allowing the user to select a map object type
//              (line / thing / etc) and enter an index.Used for the
//              'Show Item...' menu item in the map editor
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
#include "ShowItemDialog.h"
#include "UI/WxUtils.h"


// -----------------------------------------------------------------------------
//
// ShowItemDialog Class Functions
//
// -----------------------------------------------------------------------------


// -----------------------------------------------------------------------------
// ShowItemDialog class constructor
// -----------------------------------------------------------------------------
ShowItemDialog::ShowItemDialog(wxWindow* parent) : wxDialog(parent, -1, "Show Item")
{
	// Setup sizer
	auto sizer = new wxBoxSizer(wxVERTICAL);
	SetSizer(sizer);
	auto gb_sizer = new wxGridBagSizer(UI::pad(), UI::pad());
	sizer->Add(gb_sizer, 1, wxEXPAND | wxLEFT | wxRIGHT | wxTOP, UI::padLarge());

	// Object type
	wxString types[] = { "Vertex", "Line", "Side", "Sector", "Thing" };
	gb_sizer->Add(new wxStaticText(this, -1, "Type:"), { 0, 0 }, { -1, -1 }, wxALIGN_CENTER_VERTICAL);
	choice_type_ = new wxChoice(this, -1, wxDefaultPosition, wxDefaultSize, 5, types);
	gb_sizer->Add(choice_type_, { 0, 1 }, { -1, -1 }, wxEXPAND);

	// Index
	gb_sizer->Add(new wxStaticText(this, -1, "Index:"), { 1, 0 }, { -1, -1 }, wxALIGN_CENTER_VERTICAL);
	text_index_ = new wxTextCtrl(this, -1, "", wxDefaultPosition, wxDefaultSize, 0, wxDefaultValidator);
	gb_sizer->Add(text_index_, { 1, 1 }, { -1, -1 }, wxEXPAND);

	// Dialog buttons
	sizer->AddSpacer(UI::pad());
	sizer->Add(CreateButtonSizer(wxOK | wxCANCEL), 0, wxEXPAND | wxLEFT | wxRIGHT | wxBOTTOM, UI::padLarge());

	// Init layout
	gb_sizer->AddGrowableCol(1, 1);
	SetInitialSize(WxUtils::scaledSize(300, -1));
	CenterOnParent();
	wxDialog::Layout();
	text_index_->SetFocus();
	text_index_->SetFocusFromKbd();
}

// -----------------------------------------------------------------------------
// Returns the selected object type
// -----------------------------------------------------------------------------
int ShowItemDialog::selectedType() const
{
	return choice_type_->GetSelection() + 1;
}

// -----------------------------------------------------------------------------
// Returns the entered index, or -1 if invalid
// -----------------------------------------------------------------------------
int ShowItemDialog::selectedIndex() const
{
	long value;
	if (text_index_->GetValue().ToLong(&value))
		return value;
	else
		return -1;
}

// -----------------------------------------------------------------------------
// Sets the object type dropdown to [type]
// -----------------------------------------------------------------------------
void ShowItemDialog::setType(MapObject::Type type) const
{
	switch (type)
	{
	case MapObject::Type::Line: choice_type_->Select(1); break;
	case MapObject::Type::Side: choice_type_->Select(2); break;
	case MapObject::Type::Sector: choice_type_->Select(3); break;
	case MapObject::Type::Thing: choice_type_->Select(4); break;
	default: choice_type_->Select(0); break;
	}
}
