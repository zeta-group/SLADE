#pragma once

#include "OGLCanvas.h"

class PaletteCanvas : public OGLCanvas
{
public:
	enum class SelectionType
	{
		None,
		Single,
		Range
	};

	PaletteCanvas(wxWindow* parent, int id);
	~PaletteCanvas() = default;

	bool          doubleWidth() const { return double_width_; }
	int           getSelectionStart() const { return sel_begin_; }
	int           getSelectionEnd() const { return sel_end_; }
	SelectionType allowSelection() const { return allow_selection_; }

	void doubleWidth(bool dw) { double_width_ = dw; }
	void setSelection(int begin, int end = -1);
	void allowSelection(SelectionType sel) { allow_selection_ = sel; }

	void   draw() override;
	ColRGBA getSelectedColour();

	// Events
	void onMouseLeftDown(wxMouseEvent& e);

private:
	int           sel_begin_       = -1;
	int           sel_end_         = -1;
	bool          double_width_    = false;
	SelectionType allow_selection_ = SelectionType::None;

	// Events
	void onMouseRightDown(wxMouseEvent& e);
	void onMouseMotion(wxMouseEvent& e);
};
