
// -----------------------------------------------------------------------------
// SLADE - It's a Doom Editor
// Copyright(C) 2008 - 2017 Simon Judd
//
// Email:       sirjuddington@gmail.com
// Web:         https://slade.mancubus.net
// Filename:    MapTextureManager.cpp
// Description: Handles and keeps track of all OpenGL textures for the map
//              editor - textures, thing sprites, etc.
//
// This program is free software; you can redistribute it and/or modify it
// under the terms of the GNU General Public License as published by the Free
// Software Foundation; either version 2 of the License, or (at your option)
// any later version.
//
// This program is distributed in the hope that it will be useful, but WITHOUT
// ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
// FITNESS FOR A PARTICULAR PURPOSE. See the GNU General Public License for
// more details.
//
// You should have received a copy of the GNU General Public License along with
// this program; if not, write to the Free Software Foundation, Inc.,
// 51 Franklin Street, Fifth Floor, Boston, MA  02110 - 1301, USA.
// -----------------------------------------------------------------------------


// -----------------------------------------------------------------------------
//
// Includes
//
// -----------------------------------------------------------------------------
#include "Main.h"
#include "Archive/ArchiveManager.h"
#include "Game/Configuration.h"
#include "General/Misc.h"
#include "General/ResourceManager.h"
#include "Graphics/CTexture/CTexture.h"
#include "Graphics/SImage/SImage.h"
#include "MainEditor/MainEditor.h"
#include "MainEditor/UI/MainWindow.h"
#include "MapEditContext.h"
#include "MapEditor.h"
#include "MapTextureManager.h"
#include "OpenGL/OpenGL.h"
#include "UI/Controls/PaletteChooser.h"


// -----------------------------------------------------------------------------
//
// Variables
//
// -----------------------------------------------------------------------------
CVAR(Int, map_tex_filter, 0, CVAR_SAVE)

GLTexture::Filter configuredTexFilter()
{
	auto filter = GLTexture::Filter::Linear;
	if (map_tex_filter == 0)
		filter = GLTexture::Filter::NearestLinearMin;
	else if (map_tex_filter == 1)
		filter = GLTexture::Filter::Linear;
	else if (map_tex_filter == 2)
		filter = GLTexture::Filter::LinearMipmap;
	else if (map_tex_filter == 3)
		filter = GLTexture::Filter::NearestMipmap;
	return filter;
}


// -----------------------------------------------------------------------------
//
// MapTextureManager Class Functions
//
// -----------------------------------------------------------------------------


// -----------------------------------------------------------------------------
// Initialises the texture manager
// -----------------------------------------------------------------------------
void MapTextureManager::init()
{
	// Listen to the various managers
	listenTo(theResourceManager);
	listenTo(&App::archiveManager());
	listenTo(theMainWindow->paletteChooser());
	loadResourcePalette();
}

// -----------------------------------------------------------------------------
// Loads the current resource palette
// (depending on open archives and palette toolbar selection)
// -----------------------------------------------------------------------------
void MapTextureManager::loadResourcePalette()
{
	if (theMainWindow->paletteChooser()->globalSelected())
	{
		ArchiveEntry* entry = theResourceManager->getPaletteEntry("PLAYPAL", archive_);
		if (entry)
		{
			palette_.loadMem(entry->data());
			return;
		}
	}

	palette_.copyPalette(theMainWindow->paletteChooser()->selectedPalette());
}

// -----------------------------------------------------------------------------
// Returns the texture matching [name].
// Loads it from resources if necessary.
// If [mixed] is true, flats are also searched if no matching texture is found
// -----------------------------------------------------------------------------
GLTexture* MapTextureManager::texture(string_view name, bool mixed)
{
	// Get texture matching name
	MapTex& mtex = textures_[StrUtil::upper(name)];

	// Get desired filter type
	auto filter = configuredTexFilter();

	// If the texture is loaded
	if (mtex.texture)
	{
		// If the texture filter matches the desired one, return it
		if (mtex.texture->getFilter() == filter)
			return mtex.texture;
		else
		{
			// Otherwise, reload the texture
			if (mtex.texture != &(GLTexture::missingTex()))
				delete mtex.texture;
			mtex.texture = nullptr;
		}
	}

	// Texture not found or unloaded, look for it
	// Palette8bit* pal = getResourcePalette();

	// Look for stand-alone textures first
	ArchiveEntry* etex         = theResourceManager->getTextureEntry(name, "hires", archive_);
	auto          textypefound = CTexture::Type::HiRes;
	if (etex == nullptr)
	{
		etex         = theResourceManager->getTextureEntry(name, "textures", archive_);
		textypefound = CTexture::Type::Texture;
	}
	if (etex)
	{
		SImage image;
		// Get image format hint from type, if any
		if (Misc::loadImageFromEntry(&image, etex))
		{
			mtex.texture = new GLTexture(false);
			mtex.texture->setFilter(filter);
			mtex.texture->loadImage(&image, &palette_);

			// Handle hires texture scale
			if (textypefound == CTexture::Type::HiRes)
			{
				ArchiveEntry* ref = theResourceManager->getTextureEntry(name, "textures", archive_);
				if (ref)
				{
					SImage imgref;
					if (Misc::loadImageFromEntry(&imgref, ref))
					{
						mtex.texture->setScale(
							(double)imgref.width() / (double)image.width(),
							(double)imgref.height() / (double)image.height());
					}
				}
			}
		}
	}

	// Try composite textures then
	CTexture* ctex = theResourceManager->getTexture(name, archive_);
	if (ctex && (!mtex.texture || textypefound == CTexture::Type::Flat))
	{
		SImage image;
		if (ctex->toImage(image, archive_, &palette_, true))
		{
			mtex.texture = new GLTexture(false);
			mtex.texture->setFilter(filter);
			mtex.texture->loadImage(&image, &palette_);
			double sx = ctex->scaleX();
			if (sx == 0)
				sx = 1.0;
			double sy = ctex->scaleY();
			if (sy == 0)
				sy = 1.0;
			mtex.texture->setScale(1.0 / sx, 1.0 / sy);
		}
	}

	// Not found
	if (!mtex.texture)
	{
		// Try flats if mixed
		if (mixed)
			return flat(name, false);

		// Otherwise use missing texture
		mtex.texture = &(GLTexture::missingTex());
	}

	return mtex.texture;
}

// -----------------------------------------------------------------------------
// Returns the flat matching [name].
// Loads it from resources if necessary.
// If [mixed] is true, textures are also searched if no matching flat is found
// -----------------------------------------------------------------------------
GLTexture* MapTextureManager::flat(string_view name, bool mixed)
{
	// Get flat matching name
	MapTex& mtex = flats_[StrUtil::upper(name)];

	// Get desired filter type
	auto filter = configuredTexFilter();

	// If the texture is loaded
	if (mtex.texture)
	{
		// If the texture filter matches the desired one, return it
		if (mtex.texture->getFilter() == filter)
			return mtex.texture;
		else
		{
			// Otherwise, reload the texture
			if (mtex.texture != &(GLTexture::missingTex()))
				delete mtex.texture;
			mtex.texture = nullptr;
		}
	}

	// Flat not found, look for it
	if (!mtex.texture)
	{
		ArchiveEntry* entry = theResourceManager->getTextureEntry(name, "hires", archive_);
		if (!entry)
			entry = theResourceManager->getTextureEntry(name, "flats", archive_);
		if (!entry)
			entry = theResourceManager->getFlatEntry(name, archive_);
		if (entry)
		{
			SImage image;
			if (Misc::loadImageFromEntry(&image, entry))
			{
				mtex.texture = new GLTexture(false);
				mtex.texture->setFilter(filter);
				mtex.texture->loadImage(&image, &palette_);
			}
		}
	}

	// Not found
	if (!mtex.texture)
	{
		// Try textures if mixed
		if (mixed)
			return texture(name, false);

		// Otherwise use missing texture
		mtex.texture = &(GLTexture::missingTex());
	}

	return mtex.texture;
}

// -----------------------------------------------------------------------------
// Returns the sprite matching [name].
// Loads it from resources if necessary.
// Sprite name also supports wildcards (?)
// -----------------------------------------------------------------------------
GLTexture* MapTextureManager::sprite(string_view name, string_view translation, string_view palette)
{
	// Don't bother looking for nameless sprites
	if (name.empty())
		return nullptr;

	// Get sprite matching name
	string hashname = StrUtil::upper(name);
	if (!translation.empty())
		hashname += StrUtil::lower(translation);
	if (!palette.empty())
		hashname += StrUtil::upper(palette);
	MapTex& mtex = sprites_[hashname];

	// Get desired filter type
	auto filter = configuredTexFilter();

	// If the texture is loaded
	if (mtex.texture)
	{
		// If the texture filter matches the desired one, return it
		if (mtex.texture->getFilter() == filter)
			return mtex.texture;
		else
		{
			// Otherwise, reload the texture
			delete mtex.texture;
			mtex.texture = nullptr;
		}
	}

	// Sprite not found, look for it
	bool          found  = false;
	bool          mirror = false;
	SImage        image;
	ArchiveEntry* entry = theResourceManager->getPatchEntry(name, "sprites", archive_);
	if (!entry)
		entry = theResourceManager->getPatchEntry(name, "", archive_);
	if (!entry && name.length() == 8)
	{
		string newname{ name.data(), name.size() };
		newname[4] = name[6];
		newname[5] = name[7];
		newname[6] = name[4];
		newname[7] = name[5];
		entry      = theResourceManager->getPatchEntry(newname, "sprites", archive_);
		if (entry)
			mirror = true;
	}
	if (entry)
	{
		found = true;
		Misc::loadImageFromEntry(&image, entry);
	}
	else // Try composite textures then
	{
		CTexture* ctex = theResourceManager->getTexture(name, archive_);
		if (ctex && ctex->toImage(image, archive_, &palette_, true))
			found = true;
	}

	// We have a valid image either from an entry or a composite texture.
	if (found)
	{
		Palette pal{ palette_ };

		// Apply translation
		if (!translation.empty())
			image.applyTranslation(translation, &pal, true);
		// Apply palette override
		if (!palette.empty())
		{
			ArchiveEntry* newpal = theResourceManager->getPaletteEntry(palette, archive_);
			if (newpal && newpal->size() == 768)
				pal.loadMem(newpal->data());
		}
		// Apply mirroring
		if (mirror)
			image.mirror(false);
		// Turn into GL texture
		mtex.texture = new GLTexture(false);
		mtex.texture->setFilter(filter);
		mtex.texture->setTiling(false);
		mtex.texture->loadImage(&image, &pal);
		return mtex.texture;
	}
	else if (name.back() == '?')
	{
		name.remove_suffix(1);
		string spr_name{ name.data(), name.size() };

		spr_name.push_back('0');
		GLTexture* tex_spr = sprite(spr_name, translation, palette);
		if (!tex_spr)
		{
			spr_name[spr_name.size() - 1] = '1';
			tex_spr                        = sprite(spr_name, translation, palette);
		}
		if (tex_spr)
			return tex_spr;
		if (!tex_spr && name.length() == 5)
		{
			spr_name.assign(name.data(), name.size());
			spr_name += "???";
			for (char chr = 'A'; chr <= ']'; ++chr)
			{
				spr_name[6] = chr;
				spr_name[5] = '0';
				spr_name[7] = '0';
				tex_spr      = sprite(spr_name, translation, palette);
				if (tex_spr)
					return tex_spr;
				spr_name[5] = '1';
				spr_name[7] = '1';
				tex_spr      = sprite(spr_name, translation, palette);
				if (tex_spr)
					return tex_spr;
			}
		}
	}

	return nullptr;
}

// -----------------------------------------------------------------------------
// Detects offset hacks such as that used by the wall torch thing in Heretic
// (type 50).
// If the Y offset is noticeably larger than the sprite height, that means the
// thing is supposed to be rendered above its real position.
// -----------------------------------------------------------------------------
int MapTextureManager::verticalOffset(string_view name) const
{
	// Don't bother looking for nameless sprites
	if (name.empty())
		return 0;

	// Get sprite matching name
	ArchiveEntry* entry = theResourceManager->getPatchEntry(name, "sprites", archive_);
	if (!entry)
		entry = theResourceManager->getPatchEntry(name, "", archive_);
	if (entry)
	{
		SImage image;
		Misc::loadImageFromEntry(&image, entry);
		int h = image.height();
		int o = image.offset().y;
		if (o > h)
			return o - h;
		else
			return 0;
	}

	return 0;
}

// -----------------------------------------------------------------------------
// Loads all editor images (thing icons, etc) from the program resource archive
// -----------------------------------------------------------------------------
void MapTextureManager::importEditorImages(MapTexHashMap& map, ArchiveTreeNode* dir, string_view path) const
{
	SImage image;

	// Go through entries
	for (const auto& entry : dir->entries())
	{
		// Load entry to image
		if (image.open(entry->data()))
		{
			// Create texture in hashmap
			string name{ path.data(), path.size() };
			auto   ename = entry->nameNoExt();
			name.append(ename.data(), ename.size());
			Log::info(4, fmt::sprintf("Loading editor texture %s", name));
			auto& mtex   = map[name];
			mtex.texture = new GLTexture(false);
			mtex.texture->setFilter(GLTexture::Filter::Mipmap);
			mtex.texture->loadImage(&image);
		}
	}

	// Go through subdirs
	for (auto subdir : dir->allChildren())
		importEditorImages(map, (ArchiveTreeNode*)subdir, StrUtil::join(path, subdir->name(), '/'));
}

// -----------------------------------------------------------------------------
// Returns the editor image matching [name]
// -----------------------------------------------------------------------------
GLTexture* MapTextureManager::editorImage(string_view name)
{
	if (!OpenGL::isInitialised())
		return nullptr;

	// Load thing image textures if they haven't already
	if (!editor_images_loaded_)
	{
		// Load all thing images to textures
		ArchiveTreeNode* dir = App::archiveManager().programResourceArchive()->getDir("images");
		if (dir)
			importEditorImages(editor_images_, dir, "");

		editor_images_loaded_ = true;
	}

	return editor_images_[name.to_string()].texture;
}

// -----------------------------------------------------------------------------
// Unloads all cached textures, flats and sprites
// -----------------------------------------------------------------------------
void MapTextureManager::refreshResources()
{
	// Just clear all cached textures
	textures_.clear();
	flats_.clear();
	sprites_.clear();
	theMainWindow->paletteChooser()->setGlobalFromArchive(archive_);
	MapEditor::forceRefresh(true);
	loadResourcePalette();
	buildTexInfoList();
}

// -----------------------------------------------------------------------------
// (Re)builds lists with information about all currently available resource
// textures and flats
// -----------------------------------------------------------------------------
void MapTextureManager::buildTexInfoList()
{
	// Clear
	tex_info_.clear();
	flat_info_.clear();

	// --- Textures ---

	// Composite textures
	vector<TextureResource::Texture*> textures;
	theResourceManager->getAllTextures(textures, App::archiveManager().baseResourceArchive());
	for (auto& texture : textures)
	{
		CTexture* tex    = &texture->tex;
		Archive*  parent = texture->parent;
		if (tex->isExtended())
		{
			if (StrUtil::equalCI(tex->type(), "texture") || StrUtil::equalCI(tex->type(), "walltexture"))
				tex_info_.emplace_back(tex->name(), Category::TEXTURES, parent);
			else if (StrUtil::equalCI(tex->type(), "define"))
				tex_info_.emplace_back(tex->name(), Category::HIRES, parent);
			else if (StrUtil::equalCI(tex->type(), "flat"))
				flat_info_.emplace_back(tex->name(), Category::TEXTURES, parent);
			// Ignore graphics, patches and sprites
		}
		else
			tex_info_.emplace_back(tex->name(), Category::TEXTUREX, parent, "", tex->index() + 1);
	}

	// Texture namespace patches (TX_)
	if (Game::configuration().featureSupported(Game::Feature::TxTextures))
	{
		vector<ArchiveEntry*> patches;
		theResourceManager->getAllPatchEntries(patches, nullptr);
		for (auto& entry : patches)
		{
			if (entry->isInNamespace("textures") || entry->isInNamespace("hires"))
			{
				// Determine texture path if it's in a pk3
				string path = entry->path();
				if (StrUtil::startsWith(path, "/textures/"))
					path.erase(0, 9);
				else if (StrUtil::startsWith(path, "/hires/"))
					path.erase(0, 6);
				else
					path = "";

				tex_info_.emplace_back(entry->nameNoExt(), Category::TX, entry->parent(), path);
			}
		}
	}

	// Flats
	vector<ArchiveEntry*> flats;
	theResourceManager->getAllFlatEntries(flats, nullptr);
	for (auto& flat : flats)
	{
		ArchiveEntry* entry = flat;

		// Determine flat path if it's in a pk3
		string path = entry->path();
		if (StrUtil::startsWith(path, "/flats/") || StrUtil::startsWith(path, "/hires/"))
			path.erase(0, 6);
		else
			path.clear();

		flat_info_.emplace_back(entry->nameNoExt(), Category::None, flat->parent(), path);
	}
}

// -----------------------------------------------------------------------------
// Sets the current archive to [archive], and refreshes all resources
// -----------------------------------------------------------------------------
void MapTextureManager::setArchive(Archive* archive)
{
	archive_ = archive;
	refreshResources();
}

// -----------------------------------------------------------------------------
// Handles announcements from any announcers listened to
// -----------------------------------------------------------------------------
void MapTextureManager::onAnnouncement(Announcer* announcer, string_view event_name, MemChunk& event_data)
{
	// Only interested in the resource manager,
	// archive manager and palette chooser.
	if (announcer != theResourceManager && announcer != theMainWindow->paletteChooser()
		&& announcer != &App::archiveManager())
		return;

	// If the map's archive is being closed,
	// we need to close the map editor
	if (event_name == "archive_closing")
	{
		event_data.seek(0, SEEK_SET);
		int32_t ac_index;
		event_data.read(&ac_index, 4);
		if (App::archiveManager().getArchive(ac_index) == archive_)
		{
			MapEditor::windowWx()->Hide();
			MapEditor::editContext().clearMap();
			archive_ = nullptr;
		}
	}

	// If the resources have been updated
	if (event_name == "resources_updated")
		refreshResources();

	if (event_name == "main_palette_changed")
		refreshResources();
}
