#pragma once

#include "Graphics/Palette/Palette.h"
#include "General/ListenerAnnouncer.h"

class Translation;
class SIFormat;

class SImage : public Announcer
{
	friend class SIFormat;

public:
	// Alpha map generation sources
	enum class AlphaSource
	{
		Brightness,
		Alpha,
	};

	enum class PixelFormat
	{
		PalMask,	// 2 bytes per pixel: palette index and alpha value
		RGBA,		// 4 bytes per pixel: RGBA
		AlphaMap,	// 1 byte per pixel: alpha
		Any			// For conversion, don't use for an actual SImage
	};

	enum class BlendType
	{
		Normal,				// Normal blend
		Add,				// Additive blend
		Subtract,			// Subtractive blend
		ReverseSubtract,	// Reverse-subtractive blend
		Modulate,			// 'Modulate' blend
	};

	// Simple struct to hold pixel drawing properties
	struct DrawProps
	{
		BlendType	blend;		// The blending mode
		float		alpha;
		bool		src_alpha;	// Whether to respect source pixel alpha

		DrawProps() { blend = BlendType::Normal; alpha = 1.0f; src_alpha = true; }
	};

	struct Info
	{
		int			width;
		int			height;
		PixelFormat	colformat;
		string		format;
		int			numimages;
		int			imgindex;
		point2_t	offset;
		bool		has_palette;

		Info()
		{
			width = height = offset.x = offset.y = imgindex = 0;
			colformat = PixelFormat::RGBA;
			numimages = 1;
			has_palette = false;
		}
	};

	SImage(PixelFormat type = PixelFormat::RGBA);
	SImage(int width, int height, PixelFormat pixel_format, Palette* pal = nullptr, int index = 0, int numimages = 1)
		: SImage(pixel_format)
		{ create(width, height, pixel_format, pal, index, numimages); }
	virtual ~SImage();

	bool	isValid() { return (size_.x > 0 && size_.y > 0 && data_); }

	PixelFormat	pixelFormat() { return pixel_format_; }
	bool		getRGBAData(MemChunk& mc, Palette* pal = nullptr);
	bool		getRGBData(MemChunk& mc, Palette* pal = nullptr);
	bool		getIndexedData(MemChunk& mc);
	int			width() { return size_.x; }
	int			height() { return size_.y; }
	int			getIndex() { return img_index_; }
	int			getSize() { return num_images_; }
	bool		hasPalette() { return has_palette_; }
	Palette*	palette() { return &palette_; }
	point2_t	offset() { return offset_; }
	unsigned	stride();
	uint8_t		bytesPerPixel();
	rgba_t		colourAt(unsigned x, unsigned y, Palette* pal = nullptr);
	uint8_t		paletteIndexAt(unsigned x, unsigned y);
	SIFormat*	format() { return format_; }
	Info		info();

	void	setOffset(point2_t offset);
	void	setXOffset(int offset);
	void	setYOffset(int offset);
	void	setPalette(Palette* pal) { palette_.copyPalette(pal); has_palette_ = true; }
	void	setWidth(int w);
	void	setHeight(int h);

	// Misc
	void	clear();
	void	create(int width, int height, PixelFormat type, Palette* pal = nullptr, int index = 0, int numimages = 1);
	void	create(Info info, Palette* pal = nullptr);
	void	fillAlpha(uint8_t alpha = 0);
	short	findUnusedColour();
	bool	validFlatSize();
	size_t	countColours();
	void	shrinkPalette(Palette* pal = nullptr);
	bool	copyImage(SImage* image);

	// Image format reading
	bool	open(MemChunk& data, int index = 0, string type_hint = "");
	bool	loadFont0(const uint8_t* gfx_data, int size);
	bool	loadFont1(const uint8_t* gfx_data, int size);
	bool	loadFont2(const uint8_t* gfx_data, int size);
	bool	loadFontM(const uint8_t* gfx_data, int size);
	bool	loadBMF(const uint8_t* gfx_data, int size);
	bool	loadWolfFont(const uint8_t* gfx_data, int size);
	bool	loadJediFNT(const uint8_t* gfx_data, int size);
	bool	loadJediFONT(const uint8_t* gfx_data, int size);
	bool	loadJaguarSprite(const uint8_t* header, int hdr_size, const uint8_t* gfx_data, int size);
	bool	loadJaguarTexture(const uint8_t* gfx_data, int size, int i_width, int i_height);

	// Conversion stuff
	bool	convertRGBA(Palette* pal = nullptr);
	bool	convertPaletted(Palette* pal_target, Palette* pal_current = nullptr);
	bool	convertAlphaMap(AlphaSource alpha_source = AlphaSource::Brightness, Palette* pal = nullptr);
	bool	maskFromColour(rgba_t colour, Palette* pal = nullptr);
	bool	maskFromBrightness(Palette* pal = nullptr);
	bool	cutoffMask(uint8_t threshold);

	// Image modification
	bool	setPixel(int x, int y, rgba_t colour, Palette* pal = nullptr);
	bool	setPixel(int x, int y, uint8_t pal_index, uint8_t alpha = 255);
	bool	imgconv();
	bool	rotate(int angle);
	bool	mirror(bool vert);
	bool	crop(long x1, long y1, long x2, long y2);
	bool	resize(int nwidth, int nheight);
	bool	setImageData(uint8_t* ndata, int nwidth, int nheight, PixelFormat ntype);
	bool	applyTranslation(Translation* tr, Palette* pal = nullptr, bool truecolor = false);
	bool	applyTranslation(string tr, Palette* pal = nullptr, bool truecolor = false);
	bool	drawPixel(int x, int y, rgba_t colour, DrawProps& properties, Palette* pal);
	bool	drawImage(SImage& img, int x, int y, DrawProps& properties, Palette* pal_src = nullptr, Palette* pal_dest = nullptr);
	bool	colourise(rgba_t colour, Palette* pal = nullptr, int start = -1, int stop = -1);
	bool	tint(rgba_t colour, float amount, Palette* pal = nullptr, int start = -1, int stop = -1);
	bool	adjust();
	bool	mirrorpad();

private:
	point2_t	size_;
	uint8_t*	data_;
	uint8_t*	mask_;
	PixelFormat	pixel_format_;
	Palette		palette_;
	bool		has_palette_;
	point2_t	offset_;
	SIFormat*	format_;

	// For multi-image files
	int	img_index_;
	int	num_images_;

	// Internal functions
	void	clearData(bool clear_mask = true);
};
