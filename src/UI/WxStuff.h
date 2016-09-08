
#ifndef __WXSTUFF_H__
#define __WXSTUFF_H__

#undef MIN
#undef MAX

#include "common.h"

#define MAX(a,b) (((a)>(b))?(a):(b))
#define MIN(a,b) (((a)<(b))?(a):(b))

// Some misc wx-related functions
wxMenuItem* createMenuItem(wxMenu* menu, int id, string label, string help = wxEmptyString, string icon = wxEmptyString);
wxFont		getMonospaceFont(wxFont base);

// wxBoxSizer wrapper class to scale by DPI
class wxBoxSizerScaled : public wxBoxSizer
{
public:
	wxBoxSizerScaled(int orient) : wxBoxSizer(orient) {}

	wxSizerItem* Add(wxWindow* window, int proportion = 0, int flag = 0, int border = 0)
	{
		return wxBoxSizer::Add(window, proportion, flag, SM(border));
	}

	wxSizerItem* Add(wxSizer* window, int proportion = 0, int flag = 0, int border = 0)
	{
		return wxBoxSizer::Add(window, proportion, flag, SM(border));
	}
};

class wxStaticBoxSizerScaled : public wxStaticBoxSizer
{
public:
	wxStaticBoxSizerScaled(wxStaticBox* box, int orient) : wxStaticBoxSizer(box, orient) {}

	wxSizerItem* Add(wxWindow* window, int proportion = 0, int flag = 0, int border = 0)
	{
		return wxStaticBoxSizer::Add(window, proportion, flag, SM(border));
	}

	wxSizerItem* Add(wxSizer* window, int proportion = 0, int flag = 0, int border = 0)
	{
		return wxStaticBoxSizer::Add(window, proportion, flag, SM(border));
	}
};

#endif //__WXSTUFF_H__
