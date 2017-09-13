
// ----------------------------------------------------------------------------
// SLADE - It's a Doom Editor
// Copyright(C) 2008 - 2017 Simon Judd
//
// Email:       sirjuddington@gmail.com
// Web:         http://slade.mancubus.net
// Filename:    SImage.cpp
// Description: SImage class - Encapsulates a paletted or 32bit image. Handles
//              loading/saving different formats, palette conversions, offsets,
//              and a bunch of other stuff
//              This file contains all the generic operations. Format-dependent
//              functions are in SImageFormats.cpp.
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
// ----------------------------------------------------------------------------


// ----------------------------------------------------------------------------
//
// Includes
//
// ----------------------------------------------------------------------------
#include "Main.h"
#include "SImage.h"
#include "SIFormat.h"
#include "Graphics/Translation.h"
#include "Utility/MathStuff.h"

#undef BOOL


// ----------------------------------------------------------------------------
//
// Variables
//
// ----------------------------------------------------------------------------
EXTERN_CVAR(Float, col_greyscale_r)
EXTERN_CVAR(Float, col_greyscale_g)
EXTERN_CVAR(Float, col_greyscale_b)


// ----------------------------------------------------------------------------
//
// SImage Class Functions
//
// ----------------------------------------------------------------------------


// ----------------------------------------------------------------------------
// SImage::SImage
//
// SImage class constructor
// ----------------------------------------------------------------------------
SImage::SImage(PixelFormat type) :
	data_(nullptr),
	mask_(nullptr),
	pixel_format_(type),
	has_palette_(false),
	format_(nullptr),
	img_index_(0),
	num_images_(1)
{
}

// ----------------------------------------------------------------------------
// SImage::~SImage
//
// SImage class destructor
// ----------------------------------------------------------------------------
SImage::~SImage()
{
	clearData();
}

// ----------------------------------------------------------------------------
// SImage::getRGBAData
//
// Writes the image as RGBA data into [mc].
// Returns false if image is invalid, true otherwise
// ----------------------------------------------------------------------------
bool SImage::getRGBAData(MemChunk& mc, Palette* pal)
{
	// Check the image is valid
	if (!isValid())
		return false;

	// Init rgba data
	mc.reSize(size_.x * size_.y * 4, false);

	// If data is already in RGBA format just return a copy
	if (pixel_format_ == PixelFormat::RGBA)
	{
		mc.importMem(data_, size_.x * size_.y * 4);
		return true;
	}

	// Convert paletted
	else if (pixel_format_ == PixelFormat::PalMask)
	{
		// Get palette to use
		if (has_palette_ || !pal)
			pal = &palette_;

		uint8_t rgba[4];
		for (int a = 0; a < size_.x * size_.y; a++)
		{
			// Get colour
			rgba_t col = pal->colour(data_[a]);

			// Set alpha
			if (mask_)
				col.a = mask_[a];
			else
				col.a = 255;

			col.write(rgba);	// Write colour to array
			mc.write(rgba, 4);	// Write array to MemChunk
		}

		return true;
	}

	// Convert if alpha map
	else if (pixel_format_ == PixelFormat::AlphaMap)
	{
		uint8_t rgba[4];
		rgba_t col;
		for (int a = 0; a < size_.x * size_.y; a++)
		{
			// Get pixel as colour (greyscale)
			col.set(data_[a], data_[a], data_[a], data_[a]);

			col.write(rgba);	// Write colour to array
			mc.write(rgba, 4);	// Write array to MemChunk
		}
	}

	return false;	// Invalid image type
}

// ----------------------------------------------------------------------------
// SImage::getRGBData
//
// Writes the image as RGB data into [mc].
// Returns false if image is invalid, true otherwise
// ----------------------------------------------------------------------------
bool SImage::getRGBData(MemChunk& mc, Palette* pal)
{
	// Check the image is valid
	if (!isValid())
		return false;

	// Init rgb data
	mc.reSize(size_.x * size_.y * 3, false);

	if (pixel_format_ == PixelFormat::RGBA)
	{
		// RGBA format, remove alpha information
		for (int a = 0; a < size_.x * size_.y * 4; a += 4)
			mc.write(&data_[a], 3);

		return true;
	}
	else if (pixel_format_ == PixelFormat::PalMask)
	{
		// Paletted, convert to RGB

		// Get palette to use
		if (has_palette_ || !pal)
			pal = &palette_;

		// Build RGB data
		uint8_t rgba[4];
		for (int a = 0; a < size_.x * size_.y; a ++)
		{
			pal->colour(data_[a]).write(rgba);
			mc.write(rgba, 3);
		}

		return true;
	}
	else if (pixel_format_ == PixelFormat::AlphaMap)
	{
		// Alpha map, convert to RGB

		uint8_t rgba[4];
		rgba_t col;
		for (int a = 0; a < size_.x * size_.y; a++)
		{
			// Get pixel as colour (greyscale)
			col.set(data_[a], data_[a], data_[a], data_[a]);

			col.write(rgba);	// Write colour to array
			mc.write(rgba, 3);	// Write array to MemChunk
		}
	}

	return false;	// Invalid image type
}

// ----------------------------------------------------------------------------
// SImage::getIndexedData
//
// Writes the image as index data into [mc].
// Returns false if image is invalid, true otherwise
// ----------------------------------------------------------------------------
bool SImage::getIndexedData(MemChunk& mc)
{
	// Check the image is valid
	if (!isValid())
		return false;

	// Init rgb data
	mc.reSize(size_.x * size_.y, false);

	// Cannot do this for trucolor graphics.
	if (pixel_format_ == PixelFormat::RGBA)
		return false;

	else if (pixel_format_ == PixelFormat::PalMask)
	{
		mc.write(data_, size_.x * size_.y);
		return true;
	}

	else if (pixel_format_ == PixelFormat::AlphaMap)
	{
		mc.write(data_, size_.x * size_.y);
		return true;
	}

	return false;	// Invalid image type
}

// ----------------------------------------------------------------------------
// SImage::stride
//
// Returns the number of bytes per image row
// ----------------------------------------------------------------------------
unsigned SImage::stride()
{
	if (pixel_format_ == PixelFormat::RGBA)
		return size_.x*4;
	else
		return size_.x;
}

// ----------------------------------------------------------------------------
// SImage::bytesPerPixel
//
// Returns the number of bytes per image pixel
// ----------------------------------------------------------------------------
uint8_t SImage::bytesPerPixel()
{
	if (pixel_format_ == PixelFormat::RGBA)
		return 4;
	else
		return 1;
}

// ----------------------------------------------------------------------------
// SImage::info
//
// Returns an SImage::Info struct with image information
// ----------------------------------------------------------------------------
SImage::Info SImage::info()
{
	Info info;

	// Set values
	info.width = size_.x;
	info.height = size_.y;
	info.colformat = pixel_format_;
	if (format_) info.format = format_->getId();
	info.numimages = num_images_;
	info.imgindex = img_index_;
	info.offset = offset_;
	info.has_palette = has_palette_;

	return info;
}

// ----------------------------------------------------------------------------
// SImage::colourAt
//
// Returns the colour of the pixel at [x,y] in the image, or black+invisible
// if out of range
// ----------------------------------------------------------------------------
rgba_t SImage::colourAt(unsigned x, unsigned y, Palette* pal)
{
	// Get pixel index
	unsigned index = y * stride() + x * bytesPerPixel();

	// Check it
	if (index >= unsigned(size_.x*size_.y*bytesPerPixel()))
		return rgba_t(0, 0, 0, 0);

	// Get colour at pixel
	rgba_t col;
	if (pixel_format_ == PixelFormat::RGBA)
	{
		col.r = data_[index];
		col.g = data_[index+1];
		col.b = data_[index+2];
		col.a = data_[index+3];
	}
	else if (pixel_format_ == PixelFormat::PalMask)
	{
		// Get palette to use
		if (has_palette_ || !pal)
			pal = &palette_;

		col.set(pal->colour(data_[index]));
		if (mask_) col.a = mask_[index];
	}
	else if (pixel_format_ == PixelFormat::AlphaMap)
	{
		col.r = data_[index];
		col.g = data_[index];
		col.b = data_[index];
		col.a = data_[index];
	}

	return col;
}

// ----------------------------------------------------------------------------
// SImage::paletteIndexAt
//
// Returns the palette index of the pixel at [x,y] in the image, or 0 if the
// position is out of bounds or the image is not paletted
// ----------------------------------------------------------------------------
uint8_t SImage::paletteIndexAt(unsigned x, unsigned y)
{
	// Get pixel index
	unsigned index = y * stride() + x * bytesPerPixel();

	// Check it
	if (index >= unsigned(size_.x*size_.y*bytesPerPixel()) || pixel_format_ == PixelFormat::RGBA)
		return 0;

	return data_[index];
}

// ----------------------------------------------------------------------------
// SImage::setOffset
//
// Changes the image offset
// ----------------------------------------------------------------------------
void SImage::setOffset(point2_t offset)
{
	// Change the offset
	offset_ = offset;

	// Announce change
	announce("offsets_changed");
}

// ----------------------------------------------------------------------------
// SImage::setXOffset
//
// Changes the image X offset
// ----------------------------------------------------------------------------
void SImage::setXOffset(int offset)
{
	// Change the X offset
	offset_.x = offset;

	// Announce change
	announce("offsets_changed");
}

// ----------------------------------------------------------------------------
// SImage::setYOffset
//
// Changes the image Y offset
// ----------------------------------------------------------------------------
void SImage::setYOffset(int offset)
{
	// Change the Y offset
	offset_.y = offset;

	// Announce change
	announce("offsets_changed");
}

// ----------------------------------------------------------------------------
// SImage::clearData
//
// Deletes/clears any existing image data
// ----------------------------------------------------------------------------
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

// ----------------------------------------------------------------------------
// SImage::create
//
// Creates an empty image
// ----------------------------------------------------------------------------
void SImage::create(int width, int height, PixelFormat type, Palette* pal, int index, int numimages)
{
	// Check valid width/height
	if (width < 0 || height < 0)
		return;

	// Clear current image
	clearData();

	// Create blank image
	if (type == PixelFormat::PalMask)
	{
		data_ = new uint8_t[width*height];
		memset(data_, 0, width*height);
		mask_ = new uint8_t[width*height];
		memset(mask_, 0, width*height);
	}
	else if (type == PixelFormat::RGBA)
	{
		data_ = new uint8_t[width*height*4];
		memset(data_, 0, width*height*4);
	}
	else if (type == PixelFormat::AlphaMap)
	{
		data_ = new uint8_t[width*height];
		memset(data_, 0, width*height);
	}

	// Set image properties
	this->size_.x = width;
	this->size_.y = height;
	this->pixel_format_ = type;
	this->offset_ = { 0, 0 };
	this->num_images_ = numimages;
	this->img_index_ = index;
	if (pal)
	{
		palette_.copyPalette(pal);
		has_palette_ = true;
	}
	else
		has_palette_ = false;
}

// ----------------------------------------------------------------------------
// SImage::create
//
// Creates an empty image, initialising with properties from [info]
// ----------------------------------------------------------------------------
void SImage::create(SImage::Info info, Palette* pal)
{
	// Normal creation
	create(info.width, info.height, (PixelFormat)info.colformat, pal, info.imgindex, info.numimages);

	// Set other info
	offset_ = info.offset;
	has_palette_ = info.has_palette;
}

// ----------------------------------------------------------------------------
// SImage::clear
//
// Deletes/clears any existing image data, and resets the image to zero-sized
// ----------------------------------------------------------------------------
void SImage::clear()
{
	// Clear image data
	clearData(true);

	// Reset variables
	size_.x = 0;
	size_.y = 0;
	offset_ = { 0, 0 };

	// Announce change
	announce("image_changed");
}

// ----------------------------------------------------------------------------
// SImage::fillAlpha
//
// 'Fills' the alpha channel or mask with the given [alpha] value
// ----------------------------------------------------------------------------
void SImage::fillAlpha(uint8_t alpha)
{
	// Check image is valid
	if (!isValid())
		return;

	if (pixel_format_ == PixelFormat::RGBA)
	{
		// RGBA format, set alpha values to given one
		for (int a = 3; a < size_.x * size_.y * 4; a += 4)
			data_[a] = alpha;
	}
	else if (pixel_format_ == PixelFormat::PalMask)
	{
		// Paletted masked format, fill mask with alpha value
		if (!mask_)
			mask_ = new uint8_t[size_.x * size_.y];

		memset(mask_, alpha, size_.x * size_.y);
	}
	else if (pixel_format_ == PixelFormat::AlphaMap)
		memset(data_, alpha, size_.x * size_.y);

	// Announce change
	announce("image_changed");
}

// ----------------------------------------------------------------------------
// SImage::findUnusedColour
//
// Returns the first unused palette index, or -1 if the image is not paletted
// or uses all 256 colours
// ----------------------------------------------------------------------------
short SImage::findUnusedColour()
{
	// Only for paletted images
	if (pixel_format_ != PixelFormat::PalMask)
		return -1;

	// Init used colours list
	uint8_t used[256];
	memset(used, 0, 256);

	// Go through image data and mark used colours
	for (int a = 0; a < size_.x * size_.y; a++)
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

// ----------------------------------------------------------------------------
// SImage::countColours
//
// Returns the number of unique colors in a paletted image
// ----------------------------------------------------------------------------
size_t SImage::countColours()
{
	// If the picture is not paletted, return 0.
	if (pixel_format_ != PixelFormat::PalMask)
		return 0;

	bool* usedcolours = new bool[256];
	memset(usedcolours, 0, 256);
	size_t used = 0;

	for (int a = 0; a < size_.x*size_.y; ++a)
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

// ----------------------------------------------------------------------------
// SImage::shrinkPalette
//
// Shifts all the used colours to the beginning of the palette
// ----------------------------------------------------------------------------
void SImage::shrinkPalette(Palette* pal)
{
	// If the picture is not paletted, stop.
	if (pixel_format_ != PixelFormat::PalMask)
		return;

	// Get palette to use
	if (has_palette_ || !pal)
		pal = &palette_;

	// Init variables
	Palette newpal;
	bool* usedcolours = new bool[256];
	int* remap = new int[256];
	memset(usedcolours, 0, 256);
	size_t used = 0;

	// Count all color indices actually used on the picture
	for (int a = 0; a < size_.x*size_.y; ++a)
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
	for (int c = 0; c < size_.x*size_.y; ++c)
	{
		data_[c] = remap[data_[c]];
	}
	pal->copyPalette(&newpal);

	// Cleanup
	delete[] usedcolours;
	delete[] remap;
}

// ----------------------------------------------------------------------------
// SImage::copyImage
//
// Copies all data and properties from [image]
// ----------------------------------------------------------------------------
bool SImage::copyImage(SImage* image)
{
	// Check image was given
	if (!image)
		return false;

	// Clear current data
	clearData();

	// Copy image properties
	size_.x = image->size_.x;
	size_.y = image->size_.y;
	pixel_format_ = image->pixel_format_;
	palette_.copyPalette(&image->palette_);
	has_palette_ = image->has_palette_;
	offset_ = image->offset_;
	img_index_ = image->img_index_;
	num_images_ = image->num_images_;

	// Copy image data
	if (image->data_)
	{
		data_ = new uint8_t[size_.x*size_.y*bytesPerPixel()];
		memcpy(data_, image->data_, size_.x*size_.y*bytesPerPixel());
	}
	if (image->mask_)
	{
		mask_ = new uint8_t[size_.x*size_.y];
		memcpy(mask_, image->mask_, size_.x*size_.y);
	}

	// Announce change
	announce("image_changed");

	return true;
}

// ----------------------------------------------------------------------------
// SImage::open
//
// Detects the format of [data] and, if it's a valid image format, loads it
// into this image
// ----------------------------------------------------------------------------
bool SImage::open(MemChunk& data, int index, string type_hint)
{
	// Check with type hint format first
	if (!type_hint.IsEmpty())
	{
		SIFormat* format = SIFormat::getFormat(type_hint);
		if (format != SIFormat::unknownFormat() && format->isThisFormat(data))
			return format->loadImage(*this, data, index);
	}

	// No type hint given or didn't match, autodetect format with SIFormat system instead
	return SIFormat::determineFormat(data)->loadImage(*this, data, index);
}

// ----------------------------------------------------------------------------
// SImage::convertRGBA
//
// Converts the image to 32bpp (RGBA).
// Returns false if the image was already 32bpp, true otherwise.
// ----------------------------------------------------------------------------
bool SImage::convertRGBA(Palette* pal)
{
	// If it's already 32bpp do nothing
	if (pixel_format_ == PixelFormat::RGBA)
		return false;

	// Get 32bit data
	MemChunk rgba_data;
	getRGBAData(rgba_data, pal);

	// Clear current data
	clearData(true);

	// Copy it
	data_ = new uint8_t[size_.x * size_.y * 4];
	memcpy(data_, rgba_data.getData(), size_.x * size_.y * 4);

	// Set new type & update variables
	pixel_format_ = PixelFormat::RGBA;
	has_palette_ = false;

	// Announce change
	announce("image_changed");

	// Done
	return true;
}

// ----------------------------------------------------------------------------
// SImage::convertPaletted
//
// Converts the image to paletted + mask. [pal_target] is the new palette to
// convert to (the image's palette will also be set to this). [pal_current]
// will be used as the image's current palette if it doesn't already have one
// ----------------------------------------------------------------------------
bool SImage::convertPaletted(Palette* pal_target, Palette* pal_current)
{
	// Check image/parameters are valid
	if (!isValid() || !pal_target)
		return false;

	// Get image data as RGBA
	MemChunk rgba_data;
	getRGBAData(rgba_data, pal_current);

	// Create mask from alpha info (if converting from RGBA)
	if (pixel_format_ == PixelFormat::RGBA || pixel_format_ == PixelFormat::AlphaMap)
	{
		// Clear current mask
		if (mask_)
			delete[] mask_;

		// Init mask
		mask_ = new uint8_t[size_.x * size_.y];

		// Get values from alpha channel
		int c = 0;
		for (int a = 3; a < size_.x * size_.y * 4; a += 4)
			mask_[c++] = rgba_data[a];
	}

	// Load given palette
	palette_.copyPalette(pal_target);

	// Clear current image data (but not mask)
	clearData(false);

	// Do conversion
	data_ = new uint8_t[size_.x * size_.y];
	unsigned i = 0;
	rgba_t col;
	for (int a = 0; a < size_.x*size_.y; a++)
	{
		col.r = rgba_data[i++];
		col.g = rgba_data[i++];
		col.b = rgba_data[i++];
		data_[a] = palette_.nearestColour(col);
		i++;	// Skip alpha
	}

	// Update variables
	pixel_format_ = PixelFormat::PalMask;
	has_palette_ = true;

	// Announce change
	announce("image_changed");

	// Success
	return true;
}

// ----------------------------------------------------------------------------
// SImage::convertAlphaMap
//
// Converts the image to an alpha map, generating alpha values from either
// pixel brightness or existing alpha, depending on the value of [alpha_source]
// ----------------------------------------------------------------------------
bool SImage::convertAlphaMap(AlphaSource alpha_source, Palette* pal)
{
	// Get RGBA data
	MemChunk rgba;
	getRGBAData(rgba, pal);

	// Recreate image
	create(size_.x, size_.y, PixelFormat::AlphaMap);

	// Generate alpha mask
	unsigned c = 0;
	for (int a = 0; a < size_.x * size_.y; a++)
	{
		// Determine alpha for this pixel
		uint8_t alpha = 0;
		if (alpha_source == AlphaSource::Brightness)	// Pixel brightness
			alpha = double(rgba[c])*0.3 + double(rgba[c+1])*0.59 + double(rgba[c+2])*0.11;
		else							// Existing alpha
			alpha = rgba[c+3];

		// Set pixel
		data_[a] = alpha;

		// Next RGBA pixel
		c += 4;
	}

	// Announce change
	announce("image_changed");

	return true;
}

// ----------------------------------------------------------------------------
// SImage::maskFromColour
//
// Changes the mask/alpha channel so that pixels that match [colour] are fully
// transparent, and all other pixels fully opaque
// ----------------------------------------------------------------------------
bool SImage::maskFromColour(rgba_t colour, Palette* pal)
{
	if (pixel_format_ == PixelFormat::PalMask)
	{
		// Get palette to use
		if (has_palette_ || !pal)
			pal = &palette_;

		// Palette+Mask type, go through the mask
		for (int a = 0; a < size_.x * size_.y; a++)
		{
			if (pal->colour(data_[a]).equals(colour))
				mask_[a] = 0;
			else
				mask_[a] = 255;
		}
	}
	else if (pixel_format_ == PixelFormat::RGBA)
	{
		// RGBA type, go through alpha channel
		uint32_t c = 0;
		for (int a = 0; a < size_.x * size_.y; a++)
		{
			rgba_t pix_col(data_[c], data_[c + 1], data_[c + 2], 255);

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

// ----------------------------------------------------------------------------
// SImage::maskFromBrightness
//
// Changes the mask/alpha channel so that each pixel's transparency matches its
// brigntness level (where black is fully transparent)
// ----------------------------------------------------------------------------
bool SImage::maskFromBrightness(Palette* pal)
{
	if (pixel_format_ == PixelFormat::PalMask)
	{
		// Get palette to use
		if (has_palette_ || !pal)
			pal = &palette_;

		// Go through pixel data
		for (int a = 0; a < size_.x * size_.y; a++)
		{
			// Set mask from pixel colour brightness value
			rgba_t col = pal->colour(data_[a]);
			mask_[a] = ((double)col.r*0.3)+((double)col.g*0.59)+((double)col.b*0.11);
		}
	}
	else if (pixel_format_ == PixelFormat::RGBA)
	{
		// Go through pixel data
		unsigned c = 0;
		for (int a = 0; a < size_.x * size_.y; a++)
		{
			// Set alpha from pixel colour brightness value
			data_[c+3] = (double)data_[c]*0.3 + (double)data_[c+1]*0.59 + (double)data_[c+2]*0.11;
			// Skip alpha
			c += 4;
		}
	}
	// ALPHAMASK type is already a brightness mask

	// Announce change
	announce("image_changed");

	return true;
}

// ----------------------------------------------------------------------------
// SImage::cutoffMask
//
// Changes the mask/alpha channel so that any pixel alpha level currently
// greater than [threshold] is fully opaque, and all other pixels fully
// transparent
// ----------------------------------------------------------------------------
bool SImage::cutoffMask(uint8_t threshold)
{
	if (pixel_format_ == PixelFormat::PalMask)
	{
		// Paletted, go through mask
		for (int a = 0; a < size_.x * size_.y; a++)
		{
			if (mask_[a] > threshold)
				mask_[a] = 255;
			else
				mask_[a] = 0;
		}
	}
	else if (pixel_format_ == PixelFormat::RGBA)
	{
		// RGBA format, go through alpha channel
		uint32_t c = 0;
		for (int a = 3; a < size_.x * size_.y * 4; a += 4)
		{
			if (data_[a] > threshold)
				data_[a] = 255;
			else
				data_[a] = 0;
		}
	}
	else if (pixel_format_ == PixelFormat::AlphaMap)
	{
		// Alpha map, go through pixels
		for (int a = 0; a < size_.x * size_.y; a++)
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

// ----------------------------------------------------------------------------
// SImage::setPixel
//
// Sets the pixel at [x],[y] to [colour].
// Returns false if the position is out of range, true otherwise
// ----------------------------------------------------------------------------
bool SImage::setPixel(int x, int y, rgba_t colour, Palette* pal)
{
	// Check position
	if (x < 0 || x >= size_.x || y < 0 || y >= size_.y)
		return false;

	// Set the pixel
	if (pixel_format_ == PixelFormat::RGBA)
		colour.write(data_ + (y * (size_.x*4) + (x*4)));
	else if (pixel_format_ == PixelFormat::PalMask)
	{
		// Get palette to use
		if (has_palette_ || !pal)
			pal = &palette_;

		// Get color index to use (the rgba_t's index if defined, nearest colour otherwise)
		uint8_t index = (colour.index == -1) ? pal->nearestColour(colour) : colour.index;

		data_[y * size_.x + x] = index;
		if (mask_) mask_[y * size_.x + x] = colour.a;
	}
	else if (pixel_format_ == PixelFormat::AlphaMap)
	{
		// Just use colour alpha
		data_[y * size_.x + x] = colour.a;
	}

	// Announce
	announce("image_changed");

	return true;
}

// ----------------------------------------------------------------------------
// SImage::setPixel
//
// Sets the pixel at [x],[y] to the palette colour at [pal_index], and the
// transparency of the pixel to [alpha] (if possible).
// Returns false if the position is out of bounds, true otherwise
// ----------------------------------------------------------------------------
bool SImage::setPixel(int x, int y, uint8_t pal_index, uint8_t alpha)
{
	// Check position
	if (x < 0 || x >= size_.x || y < 0 || y >= size_.y)
		return false;

	// RGBA (use palette colour, probably don't want this, but it's here anyway :P)
	if (pixel_format_ == PixelFormat::RGBA)
	{
		// Set the pixel
		rgba_t col = palette_.colour(pal_index);
		col.a = alpha;
		col.write(data_ + (y * (size_.x*4) + (x*4)));
	}

	// Paletted
	else if (pixel_format_ == PixelFormat::PalMask)
	{
		// Set the pixel
		data_[y*size_.x+x] = pal_index;
		if (mask_) mask_[y*size_.x+x] = alpha;
	}

	// Alpha map
	else if (pixel_format_ == PixelFormat::AlphaMap)
	{
		// Set the pixel
		data_[y*size_.x+x] = alpha;
	}

	// Invalid type
	else
		return false;

	// Announce
	announce("image_changed");

	// Invalid type
	return true;
}

// ----------------------------------------------------------------------------
// SImage::setWidth
//
// Change the width of the image to the given value, adjusting the height
// automatically
// ----------------------------------------------------------------------------
void SImage::setWidth(int w)
{
	int numpixels = size_.x * size_.y;
	if ((numpixels > w) && ((numpixels % w) == 0))
	{
		size_.x = w;
		size_.y = numpixels/w;
	}
}

// ----------------------------------------------------------------------------
// SImage::setHeight
//
// Change the height of the image to the given value, adjusting the height
// automatically
// ----------------------------------------------------------------------------
void SImage::setHeight(int h)
{
	int numpixels = size_.x * size_.y;
	if ((numpixels > h) && ((numpixels % h) == 0))
	{
		size_.y = h;
		size_.x = numpixels/h;
	}
}

// ----------------------------------------------------------------------------
// SImage::rotate
//
// Rotates the image with an angle of 90°, 180° or 270°.
// Why not use FreeImage_Rotate instead? So as not to bother converting to and
// fro a FIBITMAP...
// ----------------------------------------------------------------------------
bool SImage::rotate(int angle)
{
	if (!data_)
		return false;

	if (angle == 0)	return true;	// Nothing to do
	if (angle % 90) return false;	// Unsupported angle
	while (angle < 0) angle += 360;
	angle %= 360;
	angle = 360-angle;

	uint8_t* nd, * nm;
	int nw, nh;

	// Compute new dimensions and numbers of pixels and bytes
	if (angle % 180) { nw = size_.y; nh = size_.x; }
	else {	nw = size_.x; nh = size_.y; }
	int numpixels = size_.x*size_.y; int numbpp = 0;
	if (pixel_format_==PixelFormat::PalMask)	numbpp = 1;
	else if (pixel_format_==PixelFormat::RGBA)	numbpp = 4;
	else return false;

	// Create new data and mask
	nd = new uint8_t[numpixels*numbpp];
	if (mask_) nm = new uint8_t[numpixels*numbpp];
	else nm = nullptr;

	// Remapping loop
	int i, j, k;
	for (i = 0; i < numpixels; ++i)
	{
		switch (angle)
		{
			// Urgh maths...
		case 90:	j = (((nh - 1) - (i%size_.x)) * nw) + (i/size_.x);	break;
		case 180:	j = (numpixels - 1) - i;						break;
		case 270:	j = ((i%size_.x) * nw) + ((nw - 1) - (i/size_.x));	break;
		default: delete[] nd; if (mask_) delete[] nm; return false;
		}
		if (j >= numpixels)
		{
			LOG_MESSAGE(1, "Pixel %i remapped to %i, how did this even happen?", i, j);
			delete [] nd; if (mask_) delete[] nm;
			return false;
		}
		for (k = 0; k < numbpp; ++k)
		{
			nd[(j*numbpp)+k] = data_[(i*numbpp)+k];
			if (mask_) nm[(j*numbpp)+k] = mask_[(i*numbpp)+k];
		}
	}

	// It worked, yay
	clearData();
	data_ = nd; mask_ = nm; size_.x = nw; size_.y = nh;

	// Announce change
	announce("image_changed");
	return true;
}

// ----------------------------------------------------------------------------
// SImage::mirror
//
// Mirrors the image horizontally or vertically, depending on [vertical]
// ----------------------------------------------------------------------------
bool SImage::mirror(bool vertical)
{
	uint8_t* nd, * nm;

	// Compute numbers of pixels and bytes
	int numpixels = size_.x*size_.y; int numbpp = 0;
	if (pixel_format_==PixelFormat::PalMask)	numbpp = 1;
	else if (pixel_format_==PixelFormat::RGBA)	numbpp = 4;
	else return false;

	// Create new data and mask
	nd = new uint8_t[numpixels*numbpp];
	if (mask_) nm = new uint8_t[numpixels*numbpp];
	else nm = nullptr;

	// Remapping loop
	int i, j, k;
	for (i = 0; i < numpixels; ++i)
	{
		if (vertical)
			j = (((size_.y - 1) - (i/size_.x)) * size_.x) + (i%size_.x);
		else // horizontal
			j = ((i/size_.x) * size_.x) + ((size_.x - 1) - (i%size_.x));
		if (j >= numpixels)
		{
			LOG_MESSAGE(1, "Pixel %i remapped to %i, how did this even happen?", i, j);
			delete [] nd; if (mask_) delete[] nm;
			return false;
		}
		for (k = 0; k < numbpp; ++k)
		{
			nd[(j*numbpp)+k] = data_[(i*numbpp)+k];
			if (mask_) nm[(j*numbpp)+k] = mask_[(i*numbpp)+k];
		}
	}

	// It worked, yay
	clearData();
	data_ = nd; mask_ = nm;

	// Announce change
	announce("image_changed");
	return true;
}

// ----------------------------------------------------------------------------
// SImage::imgconv
//
// Converts from column-major to row-major
// ----------------------------------------------------------------------------
bool SImage::imgconv()
{
	int oldwidth = size_.x;
	size_.x = size_.y;
	size_.y = oldwidth;
	rotate(90);
	mirror(true);
	return true;
}

// ----------------------------------------------------------------------------
// SImage::crop
//
// Crops a section of the image
// ----------------------------------------------------------------------------
bool SImage::crop(long x1, long y1, long x2, long y2)
{
	if (x2 == 0 || x2 > size_.x) x2 = size_.x;
	if (y2 == 0 || y2 > size_.y) y2 = size_.y;

	// No need to bother with incorrect values
	if (x2 <= x1 || y2 <= y1 || x1 > size_.x || y1 > size_.y)
		return false;

	uint8_t* nd, * nm;
	size_t nw, nh;
	nw = x2 - x1;
	nh = y2 - y1;

	// Compute numbers of pixels and bytes
	int numpixels = nw*nh; int numbpp = 0;
	if (pixel_format_==PixelFormat::PalMask || pixel_format_==PixelFormat::AlphaMap)
		numbpp = 1;
	else if (pixel_format_==PixelFormat::RGBA)
		numbpp = 4;
	else
		return false;

	// Create new data and mask
	nd = new uint8_t[numpixels*numbpp];
	if (mask_) nm = new uint8_t[numpixels*numbpp];
	else nm = nullptr;

	// Remapping loop
	size_t i, a, b;
	for (i = 0; i < nh; ++i)
	{
		a = i*nw*numbpp;
		b = (((i+y1)*size_.x)+x1)*numbpp;
		memcpy(nd+a, data_+b, nw*numbpp);
		if (mask_) memcpy(nm+a, mask_+b, nw*numbpp);
	}

	// It worked, yay
	clearData();
	data_ = nd; mask_ = nm;
	size_.x = nw; size_.y = nh;

	// Announce change
	announce("image_changed");
	return true;
}

// ----------------------------------------------------------------------------
// SImage::resize
//
// Resizes the image to [nwidth]x[nheight], conserving current data
// (will be cropped if new size is smaller)
// ----------------------------------------------------------------------------
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
	uint8_t* newdata, *newmask;
	uint8_t bpp = 1;
	if (pixel_format_ == PixelFormat::RGBA) bpp = 4;
	// Create new image data
	newdata = new uint8_t[nwidth * nheight * bpp];
	memset(newdata, 0, nwidth*nheight*bpp);
	// Create new mask if needed
	if (pixel_format_ == PixelFormat::PalMask)
	{
		newmask = new uint8_t[nwidth * nheight];
		memset(newmask, 0, nwidth*nheight);
	}
	else
		newmask = nullptr;
	
	// Write new image data
	unsigned offset = 0;
	unsigned rowlen = std::min(size_.x, nwidth)*bpp;
	unsigned nrows = std::min(size_.y, nheight);
	for (unsigned y = 0; y < nrows; y++)
	{
		// Copy data row
		memcpy(newdata + offset, data_ + (y * size_.x * bpp), rowlen);

		// Copy mask row
		if (newmask)
			memcpy(newmask + offset, mask_ + (y * size_.x), rowlen);

		// Go to next row
		offset += nwidth*bpp;
	}

	// Update variables
	size_.x = nwidth;
	size_.y = nheight;
	clearData();
	data_ = newdata;
	mask_ = newmask;

	// Announce change
	announce("image_changed");

	return true;
}

// ----------------------------------------------------------------------------
// SImage::setImageData
//
// Sets the image data, size, and type from raw data
// ----------------------------------------------------------------------------
bool SImage::setImageData(uint8_t* ndata, int nwidth, int nheight, PixelFormat ntype)
{
	if (ndata)
	{
		clearData();
		pixel_format_ = ntype;
		size_.x = nwidth;
		size_.y = nheight;
		data_ = ndata;

		// Announce change
		announce("image_changed");

		return true;
	}
	return false;
}

// ----------------------------------------------------------------------------
// SImage::applyTranslation
//
// Applies a palette translation to the image
// ----------------------------------------------------------------------------
bool SImage::applyTranslation(Translation* tr, Palette* pal, bool truecolor)
{
	// Check image is ok
	if (!data_)
		return false;

	// Can't apply a translation to a non-coloured image
	if (pixel_format_ == PixelFormat::AlphaMap)
		return false;

	// Handle truecolor images
	if (pixel_format_ == PixelFormat::RGBA)
		truecolor = true;
	size_t bpp = bytesPerPixel();

	// Get palette to use
	if (has_palette_ || !pal)
		pal = &palette_;

	uint8_t* newdata = nullptr;
	if (truecolor && pixel_format_ == PixelFormat::PalMask)
	{
		newdata = new uint8_t[size_.x*size_.y*4];
		memset(newdata, 0, size_.x*size_.y*4);
	}
	else newdata = data_;

	// Go through pixels
	for (int p = 0; p < size_.x*size_.y; p++)
	{

		// No need to process transparent pixels
		if (mask_ && mask_[p] == 0)
			continue;

		rgba_t col;
		int q = p * bpp;
		if (pixel_format_ == PixelFormat::PalMask)
			col.set(pal->colour(data_[p]));
		else if (pixel_format_ == PixelFormat::RGBA)
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
			q = p*4;
			newdata[q+0] = col.r;
			newdata[q+1] = col.g;
			newdata[q+2] = col.b;
			newdata[q+3] = mask_ ? mask_[p] : col.a;
		}
		else data_[p] = col.index;
	}

	if (truecolor && pixel_format_ == PixelFormat::PalMask)
	{
		clearData(true);
		data_ = newdata;
		pixel_format_ = PixelFormat::RGBA;
	}

	return true;
}

// ----------------------------------------------------------------------------
// SImage::applyTranslation
//
// Applies a palette translation to the image
// ----------------------------------------------------------------------------
bool SImage::applyTranslation(string tr, Palette* pal, bool truecolor)
{
	Translation trans;
	trans.clear();
	trans.parse(tr);
	return applyTranslation(&trans, pal, truecolor);
}

// ----------------------------------------------------------------------------
// SImage::drawPixel
//
// Draws a pixel of [colour] at [x],[y], blending it according to the options
// set in [properties]. If the image is paletted, the resulting pixel colour is
// converted to its nearest match in [pal]
// ----------------------------------------------------------------------------
bool SImage::drawPixel(int x, int y, rgba_t colour, DrawProps& properties, Palette* pal)
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
		colour.a = 255*properties.alpha;

	// Do nothing if completely transparent
	if (colour.a == 0)
		return true;
	
	// Get pixel index
	unsigned p = y * stride() + x * bytesPerPixel();

	// Check for simple case (normal blending, no transparency involved)
	if (colour.a == 255 && properties.blend == BlendType::Normal)
	{
		if (pixel_format_ == PixelFormat::RGBA)
			colour.write(data_+p);
		else
		{
			data_[p] = pal->nearestColour(colour);
			mask_[p] = colour.a;
		}

		return true;
	}

	// Not-so-simple case, do full processing
	rgba_t d_colour;
	if (pixel_format_ == PixelFormat::PalMask)
		d_colour = pal->colour(data_[p]);
	else
		d_colour.set(data_[p], data_[p+1], data_[p+2], data_[p+3]);
	float alpha = (float)colour.a / 255.0f;

	// Additive blending
	if (properties.blend == BlendType::Add)
	{
		d_colour.set(	MathStuff::clamp(d_colour.r+colour.r*alpha, 0, 255),
		                MathStuff::clamp(d_colour.g+colour.g*alpha, 0, 255),
		                MathStuff::clamp(d_colour.b+colour.b*alpha, 0, 255),
		                MathStuff::clamp(d_colour.a + colour.a, 0, 255));
	}

	// Subtractive blending
	else if (properties.blend == BlendType::Subtract)
	{
		d_colour.set(	MathStuff::clamp(d_colour.r-colour.r*alpha, 0, 255),
		                MathStuff::clamp(d_colour.g-colour.g*alpha, 0, 255),
		                MathStuff::clamp(d_colour.b-colour.b*alpha, 0, 255),
		                MathStuff::clamp(d_colour.a + colour.a, 0, 255));
	}

	// Reverse-Subtractive blending
	else if (properties.blend == BlendType::ReverseSubtract)
	{
		d_colour.set(	MathStuff::clamp((-d_colour.r)+colour.r*alpha, 0, 255),
		                MathStuff::clamp((-d_colour.g)+colour.g*alpha, 0, 255),
		                MathStuff::clamp((-d_colour.b)+colour.b*alpha, 0, 255),
		                MathStuff::clamp(d_colour.a + colour.a, 0, 255));
	}

	// 'Modulate' blending
	else if (properties.blend == BlendType::Modulate)
	{
		d_colour.set(	MathStuff::clamp(colour.r*d_colour.r / 255, 0, 255),
		                MathStuff::clamp(colour.g*d_colour.g / 255, 0, 255),
		                MathStuff::clamp(colour.b*d_colour.b / 255, 0, 255),
		                MathStuff::clamp(d_colour.a + colour.a, 0, 255));
	}

	// Normal blending (or unknown blend type)
	else
	{
		float inv_alpha = 1.0f - alpha;
		d_colour.set(	d_colour.r*inv_alpha + colour.r*alpha,
		                d_colour.g*inv_alpha + colour.g*alpha,
		                d_colour.b*inv_alpha + colour.b*alpha,
		                MathStuff::clamp(d_colour.a + colour.a, 0, 255));
	}

	// Apply new colour
	if (pixel_format_ == PixelFormat::PalMask)
	{
		data_[p] = pal->nearestColour(d_colour);
		mask_[p] = d_colour.a;
	}
	else if (pixel_format_ == PixelFormat::RGBA)
		d_colour.write(data_+p);
	else if (pixel_format_ == PixelFormat::AlphaMap)
		data_[p] = d_colour.a;

	return true;
}

// ----------------------------------------------------------------------------
// SImage::drawImage
//
// Draws an image on to this image at [x],[y], with blending options set in
// [properties]. [pal_src] is used for the source image, and [pal_dest] is used
// for the destination image, if either is paletted
// ----------------------------------------------------------------------------
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
	uint8_t s_bpp = img.bytesPerPixel();
	unsigned sp = 0;
	for (int y = y_pos; y < y_pos + img.size_.y; y++)  		// Rows
	{
		// Skip out-of-bounds rows
		if (y < 0 || y >= size_.y)
		{
			sp += s_stride;
			continue;
		}

		for (int x = x_pos; x < x_pos + img.size_.x; x++)  	// Columns
		{
			// Skip out-of-bounds columns
			if (x < 0 || x >= size_.x)
			{
				sp += s_bpp;
				continue;
			}

			// Skip if source pixel is fully transparent
			if ((img.pixel_format_ == PixelFormat::PalMask && img.mask_[sp] == 0) ||
			        (img.pixel_format_ == PixelFormat::AlphaMap && img.data_[sp] == 0) ||
			        (img.pixel_format_ == PixelFormat::RGBA && img.data_[sp+3] == 0))
			{
				sp += s_bpp;
				continue;
			}

			// Draw pixel
			if (img.pixel_format_ == PixelFormat::PalMask)
			{
				rgba_t col = pal_src->colour(img.data_[sp]);
				col.a = img.mask_[sp];
				drawPixel(x, y, col, properties, pal_dest);
			}
			else if (img.pixel_format_ == PixelFormat::RGBA)
				drawPixel(x, y, rgba_t(img.data_[sp], img.data_[sp+1], img.data_[sp+2], img.data_[sp+3]), properties, pal_dest);
			else if (img.pixel_format_ == PixelFormat::AlphaMap)
				drawPixel(x, y, rgba_t(img.data_[sp], img.data_[sp], img.data_[sp], img.data_[sp]), properties, pal_dest);

			// Go to next source pixel
			sp += s_bpp;
		}
	}

	return true;
}

// ----------------------------------------------------------------------------
// SImage::colourise
//
// Colourises the image to [colour]. If the image is paletted, each pixel will
// be set to its nearest matching colour in [pal]
// ----------------------------------------------------------------------------
bool SImage::colourise(rgba_t colour, Palette* pal, int start, int stop)
{
	// Can't do this with alpha maps
	if (pixel_format_ == PixelFormat::AlphaMap)
		return false;

	// Get palette to use
	if (has_palette_ || !pal)
		pal = &palette_;

	// Go through all pixels
	uint8_t bpp = bytesPerPixel();
	rgba_t col;
	for (int a = 0; a < size_.x*size_.y*bpp; a+= bpp)
	{
		// Skip colors out of range if desired
		if (pixel_format_ == PixelFormat::PalMask && start >= 0 && stop >= start && stop < 256) 
		{
			if (data_[a] < start || data_[a] > stop)
				continue;
		}

		// Get current pixel colour
		if (pixel_format_ == PixelFormat::RGBA)
			col.set(data_[a], data_[a+1], data_[a+2], data_[a+3]);
		else
			col.set(pal->colour(data_[a]));

		// Colourise it
		float grey = (col.r*col_greyscale_r + col.g*col_greyscale_g + col.b*col_greyscale_b) / 255.0f;
		if (grey > 1.0) grey = 1.0;
		col.r = colour.r*grey;
		col.g = colour.g*grey;
		col.b = colour.b*grey;

		// Set pixel colour
		if (pixel_format_ == PixelFormat::RGBA)
			col.write(data_+a);
		else
			data_[a] = pal->nearestColour(col);
	}

	return true;
}

// ----------------------------------------------------------------------------
// SImage::tint
//
// Tints the image to [colour] by [amount]. If the image is paletted, each
// pixel will be set to its nearest matching colour in [pal]
// ----------------------------------------------------------------------------
bool SImage::tint(rgba_t colour, float amount, Palette* pal, int start, int stop)
{
	// Can't do this with alpha maps
	if (pixel_format_ == PixelFormat::AlphaMap)
		return false;

	// Get palette to use
	if (has_palette_ || !pal)
		pal = &palette_;

	// Go through all pixels
	uint8_t bpp = bytesPerPixel();
	rgba_t col;
	for (int a = 0; a < size_.x*size_.y*bpp; a+= bpp)
	{
		// Skip colors out of range if desired
		if (pixel_format_ == PixelFormat::PalMask && start >= 0 && stop >= start && stop < 256)
		{
			if (data_[a] < start || data_[a] > stop)
				continue;
		}

		// Get current pixel colour
		if (pixel_format_ == PixelFormat::RGBA)
			col.set(data_[a], data_[a+1], data_[a+2], data_[a+3]);
		else
			col.set(pal->colour(data_[a]));

		// Tint it
		float inv_amt = 1.0f - amount;
		col.set(col.r*inv_amt + colour.r*amount,
		        col.g*inv_amt + colour.g*amount,
		        col.b*inv_amt + colour.b*amount, col.a);

		// Set pixel colour
		if (pixel_format_ == PixelFormat::RGBA)
			col.write(data_+a);
		else
			data_[a] = pal->nearestColour(col);
	}

	return true;
}

// ----------------------------------------------------------------------------
// SImage::adjust
//
// Automatically crop the image to remove fully transparent rows and columns
// from the sides. Returns true if successfully cropped.
// ----------------------------------------------------------------------------
bool SImage::adjust()
{
	int x1 = 0, x2 = size_.x, y1 = 0, y2 = size_.y;

	// Loop for empty columns on the left
	bool opaquefound = false;
	while (x1 < x2)
	{
		for (int i = 0; i < y2; ++i)
		{
			size_t p = i*size_.x + x1; // Pixel position
			switch (pixel_format_)
			{
			case PixelFormat::PalMask:	// Transparency is mask[p] == 0
				if (mask_[p])
					opaquefound = true;
				break;
			case PixelFormat::RGBA:		// Transparency is data[p*4 + 3] == 0
				if (data_[p * 4 + 3])
					opaquefound = true;
				break;
			case PixelFormat::AlphaMap:	// Transparency is data[p] == 0
				if (data_[p])
					opaquefound = true;
				break;
			}
			if (opaquefound) break;
		}
		if (opaquefound) break;
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
			size_t p = i*size_.x + x2 - 1;
			switch (pixel_format_)
			{
			case PixelFormat::PalMask:	if (mask_[p]) opaquefound = true; break;
			case PixelFormat::RGBA:		if (data_[p * 4 + 3]) opaquefound = true; break;
			case PixelFormat::AlphaMap:	if (data_[p]) opaquefound = true; break;
			}
			if (opaquefound) break;
		}
		if (opaquefound) break;
		--x2;
	}

	// Now loop for empty rows from the top
	opaquefound = false;
	while (y1 < y2)
	{
		for (int i = x1; i < x2; ++i)
		{
			size_t p = y1*size_.x + i;
			switch (pixel_format_)
			{
			case PixelFormat::PalMask:	if (mask_[p]) opaquefound = true; break;
			case PixelFormat::RGBA:		if (data_[p * 4 + 3]) opaquefound = true; break;
			case PixelFormat::AlphaMap:	if (data_[p]) opaquefound = true; break;
			}
			if (opaquefound) break;
		}
		if (opaquefound) break;
		++y1;
	}

	// Finally loop for empty rows from the bottom

	opaquefound = false;
	while (y2 > y1)
	{
		for (int i = x1; i < x2; ++i)
		{
			size_t p = (y2 - 1)*size_.x + i;
			switch (pixel_format_)
			{
			case PixelFormat::PalMask:	if (mask_[p]) opaquefound = true; break;
			case PixelFormat::RGBA:		if (data_[p * 4 + 3]) opaquefound = true; break;
			case PixelFormat::AlphaMap:	if (data_[p]) opaquefound = true; break;
			}
			if (opaquefound) break;
		}
		if (opaquefound) break;
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
	int extra = abs((offset_.x * 2) - size_.x);

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
