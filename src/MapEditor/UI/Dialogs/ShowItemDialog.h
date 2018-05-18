#pragma once

#include "MapEditor/SLADEMap/MapObject.h"

class wxChoice;
class wxTextCtrl;

class ShowItemDialog : public wxDialog
{
public:
	ShowItemDialog(wxWindow* parent);
	~ShowItemDialog() = default;

	int		selectedType() const;
	int		selectedIndex() const;
	void	setType(MapObject::Type type) const;

private:
	wxChoice*	choice_type_;
	wxTextCtrl*	text_index_;
};
