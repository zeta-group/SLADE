
// -----------------------------------------------------------------------------
// SLADE - It's a Doom Editor
// Copyright(C) 2008 - 2017 Simon Judd
//
// Email:       sirjuddington@gmail.com
// Web:         http://slade.mancubus.net
// Filename:    Translation.cpp
// Description: Translation class. Encapsulates a palette translation.
//              A translation contains one or more translation ranges,
//              where each range has an origin palette range and some kind
//              of target range. The target range can be another palette
//              range, a colour gradient or a desaturated colour gradient.
//              eg:
//              Palette range: 0...16 -> 32...48
//                  (in zdoom format: "0:16=32:48")
//              Colour gradient: 0...16 -> Red...Black
//                  (in zdoom format: "0:16=[255,0,0]:[0,0,0]")
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
#include "Translation.h"
#include "Archive/ArchiveManager.h"
#include "MainEditor/MainEditor.h"
#include "Palette/Palette.h"
#include "Utility/StringUtils.h"
#include "Utility/Tokenizer.h"


// -----------------------------------------------------------------------------
//
// Variables
//
// -----------------------------------------------------------------------------
EXTERN_CVAR(Float, col_greyscale_r)
EXTERN_CVAR(Float, col_greyscale_g)
EXTERN_CVAR(Float, col_greyscale_b)


// -----------------------------------------------------------------------------
//
// Constants
//
// -----------------------------------------------------------------------------
namespace
{
// Colours used by the "Ice" translation, based on the Hexen palette
ColRGBA IceRange[16] = {
	ColRGBA(10, 8, 18),     ColRGBA(15, 15, 26),    ColRGBA(20, 16, 36),    ColRGBA(30, 26, 46),
	ColRGBA(40, 36, 57),    ColRGBA(50, 46, 67),    ColRGBA(59, 57, 78),    ColRGBA(69, 67, 88),
	ColRGBA(79, 77, 99),    ColRGBA(89, 87, 109),   ColRGBA(99, 97, 120),   ColRGBA(109, 107, 130),
	ColRGBA(118, 118, 141), ColRGBA(128, 128, 151), ColRGBA(138, 138, 162), ColRGBA(148, 148, 172),
};

enum SpecialBlend
{
	BLEND_ICE         = 0,
	BLEND_DESAT_FIRST = 1,
	BLEND_DESAT_LAST  = 31,
	BLEND_INVERSE,
	BLEND_RED,
	BLEND_GREEN,
	BLEND_BLUE,
	BLEND_GOLD,
	BLEND_INVALID,
};
} // namespace

// -----------------------------------------------------------------------------
//
// Translation Class Functions
//
// -----------------------------------------------------------------------------


// -----------------------------------------------------------------------------
// Translation class destructor
// -----------------------------------------------------------------------------
Translation::~Translation()
{
	for (auto& translation : translations_)
		delete translation;
}

// -----------------------------------------------------------------------------
// Parses a text definition [def] (in zdoom format, detailed here:
// http://zdoom.org/wiki/Translation)
// -----------------------------------------------------------------------------
void Translation::parse(string_view def)
{
	// Test for ZDoom built-in translation
	string test = StrUtil::lower(def);
	if (test == "inverse")
	{
		built_in_name_ = "Inverse";
		return;
	}
	else if (test == "gold")
	{
		built_in_name_ = "Gold";
		return;
	}
	else if (test == "red")
	{
		built_in_name_ = "Red";
		return;
	}
	else if (test == "green")
	{
		built_in_name_ = "Green";
		return;
	}
	else if (test == "blue")
	{
		built_in_name_ = "Blue";
		return;
	}
	else if (test == "ice")
	{
		built_in_name_ = "Ice";
		return;
	}
	else if (StrUtil::startsWith(test, "desaturate,"))
	{
		built_in_name_ = "Desaturate";
		auto amt       = std::stoi(test.substr(11));
		desat_amount_  = std::max<int>(std::min<int>(amt, 31), 1);
		return;
	}

	// Get Hexen tables
	else if (StrUtil::startsWith(test, "\"$@"))
	{
		auto temp = test.substr(3);
		temp.pop_back(); // remove the closing '\"'
		ArchiveEntry* trantbl = App::archiveManager().getResourceEntry(temp);

		if (trantbl && trantbl->size() == 256)
		{
			read(trantbl->dataRaw());
			return;
		}
	}

	else
	{
		// Test for hardcoded predefined translations
		getPredefined(test);

		// Now we're guaranteed to have normal translation strings to parse
		Tokenizer tz;
		tz.setSpecialCharacters(",");
		tz.openString(test);
		parseRange(tz.current().text);
		while (tz.advIfNext(','))
			parseRange(tz.next().text);
	}
}

// -----------------------------------------------------------------------------
// Parses a single translation range
// -----------------------------------------------------------------------------
void Translation::parseRange(string_view range)
{
	// Open definition string for processing w/tokenizer
	Tokenizer tz;
	tz.setSpecialCharacters("[]:%,=#@$");
	tz.openString(range);
	Log::debug(S_FMT("Processing range %s", range));

	// Read original range
	int o_start, o_end;
	tz.current().toInt(o_start);
	if (tz.advIfNext(':'))
		tz.next().toInt(o_end);
	else
		o_end = o_start;

	// Check for =
	if (!tz.advIfNext('='))
		return;

	// Check for reverse origin range
	const bool reverse = (o_start > o_end);

	// Type of translation depends on next token
	if (tz.advIfNext('['))
	{
		// Colour translation
		ColRGBA start, end;

		// Read start colour
		start.r = std::stoi(tz.next().text);
		if (!tz.advIfNext(','))
			return;
		start.g = std::stoi(tz.next().text);
		if (!tz.advIfNext(','))
			return;
		start.b = std::stoi(tz.next().text);

		// Syntax check
		if (!tz.advIfNext(']'))
			return;
		if (!tz.advIfNext(':'))
			return;
		if (!tz.advIfNext('['))
			return;

		// Read end colour
		end.r = std::stoi(tz.next().text);
		if (!tz.advIfNext(','))
			return;
		end.g = std::stoi(tz.next().text);
		if (!tz.advIfNext(','))
			return;
		end.b = std::stoi(tz.next().text);

		if (!tz.advIfNext(']'))
			return;
		
		// Add translation
		if (reverse)
			translations_.push_back(new TransRangeColour{ { o_end, o_start }, end, start });
		else
			translations_.push_back(new TransRangeColour{ { o_start, o_end }, start, end });
	}
	else if (tz.advIfNext('%'))
	{
		// Desat colour translation
		float sr, sg, sb, er, eg, eb;

		if (!tz.advIfNext('['))
			return;

		// Read start colour
		tz.next().toFloat(sr);
		if (!tz.advIfNext(','))
			return;
		tz.next().toFloat(sg);
		if (!tz.advIfNext(','))
			return;
		tz.next().toFloat(sb);

		// Syntax check
		if (!tz.advIfNext(']'))
			return;
		if (!tz.advIfNext(':'))
			return;
		if (!tz.advIfNext('['))
			return;

		// Read end colour
		tz.next().toFloat(er);
		if (!tz.advIfNext(','))
			return;
		tz.next().toFloat(eg);
		if (!tz.advIfNext(','))
			return;
		tz.next().toFloat(eb);

		if (!tz.advIfNext(']'))
			return;

		// Add translation
		if (reverse)
			translations_.push_back(new TransRangeDesat{ { o_end, o_start }, { er, eg, eb }, { sr, sg, sb } });
		else
			translations_.push_back(new TransRangeDesat{ { o_start, o_end }, { sr, sg, sb }, { er, eg, eb } });
	}
	else if (tz.advIfNext('#'))
	{
		// Colourise translation
		ColRGBA colour;

		if (!tz.advIfNext('['))
			return;
		colour.r = std::stoi(tz.next().text);
		if (!tz.advIfNext(','))
			return;
		colour.g = std::stoi(tz.next().text);
		if (!tz.advIfNext(','))
			return;
		colour.b = std::stoi(tz.next().text);
		if (!tz.advIfNext(']'))
			return;

		translations_.push_back(new TransRangeBlend{ { o_start, o_end }, colour });
	}
	else if (tz.advIfNext('@'))
	{
		// Tint translation
		ColRGBA colour;

		uint8_t amount = std::stoi(tz.next().text);
		if (!tz.advIfNext('['))
			return;
		colour.r = std::stoi(tz.next().text);
		if (!tz.advIfNext(','))
			return;
		colour.g = std::stoi(tz.next().text);
		if (!tz.advIfNext(','))
			return;
		colour.b = std::stoi(tz.next().text);
		if (!tz.advIfNext(']'))
			return;

		translations_.push_back(new TransRangeTint{ { o_start, o_end }, colour, amount });
	}
	else if (tz.advIfNext('$'))
	{
		translations_.push_back(new TransRangeSpecial{ { o_start, o_end }, tz.next().text });
	}
	else
	{
		// Palette range translation
		int d_start, d_end;

		// Read range values
		tz.next().toInt(d_start);
		if (!tz.advIfNext(':'))
			d_end = d_start;
		else
			tz.next().toInt(d_end);

		// Add translation
		if (reverse)
			translations_.push_back(new TransRangePalette{ { o_end, o_start }, { d_end, d_start } });
		else
			translations_.push_back(new TransRangePalette{ { o_start, o_end }, { d_start, d_end } });
	}
}

// -----------------------------------------------------------------------------
// Read an entry as a translation table. We're only looking for translations
// where the original range and the target range have the same length, so the
// index value is only ever increased by 1. This should be enough to handle
// Hexen. Asymmetric translations or reversed translations would need a lot
// more heuristics to be handled appropriately. And of course, we're not
// handling any sort of palettized translations to RGB gradients. In short,
// converting a translation string to a translation table would be lossy.
// -----------------------------------------------------------------------------
void Translation::read(const uint8_t* data)
{
	int     i = 0;
	uint8_t val, o_start, o_end, d_start, d_end;
	o_start = 0;
	d_start = val = data[0];
	while (i < 255)
	{
		++i;
		if ((data[i] != (val + 1)) || (i == 255))
		{
			o_end = i - 1;
			d_end = val;
			// Only keep actual translations
			if (o_start != d_start && o_end != d_end)
				translations_.push_back(new TransRangePalette{ { o_start, o_end }, { d_start, d_end } });
			o_start = i;
			d_start = data[i];
		}
		val = data[i];
	}
	LOG_MESSAGE(3, "Translation table analyzed as " + asText());
}

// -----------------------------------------------------------------------------
// Returns a string representation of the translation (in zdoom format)
// -----------------------------------------------------------------------------
string Translation::asText()
{
	string ret;

	if (built_in_name_.empty())
	{
		// Go through translation ranges
		for (auto& translation : translations_)
			ret += S_FMT("\"%s\", ", translation->asText()); // Add range to string

		// If any translations were defined, remove last ", "
		if (!ret.empty())
		{
			ret.pop_back();
			ret.pop_back();
		}
	}
	else
	{
		// ZDoom built-in translation
		ret = built_in_name_;
		if (built_in_name_ == "Desaturate")
			ret += S_FMT(", %d", desat_amount_);
	}

	return ret;
}

// -----------------------------------------------------------------------------
// Clears the translation
// -----------------------------------------------------------------------------
void Translation::clear()
{
	for (auto& translation : translations_)
		delete translation;
	translations_.clear();
	built_in_name_ = "";
	desat_amount_  = 0;
}

// -----------------------------------------------------------------------------
// Copies translation information from [copy]
// -----------------------------------------------------------------------------
void Translation::copy(const Translation& copy)
{
	// Clear current definitions
	clear();

	// Copy translations
	for (auto& translation : copy.translations_)
	{
		switch (translation->type())
		{
		case TransRange::Type::Palette:
			translations_.push_back(new TransRangePalette((TransRangePalette*)translation));
			break;
		case TransRange::Type::Colour:
			translations_.push_back(new TransRangeColour((TransRangeColour*)translation));
			break;
		case TransRange::Type::Desaturate:
			translations_.push_back(new TransRangeDesat((TransRangeDesat*)translation));
			break;
		case TransRange::Type::Blend:
			translations_.push_back(new TransRangeBlend((TransRangeBlend*)translation));
			break;
		case TransRange::Type::Tint: translations_.push_back(new TransRangeTint((TransRangeTint*)translation)); break;
		case TransRange::Type::Special:
			translations_.push_back(new TransRangeSpecial((TransRangeSpecial*)translation));
			break;
		default: break;
		}
	}

	built_in_name_ = copy.built_in_name_;
	desat_amount_  = copy.desat_amount_;
}

// -----------------------------------------------------------------------------
// Returns the translation range at [index]
// -----------------------------------------------------------------------------
TransRange* Translation::getRange(unsigned index)
{
	if (index >= translations_.size())
		return nullptr;
	else
		return translations_[index];
}

// -----------------------------------------------------------------------------
// Apply the translation to the given color
// -----------------------------------------------------------------------------
ColRGBA Translation::translate(const ColRGBA& col, Palette* pal)
{
	ColRGBA colour(col);
	colour.blend = -1;
	if (pal == nullptr)
		pal = MainEditor::currentPalette();
	uint8_t i = (col.index == -1) ? pal->nearestColour(col) : col.index;

	// Handle ZDoom's predefined texture blending:
	// blue, gold, green, red, ice, inverse, and desaturate
	if (!built_in_name_.empty())
	{
		uint8_t type = BLEND_INVALID;
		if (StrUtil::equalCI(built_in_name_, "ice"))
			type = SpecialBlend::BLEND_ICE;
		else if (StrUtil::equalCI(built_in_name_, "inverse"))
			type = SpecialBlend::BLEND_INVERSE;
		else if (StrUtil::equalCI(built_in_name_, "red"))
			type = SpecialBlend::BLEND_RED;
		else if (StrUtil::equalCI(built_in_name_, "green"))
			type = SpecialBlend::BLEND_GREEN;
		else if (StrUtil::equalCI(built_in_name_, "blue"))
			type = SpecialBlend::BLEND_BLUE;
		else if (StrUtil::equalCI(built_in_name_, "gold"))
			type = SpecialBlend::BLEND_GOLD;
		else if (StrUtil::equalCI(built_in_name_, "desaturate"))
			type = desat_amount_; // min 1, max 31 required

		return specialBlend(col, type, pal);
	}

	// Check for perfect palette matches
	bool match = col.equals(pal->colour(i));

	// Go through each translation component
	for (unsigned a = 0; a < nRanges(); a++)
	{
		TransRange* r = translations_[a];

		// Check pixel is within translation range
		if (i < r->start() || i > r->end())
			continue;

		// Only allow exact matches unless the translation applies to all colours
		if (!match && r->start() != 0 && r->end() != 255)
			continue;

		// Palette range translation
		if (r->type() == TransRange::Type::Palette)
		{
			TransRangePalette* tp = (TransRangePalette*)translations_[a];

			// Figure out how far along the range this colour is
			double range_frac = 0;
			if (tp->start() != tp->end())
				range_frac = double(i - tp->start()) / double(tp->end() - tp->start());

			// Determine destination palette index
			uint8_t di = tp->dStart() + range_frac * (tp->dEnd() - tp->dStart());

			// Apply new colour
			ColRGBA c     = pal->colour(di);
			colour.r     = c.r;
			colour.g     = c.g;
			colour.b     = c.b;
			colour.a     = c.a;
			colour.index = di;
		}

		// Colour range
		else if (r->type() == TransRange::Type::Colour)
		{
			TransRangeColour* tc = (TransRangeColour*)r;

			// Figure out how far along the range this colour is
			double range_frac = 0;
			if (tc->start() != tc->end())
				range_frac = double(i - tc->start()) / double(tc->end() - tc->start());

			// Apply new colour
			colour.r     = tc->startColour().r + range_frac * (tc->endColour().r - tc->startColour().r);
			colour.g     = tc->startColour().g + range_frac * (tc->endColour().g - tc->startColour().g);
			colour.b     = tc->startColour().b + range_frac * (tc->endColour().b - tc->startColour().b);
			colour.index = pal->nearestColour(colour);
		}

		// Desaturated colour range
		else if (r->type() == TransRange::Type::Desaturate)
		{
			TransRangeDesat* td = (TransRangeDesat*)r;

			// Get greyscale colour
			ColRGBA gcol = pal->colour(i);
			float  grey = (gcol.r * 0.3f + gcol.g * 0.59f + gcol.b * 0.11f) / 255.0f;

			// Apply new colour
			colour.r     = std::min(255, int((td->rgbStart().r + grey * (td->rgbEnd().r - td->rgbStart().r)) * 255.0f));
			colour.g     = std::min(255, int((td->rgbStart().g + grey * (td->rgbEnd().g - td->rgbStart().g)) * 255.0f));
			colour.b     = std::min(255, int((td->rgbStart().b + grey * (td->rgbEnd().b - td->rgbStart().b)) * 255.0f));
			colour.index = pal->nearestColour(colour);
		}

		// Blended range
		else if (r->type() == TransRange::Type::Blend)
		{
			TransRangeBlend* tc = (TransRangeBlend*)r;

			// Get colours
			ColRGBA blend = tc->colour();

			// Colourise
			float grey = (col.r * col_greyscale_r + col.g * col_greyscale_g + col.b * col_greyscale_b) / 255.0f;
			if (grey > 1.0)
				grey = 1.0;

			// Apply new colour
			colour.r     = blend.r * grey;
			colour.g     = blend.g * grey;
			colour.b     = blend.b * grey;
			colour.index = pal->nearestColour(colour);
		}

		// Tinted range
		else if (r->type() == TransRange::Type::Tint)
		{
			TransRangeTint* tt = (TransRangeTint*)r;

			// Get colours
			ColRGBA tint = tt->colour();

			// Colourise
			float amount  = tt->amount() * 0.01f;
			float inv_amt = 1.0f - amount;

			// Apply new colour
			colour.r = col.r * inv_amt + tint.r * amount;
			;
			colour.g     = col.g * inv_amt + tint.g * amount;
			colour.b     = col.b * inv_amt + tint.b * amount;
			colour.index = pal->nearestColour(colour);
		}

		// Special range
		else if (r->type() == TransRange::Type::Special)
		{
			auto*   ts   = (TransRangeSpecial*)translations_[a];
			string  spec = ts->getSpecial();
			uint8_t type = BLEND_INVALID;
			if (StrUtil::equalCI(spec, "ice"))
				type = SpecialBlend::BLEND_ICE;
			else if (StrUtil::equalCI(spec, "inverse"))
				type = SpecialBlend::BLEND_INVERSE;
			else if (StrUtil::equalCI(spec, "red"))
				type = SpecialBlend::BLEND_RED;
			else if (StrUtil::equalCI(spec, "green"))
				type = SpecialBlend::BLEND_GREEN;
			else if (StrUtil::equalCI(spec, "blue"))
				type = SpecialBlend::BLEND_BLUE;
			else if (StrUtil::equalCI(spec, "gold"))
				type = SpecialBlend::BLEND_GOLD;
			else if (StrUtil::startsWithCI(spec, "desat"))
			{
				// This relies on SpecialBlend::1 to ::31 being occupied with desat
				auto num = std::stoi(spec.substr(spec.size() - 2));
				if (num < 32 && num > 0)
					type = num;
			}
			return specialBlend(col, type, pal);
		}
	}
	return colour;
}

// -----------------------------------------------------------------------------
// Apply one of the special colour blending modes from ZDoom:
// Desaturate, Ice, Inverse, Blue, Gold, Green, Red.
// -----------------------------------------------------------------------------
ColRGBA Translation::specialBlend(const ColRGBA& col, uint8_t type, Palette* pal)
{
	// Abort just in case
	if (type == SpecialBlend::BLEND_INVALID)
		return col;

	ColRGBA colour = col;

	// Get greyscale using ZDoom formula
	float grey = (col.r * 77 + col.g * 143 + col.b * 37) / 256.f;

	// Ice is a special case as it uses a colour range derived
	// from the Hexen palette instead of a linear gradient.
	if (type == BLEND_ICE)
	{
		// Determine destination palette index in IceRange
		uint8_t di   = MIN(((int)grey >> 4), 15);
		ColRGBA  c    = IceRange[di];
		colour.r     = c.r;
		colour.g     = c.g;
		colour.b     = c.b;
		colour.a     = c.a;
		colour.index = pal->nearestColour(colour);
	}
	// Desaturated blending goes from no effect to nearly fully desaturated
	else if (type >= BLEND_DESAT_FIRST && type <= BLEND_DESAT_LAST)
	{
		float amount = type - 1; // get value between 0 and 30

		colour.r     = MIN(255, int((colour.r * (31 - amount) + grey * amount) / 31));
		colour.g     = MIN(255, int((colour.g * (31 - amount) + grey * amount) / 31));
		colour.b     = MIN(255, int((colour.b * (31 - amount) + grey * amount) / 31));
		colour.index = pal->nearestColour(colour);
	}
	// All others are essentially preset desaturated translations
	else
	{
		float sr, sg, sb, er, eg, eb;      // start and end ranges
		sr = sg = sb = er = eg = eb = 0.0; // let's init them to 0.

		switch (type)
		{
		case BLEND_INVERSE:
			// inverted grayscale: Doom invulnerability, Strife sigil
			// start white, ends black
			sr = sg = sb = 1.0;
			break;
		case BLEND_GOLD:
			// Heretic invulnerability
			// starts black, ends reddish yellow
			er = 1.5;
			eg = 0.75;
			break;
		case BLEND_RED:
			// Skulltag doomsphere
			// starts black, ends red
			er = 1.5;
			break;
		case BLEND_GREEN:
			// Skulltag guardsphere
			// starts black, ends greenish-white
			er = 1.25;
			eg = 1.5;
			eb = 1.0;
			break;
		case BLEND_BLUE:
			// Hacx invulnerability
			// starts black, ends blue
			eb = 1.5;
		default: break;
		}
		// Apply new colour
		colour.r     = MIN(255, int(sr + grey * (er - sr)));
		colour.g     = MIN(255, int(sg + grey * (eg - sg)));
		colour.b     = MIN(255, int(sb + grey * (eb - sb)));
		colour.index = pal->nearestColour(colour);
	}
	return colour;
}

// -----------------------------------------------------------------------------
// Adds a new translation range of [type] at [pos] in the list
// -----------------------------------------------------------------------------
void Translation::addRange(TransRange::Type type, int pos)
{
	// Create range
	TransRange* tr;
	switch (type)
	{
	case TransRange::Type::Colour: tr = new TransRangeColour({ 0, 0 }); break;
	case TransRange::Type::Desaturate: tr = new TransRangeDesat({ 0, 0 }); break;
	case TransRange::Type::Blend: tr = new TransRangeBlend({ 0, 0 }); break;
	case TransRange::Type::Tint: tr = new TransRangeTint({ 0, 0 }); break;
	case TransRange::Type::Special: tr = new TransRangeSpecial({ 0, 0 }); break;
	default: tr = new TransRangePalette({ 0, 0 }, { 0, 0 }); break;
	};

	// Add to list
	if (pos < 0 || pos >= (int)translations_.size())
		translations_.push_back(tr);
	else
		translations_.insert(translations_.begin() + pos, tr);
}

// -----------------------------------------------------------------------------
// Removes the translation range at [pos]
// -----------------------------------------------------------------------------
void Translation::removeRange(int pos)
{
	// Check position
	if (pos < 0 || pos >= (int)translations_.size())
		return;

	// Remove it
	delete translations_[pos];
	translations_.erase(translations_.begin() + pos);
}

// -----------------------------------------------------------------------------
// Swaps the translation range at [pos1] with the one at [pos2]
// -----------------------------------------------------------------------------
void Translation::swapRanges(int pos1, int pos2)
{
	// Check positions
	if (pos1 < 0 || pos2 < 0 || pos1 >= (int)translations_.size() || pos2 >= (int)translations_.size())
		return;

	// Swap them
	TransRange* temp    = translations_[pos1];
	translations_[pos1] = translations_[pos2];
	translations_[pos2] = temp;
}

// -----------------------------------------------------------------------------
// Translation::getPredefined
//
// Replaces a hardcoded translation name [def] with its transcription
// -----------------------------------------------------------------------------
bool Translation::getPredefined(string& def)
{
	// Some hardcoded translations from ZDoom, used in config files
	static std::map<string, string> predefined = {
		{ "\"doom0\"", "\"112:127=96:111\"" },
		{ "\"doom1\"", "\"112:127=64:79\"" },
		{ "\"doom2\"", "\"112:127=32:47\"" },
		{ "\"doom3\"", "\"112:127=88:103\"" },
		{ "\"doom4\"", "\"112:127=56:71\"" },
		{ "\"doom5\"", "\"112:127=176:191\"" },
		{ "\"doom6\"", "\"112:127=192:207\"" },
		{ "\"heretic0\"", "\"225:240=114:129\"" },
		{ "\"heretic1\"", "\"225:240=145:160\"" },
		{ "\"heretic2\"", "\"225:240=190:205\"" },
		{ "\"heretic3\"", "\"225:240=67:82\"" },
		{ "\"heretic4\"", "\"225:240=9:24\"" },
		{ "\"heretic5\"", "\"225:240=74:89\"" },
		{ "\"heretic6\"", "\"225:240=150:165\"" },
		{ "\"heretic7\"", "\"225:240=192:207\"" },
		{ "\"heretic8\"", "\"225:240=95:110\"" },
		{ "\"strife0\"", "\"32:63=0:31\", \"128:143=64:79\", \"241:246=224:229\", \"247:251=241:245\"" },
		{ "\"strife1\"", "\"32:63=0:31\", \"128:143=176:191\"" },
		{ "\"strife2\"", "\"32:47=208:223\", \"48:63=208:223\", \"128:143=16:31\"" },
		{ "\"strife3\"", "\"32:47=208:223\", \"48:63=208:223\", \"128:143=48:63\"" },
		{ "\"strife4\"", "\"32:63=0:31\", \"80:95=128:143\", \"128:143=80:95\", \"192:223=160:191\"" },
		{ "\"strife5\"", "\"32:63=0:31\", \"80:95=16:31\", \"128:143=96:111\", \"192:223=32:63\"" },
		{ "\"strife6\"", "\"32:63=0:31\", \"80:95=64:79\", \"128:143=144:159\", \"192=1\", \"193:223=1:31\"" },
		{ "\"chex0\"", "\"192:207=112:127\"" },
		{ "\"chex1\"", "\"192:207=96:111\"" },
		{ "\"chex2\"", "\"192:207=64:79\"" },
		{ "\"chex3\"", "\"192:207=32:47\"" },
		{ "\"chex4\"", "\"192:207=88:103\"" },
		{ "\"chex5\"", "\"192:207=56:71\"" },
		{ "\"chex6\"", "\"192:207=176:191\"" },
		// Some more from Eternity
		{ "\"tomato\"",
		  "\"112:113=171:171\", \"114:114=172:172\", \"115:122=173:187\", \"123:124=188:189\", \"125:126=45:47\", "
		  "\"127:127=1:1\"" },
		{ "\"dirt\"",
		  "\"112:117=128:133\", \"118:120=135:137\", \"121:123=139:143\", \"124:125=237:239\", \"126:127=1:2\"" },
		{ "\"blue\"", "\"112:121=197:206\", \"122:127=240:245" },
		{ "\"gold\"",
		  "\"112:113=160:160\", \"114:119=161:166\", \"120:123=236:239\", \"124:125=1:2\", \"126:127=7:8\"" },
		{ "\"sea\"", "\"112:112=91:91\", \"113:114=94:95\", \"115:122=152:159\", \"123:126=9:12\", \"127:127=8:8\"" },
		{ "\"black\"", "\"112:112=101:101\", \"113:121=103:111\", \"122:125=5:8\", \"126:127=0:0\"" },
		{ "\"purple\"", "\"112:113=4:4\", \"114:115=170:170\", \"116:125=250:254\", \"126:127=46:46\"" },
		{ "\"vomit\"", "\"112:119=209:216\", \"120:121=218:220\", \"122:124=69:75\", \"125:127=237:239\"" },
		{ "\"pink\"",
		  "\"112:113=16:17\", \"114:117=19:25\", \"118:119=27:28\", \"120:124=30:38\", \"125:126=41:43\", "
		  "\"127:127=46:46\"" },
		{ "\"cream\"",
		  "\"112:112=4:4\", \"113:118=48:63\", \"119:119=65:65\", \"120:124=68:76\", \"125:126=77:79\", "
		  "\"127:127=1:1\"" },
		{ "\"white\"", "\"112:112=4:4\", \"113:115=80:82\", \"116:117=84:86\", \"118:120=89:93\", \"121:127=96:108\"" },
		// And why not this one too
		{ "\"stealth\"", "\"0:255=%[0.00,0.00,0.00]:[1.31,0.84,0.84]\"" }
	};

	if (!predefined[def].empty())
	{
		def = predefined[def];
		return true;
	}

	return false;
}
