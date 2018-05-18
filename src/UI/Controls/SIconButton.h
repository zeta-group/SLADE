#pragma once

#include "Graphics/Icons.h"

class SIconButton : public wxBitmapButton
{
public:
	SIconButton(wxWindow* parent, Icons::Type icon_type, string_view icon, string_view tooltip = "");
	SIconButton(wxWindow* parent, string_view icon, string_view tooltip = "") :
		SIconButton(parent, Icons::General, icon, tooltip)
	{
	}
	~SIconButton() = default;
};
