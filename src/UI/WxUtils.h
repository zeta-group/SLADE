#pragma once

#include "General/UI.h"

namespace WxUtils
{
wxMenuItem*  createMenuItem(wxMenu* menu, int id, string_view label, string_view help = "", string_view icon = "");
wxFont       getMonospaceFont(wxFont base);
wxImageList* createSmallImageList();
wxPanel*     createPadPanel(wxWindow* parent, wxWindow* control, int pad = -1);

wxSizer* createLabelHBox(wxWindow* parent, const string& label, wxWindow* widget);
wxSizer* createLabelHBox(wxWindow* parent, const string& label, wxSizer* sizer);
wxSizer* createLabelVBox(wxWindow* parent, const string& label, wxWindow* widget);
wxSizer* createLabelVBox(wxWindow* parent, const string& label, wxSizer* sizer);

wxSizer* layoutHorizontally(const vector<wxObject*>& widgets, int expand_col = -1);
void     layoutHorizontally(
		wxSizer*                 sizer,
		const vector<wxObject*>& widgets,
		const wxSizerFlags&      flags      = {},
		int                      expand_col = -1);

wxSizer* layoutVertically(const vector<wxObject*>& widgets, int expand_row = -1);
void     layoutVertically(
		wxSizer*                 sizer,
		const vector<wxObject*>& widgets,
		const wxSizerFlags&      flags      = {},
		int                      expand_row = -1);

wxArrayString arrayString(const vector<string>& vector);
vector<string> vectorString(const wxArrayString& list);

// Scaling
wxSize  scaledSize(int x, int y);
wxPoint scaledPoint(int x, int y);
wxRect  scaledRect(int x, int y, int width, int height);

// Strings
wxString           stringFromView(string_view view);
inline string_view stringToView(const wxString& str)
{
	return { str.data(), str.size() };
}
} // namespace WxUtils
