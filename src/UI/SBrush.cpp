
// -----------------------------------------------------------------------------
// SLADE - It's a Doom Editor
// Copyright(C) 2008 - 2017 Simon Judd
//
// Email:       sirjuddington@gmail.com
// Web:         http://slade.mancubus.net
// Filename:    SBrush.cpp
// Description: SBrush class. Handles pixel painting for GfxCanvas.
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
#include "General/SAction.h"
#include "Graphics/SImage/SImage.h"
#include "SBrush.h"
#include "Utility/StringUtils.h"


// -----------------------------------------------------------------------------
//
// Variables
//
// -----------------------------------------------------------------------------
SBrushManager* SBrushManager::instance_ = nullptr;


// -----------------------------------------------------------------------------
//
// SBrush Class Functions
//
// -----------------------------------------------------------------------------


// -----------------------------------------------------------------------------
// SBrush class constructor
// -----------------------------------------------------------------------------
SBrush::SBrush(string_view name) : name_{ name.data(), name.size() }, icon_{ StrUtil::afterFirst(name, '_') }
{
	Archive* res = App::archiveManager().programResourceArchive();
	if (res == nullptr)
		return;
	ArchiveEntry* file = res->entryAtPath(fmt::sprintf("icons/general/%s.png", icon_));
	if (file == nullptr || file->size() == 0)
	{
		Log::info(2, fmt::sprintf("error, no file at icons/general/%s.png", icon_));
		return;
	}
	if (!image_.open(file->data(), 0, "png"))
	{
		Log::info(2, fmt::sprintf("couldn't load image data for icons/general/%s.png", icon_));
		return;
	}
	image_.convertAlphaMap(SImage::AlphaSource::Alpha);
	center_.x = image_.width() >> 1;
	center_.y = image_.height() >> 1;

	theBrushManager->add(this);
}

uint8_t SBrush::getPixel(int x, int y) const
{
	x += center_.x;
	y += center_.y;
	if (image_.isValid() && x >= 0 && x < image_.width() && y >= 0 && y < image_.height())
		return image_.pixelIndexAt((unsigned)x, (unsigned)y);
	return 0;
}

SBrushManager::SBrushManager()
{
	brushes_.clear();
}

SBrushManager::~SBrushManager()
{
	for (size_t i = brushes_.size(); i > 0; --i)
	{
		delete brushes_[i - 1];
		brushes_[i - 1] = nullptr;
	}
	brushes_.clear();
}

SBrush* SBrushManager::get(string_view name)
{
	for (auto& brush : brushes_)
		if (StrUtil::equalCI(name, brush->getName()))
			return brush;

	return nullptr;
}

void SBrushManager::add(SBrush* brush)
{
	brushes_.push_back(brush);
}

bool SBrushManager::initBrushes()
{
	new SBrush("pgfx_brush_sq_1");
	new SBrush("pgfx_brush_sq_3");
	new SBrush("pgfx_brush_sq_5");
	new SBrush("pgfx_brush_sq_7");
	new SBrush("pgfx_brush_sq_9");
	new SBrush("pgfx_brush_ci_5");
	new SBrush("pgfx_brush_ci_7");
	new SBrush("pgfx_brush_ci_9");
	new SBrush("pgfx_brush_di_3");
	new SBrush("pgfx_brush_di_5");
	new SBrush("pgfx_brush_di_7");
	new SBrush("pgfx_brush_di_9");
	new SBrush("pgfx_brush_pa_a");
	new SBrush("pgfx_brush_pa_b");
	new SBrush("pgfx_brush_pa_c");
	new SBrush("pgfx_brush_pa_d");
	new SBrush("pgfx_brush_pa_e");
	new SBrush("pgfx_brush_pa_f");
	new SBrush("pgfx_brush_pa_g");
	new SBrush("pgfx_brush_pa_h");
	new SBrush("pgfx_brush_pa_i");
	new SBrush("pgfx_brush_pa_j");
	new SBrush("pgfx_brush_pa_k");
	new SBrush("pgfx_brush_pa_l");
	new SBrush("pgfx_brush_pa_m");
	new SBrush("pgfx_brush_pa_n");
	new SBrush("pgfx_brush_pa_o");
	return true;
}
