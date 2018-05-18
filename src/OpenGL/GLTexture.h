#pragma once

class SImage;
class Palette;

class GLTexture
{
public:
	enum class Filter
	{
		// Filter types
		Nearest,
		Linear,
		Mipmap,
		LinearMipmap, // (same as Mipmap)
		NearestLinearMin,
		NearestMipmap,
	};

	GLTexture(bool allow_split = true) : allow_split_{ allow_split } {}
	~GLTexture();

	bool     isLoaded() const { return loaded_; }
	uint32_t getWidth() const { return width_; }
	uint32_t getHeight() const { return height_; }
	Filter   getFilter() const { return filter_; }
	double   getScaleX() const { return scale_.x; }
	double   getScaleY() const { return scale_.y; }
	bool     isTiling() const { return tiling_; }
	unsigned glId()
	{
		if (!parts_.empty())
			return parts_[0].id;
		else
			return 0;
	}

	void setFilter(Filter filter) { this->filter_ = filter; }
	void setTiling(bool tiling) { this->tiling_ = tiling; }
	void setScale(double sx, double sy)
	{
		this->scale_.x = sx;
		this->scale_.y = sy;
	}

	bool loadImage(SImage* image, Palette* pal = nullptr);
	bool loadRawData(const uint8_t* data, uint32_t width, uint32_t height);

	bool clear();
	bool genChequeredTexture(uint8_t block_size, const ColRGBA& col1, const ColRGBA& col2);

	bool bind();
	bool draw2d(double x = 0, double y = 0, bool flipx = false, bool flipy = false);
	bool draw2dTiled(uint32_t width, uint32_t height);

	ColRGBA averageColour(rect_t area);

	static GLTexture& bgTex();
	static GLTexture& missingTex();
	static void       resetBgTex();

	typedef std::unique_ptr<GLTexture> UPtr;

private:
	struct SubTex
	{
		unsigned id     = 0;
		uint32_t width  = 0;
		uint32_t height = 0;
	};

	uint32_t       width_  = 0;
	uint32_t       height_ = 0;
	vector<SubTex> parts_;
	Filter         filter_      = Filter::Nearest;
	bool           loaded_      = false;
	bool           allow_split_ = false;
	bool           tiling_      = false;
	Vec2<double>   scale_       = { 1., 1. };

	// Some generic/global textures
	static GLTexture tex_background_; // Checkerboard background texture
	static GLTexture tex_missing_;    // Checkerboard 'missing' texture

	// Stuff used internally
	bool loadData(const uint8_t* data, uint32_t width, uint32_t height, bool add = false);
	bool loadImagePortion(SImage* image, const rect_t& rect, Palette* pal = nullptr, bool add = false);
};
