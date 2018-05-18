#pragma once

#include "MCOverlay.h"
#include "MapEditor/SLADEMap/MapLine.h"

class MapSide;

class LineTextureOverlay : public MCOverlay
{
public:
	LineTextureOverlay() = default;
	~LineTextureOverlay() = default;

	void openLines(vector<MapLine*>& list);
	void close(bool cancel) override;

	// Drawing
	void updateLayout(int width, int height);
	void draw(int width, int height, float fade) override;

	// Input
	void mouseMotion(int x, int y) override;
	void mouseLeftClick() override;
	void keyDown(string_view key) override;

private:
	struct TexInfo
	{
		point2_t       position;
		bool           hover = false;
		vector<string> textures;
		bool           changed = false;

		void addTexture(string_view texture)
		{
			if (texture == "-")
				return;

			for (const auto& tex : textures)
				if (tex == texture)
					return;

			textures.emplace_back(texture.data(), texture.size());
		}

		void checkHover(int x, int y, int halfsize)
		{
			if (x >= position.x - halfsize && x <= position.x + halfsize && y >= position.y - halfsize
				&& y <= position.y + halfsize)
				hover = true;
			else
				hover = false;
		}
	};

	vector<MapLine*> lines_;
	int              selected_side_ = 0;

	// Texture info
	std::map<MapLine::Part, TexInfo> textures_;
	bool                             side1_ = false;
	bool                             side2_ = false;

	// Drawing info
	int tex_size_    = 0;
	int last_width_  = 0;
	int last_height_ = 0;

	void browseTexture(TexInfo& tex, string_view position);
	void drawTexture(float alpha, int size, TexInfo& tex, const string& position) const;
};
