
class SIFImgz : public SIFormat
{
public:
	SIFImgz() : SIFormat{ "imgz", "IMGZ", "imgz" } {}
	~SIFImgz() = default;

	bool isThisFormat(MemChunk& mc) override
	{
		if (EntryDataFormat::getFormat("img_imgz")->isThisFormat(mc))
			return true;
		else
			return false;
	}

	SImage::Info getInfo(MemChunk& mc, int index) override
	{
		SImage::Info info;

		// Get width & height
		auto* header = (imgz_header_t*)mc.data();
		info.width   = wxINT16_SWAP_ON_BE(header->width);
		info.height  = wxINT16_SWAP_ON_BE(header->height);

		// Other image info
		info.colformat = SImage::Type::AlphaMap;
		info.format    = "imgz";

		return info;
	}

protected:
	bool readImage(SImage& image, MemChunk& data, int index) override
	{
		// Setup variables
		auto* header = (imgz_header_t*)data.data();
		int   width  = wxINT16_SWAP_ON_BE(header->width);
		int   height = wxINT16_SWAP_ON_BE(header->height);

		// Create image
		image.create(width, height, SImage::Type::AlphaMap);
		uint8_t* img_data = imageData(image);

		if (!header->compression)
		{
			// No compression
			memcpy(img_data, data.data() + sizeof(imgz_header_t), data.size() - sizeof(imgz_header_t));

			return true;
		}

		// We'll use wandering pointers. The original pointer is kept for cleanup.
		const uint8_t* read    = data.data() + sizeof(imgz_header_t);
		const uint8_t* readend = read + data.size() - 1;
		uint8_t*       dest    = img_data;
		uint8_t*       destend = dest + width * height;

		uint8_t code;
		size_t  length;

		while (read < readend && dest < destend)
		{
			code = *read++;
			if (code < 0x80)
			{
				length = code + 1;
				memcpy(dest, read, length);
				dest += length;
				read += length;
			}
			else if (code > 0x80)
			{
				length = 0x101 - code;
				code   = *read++;
				memset(dest, code, length);
				dest += length;
			}
		}

		return true;
	}
};
