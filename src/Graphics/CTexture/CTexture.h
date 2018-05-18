#pragma once

#include "General/ListenerAnnouncer.h"
#include "Graphics/Translation.h"

class Archive;
class ArchiveEntry;
class Palette;
class SImage;
class SImage;
class TextureXList;
class Tokenizer;

// Basic patch
class CTPatch
{
public:
	CTPatch() = default;
	CTPatch(string_view name, int16_t offset_x = 0, int16_t offset_y = 0) :
		name_{ name.data(), name.size() },
		offset_x_{ offset_x },
		offset_y_{ offset_y }
	{
	}
	CTPatch(CTPatch* copy);
	virtual ~CTPatch() = default;

	const string& name() const { return name_; }
	int16_t       xOffset() const { return offset_x_; }
	int16_t       yOffset() const { return offset_y_; }

	void setName(string_view name) { S_SET_VIEW(this->name_, name); }
	void setOffsetX(int16_t offset) { offset_x_ = offset; }
	void setOffsetY(int16_t offset) { offset_y_ = offset; }

	virtual ArchiveEntry* getPatchEntry(Archive* parent = nullptr);

protected:
	string  name_;
	int16_t offset_x_ = 0;
	int16_t offset_y_ = 0;
};

// Extended patch (for TEXTURES)
class CTPatchEx : public CTPatch
{
public:
	enum class Type
	{
		Patch,
		Graphic
	};

	CTPatchEx() = default;
	CTPatchEx(string_view name, int16_t offset_x = 0, int16_t offset_y = 0, Type type = Type::Patch) :
		CTPatch{ name, offset_x, offset_y },
		type_{ type }
	{
	}
	CTPatchEx(CTPatch* copy);
	CTPatchEx(CTPatchEx* copy);
	~CTPatchEx() = default;

	bool          flipX() const { return flip_x_; }
	bool          flipY() const { return flip_y_; }
	bool          useOffsets() const { return use_offsets_; }
	int16_t       rotation() const { return rotation_; }
	ColRGBA        colour() const { return colour_; }
	float         alpha() const { return alpha_; }
	const string& style() const { return style_; }
	uint8_t       blendType() const { return blendtype_; }
	Translation&  translation() { return translation_; }

	void flipX(bool flip) { flip_x_ = flip; }
	void flipY(bool flip) { flip_y_ = flip; }
	void useOffsets(bool use) { use_offsets_ = use; }
	void setRotation(int16_t rot) { rotation_ = rot; }
	void setColour(uint8_t r, uint8_t g, uint8_t b, uint8_t a) { colour_.set(r, g, b, a); }
	void setAlpha(float a) { alpha_ = a; }
	void setStyle(string_view s) { S_SET_VIEW(style_, s); }
	void setBlendType(uint8_t type) { blendtype_ = type; }

	ArchiveEntry* getPatchEntry(Archive* parent = nullptr) override;

	bool   parse(Tokenizer& tz, Type type = Type::Patch);
	string asText();

private:
	Type        type_        = Type::Patch;
	bool        flip_x_      = false;
	bool        flip_y_      = false;
	bool        use_offsets_ = false;
	int16_t     rotation_    = 0;
	Translation translation_;
	ColRGBA      colour_    = ColRGBA::WHITE;
	float       alpha_     = 1.f;
	string      style_     = "Copy";
	uint8_t     blendtype_ = 0; // 0=none, 1=translation, 2=blend, 3=tint
};

class CTexture : public Announcer
{
	friend class TextureXList;

public:
	enum class Type
	{
		Texture,
		Sprite,
		Graphic,
		WallTexture,
		Flat,
		HiRes
	};

	CTexture(bool extended = false) : extended_{ extended } {}
	~CTexture() = default;

	void copyTexture(CTexture* copy, bool keep_type = false);

	const string& name() const { return name_; }
	uint16_t      width() const { return width_; }
	uint16_t      height() const { return height_; }
	double        scaleX() const { return scale_x_; }
	double        scaleY() const { return scale_y_; }
	int16_t       offsetX() const { return offset_x_; }
	int16_t       offsetY() const { return offset_y_; }
	bool          worldPanning() const { return world_panning_; }
	const string& type() const { return type_; }
	bool          isExtended() const { return extended_; }
	bool          isOptional() const { return optional_; }
	bool          noDecals() const { return no_decals_; }
	bool          nullTexture() const { return null_texture_; }
	size_t        nPatches() const { return patches_.size(); }
	CTPatch*      patch(size_t index);
	uint8_t       state() const { return state_; }
	int           index() const;

	void setName(string_view name) { S_SET_VIEW(this->name_, name); }
	void setWidth(uint16_t width) { this->width_ = width; }
	void setHeight(uint16_t height) { this->height_ = height; }
	void setScaleX(double scale) { this->scale_x_ = scale; }
	void setScaleY(double scale) { this->scale_y_ = scale; }
	void setScale(double x, double y)
	{
		this->scale_x_ = x;
		this->scale_y_ = y;
	}
	void setOffsetX(int16_t offset) { this->offset_x_ = offset; }
	void setOffsetY(int16_t offset) { this->offset_y_ = offset; }
	void setWorldPanning(bool wp) { this->world_panning_ = wp; }
	void setType(string_view type) { S_SET_VIEW(this->type_, type); }
	void setExtended(bool ext) { this->extended_ = ext; }
	void setOptional(bool opt) { this->optional_ = opt; }
	void setNoDecals(bool nd) { this->no_decals_ = nd; }
	void setNullTexture(bool nt) { this->null_texture_ = nt; }
	void setState(uint8_t state) { this->state_ = state; }
	void setList(TextureXList* list) { this->in_list_ = list; }

	void clear();

	bool addPatch(string_view patch, int16_t offset_x = 0, int16_t offset_y = 0, int index = -1);
	bool removePatch(size_t index);
	bool removePatch(string_view patch);
	bool replacePatch(size_t index, string_view newpatch);
	bool duplicatePatch(size_t index, int16_t offset_x = 8, int16_t offset_y = 8);
	bool swapPatches(size_t p1, size_t p2);

	bool   parse(Tokenizer& tz, string_view type);
	bool   parseDefine(Tokenizer& tz);
	string asText();

	bool convertExtended();
	bool convertRegular();
	bool loadPatchImage(unsigned pindex, SImage& image, Archive* parent = nullptr, Palette* pal = nullptr);
	bool toImage(SImage& image, Archive* parent = nullptr, Palette* pal = nullptr, bool force_rgba = false);

	typedef std::unique_ptr<CTexture> UPtr;
	typedef std::shared_ptr<CTexture> SPtr;

private:
	// Basic info
	string   name_;
	uint16_t width_         = 0;
	uint16_t height_        = 0;
	double   scale_x_       = 1.;
	double   scale_y_       = 1.;
	bool     world_panning_ = false;
	int      index_         = -1;

	// Patches
	vector<std::unique_ptr<CTPatch>> patches_;

	// Extended (TEXTURES) info
	string  type_         = "Texture";
	bool    extended_     = false;
	bool    defined_      = false;
	bool    optional_     = false;
	bool    no_decals_    = false;
	bool    null_texture_ = false;
	int16_t offset_x_     = 0;
	int16_t offset_y_     = 0;
	int16_t def_width_    = 0;
	int16_t def_height_   = 0;

	// Editor info
	uint8_t       state_   = 0;
	TextureXList* in_list_ = nullptr;
};
