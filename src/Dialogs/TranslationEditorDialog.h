#pragma once

#include "Graphics/SImage/SImage.h"
#include "Graphics/Translation.h"
#include "UI/Canvas/OGLCanvas.h"
#include "UI/Controls/ColourBox.h"
#include "UI/WxBasicControls.h"

class GfxCanvas;
class PaletteCanvas;
class Palette;
class ArchiveEntry;
class wxListBox;
class wxPanel;

class GradientBox : public OGLCanvas
{
public:
	GradientBox(wxWindow* parent, int steps = -1);
	~GradientBox() = default;

	void setStartCol(ColRGBA col) { col_start_.set(col.r, col.g, col.b, 255); }
	void setEndCol(ColRGBA col) { col_end_.set(col.r, col.g, col.b, 255); }
	void setSteps(int steps) { this->steps_ = steps; }

	void draw() override;

private:
	ColRGBA col_start_;
	ColRGBA col_end_;
	int    steps_;
};


class TranslationEditorDialog : public wxDialog
{
public:
	TranslationEditorDialog(
		wxWindow* parent,
		Palette*  pal,
		string    title         = "Edit Translation",
		SImage*   preview_image = nullptr);
	~TranslationEditorDialog() = default;

	Translation& getTranslation() { return translation_; }
	bool         getTruecolor();

	void openTranslation(Translation& trans);
	void openRange(int index);
	void updateListItem(int index);
	void setStartColour(ColRGBA col);
	void setEndColour(ColRGBA col);
	void setTintColour(ColRGBA col);
	void setTintAmount(int amount);
	void showPaletteTarget();
	void showGradientTarget();
	void showTintTarget(bool tint);
	void updatePreviews();

private:
	Palette*    palette_;
	Translation translation_;
	SImage      image_preview_;

	PaletteCanvas* pal_canvas_original_;
	wxListBox*     list_translations_;
	wxButton*      btn_add_;
	wxButton*      btn_remove_;
	wxButton*      btn_up_;
	wxButton*      btn_down_;
	wxButton*      btn_load_;
	wxButton*      btn_save_;
	wxRadioButton* rb_type_palette_;
	wxRadioButton* rb_type_colour_;
	wxRadioButton* rb_type_desaturate_;
	wxRadioButton* rb_type_colourise_;
	wxRadioButton* rb_type_tint_;
	wxTextCtrl*    text_string_;

	// Paletted target range
	wxPanel*       panel_target_palette_;
	PaletteCanvas* pal_canvas_target_;
	wxCheckBox*    cb_target_reverse_;

	// Colour gradient target range
	wxPanel*     panel_target_gradient_;
	ColourBox*   cb_range_begin_;
	ColourBox*   cb_range_end_;
	GradientBox* gb_gradient_;

	// Colourise/tint target range
	wxPanel*      panel_target_tint_;
	ColourBox*    cb_target_tint_;
	wxSlider*     slider_tint_;
	wxStaticText* label_tint_;
	wxStaticText* label_amount_;

	// Preview
	PaletteCanvas* pal_canvas_preview_;
	GfxCanvas*     gfx_preview_;

	// Truecolor
	wxCheckBox* cb_truecolor_;
	wxCheckBox* cb_paletteonly_;

	// Events
	void onSize(wxSizeEvent& e);
	void onTranslationListItemSelected(wxCommandEvent& e);
	void onRBPaletteSelected(wxCommandEvent& e);
	void onRBColourSelected(wxCommandEvent& e);
	void onRBDesaturateSelected(wxCommandEvent& e);
	void onRBColouriseSelected(wxCommandEvent& e);
	void onRBTintSelected(wxCommandEvent& e);
	void onBeginColourChanged(wxEvent& e);
	void onEndColourChanged(wxEvent& e);
	void onTintColourChanged(wxEvent& e);
	void onPalOriginLeftUp(wxMouseEvent& e);
	void onPalTargetLeftUp(wxMouseEvent& e);
	void onTintAmountChanged(wxCommandEvent& e);
	void onBtnRemove(wxCommandEvent& e);
	void onBtnAdd(wxCommandEvent& e);
	void onBtnUp(wxCommandEvent& e);
	void onBtnDown(wxCommandEvent& e);
	void onBtnLoad(wxCommandEvent& e);
	void onBtnSave(wxCommandEvent& e);
	void onGfxPreviewMouseMotion(wxMouseEvent& e);
	void onCBTargetReverse(wxCommandEvent& e);
	void onCBTruecolor(wxCommandEvent& e);
	void onCBPaletteOnly(wxCommandEvent& e);
};
