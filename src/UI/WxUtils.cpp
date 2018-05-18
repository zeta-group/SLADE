
// -----------------------------------------------------------------------------
// SLADE - It's a Doom Editor
// Copyright(C) 2008 - 2017 Simon Judd
//
// Email:       sirjuddington@gmail.com
// Web:         http://slade.mancubus.net
// Filename:    WxUtils.cpp
// Description: WxWidgets-related utility functions
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
#include "General/UI.h"
#include "Graphics/Icons.h"
#include "UI/WxUtils.h"
#include "Utility/StringUtils.h"


// -----------------------------------------------------------------------------
//
// Variables
//
// -----------------------------------------------------------------------------
CVAR(String, font_monospace, "Consolas,Lucida Console", CVAR_SAVE)
CVAR(Int, tab_style, 1, CVAR_SAVE)


// -----------------------------------------------------------------------------
//
// WxUtils Namespace Functions
//
// -----------------------------------------------------------------------------


// -----------------------------------------------------------------------------
// Creates a wxMenuItem from the given parameters, including giving it an icon
// from slade.pk3 if specified
// -----------------------------------------------------------------------------
wxMenuItem* WxUtils::createMenuItem(wxMenu* menu, int id, string_view label, string_view help, string_view icon)
{
	auto item = new wxMenuItem(menu, id, { label.data(), label.size() }, { help.data(), help.size() });

	if (!icon.empty())
		item->SetBitmap(Icons::getIcon(Icons::Any, icon));

	return item;
}

// -----------------------------------------------------------------------------
// Returns a copy of the font [base] with a monospace face/family
// -----------------------------------------------------------------------------
wxFont WxUtils::getMonospaceFont(wxFont base)
{
	wxFont font(base.GetPointSize(), wxFONTFAMILY_MODERN, base.GetStyle(), base.GetWeight());

	auto split = wxSplit(font_monospace.value, ',');
	for (const auto& a : split)
	{
		if (base.SetFaceName(a))
		{
			font.SetFaceName(a);
			break;
		}
	}

	return font;
}

// -----------------------------------------------------------------------------
// Creates an image list with a DPI-scaled 'small' image size (for lists, etc.)
// -----------------------------------------------------------------------------
wxImageList* WxUtils::createSmallImageList()
{
	auto icon_size = UI::scaleFactor() > 1.25 ? 32 : 16;
	return new wxImageList(icon_size, icon_size, false, 0);
}

// -----------------------------------------------------------------------------
// Creates a wxPanel and places [control] on it, with [pad] padding around it
// -----------------------------------------------------------------------------
wxPanel* WxUtils::createPadPanel(wxWindow* parent, wxWindow* control, int pad)
{
	if (pad < 0)
		pad = UI::pad();

	auto panel = new wxPanel(parent);
	panel->SetSizer(new wxBoxSizer(wxVERTICAL));
	panel->GetSizer()->Add(control, 1, wxEXPAND | wxALL, pad);
	control->Reparent(panel);

	return panel;
}

// -----------------------------------------------------------------------------
// Creates a simple horizontal box sizer with a [label] on the left and [widget]
// on the right
// -----------------------------------------------------------------------------
wxSizer* WxUtils::createLabelHBox(wxWindow* parent, const string& label, wxWindow* widget)
{
	auto hbox = new wxBoxSizer(wxHORIZONTAL);
	hbox->Add(new wxStaticText(parent, -1, label), 0, wxALIGN_CENTER_VERTICAL | wxRIGHT, UI::pad());
	hbox->Add(widget, 1, wxEXPAND);
	return hbox;
}

// -----------------------------------------------------------------------------
// Creates a simple horizontal box sizer with a [label] on the left and another
// [sizer] on the right
// -----------------------------------------------------------------------------
wxSizer* WxUtils::createLabelHBox(wxWindow* parent, const string& label, wxSizer* sizer)
{
	auto hbox = new wxBoxSizer(wxHORIZONTAL);
	hbox->Add(new wxStaticText(parent, -1, label), 0, wxALIGN_CENTER_VERTICAL | wxRIGHT, UI::pad());
	hbox->Add(sizer, 1, wxEXPAND);
	return hbox;
}

// -----------------------------------------------------------------------------
// Creates a simple vertical box sizer with a [label] on the top and [widget]
// on the bottom
// -----------------------------------------------------------------------------
wxSizer* ::WxUtils::createLabelVBox(wxWindow* parent, const string& label, wxWindow* widget)
{
	auto vbox = new wxBoxSizer(wxVERTICAL);
	vbox->Add(new wxStaticText(parent, -1, label), 0, wxBOTTOM, UI::px(UI::Size::PadMinimum));
	vbox->Add(widget, 1, wxEXPAND);
	return vbox;
}

// -----------------------------------------------------------------------------
// Creates a simple vertical box sizer with a [label] on the top and another
// [sizer] on the bottom
// -----------------------------------------------------------------------------
wxSizer* WxUtils::createLabelVBox(wxWindow* parent, const string& label, wxSizer* sizer)
{
	auto vbox = new wxBoxSizer(wxVERTICAL);
	vbox->Add(new wxStaticText(parent, -1, label), 0, wxBOTTOM, UI::px(UI::Size::PadMinimum));
	vbox->Add(sizer, 1, wxEXPAND);
	return vbox;
}

// -----------------------------------------------------------------------------
// Returns a horizontal box sizer containing [widgets].
// [widgets] can contain a combination of wxWindow and wxSizer objects
// -----------------------------------------------------------------------------
wxSizer* WxUtils::layoutHorizontally(const vector<wxObject*>& widgets, int expand_col)
{
	auto hbox = new wxBoxSizer(wxHORIZONTAL);

	// Add widgets/sizers
	for (auto a = 0u; a < widgets.size(); ++a)
	{
		auto widget = widgets[a];

		// Widget
		if (widget->IsKindOf(&wxWindow::ms_classInfo))
		{
			hbox->Add(
				(wxWindow*)widget,
				expand_col == (int)a ? 1 : 0,
				widget == widgets[0] ? wxEXPAND : wxEXPAND | wxLEFT,
				UI::pad());
		}

		// Sizer
		else if (widget->IsKindOf(&wxSizer::ms_classInfo))
		{
			hbox->Add(
				(wxSizer*)widget,
				expand_col == (int)a ? 1 : 0,
				widget == widgets[0] ? wxEXPAND : wxEXPAND | wxLEFT,
				UI::pad());
		}
	}

	return hbox;
}

// -----------------------------------------------------------------------------
// Same as above, however instead of returning a new sizer, it adds it to the
// given [sizer] with [flags]
// -----------------------------------------------------------------------------
void WxUtils::layoutHorizontally(
	wxSizer*                 sizer,
	const vector<wxObject*>& widgets,
	const wxSizerFlags&      flags,
	int                      expand_col)
{
	sizer->Add(layoutHorizontally(widgets, expand_col), flags);
}

// -----------------------------------------------------------------------------
// Returns a vertical box sizer containing [widgets].
// [widgets] can contain a combination of wxWindow and wxSizer objects
// -----------------------------------------------------------------------------
wxSizer* WxUtils::layoutVertically(const vector<wxObject*>& widgets, int expand_row)
{
	auto vbox = new wxBoxSizer(wxVERTICAL);

	// Add widgets/sizers
	for (auto a = 0u; a < widgets.size(); ++a)
	{
		auto widget = widgets[a];

		// Widget
		if (widget->IsKindOf(&wxWindow::ms_classInfo))
		{
			vbox->Add(
				(wxWindow*)widget,
				expand_row == (int)a ? 1 : 0,
				widget == widgets[0] ? wxEXPAND : wxEXPAND | wxTOP,
				UI::pad());
		}

		// Sizer
		else if (widget->IsKindOf(&wxSizer::ms_classInfo))
		{
			vbox->Add(
				(wxSizer*)widget,
				expand_row == (int)a ? 1 : 0,
				widget == widgets[0] ? wxEXPAND : wxEXPAND | wxTOP,
				UI::pad());
		}
	}

	return vbox;
}

// -----------------------------------------------------------------------------
// Same as above, however instead of returning a new sizer, it adds it to the
// given [sizer] with [flags]
// -----------------------------------------------------------------------------
void WxUtils::layoutVertically(
	wxSizer*                 sizer,
	const vector<wxObject*>& widgets,
	const wxSizerFlags&      flags,
	int                      expand_row)
{
	sizer->Add(layoutVertically(widgets, expand_row), flags);
}

// -----------------------------------------------------------------------------
// Returns a wxArrayString containing the strings in [vector]
// -----------------------------------------------------------------------------
wxArrayString WxUtils::arrayString(const vector<string>& vector)
{
	wxArrayString array;
	for (const auto& str : vector)
		array.Add(str);
	return array;
}

vector<string> WxUtils::vectorString(const wxArrayString& list)
{
	vector<string> vec;
	for (const auto& wxs : list)
		vec.push_back(wxs.ToStdString());
	return vec;
}

// -----------------------------------------------------------------------------
// Returns a DPI-scaled wxSize [x,y]
// -----------------------------------------------------------------------------
wxSize WxUtils::scaledSize(int x, int y)
{
	return { x < 0 ? -1 : UI::scalePx(x), y < 0 ? -1 : UI::scalePx(y) };
}

// -----------------------------------------------------------------------------
// Returns a DPI-scaled wxPoint [x,y]
// -----------------------------------------------------------------------------
wxPoint WxUtils::scaledPoint(int x, int y)
{
	return { UI::scalePx(x), UI::scalePx(y) };
}

// -----------------------------------------------------------------------------
// Returns a DPI-scaled wxRect at [x,y] of size [width,height]
// -----------------------------------------------------------------------------
wxRect WxUtils::scaledRect(int x, int y, int width, int height)
{
	return { UI::scalePx(x), UI::scalePx(y), UI::scalePx(width), UI::scalePx(height) };
}

// -----------------------------------------------------------------------------
// Creates and returns a wxString from a string_view [view]
// -----------------------------------------------------------------------------
wxString WxUtils::stringFromView(string_view view)
{
	return wxString{ view.data(), view.size() };
}
