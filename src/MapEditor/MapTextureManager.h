#pragma once

#include "General/ListenerAnnouncer.h"
#include "Graphics/Palette/Palette.h"
#include "OpenGL/GLTexture.h"

class Archive;
class ArchiveTreeNode;

class MapTextureManager : public Listener
{
public:
	enum class Category
	{
		// Texture categories
		None = 0,
		TEXTUREX,
		TX,
		TEXTURES,
		HIRES
	};

	struct TexInfo
	{
		string   name;
		Category category;
		string   path;
		unsigned index;
		Archive* archive;

		TexInfo(string_view name, Category category, Archive* archive, string_view path = "", unsigned index = 0) :
			name{ name.data(), name.size() },
			category{ category },
			path{ path.data(), path.size() },
			index{ index },
			archive{ archive }
		{
		}
	};

	MapTextureManager(Archive* archive = nullptr) : archive_{ archive } {}
	~MapTextureManager() = default;

	void init();
	void setArchive(Archive* archive);
	void refreshResources();
	void buildTexInfoList();

	GLTexture* texture(string_view name, bool mixed);
	GLTexture* flat(string_view name, bool mixed);
	GLTexture* sprite(string_view name, string_view translation = "", string_view palette = "");
	GLTexture* editorImage(string_view name);
	int        verticalOffset(string_view name) const;

	vector<TexInfo>& allTexturesInfo() { return tex_info_; }
	vector<TexInfo>& allFlatsInfo() { return flat_info_; }

	void onAnnouncement(Announcer* announcer, string_view event_name, MemChunk& event_data) override;

private:
	struct MapTex
	{
		GLTexture* texture = nullptr;
		~MapTex()
		{
			if (texture && texture != &(GLTexture::missingTex()))
				delete texture;
		}
	};
	typedef std::map<string, MapTex> MapTexHashMap;

	void importEditorImages(MapTexHashMap& map, ArchiveTreeNode* dir, string_view path) const;
	void loadResourcePalette();

	Archive*        archive_ = nullptr;
	MapTexHashMap   textures_;
	MapTexHashMap   flats_;
	MapTexHashMap   sprites_;
	MapTexHashMap   editor_images_;
	bool            editor_images_loaded_ = false;
	Palette         palette_;
	vector<TexInfo> tex_info_;
	vector<TexInfo> flat_info_;
};
