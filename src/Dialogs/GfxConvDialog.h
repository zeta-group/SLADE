#pragma once

#include "Graphics/SImage/SIFormat.h"
#include "Graphics/SImage/SImage.h"
#include "UI/SDialog.h"

/* Convert from anything to:
 * Doom Gfx
 * Doom Flat
 * PNG 32bit
 * PNG Paletted
 *
 * Conversion options:
 *	Colours:
 *		- Specify target palette (only if converting to paletted)
 *		- Specify palette conversion type:
 *			- Keep palette indices (only if converting from 8bit)
 *			- Nearest colour matching
 *
 *	Transparency:
 *		- Specify threshold alpha, anything above is opaque (optional if converting from 32bit)
 *		- Specify transparency info:
 *			- Keep existing transparency (threshold comes into play from 32bit-paletted)
 *			- Select transparency colour (to 32bit - select colour, to paletted - select from target palette)
 */

class Archive;
class ArchiveEntry;
class CTexture;
class Palette;
class GfxCanvas;
class PaletteChooser;
class ColourBox;

class GfxConvDialog : public SDialog
{
public:
	GfxConvDialog(wxWindow* parent);
	~GfxConvDialog();

	void setupLayout();

	void openEntry(ArchiveEntry* entry);
	void openEntries(vector<ArchiveEntry*> entries);
	void openTextures(
		vector<CTexture*> textures,
		Palette*          palette    = nullptr,
		Archive*          archive    = nullptr,
		bool              force_rgba = false);
	void updatePreviewGfx();
	void updateControls();
	void getConvertOptions(SIFormat::ConvertOptions& opt);

	bool      itemModified(int index);
	SImage*   getItemImage(int index);
	SIFormat* getItemFormat(int index);
	Palette*  getItemPalette(int index);

	void applyConversion();

private:
	struct ConvFormat
	{
		SIFormat*    format;
		SImage::Type coltype;

		ConvFormat(SIFormat* format = nullptr, SImage::Type coltype = SImage::Type::RGBA)
		{
			this->format  = format;
			this->coltype = coltype;
		}
	};

	struct ConvItem
	{
		ArchiveEntry* entry   = nullptr;
		CTexture*     texture = nullptr;
		SImage        image;
		bool          modified   = false;
		SIFormat*     new_format = nullptr;
		Palette*      palette    = nullptr;
		Archive*      archive    = nullptr;
		bool          force_rgba = false;

		ConvItem(ArchiveEntry* entry = nullptr) : entry{ entry } {}

		ConvItem(CTexture* texture, Palette* palette = nullptr, Archive* archive = nullptr, bool force_rgba = false) :
			texture{ texture },
			palette{ palette },
			archive{ archive },
			force_rgba{ force_rgba }
		{
		}
	};

	vector<ConvItem>   items_;
	size_t             current_item_;
	vector<ConvFormat> conv_formats_;
	ConvFormat         current_format_;

	wxStaticText*   label_current_format_;
	GfxCanvas*      gfx_current_;
	GfxCanvas*      gfx_target_;
	wxButton*       btn_convert_;
	wxButton*       btn_convert_all_;
	wxButton*       btn_skip_;
	wxButton*       btn_skip_all_;
	wxChoice*       combo_target_format_;
	PaletteChooser* pal_chooser_current_;
	PaletteChooser* pal_chooser_target_;
	wxButton*       btn_colorimetry_settings_;

	wxCheckBox*    cb_enable_transparency_;
	wxRadioButton* rb_transparency_existing_;
	wxRadioButton* rb_transparency_colour_;
	wxRadioButton* rb_transparency_brightness_;
	wxSlider*      slider_alpha_threshold_;
	ColourBox*     colbox_transparent_;

	// Conversion options
	Palette target_pal_;
	ColRGBA  colour_trans_;

	bool nextItem();

	// Static
	static string current_palette_name_;
	static string target_palette_name_;

	// Events
	void onResize(wxSizeEvent& e);
	void onBtnConvert(wxCommandEvent& e);
	void onBtnConvertAll(wxCommandEvent& e);
	void onTargetFormatChanged(wxCommandEvent& e);
	void onAlphaThresholdChanged(wxCommandEvent& e);
	void onPreviewCurrentMouseDown(wxMouseEvent& e);
	void onBtnColorimetrySettings(wxCommandEvent& e);
};
