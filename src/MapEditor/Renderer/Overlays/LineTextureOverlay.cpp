
// -----------------------------------------------------------------------------
// SLADE - It's a Doom Editor
// Copyright(C) 2008 - 2017 Simon Judd
//
// Email:       sirjuddington@gmail.com
// Web:         http://slade.mancubus.net
// Filename:    LineTextureOverlay.cpp
// Description: LineTextureOverlay class - A full screen map editor overlay that
//              shows (a) lines textures and allows the user to click a texture
//              to browse for it
//
// This program is free software; you can redistribute it and/or modify it
// under the terms of the GNU General Public License as published by the Free
// Software Foundation; either version 2 of the License, or (at your option)
// any later version.
//
// This program is distributed in the hope that it will be useful, but WITHOUT
// ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
// FITNESS FOR A PARTICULAR PURPOSE. See the GNU General Public License for
// more details.
//
// You should have received a copy of the GNU General Public License along with
// this program; if not, write to the Free Software Foundation, Inc.,
// 51 Franklin Street, Fifth Floor, Boston, MA  02110 - 1301, USA.
// -----------------------------------------------------------------------------


// -----------------------------------------------------------------------------
//
// Includes
//
// -----------------------------------------------------------------------------
#include "Main.h"
#include "LineTextureOverlay.h"
#include "Game/Configuration.h"
#include "General/ColourConfiguration.h"
#include "MapEditor/MapEditContext.h"
#include "MapEditor/MapTextureManager.h"
#include "MapEditor/SLADEMap/MapLine.h"
#include "MapEditor/SLADEMap/MapSide.h"
#include "MapEditor/UI/Dialogs/MapTextureBrowser.h"
#include "OpenGL/Drawing.h"


// -----------------------------------------------------------------------------
//
// LineTextureOverlay Class Functions
//
// -----------------------------------------------------------------------------


// -----------------------------------------------------------------------------
// 'Opens' all lines in [list], adds all textures from each
// -----------------------------------------------------------------------------
void LineTextureOverlay::openLines(vector<MapLine*>& list)
{
	// Clear current lines
	lines_.clear();
	this->side1_   = false;
	this->side2_   = false;
	selected_side_ = 0;
	textures_.clear();

	// Go through list
	for (auto& line : list)
	{
		// Add to lines list
		lines_.push_back(line);

		// Process first side
		MapSide* side1 = line->s1();
		if (side1)
		{
			// Add textures
			textures_[MapLine::Part::FrontUpper].addTexture(side1->texUpper());
			textures_[MapLine::Part::FrontMiddle].addTexture(side1->texMiddle());
			textures_[MapLine::Part::FrontLower].addTexture(side1->texLower());

			this->side1_ = true;
		}

		// Process second side
		MapSide* side2 = line->s2();
		if (side2)
		{
			// Add textures
			textures_[MapLine::Part::BackUpper].addTexture(side2->texUpper());
			textures_[MapLine::Part::BackMiddle].addTexture(side2->texMiddle());
			textures_[MapLine::Part::BackLower].addTexture(side2->texLower());

			this->side2_ = true;
		}
	}

	if (!side1_)
		selected_side_ = 1;
}

// -----------------------------------------------------------------------------
// Called when the user closes the overlay. Applies changes if [cancel] is false
// -----------------------------------------------------------------------------
void LineTextureOverlay::close(bool cancel)
{
	// Apply texture changes if not cancelled
	if (!cancel)
	{
		MapEditor::editContext().beginUndoRecord("Change Line Texture", true, false, false);

		// Go through lines
		for (auto& line : lines_)
		{
			// Front Upper
			if (textures_[MapLine::Part::FrontUpper].changed)
				line->setStringProperty("side1.texturetop", textures_[MapLine::Part::FrontUpper].textures[0]);

			// Front Middle
			if (textures_[MapLine::Part::FrontMiddle].changed)
				line->setStringProperty("side1.texturemiddle", textures_[MapLine::Part::FrontMiddle].textures[0]);

			// Front Lower
			if (textures_[MapLine::Part::FrontLower].changed)
				line->setStringProperty("side1.texturebottom", textures_[MapLine::Part::FrontLower].textures[0]);


			// Back Upper
			if (textures_[MapLine::Part::BackUpper].changed)
				line->setStringProperty("side2.texturetop", textures_[MapLine::Part::BackUpper].textures[0]);

			// Back Middle
			if (textures_[MapLine::Part::BackMiddle].changed)
				line->setStringProperty("side2.texturemiddle", textures_[MapLine::Part::BackMiddle].textures[0]);

			// Back Lower
			if (textures_[MapLine::Part::BackLower].changed)
				line->setStringProperty("side2.texturebottom", textures_[MapLine::Part::BackLower].textures[0]);
		}

		MapEditor::editContext().endUndoRecord();
	}

	// Deactivate
	active_ = false;
}

// -----------------------------------------------------------------------------
// Updates the layout of the overlay to fit properly within [width],[height]
// -----------------------------------------------------------------------------
void LineTextureOverlay::updateLayout(int width, int height)
{
	// Determine layout stuff
	int rows = 1;
	if (side1_ && side2_)
		rows = 2;
	int middlex = width * 0.5;
	int middley = height * 0.5;
	int maxsize = min(width / 3, height / rows);
	tex_size_   = maxsize - 64;
	if (tex_size_ > 256)
		tex_size_ = 256;
	int border = (maxsize - tex_size_) * 0.5;
	if (border > 48)
		border = 48;

	// Set texture positions
	int ymid = middley;
	if (rows == 2)
		ymid = middley - (border * 0.5) - (tex_size_ * 0.5);
	if (side1_)
	{
		// Front side textures
		int xmid                                        = middlex - border - tex_size_;
		textures_[MapLine::Part::FrontUpper].position.x = xmid;
		textures_[MapLine::Part::FrontUpper].position.y = ymid;

		xmid += border + tex_size_;
		textures_[MapLine::Part::FrontMiddle].position.x = xmid;
		textures_[MapLine::Part::FrontMiddle].position.y = ymid;

		xmid += border + tex_size_;
		textures_[MapLine::Part::FrontLower].position.x = xmid;
		textures_[MapLine::Part::FrontLower].position.y = ymid;

		ymid += border + tex_size_;
	}
	if (side2_)
	{
		// Back side textures
		int xmid                                       = middlex - border - tex_size_;
		textures_[MapLine::Part::BackUpper].position.x = xmid;
		textures_[MapLine::Part::BackUpper].position.y = ymid;

		xmid += border + tex_size_;
		textures_[MapLine::Part::BackMiddle].position.x = xmid;
		textures_[MapLine::Part::BackMiddle].position.y = ymid;

		xmid += border + tex_size_;
		textures_[MapLine::Part::BackLower].position.x = xmid;
		textures_[MapLine::Part::BackLower].position.y = ymid;
	}

	last_width_  = width;
	last_height_ = height;
}

// -----------------------------------------------------------------------------
// Draws the overlay to [width],[height]
// -----------------------------------------------------------------------------
void LineTextureOverlay::draw(int width, int height, float fade)
{
	// Update layout if needed
	if (width != last_width_ || height != last_height_)
		updateLayout(width, height);

	// Get colours
	ColRGBA col_bg = ColourConfiguration::colour("map_overlay_background");
	col_bg.a *= fade;

	// Draw background
	glDisable(GL_TEXTURE_2D);
	OpenGL::setColour(col_bg);
	Drawing::drawFilledRect(0, 0, width, height);

	// Draw textures
	glEnable(GL_LINE_SMOOTH);
	int cur_size = tex_size_ * fade;
	if (!active_)
		cur_size = tex_size_;
	if (side1_)
	{
		drawTexture(fade, cur_size, textures_[MapLine::Part::FrontLower], "Front Lower:");
		drawTexture(fade, cur_size, textures_[MapLine::Part::FrontMiddle], "Front Middle:");
		drawTexture(fade, cur_size, textures_[MapLine::Part::FrontUpper], "Front Upper:");
	}
	if (side2_)
	{
		drawTexture(fade, cur_size, textures_[MapLine::Part::BackLower], "Back Lower:");
		drawTexture(fade, cur_size, textures_[MapLine::Part::BackMiddle], "Back Middle:");
		drawTexture(fade, cur_size, textures_[MapLine::Part::BackUpper], "Back Upper:");
	}
}

// -----------------------------------------------------------------------------
// Draws the texture box from info in [tex]
// -----------------------------------------------------------------------------
void LineTextureOverlay::drawTexture(float alpha, int size, TexInfo& tex, const string& position) const
{
	// Get colours
	ColRGBA col_fg  = ColourConfiguration::colour("map_overlay_foreground");
	ColRGBA col_sel = ColourConfiguration::colour("map_hilight");
	col_fg.a       = col_fg.a * alpha;

	// Draw background
	int halfsize = size * 0.5;
	glEnable(GL_TEXTURE_2D);
	OpenGL::setColour(255, 255, 255, 255 * alpha, 0);
	glPushMatrix();
	glTranslated(tex.position.x - halfsize, tex.position.y - halfsize, 0);
	GLTexture::bgTex().draw2dTiled(size, size);
	glPopMatrix();

	GLTexture* tex_first = nullptr;
	if (!tex.textures.empty())
	{
		// Draw first texture
		OpenGL::setColour(255, 255, 255, 255 * alpha, 0);
		tex_first = MapEditor::textureManager().texture(
			tex.textures[0], Game::configuration().featureSupported(Game::Feature::MixTexFlats));
		Drawing::drawTextureWithin(
			tex_first,
			tex.position.x - halfsize,
			tex.position.y - halfsize,
			tex.position.x + halfsize,
			tex.position.y + halfsize,
			0,
			2);

		// Draw up to 4 subsequent textures (overlaid)
		OpenGL::setColour(255, 255, 255, 127 * alpha, 0);
		for (unsigned a = 1; a < tex.textures.size() && a < 5; a++)
		{
			auto gl_tex = MapEditor::textureManager().texture(
				tex.textures[a], Game::configuration().featureSupported(Game::Feature::MixTexFlats));

			Drawing::drawTextureWithin(
				gl_tex,
				tex.position.x - halfsize,
				tex.position.y - halfsize,
				tex.position.x + halfsize,
				tex.position.y + halfsize,
				0,
				2);
		}
	}

	glDisable(GL_TEXTURE_2D);

	// Draw outline
	if (tex.hover)
	{
		OpenGL::setColour(col_sel.r, col_sel.g, col_sel.b, 255 * alpha, 0);
		glLineWidth(3.0f);
	}
	else
	{
		OpenGL::setColour(col_fg.r, col_fg.g, col_fg.b, 255 * alpha, 0);
		glLineWidth(1.5f);
	}
	Drawing::drawRect(
		tex.position.x - halfsize, tex.position.y - halfsize, tex.position.x + halfsize, tex.position.y + halfsize);

	// Draw position text
	Drawing::drawText(
		position,
		tex.position.x,
		tex.position.y - halfsize - 18,
		col_fg,
		Drawing::Font::Bold,
		Drawing::Align::Center);

	// Determine texture name text
	string str_texture;
	if (tex.textures.size() == 1)
		str_texture = fmt::sprintf("%s (%dx%d)", tex.textures[0], tex_first->getWidth(), tex_first->getHeight());
	else if (tex.textures.size() > 1)
		str_texture = fmt::sprintf("Multiple (%lu)", tex.textures.size());
	else
		str_texture = "- (None)";

	// Draw texture name
	Drawing::drawText(
		str_texture, tex.position.x, tex.position.y + halfsize + 2, col_fg, Drawing::Font::Bold, Drawing::Align::Center);
}

// -----------------------------------------------------------------------------
// Called when the mouse cursor is moved
// -----------------------------------------------------------------------------
void LineTextureOverlay::mouseMotion(int x, int y)
{
	// Check textures for hover
	int halfsize = tex_size_ * 0.5;
	if (side1_)
	{
		textures_[MapLine::Part::FrontUpper].checkHover(x, y, halfsize);
		textures_[MapLine::Part::FrontMiddle].checkHover(x, y, halfsize);
		textures_[MapLine::Part::FrontLower].checkHover(x, y, halfsize);
	}
	if (side2_)
	{
		textures_[MapLine::Part::BackUpper].checkHover(x, y, halfsize);
		textures_[MapLine::Part::BackMiddle].checkHover(x, y, halfsize);
		textures_[MapLine::Part::BackLower].checkHover(x, y, halfsize);
	}
}

// -----------------------------------------------------------------------------
// Called when the left mouse button is clicked
// -----------------------------------------------------------------------------
void LineTextureOverlay::mouseLeftClick()
{
	// Check for any hover
	if (textures_[MapLine::Part::FrontUpper].hover)
		browseTexture(textures_[MapLine::Part::FrontUpper], "Front Upper");
	else if (textures_[MapLine::Part::FrontMiddle].hover)
		browseTexture(textures_[MapLine::Part::FrontMiddle], "Front Middle");
	else if (textures_[MapLine::Part::FrontLower].hover)
		browseTexture(textures_[MapLine::Part::FrontLower], "Front Lower");
	else if (textures_[MapLine::Part::BackUpper].hover)
		browseTexture(textures_[MapLine::Part::BackUpper], "Back Upper");
	else if (textures_[MapLine::Part::BackMiddle].hover)
		browseTexture(textures_[MapLine::Part::BackMiddle], "Back Middle");
	else if (textures_[MapLine::Part::BackLower].hover)
		browseTexture(textures_[MapLine::Part::BackLower], "Back Lower");
}

// -----------------------------------------------------------------------------
// Called when a key is pressed
// -----------------------------------------------------------------------------
void LineTextureOverlay::keyDown(string_view key)
{
	// 'Select' front side
	if ((key == "F" || key == "f") && side1_)
		selected_side_ = 0;

	// 'Select' back side
	if ((key == "B" || key == "b") && side2_)
		selected_side_ = 1;

	// Browse upper texture
	if (key == "U" || key == "u")
	{
		if (selected_side_ == 0)
			browseTexture(textures_[MapLine::Part::FrontUpper], "Front Upper");
		else
			browseTexture(textures_[MapLine::Part::BackUpper], "Back Upper");
	}

	// Browse middle texture
	if (key == "M" || key == "m")
	{
		if (selected_side_ == 0)
			browseTexture(textures_[MapLine::Part::FrontMiddle], "Front Middle");
		else
			browseTexture(textures_[MapLine::Part::BackMiddle], "Back Middle");
	}

	// Browse lower texture
	if (key == "L" || key == "l")
	{
		if (selected_side_ == 0)
			browseTexture(textures_[MapLine::Part::FrontLower], "Front Lower");
		else
			browseTexture(textures_[MapLine::Part::BackLower], "Back Lower");
	}
}

// -----------------------------------------------------------------------------
// Opens the texture browser for [tex]
// -----------------------------------------------------------------------------
void LineTextureOverlay::browseTexture(TexInfo& tex, string_view position)
{
	// Get initial texture
	string texture;
	if (!tex.textures.empty())
		texture = tex.textures[0];
	else
		texture = "-";

	// Open texture browser
	MapTextureBrowser browser(MapEditor::windowWx(), 0, texture, &(MapEditor::editContext().map()));
	browser.SetTitle(fmt::sprintf("Browse %s Texture", position));
	if (browser.ShowModal() == wxID_OK)
	{
		// Set texture
		tex.textures.clear();
		tex.textures.push_back(browser.getSelectedItem()->name());
		tex.changed = true;
		close(false);
	}
}
