
Palette getPaletteFromIndex(int index)
{
	// Make a copy of the palette so we can't mess with it in scripts
	Palette pal;
	pal.copyPalette(App::paletteManager()->getPalette(index));
	return pal;
}

Palette getPaletteFromName(const char* name)
{
	// Make a copy of the palette so we can't mess with it in scripts
	Palette pal;
	pal.copyPalette(App::paletteManager()->getPalette(name));
	return pal;
}

void registerGraphicsNamespace(sol::state& lua)
{
	// Graphics enums
	auto graphics = lua.create_named_table("Graphics");
	graphics.new_enum(
		"ColourMatch",
		"Default",	Palette::ColourMatch::Default,
		"Old",		Palette::ColourMatch::Old,
		"RGB",		Palette::ColourMatch::RGB,
		"HSL",		Palette::ColourMatch::HSL,
		"C76",		Palette::ColourMatch::C76,
		"C94",		Palette::ColourMatch::C94,
		"C2K",		Palette::ColourMatch::C2K
	);

	graphics.new_enum(
		"SpecialBlend",
		"Ice",			SpecialBlend::BLEND_ICE,
		"DesatFirst",	SpecialBlend::BLEND_DESAT_FIRST,
		"DesatLast",	SpecialBlend::BLEND_DESAT_LAST,
		"Inverse",		SpecialBlend::BLEND_INVERSE,
		"Red",			SpecialBlend::BLEND_RED,
		"Green",		SpecialBlend::BLEND_GREEN,
		"Blue",			SpecialBlend::BLEND_BLUE,
		"Gold",			SpecialBlend::BLEND_GOLD
	);


	// Graphics functions

	graphics.set_function("globalPalette", []()
	{
		// Make a copy of the global palette so we can't mess with it in scripts
		Palette global;
		global.copyPalette(App::paletteManager()->globalPalette());
		return global;
	});

	graphics.set_function("palette", sol::overload(
		&getPaletteFromIndex,
		&getPaletteFromName
	));
}

void registerPalette(sol::state& lua)
{
	lua.new_usertype<Palette>(
		"Palette",

		// No constructor
		"new", sol::no_constructor,

		// Properties
		"colours",		sol::property(&Palette::colours),
		"transIndex",	sol::property(&Palette::transIndex),
		
		// Functions
		// load (mem, file)
		// save (mem, file)
		"setColour",		&Palette::setColour,
		"copyPalette",		&Palette::copyPalette,
		"findColour",		&Palette::findColour,
		"nearestColour", sol::overload(
			[](Palette& self, rgba_t colour) { return self.nearestColour(colour); },
			&Palette::nearestColour
		),
		"countColours",		&Palette::countColours,
		"applyTranslation",	&Palette::applyTranslation,
		"colourise",		&Palette::colourise,
		"tint",				&Palette::tint,
		"saturate",			&Palette::saturate,
		"illuminate",		&Palette::illuminate,
		"shift",			&Palette::shift,
		"invert",			&Palette::invert,
		"gradient",			&Palette::setGradient
	);

	sol::table type = lua["Palette"];
	type.new_enum(
		"Format",
		"Raw", Palette::Format::Raw,
		"Image", Palette::Format::Image,
		"CSV", Palette::Format::CSV,
		"JASC", Palette::Format::JASC,
		"GIMP", Palette::Format::GIMP
	);
}

void registerTranslation(sol::state& lua)
{
	// TODO: Ranges

	lua.new_usertype<Translation>(
		"Translation",

		// Functions
		"parse", &Translation::parse,
		"readTable", [](Translation& self, const std::string& data)
		{
			self.read((const uint8_t*)data.data());
		},

		"asText", &Translation::asText,
		"clear", &Translation::clear,
		"translate", sol::overload(
			[](Translation& self, rgba_t col) { return self.translate(col); },
			[](Translation& self, rgba_t col, Palette* pal) { return self.translate(col, pal); }
		),
		"specialBlend", sol::overload(
			[](Translation& self, rgba_t col, int type) { return self.specialBlend(col, type); },
			[](Translation& self, rgba_t col, int type, Palette* pal) { return self.specialBlend(col, type, pal); }
		)
	);
}

void registerGraphicsTypes(sol::state& lua)
{
	registerPalette(lua);
	registerTranslation(lua);
}
