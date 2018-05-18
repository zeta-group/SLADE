#pragma once

#include "MCOverlay.h"
#include "MapEditor/Edit/Edit3D.h"

class ItemSelection;
class GLTexture;
class MapEditContext;

class QuickTextureOverlay3d : public MCOverlay
{
public:
	QuickTextureOverlay3d(MapEditContext* editor);
	~QuickTextureOverlay3d() = default;

	void setTexture(string_view name);
	void applyTexture();

	void update(long frametime) override;

	void   draw(int width, int height, float fade = 1.0f) override;
	void   drawTexture(unsigned index, double x, double bottom, double size, float fade);

	void close(bool cancel = false) override;

	void doSearch();
	void keyDown(string_view key) override;

	static bool ok(const ItemSelection& sel);

private:
	struct Texture
	{
		GLTexture* texture = nullptr;
		string     name;
		Texture(string_view name) : name{ name.data(), name.size() } {}
	};
	vector<Texture> textures_;
	unsigned        current_index_ = 0;
	string          search_;
	double          anim_offset_ = 0.;
	MapEditContext* editor_;
	int             sel_type_; // 0=flats, 1=walls, 2=both
};
