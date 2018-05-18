#pragma once

#include "OpenGL/Drawing.h"
#include "OpenGL/GLTexture.h"

class TextBox;
class BrowserItem
{
	friend class BrowserWindow;

public:
	enum class ViewType
	{
		Normal,
		Tiles
	};

	enum class NameType
	{
		Normal,
		Index,
		None
	};

	BrowserItem(string_view name, unsigned index = 0, string_view type = "item");
	virtual ~BrowserItem() = default;

	string   name() const { return name_; }
	unsigned index() const { return index_; }

	virtual bool loadImage();
	void         draw(
				int           size,
				int           x,
				int           y,
				Drawing::Font font,
				NameType      nametype    = NameType::Normal,
				ViewType      viewtype    = ViewType::Normal,
				ColRGBA        colour      = ColRGBA::WHITE,
				bool          text_shadow = true);
	void           clearImage() const;
	virtual string itemInfo() { return ""; }

	typedef std::unique_ptr<BrowserItem> UPtr;

protected:
	string                   type_;
	string                   name_;
	unsigned                 index_  = 0;
	GLTexture*               image_  = nullptr;
	BrowserWindow*           parent_ = nullptr;
	bool                     blank_  = false;
	std::unique_ptr<TextBox> text_box_;
};
