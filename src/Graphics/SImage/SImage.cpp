
// -----------------------------------------------------------------------------
// SLADE - It's a Doom Editor
// Copyright(C) 2008 - 2017 Simon Judd
//
// Email:       sirjuddington@gmail.com
// Web:         https://slade.mancubus.net
// Filename:    SImage.cpp
// Description: SImage class - Encapsulates a paletted or 32bit image. Handles
//              loading/saving different formats, palette conversions, offsets,
//              and a bunch of other stuff
//
//              This file contains all the generic operations.
//              Format - dependent functions are in SImageFormats.cpp.
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
#include "SImage.h"
#include "Graphics/Translation.h"
#include "SIFormat.h"
#include "Utility/MathStuff.h"

#undef BOOL


// -----------------------------------------------------------------------------
//
// External Variables
//
// -----------------------------------------------------------------------------
EXTERN_CVAR(Float, col_greyscale_r)
EXTERN_CVAR(Float, col_greyscale_g)
EXTERN_CVAR(Float, col_greyscale_b)


// -----------------------------------------------------------------------------
//
// SImage Class Functions
//
// -----------------------------------------------------------------------------


// -----------------------------------------------------------------------------
// Loads the image as RGBA data into <mc>.
// Returns false if image is invalid, true otherwise
// -----------------------------------------------------------------------------
bool SImage::dataRGBA(MemChunk& mc, Palette* pal)
{
	// Check the image is valid
	if (!isValid())
		return false;

	// Init rgba data
	mc.reSize(num_pixels_ * 4, false);

	// If data is already in RGBA format just return a copy
	if (type_ == Type::RGBA)
	{
		mc.importMem(data_, num_pixels_ * 4);
		return true;
	}

	// Convert paletted
	else if (type_ == Type::PalMask)
	{
		// Get palette to use
		if (has_palette_ || !pal)
			pal = &palette_;

		uint8_t rgba[4];
		for (unsigned a = 0; a < num_pixels_; a++)
		{
			// Get colour
			ColRGBA col = pal->colour(data_[a]);

			// Set alpha
			if (mask_)
				col.a = mask_[a];
			else
				col.a = 255;

			col.write(rgba);   // Write colour to array
			mc.write(rgba, 4); // Write array to MemChunk
		}

		return true;
	}

	// Convert if alpha map
	else if (type_ == Type::AlphaMap)
	{
		uint8_t rgba[4];
		ColRGBA  col;
		for (unsigned a = 0; a < num_pixels_; a++)
		{
			// Get pixel as colour (greyscale)
			col.set(data_[a], data_[a], data_[a], data_[a]);

			col.write(rgba);   // Write colour to array
			mc.write(rgba, 4); // Write array to MemChunk
		}
	}

	return false; // Invalid image type
}

// -----------------------------------------------------------------------------
// Loads the image as RGB data into <mc>.
// Returns false if image is invalid, true otherwise
// -----------------------------------------------------------------------------
bool SImage::dataRGB(MemChunk& mc, Palette* pal)
{
	// Check the image is valid
	if (!isValid())
		return false;

	// Init rgb data
	mc.reSize(num_pixels_ * 3, false);

	if (type_ == Type::RGBA)
	{
		// RGBA format, remove alpha information
		for (unsigned a = 0; a < num_pixels_ * 4; a += 4)
			mc.write(&data_[a], 3);

		return true;
	}
	else if (type_ == Type::PalMask)
	{
		// Paletted, convert to RGB

		// Get palette to use
		if (has_palette_ || !pal)
			pal = &palette_;

		// Build RGB data
		uint8_t rgba[4];
		for (unsigned a = 0; a < num_pixels_; a++)
		{
			pal->colour(data_[a]).write(rgba);
			mc.write(rgba, 3);
		}

		return true;
	}
	else if (type_ == Type::AlphaMap)
	{
		// Alpha map, convert to RGB

		uint8_t rgba[4];
		ColRGBA  col;
		for (unsigned a = 0; a < num_pixels_; a++)
		{
			// Get pixel as colour (greyscale)
			col.set(data_[a], data_[a], data_[a], data_[a]);

			col.write(rgba);   // Write colour to array
			mc.write(rgba, 3); // Write array to MemChunk
		}
	}

	return false; // Invalid image type
}

// -----------------------------------------------------------------------------
// Loads the image as index data into <mc>.
// Returns false if image is invalid, true otherwise
// -----------------------------------------------------------------------------
bool SImage::dataIndexed(MemChunk& mc) const
{
	// Check the image is valid
	if (!isValid())
		return false;

	// Init rgb data
	mc.reSize(num_pixels_, false);

	// Cannot do this for trucolor graphics.
	if (type_ == Type::RGBA)
		return false;

	else if (type_ == Type::PalMask)
	{
		mc.write(data_, num_pixels_);
		return true;
	}

	else if (type_ == Type::AlphaMap)
	{
		mc.write(data_, num_pixels_);
		return true;
	}

	return false; // Invalid image type
}

// -----------------------------------------------------------------------------
// Returns the number of bytes per image row
// -----------------------------------------------------------------------------
unsigned SImage::stride() const
{
	if (type_ == Type::RGBA)
		return size_.x * 4;
	else
		return size_.x;
}

// -----------------------------------------------------------------------------
// Returns the number of bytes per image pixel
// -----------------------------------------------------------------------------
uint8_t SImage::bpp() const
{
	if (type_ == Type::RGBA)
		return 4;
	else
		return 1;
}

// -----------------------------------------------------------------------------
// Returns an info struct with image information
// -----------------------------------------------------------------------------
SImage::Info SImage::info() const
{
	Info info;

	// Set values
	info.width       = size_.x;
	info.height      = size_.y;
	info.colformat   = type_;
	info.format      = format_ ? format_->id() : "";
	info.numimages   = num_images_;
	info.imgindex    = img_index_;
	info.offset_x    = offset_.x;
	info.offset_y    = offset_.y;
	info.has_palette = has_palette_;

	return info;
}

void SImage::setOffset(const point2_t& offset)
{
	offset_ = offset;
	announce("offsets_changed");
}

// -----------------------------------------------------------------------------
// Returns the colour of the pixel at [x,y] in the image, or black+invisible if
// out of range
// -----------------------------------------------------------------------------
ColRGBA SImage::pixelAt(unsigned x, unsigned y, Palette* pal)
{
	// Get pixel index
	unsigned index = y * stride() + x * bpp();

	// Check it
	if (index >= unsigned(num_pixels_ * bpp()))
		return { 0, 0, 0, 0 };

	// Get colour at pixel
	ColRGBA col;
	if (type_ == Type::RGBA)
	{
		col.r = data_[index];
		col.g = data_[index + 1];
		col.b = data_[index + 2];
		col.a = data_[index + 3];
	}
	else if (type_ == Type::PalMask)
	{
		// Get palette to use
		if (has_palette_ || !pal)
			pal = &palette_;

		col.set(pal->colour(data_[index]));
		if (mask_)
			col.a = mask_[index];
	}
	else if (type_ == Type::AlphaMap)
	{
		col.r = data_[index];
		col.g = data_[index];
		col.b = data_[index];
		col.a = data_[index];
	}

	return col;
}

// -----------------------------------------------------------------------------
// Returns the palette index of the pixel at [x,y] in the image, or 0 if the
// position is out of bounds or the image is not paletted
// -----------------------------------------------------------------------------
uint8_t SImage::pixelIndexAt(unsigned x, unsigned y) const
{
	// Get pixel index
	unsigned index = y * stride() + x * bpp();

	// Check it
	if (index >= unsigned(num_pixels_ * bpp()) || type_ == Type::RGBA)
		return 0;

	return data_[index];
}

// -----------------------------------------------------------------------------
// Changes the image X offset
// -----------------------------------------------------------------------------
void SImage::setXOffset(int offset)
{
	// Change the X offset
	offset_.x = offset;

	// Announce change
	announce("offsets_changed");
}

// -----------------------------------------------------------------------------
// Changes the image Y offset
// -----------------------------------------------------------------------------
void SImage::setYOffset(int offset)
{
	// Change the Y offset
	offset_.y = offset;

	// Announce change
	announce("offsets_changed");
}

// -----------------------------------------------------------------------------
// Deletes/clears any existing image data
// -----------------------------------------------------------------------------
void SImage::clearData(bool clear_mask)
{
	// Delete data if it exists
	if (data_)
	{
		delete[] data_;
		data_ = nullptr;
	}
	// Delete mask if it exists
	if (mask_ && clear_mask)
	{
		delete[] mask_;
		mask_ = nullptr;
	}
}

void SImage::allocData()
{
	num_pixels_ = size_.x * size_.y;

	switch (type_)
	{
	case Type::PalMask:
	{
		data_size_ = num_pixels_;
		mask_      = new uint8_t[data_size_]();
		break;
	}
	case Type::AlphaMap: data_size_ = num_pixels_; break;
	case Type::RGBA: data_size_ = num_pixels_ * 4; break;
	}

	data_ = new uint8_t[data_size_]();
}

// -----------------------------------------------------------------------------
// Creates an empty image
// -----------------------------------------------------------------------------
void SImage::create(int width, int height, Type type, Palette* pal, int index, int numimages)
{
	// Check valid width/height
	if (width < 0 || height < 0)
		return;

	// Clear current image
	clearData();

	//// Create blank image
	// if (type == Type::PalMask)
	//{
	//	data_ = new uint8_t[width * height];
	//	memset(data_, 0, width * height);
	//	mask_ = new uint8_t[width * height];
	//	memset(mask_, 0, width * height);
	//}
	// else if (type == Type::RGBA)
	//{
	//	data_ = new uint8_t[width * height * 4];
	//	memset(data_, 0, width * height * 4);
	//}
	// else if (type == Type::AlphaMap)
	//{
	//	data_ = new uint8_t[width * height];
	//	memset(data_, 0, width * height);
	//}

	// Set image properties
	size_.set(width, height);
	offset_.set(0, 0);
	this->type_       = type;
	this->num_images_ = numimages;
	this->img_index_  = index;
	if (pal)
	{
		palette_.copyPalette(pal);
		has_palette_ = true;
	}
	else
		has_palette_ = false;

	// Create blank image
	allocData();
}

// -----------------------------------------------------------------------------
// Creates an empty image, initialising with properties from [info]
// -----------------------------------------------------------------------------
void SImage::create(const Info& info, Palette* pal)
{
	// Normal creation
	create(info.width, info.height, (Type)info.colformat, pal, info.imgindex, info.numimages);

	// Set other info
	offset_.x    = info.offset_x;
	offset_.y    = info.offset_y;
	has_palette_ = info.has_palette;
}

// -----------------------------------------------------------------------------
// Deletes/clears any existing image data, and resets the image to zero-sized
// -----------------------------------------------------------------------------
void SImage::clear()
{
	// Clear image data
	clearData(true);

	// Reset variables
	size_.set(0, 0);
	offset_.set(0, 0);

	// Announce change
	announce("image_changed");
}

// -----------------------------------------------------------------------------
// 'Fills' the alpha channel or mask with the given <alpha> value
// -----------------------------------------------------------------------------
void SImage::fillAlpha(uint8_t alpha)
{
	// Check image is valid
	if (!isValid())
		return;

	if (type_ == Type::RGBA)
	{
		// RGBA format, set alpha values to given one
		for (unsigned a = 3; a < num_pixels_ * 4; a += 4)
			data_[a] = alpha;
	}
	else if (type_ == Type::PalMask)
	{
		// Paletted masked format, fill mask with alpha value
		if (!mask_)
			mask_ = new uint8_t[num_pixels_];

		memset(mask_, alpha, num_pixels_);
	}
	else if (type_ == Type::AlphaMap)
		memset(data_, alpha, num_pixels_);

	// Announce change
	announce("image_changed");
}

// -----------------------------------------------------------------------------
// Returns the first unused palette index, or -1 if the image is not paletted or
// uses all 256 colours
// -----------------------------------------------------------------------------
short SImage::findUnusedColour() const
{
	// Only for paletted images
	if (type_ != Type::PalMask)
		return -1;

	// Init used colours list
	uint8_t used[256];
	memset(used, 0, 256);

	// Go through image data and mark used colours
	for (unsigned a = 0; a < num_pixels_; a++)
		used[data_[a]] = 1;

	// Find first unused
	for (int a = 0; a < 256; a++)
	{
		if (!used[a])
			return a;
	}

	// No unused colours found
	return -1;
}

// -----------------------------------------------------------------------------
// Returns the number of unique colors in a paletted image
// -----------------------------------------------------------------------------
size_t SImage::countColours() const
{
	// If the picture is not paletted, return 0.
	if (type_ != Type::PalMask)
		return 0;

	bool* usedcolours = new bool[256];
	memset(usedcolours, 0, 256);
	size_t used = 0;

	for (unsigned a = 0; a < num_pixels_; ++a)
	{
		usedcolours[data_[a]] = true;
	}
	for (size_t b = 0; b < 256; ++b)
	{
		if (usedcolours[b])
			++used;
	}

	delete[] usedcolours;
	return used;
}

// -----------------------------------------------------------------------------
// Shifts all the used colours to the beginning of the palette
// -----------------------------------------------------------------------------
void SImage::shrinkPalette(Palette* pal)
{
	// If the picture is not paletted, stop.
	if (type_ != Type::PalMask)
		return;

	// Get palette to use
	if (has_palette_ || !pal)
		pal = &palette_;

	// Init variables
	Palette newpal;
	bool*   usedcolours = new bool[256];
	int*    remap       = new int[256];
	memset(usedcolours, 0, 256);
	size_t used = 0;

	// Count all color indices actually used on the picture
	for (unsigned a = 0; a < num_pixels_; ++a)
	{
		usedcolours[data_[a]] = true;
	}

	// Create palette remapping information
	for (size_t b = 0; b < 256; ++b)
	{
		if (usedcolours[b])
		{
			newpal.setColour(used, pal->colour(b));
			remap[b] = used;
			++used;
		}
	}

	// Remap image to new palette indices
	for (unsigned c = 0; c < num_pixels_; ++c)
	{
		data_[c] = remap[data_[c]];
	}
	pal->copyPalette(&newpal);

	// Cleanup
	delete[] usedcolours;
	delete[] remap;
}

// -----------------------------------------------------------------------------
// Copies all data and properties from [image]
// -----------------------------------------------------------------------------
bool SImage::copyImage(SImage* image)
{
	// Check image was given
	if (!image)
		return false;

	// Clear current data
	clearData();

	// Copy image properties
	size_ = image->size_;
	type_ = image->type_;
	palette_.copyPalette(&image->palette_);
	has_palette_ = image->has_palette_;
	offset_      = image->offset_;
	img_index_   = image->img_index_;
	num_images_  = image->num_images_;
	num_pixels_  = image->num_pixels_;
	data_size_   = image->data_size_;

	// Copy image data
	if (image->data_)
	{
		data_ = new uint8_t[num_pixels_ * bpp()];
		memcpy(data_, image->data_, num_pixels_ * bpp());
	}
	if (image->mask_)
	{
		mask_ = new uint8_t[num_pixels_];
		memcpy(mask_, image->mask_, num_pixels_);
	}

	// Announce change
	announce("image_changed");

	return true;
}

// -----------------------------------------------------------------------------
// Detects the format of [data] and, if it's a valid image format, loads it into
// this image
// -----------------------------------------------------------------------------
bool SImage::open(MemChunk& data, int index, string_view type_hint)
{
	// Check with type hint format first
	if (!type_hint.empty())
	{
		SIFormat* format = SIFormat::getFormat(type_hint);
		if (format != SIFormat::unknownFormat() && format->isThisFormat(data))
			return format->loadImage(*this, data, index);
	}

	// No type hint given or didn't match, autodetect format with SIFormat system instead
	return SIFormat::determineFormat(data)->loadImage(*this, data, index);
}

// -----------------------------------------------------------------------------
// Converts the image to 32bpp (RGBA).
// Returns false if the image was already 32bpp, true otherwise.
// -----------------------------------------------------------------------------
bool SImage::convertRGBA(Palette* pal)
{
	// If it's already 32bpp do nothing
	if (type_ == Type::RGBA)
		return false;

	// Get 32bit data
	MemChunk rgba_data;
	dataRGBA(rgba_data, pal);

	// Clear current data
	clearData(true);

	// Copy it
	data_ = new uint8_t[num_pixels_ * 4];
	memcpy(data_, rgba_data.data(), num_pixels_ * 4);

	// Set new type & update variables
	type_        = Type::RGBA;
	has_palette_ = false;
	data_size_   = num_pixels_ * 4;

	// Announce change
	announce("image_changed");

	// Done
	return true;
}

// -----------------------------------------------------------------------------
// Converts the image to paletted + mask.
// [pal_target] is the new palette to convert to (the image's palette will also
// be set to this).
// [pal_current] will be used as the image's current palette if it doesn't
// already have one
// -----------------------------------------------------------------------------
bool SImage::convertPaletted(Palette* pal_target, Palette* pal_current)
{
	// Check image/parameters are valid
	if (!isValid() || !pal_target)
		return false;

	// Get image data as RGBA
	MemChunk rgba_data;
	dataRGBA(rgba_data, pal_current);

	// Create mask from alpha info (if converting from RGBA)
	if (type_ == Type::RGBA || type_ == Type::AlphaMap)
	{
		// Clear current mask
		delete[] mask_;

		// Init mask
		mask_ = new uint8_t[num_pixels_];

		// Get values from alpha channel
		int c = 0;
		for (unsigned a = 3; a < num_pixels_ * 4; a += 4)
			mask_[c++] = rgba_data[a];
	}

	// Load given palette
	palette_.copyPalette(pal_target);

	// Clear current image data (but not mask)
	clearData(false);

	// Do conversion
	data_      = new uint8_t[num_pixels_];
	unsigned i = 0;
	ColRGBA   col;
	for (unsigned a = 0; a < num_pixels_; a++)
	{
		col.r    = rgba_data[i++];
		col.g    = rgba_data[i++];
		col.b    = rgba_data[i++];
		data_[a] = palette_.nearestColour(col);
		i++; // Skip alpha
	}

	// Update variables
	type_        = Type::PalMask;
	has_palette_ = true;
	data_size_   = num_pixels_;

	// Announce change
	announce("image_changed");

	// Success
	return true;
}

// -----------------------------------------------------------------------------
// Converts the image to an alpha map, generating alpha values from either pixel
// brightness or existing alpha, depending on the value of [alpha_source]
// -----------------------------------------------------------------------------
bool SImage::convertAlphaMap(AlphaSource alpha_source, Palette* pal)
{
	// Get RGBA data
	MemChunk rgba;
	dataRGBA(rgba, pal);

	// Recreate image
	create(size_.x, size_.y, Type::AlphaMap);

	// Generate alpha mask
	unsigned c = 0;
	for (unsigned a = 0; a < num_pixels_; a++)
	{
		// Determine alpha for this pixel
		uint8_t alpha;
		if (alpha_source == AlphaSource::Brightness) // Pixel brightness
			alpha = double(rgba[c]) * 0.3 + double(rgba[c + 1]) * 0.59 + double(rgba[c + 2]) * 0.11;
		else // Existing alpha
			alpha = rgba[c + 3];

		// Set pixel
		data_[a] = alpha;

		// Next RGBA pixel
		c += 4;
	}

	// Announce change
	announce("image_changed");

	return true;
}

// -----------------------------------------------------------------------------
// Changes the mask/alpha channel so that pixels that match [colour] are fully
// transparent, and all other pixels fully opaque
// -----------------------------------------------------------------------------
bool SImage::maskFromColour(ColRGBA colour, Palette* pal)
{
	if (type_ == Type::PalMask)
	{
		// Get palette to use
		if (has_palette_ || !pal)
			pal = &palette_;

		// Palette+Mask type, go through the mask
		for (unsigned a = 0; a < num_pixels_; a++)
		{
			if (pal->colour(data_[a]).equals(colour))
				mask_[a] = 0;
			else
				mask_[a] = 255;
		}
	}
	else if (type_ == Type::RGBA)
	{
		// RGBA type, go through alpha channel
		uint32_t c = 0;
		for (unsigned a = 0; a < num_pixels_; a++)
		{
			ColRGBA pix_col(data_[c], data_[c + 1], data_[c + 2], 255);

			if (pix_col.equals(colour))
				data_[c + 3] = 0;
			else
				data_[c + 3] = 255;

			// Skip to next pixel
			c += 4;
		}
	}
	else
		return false;

	// Announce change
	announce("image_changed");

	return true;
}

// -----------------------------------------------------------------------------
// Changes the mask/alpha channel so that each pixel's transparency matches its
// brigntness level (where black is fully transparent)
// -----------------------------------------------------------------------------
bool SImage::maskFromBrightness(Palette* pal)
{
	if (type_ == Type::PalMask)
	{
		// Get palette to use
		if (has_palette_ || !pal)
			pal = &palette_;

		// Go through pixel data
		for (unsigned a = 0; a < num_pixels_; a++)
		{
			// Set mask from pixel colour brightness value
			ColRGBA col = pal->colour(data_[a]);
			mask_[a]   = ((double)col.r * 0.3) + ((double)col.g * 0.59) + ((double)col.b * 0.11);
		}
	}
	else if (type_ == Type::RGBA)
	{
		// Go through pixel data
		unsigned c = 0;
		for (unsigned a = 0; a < num_pixels_; a++)
		{
			// Set alpha from pixel colour brightness value
			data_[c + 3] = (double)data_[c] * 0.3 + (double)data_[c + 1] * 0.59 + (double)data_[c + 2] * 0.11;
			// Skip alpha
			c += 4;
		}
	}
	// ALPHAMASK type is already a brightness mask

	// Announce change
	announce("image_changed");

	return true;
}

// -----------------------------------------------------------------------------
// Changes the mask/alpha channel so that any pixel alpha level currently
// greater than [threshold] is fully opaque, and all other pixels fully
// transparent
// -----------------------------------------------------------------------------
bool SImage::cutoffMask(uint8_t threshold)
{
	if (type_ == Type::PalMask)
	{
		// Paletted, go through mask
		for (unsigned a = 0; a < num_pixels_; a++)
		{
			if (mask_[a] > threshold)
				mask_[a] = 255;
			else
				mask_[a] = 0;
		}
	}
	else if (type_ == Type::RGBA)
	{
		// RGBA format, go through alpha channel
		for (unsigned a = 3; a < num_pixels_ * 4; a += 4)
		{
			if (data_[a] > threshold)
				data_[a] = 255;
			else
				data_[a] = 0;
		}
	}
	else if (type_ == Type::AlphaMap)
	{
		// Alpha map, go through pixels
		for (unsigned a = 0; a < num_pixels_; a++)
		{
			if (data_[a] > threshold)
				data_[a] = 255;
			else
				data_[a] = 0;
		}
	}
	else
		return false;

	// Announce change
	announce("image_changed");

	return true;
}

// -----------------------------------------------------------------------------
// Sets the pixel at [x],[y] to [colour].
// Returns false if the position is out of range, true otherwise
// -----------------------------------------------------------------------------
bool SImage::setPixel(int x, int y, ColRGBA colour, Palette* pal)
{
	// Check position
	if (x < 0 || x >= size_.x || y < 0 || y >= size_.y)
		return false;

	// Set the pixel
	if (type_ == Type::RGBA)
		colour.write(data_ + (y * (size_.x * 4) + (x * 4)));
	else if (type_ == Type::PalMask)
	{
		// Get palette to use
		if (has_palette_ || !pal)
			pal = &palette_;

		// Get color index to use (the ColRGBA's index if defined, nearest colour otherwise)
		uint8_t index = (colour.index == -1) ? pal->nearestColour(colour) : colour.index;

		data_[y * size_.x + x] = index;
		if (mask_)
			mask_[y * size_.x + x] = colour.a;
	}
	else if (type_ == Type::AlphaMap)
	{
		// Just use colour alpha
		data_[y * size_.x + x] = colour.a;
	}

	// Announce
	announce("image_changed");

	return true;
}

// -----------------------------------------------------------------------------
// Sets the pixel at [x],[y] to the palette colour at [pal_index], and the
// transparency of the pixel to [alpha] (if possible).
// Returns false if the position is out of bounds, true otherwise
// -----------------------------------------------------------------------------
bool SImage::setPixel(int x, int y, uint8_t pal_index, uint8_t alpha)
{
	// Check position
	if (x < 0 || x >= size_.x || y < 0 || y >= size_.y)
		return false;

	// RGBA (use palette colour, probably don't want this, but it's here anyway :P)
	if (type_ == Type::RGBA)
	{
		// Set the pixel
		ColRGBA col = palette_.colour(pal_index);
		col.a      = alpha;
		col.write(data_ + (y * (size_.x * 4) + (x * 4)));
	}

	// Paletted
	else if (type_ == Type::PalMask)
	{
		// Set the pixel
		data_[y * size_.x + x] = pal_index;
		if (mask_)
			mask_[y * size_.x + x] = alpha;
	}

	// Alpha map
	else if (type_ == Type::AlphaMap)
	{
		// Set the pixel
		data_[y * size_.x + x] = alpha;
	}

	// Invalid type
	else
		return false;

	// Announce
	announce("image_changed");

	// Invalid type
	return true;
}

// -----------------------------------------------------------------------------
// Change the width of the image to the given value, adjusting the height
// automatically
// -----------------------------------------------------------------------------
void SImage::setWidth(int w)
{
	if (((int)num_pixels_ > w) && ((num_pixels_ % w) == 0))
	{
		size_.x = w;
		size_.y = num_pixels_ / w;
	}
}

// -----------------------------------------------------------------------------
// Change the height of the image to the given value, adjusting the height
// automatically
// -----------------------------------------------------------------------------
void SImage::setHeight(int h)
{
	if (((int)num_pixels_ > h) && ((num_pixels_ % h) == 0))
	{
		size_.y = h;
		size_.x = num_pixels_ / h;
	}
}

// -----------------------------------------------------------------------------
// Rotates the image with an angle of 90°, 180° or 270°.
// Why not use FreeImage_Rotate instead? So as not to bother converting to and
// fro a FIBITMAP...
// -----------------------------------------------------------------------------
bool SImage::rotate(int angle)
{
	if (!data_)
		return false;

	if (angle == 0)
		return true; // Nothing to do
	if (angle % 90)
		return false; // Unsupported angle
	while (angle < 0)
		angle += 360;
	angle %= 360;
	angle = 360 - angle;

	uint8_t* nm;
	int      nw, nh;

	// Compute new dimensions and numbers of pixels and bytes
	if (angle % 180)
	{
		nw = size_.y;
		nh = size_.x;
	}
	else
	{
		nw = size_.x;
		nh = size_.y;
	}
	int numbpp;
	if (type_ == Type::PalMask)
		numbpp = 1;
	else if (type_ == Type::RGBA)
		numbpp = 4;
	else
		return false;

	// Create new data and mask
	uint8_t* nd = new uint8_t[num_pixels_ * numbpp];
	if (mask_)
		nm = new uint8_t[num_pixels_ * numbpp];
	else
		nm = nullptr;

	// Remapping loop
	unsigned i, j, k;
	for (i = 0; i < num_pixels_; ++i)
	{
		switch (angle)
		{
			// Urgh maths...
		case 90: j = (((nh - 1) - (i % size_.x)) * nw) + (i / size_.x); break;
		case 180: j = (num_pixels_ - 1) - i; break;
		case 270: j = ((i % size_.x) * nw) + ((nw - 1) - (i / size_.x)); break;
		default:
			delete[] nd;
			if (mask_)
				delete[] nm;
			return false;
		}
		if (j >= num_pixels_)
		{
			LOG_MESSAGE(1, "Pixel %i remapped to %i, how did this even happen?", i, j);
			delete[] nd;
			if (mask_)
				delete[] nm;
			return false;
		}
		for (k = 0; k < numbpp; ++k)
		{
			nd[(j * numbpp) + k] = data_[(i * numbpp) + k];
			if (mask_)
				nm[(j * numbpp) + k] = mask_[(i * numbpp) + k];
		}
	}

	// It worked, yay
	clearData();
	data_   = nd;
	mask_   = nm;
	size_.x = nw;
	size_.y = nh;

	// Announce change
	announce("image_changed");
	return true;
}

// -----------------------------------------------------------------------------
// Mirrors the image horizontally or vertically.
// -----------------------------------------------------------------------------
bool SImage::mirror(bool vertical)
{
	uint8_t* nm;

	// Compute numbers of pixels and bytes
	int numbpp = 0;
	if (type_ == Type::PalMask)
		numbpp = 1;
	else if (type_ == Type::RGBA)
		numbpp = 4;
	else
		return false;

	// Create new data and mask
	uint8_t* nd = new uint8_t[num_pixels_ * numbpp];
	if (mask_)
		nm = new uint8_t[num_pixels_ * numbpp];
	else
		nm = nullptr;

	// Remapping loop
	unsigned i, j, k;
	for (i = 0; i < num_pixels_; ++i)
	{
		if (vertical)
			j = (((size_.y - 1) - (i / size_.x)) * size_.x) + (i % size_.x);
		else // horizontal
			j = ((i / size_.x) * size_.x) + ((size_.x - 1) - (i % size_.x));
		if (j >= num_pixels_)
		{
			LOG_MESSAGE(1, "Pixel %i remapped to %i, how did this even happen?", i, j);
			delete[] nd;
			if (mask_)
				delete[] nm;
			return false;
		}
		for (k = 0; k < numbpp; ++k)
		{
			nd[(j * numbpp) + k] = data_[(i * numbpp) + k];
			if (mask_)
				nm[(j * numbpp) + k] = mask_[(i * numbpp) + k];
		}
	}

	// It worked, yay
	clearData();
	data_ = nd;
	mask_ = nm;

	// Announce change
	announce("image_changed");
	return true;
}

// -----------------------------------------------------------------------------
// Converts from column-major to row-major
// -----------------------------------------------------------------------------
bool SImage::imgconv()
{
	int oldwidth = size_.x;
	size_.x      = size_.y;
	size_.y      = oldwidth;
	rotate(90);
	mirror(true);
	return true;
}

// -----------------------------------------------------------------------------
// Crops a section of the image.
// -----------------------------------------------------------------------------
bool SImage::crop(long x1, long y1, long x2, long y2)
{
	if (x2 == 0 || x2 > size_.x)
		x2 = size_.x;
	if (y2 == 0 || y2 > size_.y)
		y2 = size_.y;

	// No need to bother with incorrect values
	if (x2 <= x1 || y2 <= y1 || x1 > size_.x || y1 > size_.y)
		return false;

	uint8_t* nm;
	size_t   nw = x2 - x1;
	size_t   nh = y2 - y1;

	// Compute numbers of pixels and bytes
	int numpixels = nw * nh;
	int numbpp    = 0;
	if (type_ == Type::PalMask || type_ == Type::AlphaMap)
		numbpp = 1;
	else if (type_ == Type::RGBA)
		numbpp = 4;
	else
		return false;

	// Create new data and mask
	uint8_t* nd = new uint8_t[numpixels * numbpp];
	if (mask_)
		nm = new uint8_t[numpixels * numbpp];
	else
		nm = nullptr;

	// Remapping loop
	size_t i, a, b;
	for (i = 0; i < nh; ++i)
	{
		a = i * nw * numbpp;
		b = (((i + y1) * size_.x) + x1) * numbpp;
		memcpy(nd + a, data_ + b, nw * numbpp);
		if (mask_)
			memcpy(nm + a, mask_ + b, nw * numbpp);
	}

	// It worked, yay
	clearData();
	data_       = nd;
	mask_       = nm;
	size_.x     = nw;
	size_.y     = nh;
	num_pixels_ = numpixels;
	data_size_  = numpixels * numbpp;

	// Announce change
	announce("image_changed");
	return true;
}

// -----------------------------------------------------------------------------
// Resizes the image, conserving current data (will be cropped if new size is
// smaller)
// -----------------------------------------------------------------------------
bool SImage::resize(int nwidth, int nheight)
{
	// Check values
	if (nwidth < 0 || nheight < 0)
		return false;

	// If either dimension is zero, just clear the image
	if (nwidth == 0 || nheight == 0)
	{
		clear();
		return true;
	}

	// Init new image data
	uint8_t* newmask;
	uint8_t  bpp = 1;
	if (type_ == Type::RGBA)
		bpp = 4;
	// Create new image data
	uint8_t* newdata = new uint8_t[nwidth * nheight * bpp];
	memset(newdata, 0, nwidth * nheight * bpp);
	// Create new mask if needed
	if (type_ == Type::PalMask)
	{
		newmask = new uint8_t[nwidth * nheight];
		memset(newmask, 0, nwidth * nheight);
	}
	else
		newmask = nullptr;

	// Write new image data
	unsigned offset = 0;
	unsigned rowlen = MIN(size_.x, nwidth) * bpp;
	unsigned nrows  = MIN(size_.y, nheight);
	for (unsigned y = 0; y < nrows; y++)
	{
		// Copy data row
		memcpy(newdata + offset, data_ + (y * size_.x * bpp), rowlen);

		// Copy mask row
		if (newmask)
			memcpy(newmask + offset, mask_ + (y * size_.x), rowlen);

		// Go to next row
		offset += nwidth * bpp;
	}

	// Update variables
	size_.x = nwidth;
	size_.y = nheight;
	clearData();
	data_       = newdata;
	mask_       = newmask;
	num_pixels_ = nwidth * nheight;
	data_size_  = num_pixels_ * bpp;

	// Announce change
	announce("image_changed");

	return true;
}

// -----------------------------------------------------------------------------
// Sets the image data, size, and type from raw data
// -----------------------------------------------------------------------------
bool SImage::setImageData(uint8_t* ndata, int nwidth, int nheight, Type ntype)
{
	if (ndata)
	{
		clearData();
		type_       = ntype;
		size_.x     = nwidth;
		size_.y     = nheight;
		data_       = ndata;
		num_pixels_ = nwidth * nheight;
		data_size_  = ntype == Type::RGBA ? num_pixels_ * 4 : num_pixels_;

		// Announce change
		announce("image_changed");

		return true;
	}
	return false;
}

// -----------------------------------------------------------------------------
// Applies a palette translation to the image
// -----------------------------------------------------------------------------
bool SImage::applyTranslation(Translation* tr, Palette* pal, bool truecolor)
{
	// Check image is ok
	if (!data_)
		return false;

	// Can't apply a translation to a non-coloured image
	if (type_ == Type::AlphaMap)
		return false;

	// Handle truecolor images
	if (type_ == Type::RGBA)
		truecolor = true;
	size_t bpp = this->bpp();

	// Get palette to use
	if (has_palette_ || !pal)
		pal = &palette_;

	uint8_t* newdata;
	if (truecolor && type_ == Type::PalMask)
	{
		newdata = new uint8_t[num_pixels_ * 4];
		memset(newdata, 0, num_pixels_ * 4);
	}
	else
		newdata = data_;

	// Go through pixels
	for (unsigned p = 0; p < num_pixels_; p++)
	{
		// No need to process transparent pixels
		if (mask_ && mask_[p] == 0)
			continue;

		ColRGBA col;
		int    q = p * bpp;
		if (type_ == Type::PalMask)
			col.set(pal->colour(data_[p]));
		else if (type_ == Type::RGBA)
		{
			col.set(data_[q], data_[q + 1], data_[q + 2], data_[q + 3]);

			// skip colours that don't match exactly to the palette
			col.index = pal->nearestColour(col);
			if (!col.equals(pal->colour(col.index)))
				continue;
		}

		col = tr->translate(col, pal);

		if (truecolor)
		{
			q              = p * 4;
			newdata[q + 0] = col.r;
			newdata[q + 1] = col.g;
			newdata[q + 2] = col.b;
			newdata[q + 3] = mask_ ? mask_[p] : col.a;
		}
		else
			data_[p] = col.index;
	}

	if (truecolor && type_ == Type::PalMask)
	{
		clearData(true);
		data_ = newdata;
		type_ = Type::RGBA;
	}

	return true;
}

// -----------------------------------------------------------------------------
// Applies a palette translation to the image
// -----------------------------------------------------------------------------
bool SImage::applyTranslation(string_view tr, Palette* pal, bool truecolor)
{
	Translation trans;
	trans.clear();
	trans.parse(tr);
	return applyTranslation(&trans, pal, truecolor);
}

// -----------------------------------------------------------------------------
// Draws a pixel of [colour] at [x],[y], blending it according to the options
// set in [properties]. If the image is paletted, the resulting pixel colour is
// converted to its nearest match in [pal]
// -----------------------------------------------------------------------------
bool SImage::drawPixel(int x, int y, ColRGBA colour, DrawProps& properties, Palette* pal)
{
	// Check valid coords
	if (x < 0 || y < 0 || x >= size_.x || y >= size_.y)
		return false;

	// Get palette to use
	if (has_palette_ || !pal)
		pal = &palette_;

	// Setup alpha
	if (properties.src_alpha)
		colour.a *= properties.alpha;
	else
		colour.a = 255 * properties.alpha;

	// Do nothing if completely transparent
	if (colour.a == 0)
		return true;

	// Get pixel index
	unsigned p = y * stride() + x * bpp();

	// Check for simple case (normal blending, no transparency involved)
	if (colour.a == 255 && properties.blend == Blend::Normal)
	{
		if (type_ == Type::RGBA)
			colour.write(data_ + p);
		else
		{
			data_[p] = pal->nearestColour(colour);
			mask_[p] = colour.a;
		}

		return true;
	}

	// Not-so-simple case, do full processing
	ColRGBA d_colour;
	if (type_ == Type::PalMask)
		d_colour = pal->colour(data_[p]);
	else
		d_colour.set(data_[p], data_[p + 1], data_[p + 2], data_[p + 3]);
	float alpha = (float)colour.a / 255.0f;

	// Additive blending
	if (properties.blend == Blend::Add)
	{
		d_colour.set(
			MathStuff::clamp(d_colour.r + colour.r * alpha, 0, 255),
			MathStuff::clamp(d_colour.g + colour.g * alpha, 0, 255),
			MathStuff::clamp(d_colour.b + colour.b * alpha, 0, 255),
			MathStuff::clamp(d_colour.a + colour.a, 0, 255));
	}

	// Subtractive blending
	else if (properties.blend == Blend::Subtract)
	{
		d_colour.set(
			MathStuff::clamp(d_colour.r - colour.r * alpha, 0, 255),
			MathStuff::clamp(d_colour.g - colour.g * alpha, 0, 255),
			MathStuff::clamp(d_colour.b - colour.b * alpha, 0, 255),
			MathStuff::clamp(d_colour.a + colour.a, 0, 255));
	}

	// Reverse-Subtractive blending
	else if (properties.blend == Blend::ReverseSubtract)
	{
		d_colour.set(
			MathStuff::clamp((-d_colour.r) + colour.r * alpha, 0, 255),
			MathStuff::clamp((-d_colour.g) + colour.g * alpha, 0, 255),
			MathStuff::clamp((-d_colour.b) + colour.b * alpha, 0, 255),
			MathStuff::clamp(d_colour.a + colour.a, 0, 255));
	}

	// 'Modulate' blending
	else if (properties.blend == Blend::Modulate)
	{
		d_colour.set(
			MathStuff::clamp(colour.r * d_colour.r / 255., 0, 255),
			MathStuff::clamp(colour.g * d_colour.g / 255., 0, 255),
			MathStuff::clamp(colour.b * d_colour.b / 255., 0, 255),
			MathStuff::clamp(d_colour.a + colour.a, 0, 255));
	}

	// Normal blending (or unknown blend type)
	else
	{
		float inv_alpha = 1.0f - alpha;
		d_colour.set(
			d_colour.r * inv_alpha + colour.r * alpha,
			d_colour.g * inv_alpha + colour.g * alpha,
			d_colour.b * inv_alpha + colour.b * alpha,
			MathStuff::clamp(d_colour.a + colour.a, 0, 255));
	}

	// Apply new colour
	if (type_ == Type::PalMask)
	{
		data_[p] = pal->nearestColour(d_colour);
		mask_[p] = d_colour.a;
	}
	else if (type_ == Type::RGBA)
		d_colour.write(data_ + p);
	else if (type_ == Type::AlphaMap)
		data_[p] = d_colour.a;

	return true;
}

// -----------------------------------------------------------------------------
// Draws an image on to this image at [x],[y], with blending options set in
// [properties]. [pal_src] is used for the source image, and [pal_dest] is used
// for the destination image, if either is paletted
// -----------------------------------------------------------------------------
bool SImage::drawImage(SImage& img, int x_pos, int y_pos, DrawProps& properties, Palette* pal_src, Palette* pal_dest)
{
	// Check images
	if (!data_ || !img.data_)
		return false;

	// Setup palettes
	if (img.has_palette_ || !pal_src)
		pal_src = &(img.palette_);
	if (has_palette_ || !pal_dest)
		pal_dest = &palette_;

	// Go through pixels
	unsigned s_stride = img.stride();
	uint8_t  s_bpp    = img.bpp();
	unsigned sp       = 0;
	for (int y = y_pos; y < y_pos + img.size_.y; y++) // Rows
	{
		// Skip out-of-bounds rows
		if (y < 0 || y >= size_.y)
		{
			sp += s_stride;
			continue;
		}

		for (int x = x_pos; x < x_pos + img.size_.x; x++) // Columns
		{
			// Skip out-of-bounds columns
			if (x < 0 || x >= size_.x)
			{
				sp += s_bpp;
				continue;
			}

			// Skip if source pixel is fully transparent
			if ((img.type_ == Type::PalMask && img.mask_[sp] == 0)
				|| (img.type_ == Type::AlphaMap && img.data_[sp] == 0)
				|| (img.type_ == Type::RGBA && img.data_[sp + 3] == 0))
			{
				sp += s_bpp;
				continue;
			}

			// Draw pixel
			if (img.type_ == Type::PalMask)
			{
				ColRGBA col = pal_src->colour(img.data_[sp]);
				col.a      = img.mask_[sp];
				drawPixel(x, y, col, properties, pal_dest);
			}
			else if (img.type_ == Type::RGBA)
				drawPixel(
					x,
					y,
					ColRGBA(img.data_[sp], img.data_[sp + 1], img.data_[sp + 2], img.data_[sp + 3]),
					properties,
					pal_dest);
			else if (img.type_ == Type::AlphaMap)
				drawPixel(
					x, y, ColRGBA(img.data_[sp], img.data_[sp], img.data_[sp], img.data_[sp]), properties, pal_dest);

			// Go to next source pixel
			sp += s_bpp;
		}
	}

	return true;
}

// -----------------------------------------------------------------------------
// Colourises the image to [colour]. If the image is paletted, each pixel will
// be set to its nearest matching colour in [pal]
// -----------------------------------------------------------------------------
bool SImage::colourise(ColRGBA colour, Palette* pal, int start, int stop)
{
	// Can't do this with alpha maps
	if (type_ == Type::AlphaMap)
		return false;

	// Get palette to use
	if (has_palette_ || !pal)
		pal = &palette_;

	// Go through all pixels
	uint8_t bpp = this->bpp();
	ColRGBA  col;
	for (unsigned a = 0; a < num_pixels_ * bpp; a += bpp)
	{
		// Skip colors out of range if desired
		if (type_ == Type::PalMask && start >= 0 && stop >= start && stop < 256)
		{
			if (data_[a] < start || data_[a] > stop)
				continue;
		}

		// Get current pixel colour
		if (type_ == Type::RGBA)
			col.set(data_[a], data_[a + 1], data_[a + 2], data_[a + 3]);
		else
			col.set(pal->colour(data_[a]));

		// Colourise it
		float grey = (col.r * col_greyscale_r + col.g * col_greyscale_g + col.b * col_greyscale_b) / 255.0f;
		if (grey > 1.0)
			grey = 1.0;
		col.r = colour.r * grey;
		col.g = colour.g * grey;
		col.b = colour.b * grey;

		// Set pixel colour
		if (type_ == Type::RGBA)
			col.write(data_ + a);
		else
			data_[a] = pal->nearestColour(col);
	}

	return true;
}

// -----------------------------------------------------------------------------
// Tints the image to [colour] by [amount]. If the image is paletted, each pixel
// will be set to its nearest matching colour in [pal]
// -----------------------------------------------------------------------------
bool SImage::tint(ColRGBA colour, float amount, Palette* pal, int start, int stop)
{
	// Can't do this with alpha maps
	if (type_ == Type::AlphaMap)
		return false;

	// Get palette to use
	if (has_palette_ || !pal)
		pal = &palette_;

	// Go through all pixels
	uint8_t bpp = this->bpp();
	ColRGBA  col;
	for (unsigned a = 0; a < num_pixels_ * bpp; a += bpp)
	{
		// Skip colors out of range if desired
		if (type_ == Type::PalMask && start >= 0 && stop >= start && stop < 256)
		{
			if (data_[a] < start || data_[a] > stop)
				continue;
		}

		// Get current pixel colour
		if (type_ == Type::RGBA)
			col.set(data_[a], data_[a + 1], data_[a + 2], data_[a + 3]);
		else
			col.set(pal->colour(data_[a]));

		// Tint it
		float inv_amt = 1.0f - amount;
		col.set(
			col.r * inv_amt + colour.r * amount,
			col.g * inv_amt + colour.g * amount,
			col.b * inv_amt + colour.b * amount,
			col.a);

		// Set pixel colour
		if (type_ == Type::RGBA)
			col.write(data_ + a);
		else
			data_[a] = pal->nearestColour(col);
	}

	return true;
}

// -----------------------------------------------------------------------------
// Automatically crop the image to remove fully transparent rows and columns
// from the sides.
// Returns true if successfully cropped.
// -----------------------------------------------------------------------------
bool SImage::adjust()
{
	int x1 = 0, x2 = size_.x, y1 = 0, y2 = size_.y;

	// Loop for empty columns on the left
	bool opaquefound = false;
	while (x1 < x2)
	{
		for (int i = 0; i < y2; ++i)
		{
			size_t p = i * size_.x + x1; // Pixel position
			switch (type_)
			{
			case Type::PalMask: // Transparency is mask[p] == 0
				if (mask_[p])
					opaquefound = true;
				break;
			case Type::RGBA: // Transparency is data[p*4 + 3] == 0
				if (data_[p * 4 + 3])
					opaquefound = true;
				break;
			case Type::AlphaMap: // Transparency is data[p] == 0
				if (data_[p])
					opaquefound = true;
				break;
			}
			if (opaquefound)
				break;
		}
		if (opaquefound)
			break;
		++x1;
	}

	if (x1 == x2) // Empty image, all columns are empty, crop it to a single pixel
		return crop(0, 0, 1, 1);

	// Now loop for empty columns on the right
	opaquefound = false;
	while (x2 > x1)
	{
		for (int i = 0; i < y2; ++i)
		{
			size_t p = i * size_.x + x2 - 1;
			switch (type_)
			{
			case Type::PalMask:
				if (mask_[p])
					opaquefound = true;
				break;
			case Type::RGBA:
				if (data_[p * 4 + 3])
					opaquefound = true;
				break;
			case Type::AlphaMap:
				if (data_[p])
					opaquefound = true;
				break;
			}
			if (opaquefound)
				break;
		}
		if (opaquefound)
			break;
		--x2;
	}

	// Now loop for empty rows from the top
	opaquefound = false;
	while (y1 < y2)
	{
		for (int i = x1; i < x2; ++i)
		{
			size_t p = y1 * size_.x + i;
			switch (type_)
			{
			case Type::PalMask:
				if (mask_[p])
					opaquefound = true;
				break;
			case Type::RGBA:
				if (data_[p * 4 + 3])
					opaquefound = true;
				break;
			case Type::AlphaMap:
				if (data_[p])
					opaquefound = true;
				break;
			}
			if (opaquefound)
				break;
		}
		if (opaquefound)
			break;
		++y1;
	}

	// Finally loop for empty rows from the bottom

	opaquefound = false;
	while (y2 > y1)
	{
		for (int i = x1; i < x2; ++i)
		{
			size_t p = (y2 - 1) * size_.x + i;
			switch (type_)
			{
			case Type::PalMask: opaquefound = mask_[p] > 0; break;
			case Type::RGBA: opaquefound = data_[p * 4 + 3] > 0; break;
			case Type::AlphaMap: opaquefound = data_[p] > 0; break;
			}
			if (opaquefound)
				break;
		}
		if (opaquefound)
			break;
		--y2;
	}

	// Now we've found the coordinates, so we can crop
	if (x1 == 0 && y1 == 0 && x2 == size_.x && y2 == size_.y)
		return false; // No adjustment needed
	return crop(x1, y1, x2, y2);
}

bool SImage::mirrorpad()
{
	// Only pad images that actually have offsets
	if (offset_.x == 0 && offset_.y == 0)
		return false;

	// Only pad images that need it, so for instance if width is 10, and ofsx is 5,
	// then the image is already mirrored. If width is 11, accept ofsx 5 or 6 as good.
	if (offset_.x == size_.x / 2 || (size_.x % 2 == 1 && offset_.x == size_.x / 2 + 1))
		return false;

	// Now we need to pad. Padding to the right can be done just by resizing the image,
	// padding to the left requires flipping it, resizing it, and flipping it back.

	bool needflip = offset_.x < size_.x / 2;
	int  extra    = abs((offset_.x * 2) - size_.x);

	bool success = true;
	if (needflip)
		success = mirror(false);
	if (success)
		success = resize(size_.x + extra, size_.y);
	else
		return false;
	if (needflip && success)
	{
		success = mirror(false);
		offset_.x += extra;
	}
	return success;
}
