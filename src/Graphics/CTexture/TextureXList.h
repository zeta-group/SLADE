#pragma once

#include "CTexture.h"

class ArchiveEntry;
class PatchTable;

// Texture flags
#define TX_WORLDPANNING 0x8000

class TextureXList
{
public:
	// Enum for different texturex formats
	enum class Format
	{
		Normal,
		Strife11,
		Nameless,
		Textures,
		Jaguar,
	};

	// TEXTUREx texture patch
	struct Patch
	{
		int16_t  left;
		int16_t  top;
		uint16_t patch;
	};

	TextureXList(Format format = Format::Normal);
	~TextureXList() = default;

	uint32_t nTextures() const { return textures_.size(); }

	CTexture* getTexture(size_t index);
	CTexture* getTexture(string name);
	Format    getFormat() const { return format_; }
	int       textureIndex(string name);

	void setFormat(Format format) { format_ = format; }

	void           addTexture(CTexture::UPtr tex, int position = -1);
	CTexture::UPtr removeTexture(unsigned index);
	void           swapTextures(unsigned index1, unsigned index2);
	CTexture::UPtr replaceTexture(unsigned index, CTexture::UPtr replacement);

	void clear(bool clear_patches = false);
	void removePatch(string patch);

	bool readTEXTUREXData(ArchiveEntry* texturex, PatchTable& patch_table, bool add = false);
	bool writeTEXTUREXData(ArchiveEntry* texturex, PatchTable& patch_table);

	bool readTEXTURESData(ArchiveEntry* textures);
	bool writeTEXTURESData(ArchiveEntry* textures);

	bool convertToTEXTURES();

	bool findErrors();

	string getTextureXFormatString() const;

private:
	vector<CTexture::UPtr> textures_;
	Format                 format_;
	CTexture               tex_invalid_;
};
