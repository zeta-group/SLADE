#pragma once

#include "UI/Canvas/OGLCanvas.h"
#include "common.h"

class GLTexture;
class MapSide;
class NumberTextCtrl;

class SideTexCanvas : public OGLCanvas
{
public:
	SideTexCanvas(wxWindow* parent);
	~SideTexCanvas() = default;

	string texName() const { return texname_; }
	void   setTexture(string_view texture);
	void   draw() override;

private:
	GLTexture* texture_ = nullptr;
	string     texname_;
};

class TextureComboBox : public wxComboBox
{
public:
	TextureComboBox(wxWindow* parent);
	~TextureComboBox() = default;

private:
	bool list_down_;

	void onDropDown(wxCommandEvent& e);
	void onCloseUp(wxCommandEvent& e);
	void onKeyDown(wxKeyEvent& e);
};

class SidePropsPanel : public wxPanel
{
public:
	SidePropsPanel(wxWindow* parent);
	~SidePropsPanel() = default;

	void openSides(vector<MapSide*>& sides) const;
	void applyTo(vector<MapSide*>& sides) const;

private:
	SideTexCanvas*   gfx_lower_;
	SideTexCanvas*   gfx_middle_;
	SideTexCanvas*   gfx_upper_;
	TextureComboBox* tcb_lower_;
	TextureComboBox* tcb_middle_;
	TextureComboBox* tcb_upper_;
	NumberTextCtrl*  text_offsetx_;
	NumberTextCtrl*  text_offsety_;

	// Events
	void onTextureChanged(wxCommandEvent& e);
	void onTextureClicked(wxMouseEvent& e);
};
