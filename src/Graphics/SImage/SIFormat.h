#pragma once

#include "SImage.h"

class ArchiveEntry;
class SIFormat
{
public:
	// Conversion options stuff
	enum class Mask
	{
		None = 0,
		Colour,
		Alpha,
		Brightness,
	};

	struct ConvertOptions
	{
		Palette*     pal_current = nullptr;
		Palette*     pal_target  = nullptr;
		Mask         mask_source = Mask::Alpha;
		ColRGBA       mask_colour;
		uint8_t      alpha_threshold = 0;
		bool         transparency    = true;
		SImage::Type col_format      = SImage::Type::Any;
	};

	SIFormat(string_view id, string_view name = "Unknown", string_view extension = "dat", uint8_t reliability = 255);
	virtual ~SIFormat() = default;

	string  id() const { return id_; }
	string  name() const { return name_; }
	string  extension() const { return extension_; }
	uint8_t reliability() const { return reliability_; }

	virtual bool isThisFormat(MemChunk& mc) = 0;

	// Reading
	virtual SImage::Info getInfo(MemChunk& mc, int index = 0) = 0;

	bool loadImage(SImage& image, MemChunk& data, int index = 0)
	{
		// Check format
		if (!isThisFormat(data))
			return false;

		// Attempt to read image data
		bool ok = readImage(image, data, index);

		// Set image properties if successful
		if (ok)
		{
			image.format_    = this;
			image.img_index_ = index;
		}
		else
			image.clear();

		// Announce
		image.announce("image_changed");

		return ok;
	}

	// Writing
	enum
	{
		NOTWRITABLE, // Format cannot be written
		WRITABLE,    // Format can be written
		CONVERTIBLE, // Format can be written but a conversion is required
	};
	virtual int  canWrite(SImage& image) { return NOTWRITABLE; }
	virtual bool canWriteType(SImage::Type type) { return false; }
	virtual bool convertWritable(SImage& image, ConvertOptions opt) { return false; }
	virtual bool writeOffset(SImage& image, ArchiveEntry* entry, point2_t offset) { return false; }

	bool saveImage(SImage& image, MemChunk& out, Palette* pal = nullptr, int index = 0)
	{
		// Attempt to write image data
		bool ok = writeImage(image, out, pal, index);

		// Set format if successful
		if (ok)
			image.format_ = this;

		return ok;
	}

	static void      initFormats();
	static SIFormat* getFormat(string_view name);
	static SIFormat* determineFormat(MemChunk& mc);
	static SIFormat* unknownFormat();
	static SIFormat* rawFormat();
	static SIFormat* flatFormat();
	static SIFormat* generalFormat();
	static void      getAllFormats(vector<SIFormat*>& list);

protected:
	string  id_;
	string  name_;
	string  extension_;
	uint8_t reliability_;

	// Stuff to access protected image data
	static uint8_t* imageData(SImage& image) { return image.data_; }
	static uint8_t* imageMask(SImage& image) { return image.mask_; }
	static Palette& imagePalette(SImage& image) { return image.palette_; }

	virtual bool readImage(SImage& image, MemChunk& data, int index) = 0;
	virtual bool writeImage(SImage& image, MemChunk& data, Palette* pal, int index) { return false; }
};
