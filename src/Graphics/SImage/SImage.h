#pragma once

#include "General/ListenerAnnouncer.h"
#include "Graphics/Palette/Palette.h"

class Translation;
class SIFormat;

class SImage : public Announcer
{
	friend class SIFormat;

public:
	enum class Type
	{
		PalMask,  // 2 bytes per pixel: palette index and alpha value
		RGBA,     // 4 bytes per pixel: RGBA
		AlphaMap, // 1 byte per pixel: alpha

		Any
	};

	enum class Blend
	{
		Normal,          // Normal blend
		Add,             // Additive blend
		Subtract,        // Subtractive blend
		ReverseSubtract, // Reverse-subtractive blend
		Modulate,        // 'Modulate' blend
	};

	// Simple struct to hold pixel drawing properties
	struct DrawProps
	{
		Blend blend; // The blending mode
		float alpha;
		bool  src_alpha; // Whether to respect source pixel alpha

		DrawProps()
		{
			blend     = Blend::Normal;
			alpha     = 1.0f;
			src_alpha = true;
		}
	};

	enum class AlphaSource
	{
		// Alpha map generation sources
		Brightness = 0,
		Alpha,
	};

	struct Info
	{
		int    width     = 0;
		int    height    = 0;
		Type   colformat = Type::RGBA;
		string format;
		int    numimages   = 1;
		int    imgindex    = 0;
		int    offset_x    = 0;
		int    offset_y    = 0;
		bool   has_palette = false;
	};

	SImage(Type type = Type::RGBA) : type_{ type } {}
	virtual ~SImage() { clearData(); }

	bool isValid() const { return (size_.x > 0 && size_.y > 0 && data_); }

	Type      type() const { return type_; }
	bool      dataRGBA(MemChunk& mc, Palette* pal = nullptr);
	bool      dataRGB(MemChunk& mc, Palette* pal = nullptr);
	bool      dataIndexed(MemChunk& mc) const;
	point2_t  size() const { return size_; }
	int       width() const { return size_.x; }
	int       height() const { return size_.y; }
	int       imgIndex() const { return img_index_; }
	int       nImages() const { return num_images_; }
	bool      hasPalette() const { return has_palette_; }
	Palette*  palette() { return &palette_; }
	point2_t  offset() const { return offset_; }
	unsigned  stride() const;
	uint8_t   bpp() const;
	ColRGBA    pixelAt(unsigned x, unsigned y, Palette* pal = nullptr);
	uint8_t   pixelIndexAt(unsigned x, unsigned y) const;
	SIFormat* format() const { return format_; }
	Info      info() const;

	void setOffset(const point2_t& offset);
	void setXOffset(int offset);
	void setYOffset(int offset);
	void setPalette(Palette* pal)
	{
		palette_.copyPalette(pal);
		has_palette_ = true;
	}

	void setWidth(int w);
	void setHeight(int h);

	// Misc
	void   clear();
	void   create(int width, int height, Type type, Palette* pal = nullptr, int index = 0, int numimages = 1);
	void   create(const Info& info, Palette* pal = nullptr);
	void   fillAlpha(uint8_t alpha = 0);
	short  findUnusedColour() const;
	bool   validFlatSize();
	size_t countColours() const;
	void   shrinkPalette(Palette* pal = nullptr);
	bool   copyImage(SImage* image);

	// Image format reading
	bool open(MemChunk& data, int index = 0, string_view type_hint = "");
	bool loadFont0(const uint8_t* gfx_data, int size);
	bool loadFont1(const uint8_t* gfx_data, int size);
	bool loadFont2(const uint8_t* gfx_data, int size);
	bool loadFontM(const uint8_t* gfx_data, int size);
	bool loadBMF(const uint8_t* gfx_data, int size);
	bool loadWolfFont(const uint8_t* gfx_data, int size);
	bool loadJediFNT(const uint8_t* gfx_data, int size);
	bool loadJediFONT(const uint8_t* gfx_data, int size);
	bool loadJaguarSprite(const uint8_t* header, int hdr_size, const uint8_t* gfx_data, int size);
	bool loadJaguarTexture(const uint8_t* gfx_data, int size, int i_width, int i_height);

	// Conversion stuff
	bool convertRGBA(Palette* pal = nullptr);
	bool convertPaletted(Palette* pal_target, Palette* pal_current = nullptr);
	bool convertAlphaMap(AlphaSource alpha_source = AlphaSource::Brightness, Palette* pal = nullptr);
	bool maskFromColour(ColRGBA colour, Palette* pal = nullptr);
	bool maskFromBrightness(Palette* pal = nullptr);
	bool cutoffMask(uint8_t threshold);

	// Image modification
	bool setPixel(int x, int y, ColRGBA colour, Palette* pal = nullptr);
	bool setPixel(int x, int y, uint8_t pal_index, uint8_t alpha = 255);
	bool imgconv();
	bool rotate(int angle);
	bool mirror(bool vert);
	bool crop(long x1, long y1, long x2, long y2);
	bool resize(int nwidth, int nheight);
	bool setImageData(uint8_t* ndata, int nwidth, int nheight, Type ntype);
	bool applyTranslation(Translation* tr, Palette* pal = nullptr, bool truecolor = false);
	bool applyTranslation(string_view tr, Palette* pal = nullptr, bool truecolor = false);
	bool drawPixel(int x, int y, ColRGBA colour, DrawProps& properties, Palette* pal);
	bool drawImage(
		SImage&    img,
		int        x,
		int        y,
		DrawProps& properties,
		Palette*   pal_src  = nullptr,
		Palette*   pal_dest = nullptr);
	bool colourise(ColRGBA colour, Palette* pal = nullptr, int start = -1, int stop = -1);
	bool tint(ColRGBA colour, float amount, Palette* pal = nullptr, int start = -1, int stop = -1);
	bool adjust();
	bool mirrorpad();

private:
	point2_t  size_ = { 0, 0 };
	uint8_t*  data_ = nullptr;
	uint8_t*  mask_ = nullptr;
	Type      type_ = Type::RGBA;
	Palette   palette_;
	bool      has_palette_ = false;
	point2_t  offset_      = { 0, 0 };
	SIFormat* format_      = nullptr;

	// For multi-image files
	int img_index_  = 0;
	int num_images_ = 1;

	// Internal data to avoid repeated calculations
	unsigned num_pixels_;
	unsigned data_size_;

	// Internal functions
	void clearData(bool clear_mask = true);
	void allocData();
};
