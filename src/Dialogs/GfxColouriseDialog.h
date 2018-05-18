#pragma once

class GfxCanvas;
class ArchiveEntry;
class Palette;
class ColourBox;

class GfxColouriseDialog : public wxDialog
{
public:
	GfxColouriseDialog(wxWindow* parent, ArchiveEntry* entry, Palette* pal);

	ColRGBA getColour() const;
	void    setColour(string_view col) const;

private:
	GfxCanvas*    gfx_preview_;
	ArchiveEntry* entry_;
	Palette*      palette_;
	ColourBox*    cb_colour_;

	// Events
	void onColourChanged(wxEvent& e);
	void onResize(wxSizeEvent& e);
};
