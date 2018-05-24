
class SIFHalfLifeTex : public SIFormat
{
public:
	SIFHalfLifeTex() : SIFormat{ "hlt", "Half-Life Texture", "hlt", 20 } {}
	~SIFHalfLifeTex() = default;

	bool isThisFormat(MemChunk& mc) override
	{
		if (EntryDataFormat::getFormat("img_hlt")->isThisFormat(mc) >= EDF_PROBABLY)
			return true;
		else
			return false;
	}

	SImage::Info getInfo(MemChunk& mc, int index) override
	{
		SImage::Info info;

		// Get image info
		info.width     = READ_L32(mc.data(), 16) >> index;
		info.height    = READ_L32(mc.data(), 20) >> index;
		info.numimages = 4;
		info.colformat = SImage::Type::PalMask;
		info.format    = id_;

		return info;
	}

protected:
	bool readImage(SImage& image, MemChunk& data, int index) override
	{
		// Get image info
		auto info = getInfo(data, index);

		// Sanitize index if needed
		index %= info.numimages;
		if (index < 0)
			index = info.numimages + index;

		// Determine data offset
		size_t data_offset = READ_L32(data.data(), 24 + (index << 2));
		if (!info.width || !info.height || !data_offset || data.size() < data_offset + (info.width * info.height))
		{
			Global::error = "HLT file: invalid data for mip level";
			return false;
		}

		// Let's build the palette now
		Palette palette;
		size_t  pal_offset =
			READ_L32(data.data(), 36) + ((READ_L32(data.data(), 16) >> 3) * (READ_L32(data.data(), 20) >> 3));
		if (data.size() < pal_offset + 5)
		{
			Global::error = "HLT file: invalid palette offset";
			return false;
		}
		size_t palsize = READ_L16(data.data(), pal_offset);
		if (palsize == 0 || palsize > 256 || data.size() < (pal_offset + 2 + (palsize * 3)))
		{
			Log::error(fmt::format("palsize {}, paloffset {}, entry size {}", palsize, pal_offset, data.size()));
			Global::error = "HLT file: invalid palette size";
			return false;
		}
		for (size_t c = 0; c < palsize; ++c)
		{
			ColRGBA color;
			color.r = data[(c * 3) + pal_offset + 2];
			color.g = data[(c * 3) + pal_offset + 3];
			color.b = data[(c * 3) + pal_offset + 4];
			palette.setColour(c, color);
		}

		// Create image
		image.create(info.width, info.height, SImage::Type::PalMask, &palette, index, info.numimages);
		image.fillAlpha(255);

		// Fill data with pixel data
		memcpy(imageData(image), data.data() + data_offset, info.width * info.height);

		return true;
	}
};

class SIFSCSprite : public SIFormat
{
public:
	SIFSCSprite() : SIFormat{ "scsprite", "Shadowcaster Sprite", "dat", 110 } {}
	~SIFSCSprite() = default;

	bool isThisFormat(MemChunk& mc) override
	{
		if (EntryDataFormat::getFormat("img_scsprite")->isThisFormat(mc) >= EDF_UNLIKELY)
			return true;
		else
			return false;
	}

	SImage::Info getInfo(MemChunk& mc, int index) override
	{
		int            size = mc.size();
		SImage::Info info;

		// Get image width
		info.width = READ_L16(mc, 2);

		if (info.width == 0)
			return info;

		// Get image height
		for (int j = 0; j < info.width; ++j)
		{
			int pos = (j << 1) + 4;
			if (pos + 2 >= size)
				return info;
			int colstart = READ_L16(mc, pos);

			// Columns with a null offset are skipped
			if (colstart == 0)
				continue;

			if (colstart < 0 || size < colstart + 2 || colstart < (info.width * 2 + 4))
				return info;

			int start     = mc[colstart];
			int stop      = mc[colstart + 1];
			int colheight = start - stop;
			if (colheight < 0 || size < colstart + colheight + 1)
				return info;

			if (start > info.height)
				info.height = start;
		}

		// Set other properties
		info.colformat = SImage::Type::PalMask;
		info.format    = id_;

		return info;
	}

protected:
	bool readImage(SImage& image, MemChunk& data, int index) override
	{
		// Get width & height
		SImage::Info info = getInfo(data, index);
		if (info.format != id_)
			return false;

		// Create image
		image.create(info.width, info.height, SImage::Type::PalMask);

		// Format has no offsets, so just set them automatically
		image.setXOffset(info.width / 2);
		image.setYOffset(info.height);

		// Read pixel data
		uint8_t* img_data = imageData(image);
		uint8_t* img_mask = imageMask(image);
		for (int h = 0, i = 4; h < info.width; ++h, i += 2)
		{
			int colstart = READ_L16(data, i);
			if (colstart)
			{
				int start       = data[colstart];
				int stop        = data[colstart + 1];
				int colheight   = start - stop;
				int startheight = info.height - start;

				for (int z = 0; z < colheight; ++z)
				{
					int mypixel = ((z + startheight) * info.width) + h;
					if (mypixel >= info.width * info.height || unsigned(colstart + 2 + z) >= data.size())
						return false;
					if (mypixel < 0)
						return false;
					img_data[mypixel] = data[colstart + 2 + z];
					if (img_data[mypixel])
						img_mask[mypixel] = 0xFF;
				}
			}
		}

		return true;
	}
};

// The same as Doom_Arah, except for the value of the transparent
// index. Since the zerotransparent hack didn't survive the SIF
// rewrite, here it is in all its copy/pasted glory.
class SIFSCGfx : public SIFormat
{
public:
	SIFSCGfx() : SIFormat{ "scgfx", "Shadowcaster Gfx", "dat", 100 } {}
	~SIFSCGfx() = default;

	bool isThisFormat(MemChunk& mc) override
	{
		if (EntryDataFormat::getFormat("img_scgfx")->isThisFormat(mc) >= EDF_PROBABLY)
			return true;
		else
			return false;
	}

	SImage::Info getInfo(MemChunk& mc, int index) override
	{
		SImage::Info info;

		// Read header
		patch_header_t header;
		mc.read(&header, 8, 0);

		// Set info
		info.width     = wxINT16_SWAP_ON_BE(header.width);
		info.height    = wxINT16_SWAP_ON_BE(header.height);
		info.offset_x  = wxINT16_SWAP_ON_BE(header.left);
		info.offset_y  = wxINT16_SWAP_ON_BE(header.top);
		info.colformat = SImage::Type::PalMask;
		info.format    = id_;

		return info;
	}

protected:
	bool readImage(SImage& image, MemChunk& data, int index) override
	{
		// Setup variables
		patch_header_t header;
		data.read(&header, 8, 0);
		int width    = wxINT16_SWAP_ON_BE(header.width);
		int height   = wxINT16_SWAP_ON_BE(header.height);
		int offset_x = wxINT16_SWAP_ON_BE(header.left);
		int offset_y = wxINT16_SWAP_ON_BE(header.top);

		// Create image
		image.create(width, height, SImage::Type::PalMask);
		uint8_t* img_data = imageData(image);
		uint8_t* img_mask = imageMask(image);

		// Read raw pixel data
		data.read(img_data, width * height, 8);

		// Create mask (all opaque)
		memset(img_mask, 255, width * height);

		// Mark as transparent all pixels that are index 0
		for (size_t i = 0; i < (unsigned)(width * height); ++i)
			if (img_data[i] == 0)
				img_mask[i] = 0;

		// Setup other image properties
		image.setXOffset(offset_x);
		image.setYOffset(offset_y);

		return true;
	}
};


class SIFSCWall : public SIFormat
{
public:
	SIFSCWall() : SIFormat{ "scwall", "Shadowcaster Wall", "dat", 101 } {}
	~SIFSCWall() = default;

	bool isThisFormat(MemChunk& mc) override
	{
		if (EntryDataFormat::getFormat("img_scwall")->isThisFormat(mc) >= EDF_PROBABLY)
			return true;
		else
			return false;
	}

	SImage::Info getInfo(MemChunk& mc, int index) override
	{
		SImage::Info info;

		// Get image properties
		info.height    = mc[0] * 4;
		info.width     = 64;
		info.colformat = SImage::Type::PalMask;
		info.format    = id_;

		return info;
	}

protected:
	bool readImage(SImage& image, MemChunk& data, int index) override
	{
		// Determine width and height
		int height = data[0] * 4;
		int width  = 64;
		if (data.size() != unsigned(width * height + OFFSET))
			return false;

		// Create image
		image.create(width, height, SImage::Type::PalMask);

		// Read pixel data
		uint8_t* img_data = imageData(image);
		uint8_t* img_mask = imageMask(image);
		memset(img_mask, 255, height << 6);
		int pixelreader = OFFSET;
		for (int x = 0; x < width; ++x)
		{
			for (int y = 0; y < height; ++y, ++pixelreader)
			{
				img_data[(y * 64) + x] = data[pixelreader];
				/* Problem: not all textures ought to be transparent.
				There's no flag hidden in the header that would
				determine whether it's opaque or transparent. It
				seems to be a property of the map block rather
				than the texture... Black pixels in an empty area
				look less glitchy than transparency in undue places.
				if (img_data[brush] == 0)
				img_mask[brush] = 0;*/
			}
		}

		return true;
	}

private:
	static const int OFFSET = 130; // Headers contain 129 bytes of junk.
};

class SIFAnaMip : public SIFormat
{
public:
	SIFAnaMip() : SIFormat{ "mipimage", "Amulets & Armor", "dat", 100 } {}
	~SIFAnaMip() = default;

	bool isThisFormat(MemChunk& mc) override
	{
		if (EntryDataFormat::getFormat("img_mipimage")->isThisFormat(mc) >= EDF_PROBABLY)
			return true;
		else
			return false;
	}

	SImage::Info getInfo(MemChunk& mc, int index) override
	{
		SImage::Info info;

		// Get image info
		info.width     = READ_L16(mc.data(), 0);
		info.height    = READ_L16(mc.data(), 2);
		info.colformat = SImage::Type::PalMask;
		info.format    = id_;

		// Technically false, as there are multiple mipmap levels.
		// May implement them later. Not in any hurry about them.
		info.numimages = 1;

		return info;
	}

protected:
	bool readImage(SImage& image, MemChunk& data, int index) override
	{
		// Get image info
		auto info = getInfo(data, index);

		// Check data
		if (data.size() < unsigned(4 + (info.width * info.height)))
			return false;

		// Create image
		image.create(info.width, info.height, SImage::Type::PalMask);
		image.fillAlpha(255);

		// Read data
		data.read(imageData(image), info.width * info.height, 4);

		return true;
	}
};

class SIFBuildTile : public SIFormat
{
public:
	SIFBuildTile() : SIFormat{ "arttile", "Build ART", "art", 100 } {}
	~SIFBuildTile() = default;

	bool isThisFormat(MemChunk& mc) override
	{
		if (EntryDataFormat::getFormat("img_arttile")->isThisFormat(mc) >= EDF_PROBABLY)
			return true;
		else
			return false;
	}

	SImage::Info getInfo(MemChunk& mc, int index) override
	{
		SImage::Info info;

		// Get info
		getTileInfo(info, mc, index);

		return info;
	}

protected:
	bool readImage(SImage& image, MemChunk& data, int index) override
	{
		// Get info and data start
		SImage::Info info;
		unsigned       datastart = getTileInfo(info, data, index);

		// Check
		if (datastart < 16 || datastart >= data.size())
			return false;

		// Create image (swapped width/height because column-major)
		image.create(info.height, info.width, SImage::Type::PalMask, nullptr, index, info.numimages);

		// Read data
		uint8_t* img_data = imageData(image);
		uint8_t* img_mask = imageMask(image);
		data.read(img_data, info.width * info.height, datastart);

		// Create mask
		for (int a = 0; a < info.width * info.height; a++)
		{
			if (img_data[a] == 0xFF)
				img_mask[a] = 0;
			else
				img_mask[a] = 0xFF;
		}

		// Convert from column-major to row-major
		image.rotate(270);
		image.mirror(true);

		// Set offsets
		image.setXOffset(info.offset_x);
		image.setYOffset(info.offset_y);

		return true;
	}

private:
	unsigned getTileInfo(SImage::Info& info, MemChunk& mc, int index) const
	{
		// Get tile info
		uint32_t firsttile = wxUINT32_SWAP_ON_BE(((uint32_t*)mc.data())[2]);
		uint32_t lasttile  = wxUINT32_SWAP_ON_BE(((uint32_t*)mc.data())[3]);

		// Set number of images
		info.numimages = 1 + lasttile - firsttile;

		// Each tile has a 2-byte width, a 2-byte height, and a 4-byte
		// picanm struct. The header itself is 16 bytes.
		size_t x_offs = 16;
		size_t y_offs = x_offs + (info.numimages << 1);
		size_t o_offs = y_offs + (info.numimages << 1);

		// Compute the address where our tile's graphic data starts
		size_t datastart = (info.numimages * 8) + 16;
		if (index > 0)
		{
			// We can skip these steps if looking at the first tile in the ART file.
			for (int i = 0; i < index; ++i)
			{
				int width  = READ_L16(mc.data(), (x_offs + (i << 1)));
				int height = READ_L16(mc.data(), (y_offs + (i << 1)));
				datastart += (width * height);
			}

			// Increment values to that of the tile we want
			x_offs += (index << 1);
			y_offs += (index << 1);
			o_offs += (index << 2);
		}
		if ((unsigned)mc.size() < datastart)
			return 0;

		// Get width and height of tile
		info.width  = READ_L16(mc.data(), x_offs);
		info.height = READ_L16(mc.data(), y_offs);

		// Setup remaining info
		info.colformat = SImage::Type::PalMask;
		info.format    = id_;

		// Offsets are signed bytes, so they need a cast
		info.offset_x = (int8_t)mc[o_offs + 1];
		info.offset_y = (int8_t)mc[o_offs + 2];

		// Offsets are not computed from the same reference point, so convert them
		info.offset_x += (info.width >> 1);
		info.offset_y += info.height;

		// Return beginning of tile pixel data
		return datastart;
	}
};

class SIFHeretic2M8 : public SIFormat
{
public:
	SIFHeretic2M8() : SIFormat{ "m8", "Heretic 2 8bpp", "dat", 80 } {}
	~SIFHeretic2M8() = default;

	bool isThisFormat(MemChunk& mc) override
	{
		if (EntryDataFormat::getFormat("img_m8")->isThisFormat(mc) >= EDF_PROBABLY)
			return true;
		else
			return false;
	}

	SImage::Info getInfo(MemChunk& mc, int index) override
	{
		SImage::Info info;

		getLevelInfo(info, mc, index);

		return info;
	}

protected:
	bool readImage(SImage& image, MemChunk& data, int index) override
	{
		// Get miplevel info and offset
		SImage::Info info;
		unsigned       datastart = getLevelInfo(info, data, index);

		// Check
		if (datastart + info.width * info.height > data.size())
			return false;

		// Build palette
		Palette palette;
		for (size_t c = 0; c < 256; ++c)
		{
			ColRGBA color;
			color.r = data[(c * 3) + 0x104];
			color.g = data[(c * 3) + 0x105];
			color.b = data[(c * 3) + 0x106];
			palette.setColour(c, color);
		}

		// Create image
		image.create(info, &palette);
		image.fillAlpha(255);

		// Read image data
		data.read(imageData(image), info.width * info.height, datastart);

		return true;
	}

private:
	unsigned getLevelInfo(SImage::Info& info, MemChunk& mc, int index) const
	{
		// Check size
		if (mc.size() < 1040)
			return 0;

		// Determine total number of images
		int i = 9;
		while ((i < 25) && (READ_L32(mc, (i << 2)) != 0))
			++i;
		info.numimages = i - 9;

		// Set other info
		info.width = READ_L32(mc, ((index + 9) << 2));
		info.height = READ_L32(mc, ((index + 25) << 2));
		info.colformat = SImage::Type::PalMask;
		info.has_palette = true;
		info.format = id_;

		// Return offset to mip level
		return READ_L32(mc, ((index + 41) << 2));
	}
};

class SIFHeretic2M32 : public SIFormat
{
public:
	SIFHeretic2M32() : SIFormat{ "m32", "Heretic 2 32bpp", "dat", 80 } {}
	~SIFHeretic2M32() = default;

	bool isThisFormat(MemChunk& mc) override
	{
		if (EntryDataFormat::getFormat("img_m32")->isThisFormat(mc) >= EDF_PROBABLY)
			return true;
		else
			return false;
	}

	SImage::Info getInfo(MemChunk& mc, int index) override
	{
		SImage::Info info;

		getLevelInfo(info, mc, index);

		return info;
	}

protected:
	bool readImage(SImage& image, MemChunk& data, int index) override
	{
		// Get miplevel info and offset
		SImage::Info info;
		unsigned       datastart = getLevelInfo(info, data, index);

		// Check
		if (datastart + info.width * info.height * 4 > data.size())
			return false;

		// Create image
		image.create(info);
		image.fillAlpha(255);

		// Read image data
		data.read(imageData(image), info.width * info.height * 4, datastart);

		return true;
	}

private:
	unsigned getLevelInfo(SImage::Info& info, MemChunk& mc, int index) const
	{
		// Check size
		if (mc.size() < 968)
			return 0;

		// Determine total number of images
		int i = 129;
		while ((i < 145) && (READ_L32(mc, (i << 2)) != 0))
			++i;
		info.numimages = i - 129;

		// Set other info
		info.width = READ_L32(mc, ((index + 129) << 2));
		info.height = READ_L32(mc, ((index + 145) << 2));
		info.colformat = SImage::Type::RGBA;
		info.format = id_;

		// Return offset to mip level
		return READ_L32(mc, ((index + 161) << 2));
	}
};

class SIFWolfPic : public SIFormat
{
public:
	SIFWolfPic() : SIFormat("wolfpic", "Wolf3d Pic", "dat", 200) {}
	~SIFWolfPic() = default;

	bool isThisFormat(MemChunk& mc) override
	{
		if (EntryDataFormat::getFormat("img_wolfpic")->isThisFormat(mc) >= EDF_PROBABLY)
			return true;
		else
			return false;
	}

	SImage::Info getInfo(MemChunk& mc, int index) override
	{
		SImage::Info info;

		// Read dimensions
		info.width  = READ_L16(mc.data(), 0);
		info.height = READ_L16(mc.data(), 2);

		// Setup other info
		info.colformat = SImage::Type::PalMask;
		info.format    = id_;

		return info;
	}

protected:
	bool readImage(SImage& image, MemChunk& data, int index) override
	{
		// Get image info
		SImage::Info info = getInfo(data, index);

		// Check data
		if (data.size() != unsigned(4 + info.width * info.height))
			return false;

		// Create image
		image.create(info);
		image.fillAlpha(255);
		uint8_t* img_data = imageData(image);

		// Read image data
		const uint8_t* pixel = data.data() + 4;
		const uint8_t* entryend = data.data() + data.size();
		uint8_t*       brush = img_data;
		uint8_t*       dataend = img_data + data.size() - 4;

		while (pixel < entryend)
		{
			*brush = *pixel++;
			brush += 4;
			if (brush >= dataend)
				brush -= data.size() - 5;
		}

		return true;
	}
};

class SIFWolfSprite : public SIFormat
{
public:
	SIFWolfSprite() : SIFormat("wolfsprite", "Wolf3d Sprite", "dat", 200) {}
	~SIFWolfSprite() = default;

	bool isThisFormat(MemChunk& mc) override
	{
		if (EntryDataFormat::getFormat("img_wolfsprite")->isThisFormat(mc) >= EDF_PROBABLY)
			return true;
		else
			return false;
	}

	SImage::Info getInfo(MemChunk& mc, int index) override
	{
		SImage::Info info;

		uint8_t leftpix = mc[0];
		uint8_t rightpix = mc[2];
		info.width  = 1 + rightpix - leftpix;
		info.height = 64;

		// Setup other info
		info.offset_x  = 32 - leftpix;
		info.offset_y  = info.height;
		info.colformat = SImage::Type::PalMask;
		info.format    = id_;

		return info;
	}

protected:
	bool readImage(SImage& image, MemChunk& data, int index) override
	{
		// Get image info
		auto info = getInfo(data, index);

		// Create image
		image.create(info);
		uint8_t* img_data = imageData(image);
		uint8_t* img_mask = imageMask(image);

		// Read data
		const auto* cmdptr = (const uint16_t*)(data.data() + 4);
		uint32_t        i, x, y;
		for (x = 0; x < (unsigned)info.width; ++x)
		{
			const auto* linecmds = (const int16_t*)(data.data() + wxINT16_SWAP_ON_BE(*cmdptr));
			cmdptr++;
			for (; wxINT16_SWAP_ON_BE(*linecmds); linecmds += 3)
			{
				i = (wxINT16_SWAP_ON_BE(linecmds[2]) >> 1) + wxINT16_SWAP_ON_BE(linecmds[1]);
				for (y = (uint32_t)(wxINT16_SWAP_ON_BE(linecmds[2]) >> 1);
					y < (uint32_t)(wxINT16_SWAP_ON_BE(linecmds[0]) / 2);
					++y, ++i)
				{
					img_data[y * info.width + x] = data[i];
					img_mask[y * info.width + x] = 255;
				}
			}
		}

		return true;
	}
};
