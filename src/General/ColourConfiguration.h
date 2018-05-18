#pragma once

namespace ColourConfiguration
{
struct Definition
{
	bool   exists = false;
	bool   custom = false;
	string name;
	string group;
	ColRGBA colour;
};

ColRGBA     colour(const string& name);
Definition definition(const string& name);
void       setColour(const string& name, int red = -1, int green = -1, int blue = -1, int alpha = -1, int blend = -1);

double lineHilightWidth();
double lineSelectionWidth();
double flatAlpha();
void   setLineHilightWidth(double mult);
void   setLineSelectionWidth(double mult);
void   setFlatAlpha(double alpha);


bool readConfiguration(MemChunk& mc);
bool writeConfiguration(MemChunk& mc);
bool init();
void loadDefaults();

bool readConfiguration(string_view name);
void getConfigurationNames(vector<string>& names);

void getColourNames(vector<string>& list);
} // namespace ColourConfiguration
