#pragma once

class GfxCanvas;
class ArchiveEntry;
class Palette;
class ColourBox;

class GfxTintDialog : public wxDialog
{
public:
	GfxTintDialog(wxWindow* parent, ArchiveEntry* entry, Palette* pal);

	ColRGBA getColour();
	float   getAmount();
	void    setValues(string col, int val);

private:
	GfxCanvas*    gfx_preview_;
	ArchiveEntry* entry_;
	Palette*      palette_;
	ColourBox*    cb_colour_;
	wxSlider*     slider_amount_;
	wxStaticText* label_amount_;

	// Events
	void onColourChanged(wxEvent& e);
	void onAmountChanged(wxCommandEvent& e);
	void onResize(wxSizeEvent& e);
};
