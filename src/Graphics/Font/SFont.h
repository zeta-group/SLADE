#pragma once

#include "OpenGL/GLTexture.h"

//// Some defines
//#define SF_ALIGN_LEFT 0
//#define SF_Align::Right 1
//#define SF_ALIGN_CENTER 2

// class SFontChar
//{
//	friend class SFont;
// public:
//	SFontChar();
//	~SFontChar();
//
// private:
//	uint16_t	width;
//	uint16_t	height;
//	rect_t		tex_bounds;
//};

class SFont
{
public:
	struct Char
	{
		unsigned width  = 0;
		unsigned height = 0;
		rect_t   tex_bounds;

		bool valid() const { return width > 0 || height > 0; }
	};

	enum class Align
	{
		Left,
		Right,
		Center
	};

	SFont() = default;
	~SFont() = default;

	int lineHeight() const { return line_height_; }

	// Font reading
	bool loadFont0(MemChunk& mc);
	bool loadFont1(MemChunk& mc);
	bool loadFont2(MemChunk& mc);
	bool loadFontM(MemChunk& mc);
	bool loadBMF(MemChunk& mc);

	// Rendering
	void drawCharacter(char c, ColRGBA colour = ColRGBA::WHITE);
	void drawString(string_view str, ColRGBA colour = ColRGBA::WHITE, Align align = Align::Left);

	// Static
	static SFont& vgaFont();
	static SFont& sladeFont();

private:
	Char      characters_[256];
	GLTexture texture_;
	int       line_height_;
	int       spacing_;

	// Global fonts
	static SFont font_vga_;
	static SFont font_slade_;
};
