#pragma once

#include "Archive/Archive.h"
#include "OGLCanvas.h"
#include "OpenGL/GLTexture.h"

class MapPreviewCanvas : public OGLCanvas
{
public:
	// Structs for basic map features
	struct Vertex
	{
		double x;
		double y;
		Vertex(double x, double y) : x{ x }, y{ y } {}
	};

	struct Line
	{
		unsigned v1;
		unsigned v2;
		bool     twosided = false;
		bool     special  = false;
		bool     macro    = false;
		bool     segment  = false;
		Line(unsigned v1, unsigned v2) : v1{ v1 }, v2{ v2 } {}
	};

	struct Thing
	{
		double x;
		double y;
	};

	MapPreviewCanvas(wxWindow* parent) : OGLCanvas(parent, -1), tex_thing_{ false } {}
	~MapPreviewCanvas() = default;

	void addVertex(double x, double y);
	void addLine(unsigned v1, unsigned v2, bool twosided, bool special, bool macro = false);
	void addThing(double x, double y);
	bool openMap(Archive::MapDesc map);
	bool readVertices(ArchiveEntry* map_head, ArchiveEntry* map_end, int map_format);
	bool readLines(ArchiveEntry* map_head, ArchiveEntry* map_end, int map_format);
	bool readThings(ArchiveEntry* map_head, ArchiveEntry* map_end, int map_format);
	void clearMap();
	void showMap();
	void draw() override;
	void createImage(ArchiveEntry& ae, int width, int height);

	unsigned nVertices();
	unsigned nSides() const { return n_sides_; }
	unsigned nLines() const { return lines_.size(); }
	unsigned nSectors() const { return n_sectors_; }
	unsigned nThings() const { return things_.size(); }
	unsigned totalWidth();
	unsigned totalHeight();

private:
	vector<Vertex> verts_;
	vector<Line>   lines_;
	vector<Thing>  things_;
	unsigned       n_sides_   = 0;
	unsigned       n_sectors_ = 0;
	double         zoom_      = 1.;
	Vec2<double>   offset_;
	GLTexture      tex_thing_;
	bool           tex_loaded_ = false;
};
