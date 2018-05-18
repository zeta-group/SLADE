#pragma once

#ifdef USE_SFML_RENDERWINDOW
#include <SFML/Graphics.hpp>
#endif

#include "common.h"

#include "Utility/Structs.h"

class GLTexture;
class FontManager;

namespace Drawing
{
enum class Font
{
	Normal,
	Condensed,
	Bold,
	BoldCondensed,
	Monospace,
	Small
};

enum class Align
{
	Left,
	Right,
	Center
};

// Initialisation
void initFonts();

// Info
int fontSize();

// Basic drawing
void drawLine(const fpoint2_t& start, const fpoint2_t& end);
void drawLine(double x1, double y1, double x2, double y2);
void drawLineTabbed(const fpoint2_t& start, const fpoint2_t& end, double tab = 0.1, double tab_max = 16);
void drawArrow(
	const fpoint2_t& p1,
	const fpoint2_t& p2,
	const ColRGBA&    color            = ColRGBA::WHITE,
	bool             twoway           = false,
	double           arrowhead_angle  = 0.7854f,
	double           arrowhead_length = 25.f);
void drawRect(const fpoint2_t& tl, const fpoint2_t& br);
void drawRect(double x1, double y1, double x2, double y2);
void drawFilledRect(const fpoint2_t& tl, const fpoint2_t& br);
void drawFilledRect(double x1, double y1, double x2, double y2);
void drawBorderedRect(const fpoint2_t& tl, const fpoint2_t& br, const ColRGBA& colour, const ColRGBA& border_colour);
void drawBorderedRect(double x1, double y1, double x2, double y2, const ColRGBA& colour, const ColRGBA& border_colour);
void drawEllipse(const fpoint2_t& mid, double radius_x, double radius_y, int sides, const ColRGBA& colour);
void drawFilledEllipse(const fpoint2_t& mid, double radius_x, double radius_y, int sides, const ColRGBA& colour);

// Texture drawing
frect_t fitTextureWithin(
	GLTexture* tex,
	double     x1,
	double     y1,
	double     x2,
	double     y2,
	double     padding,
	double     max_scale = 1);
void drawTextureWithin(
	GLTexture* tex,
	double     x1,
	double     y1,
	double     x2,
	double     y2,
	double     padding,
	double     max_scale = 1);

// Text drawing
void drawText(
	const string& text,
	int           x         = 0,
	int           y         = 0,
	const ColRGBA& colour    = ColRGBA::WHITE,
	Font          font      = Font::Normal,
	Align         alignment = Align::Left,
	frect_t*      bounds    = nullptr);
fpoint2_t textExtents(const string& text, Font font = Font::Normal);
void      enableTextStateReset(bool enable = true);
void      setTextState(bool set = true);
void      setTextOutline(double thickness, const ColRGBA& colour = ColRGBA::BLACK);

// Specific
void drawHud();

// Implementation-specific
#ifdef USE_SFML_RENDERWINDOW
void setRenderTarget(sf::RenderWindow* target);
#endif


// From CodeLite
wxColour getPanelBGColour();
wxColour getMenuTextColour();
wxColour getMenuBarBGColour();
wxColour lightColour(const wxColour& colour, float percent);
wxColour darkColour(const wxColour& colour, float percent);
} // namespace Drawing

// TextBox class
class TextBox
{
public:
	TextBox(string_view text, Drawing::Font font, int width, int line_height = -1);
	~TextBox() = default;

	int  getHeight() const { return height_; }
	int  getWidth() const { return width_; }
	void setText(string_view text);
	void setSize(int width);
	void setLineHeight(int height) { line_height_ = height; }
	void draw(int x, int y, const ColRGBA& colour = ColRGBA::WHITE, Drawing::Align alignment = Drawing::Align::Left);

private:
	string         text_;
	vector<string> lines_;
	Drawing::Font  font_;
	int            width_;
	int            height_;
	int            line_height_;

	void split(string_view text);
};
