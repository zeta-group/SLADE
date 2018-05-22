#pragma once

#include "MapObject.h"
#include "Utility/Polygon2D.h"

class MapSide;
class MapLine;
class MapVertex;

class MapSector : public MapObject
{
	friend class SLADEMap;
	friend class MapSide;

public:
	enum SurfaceType
	{
		Floor = 1,
		Ceiling
	};

	struct Surface
	{
		string  texture;
		int     height = 0;
		Plane plane  = { 0., 0., 1., 0. };
	};

	struct DoomData
	{
		short f_height;
		short c_height;
		char  f_tex[8];
		char  c_tex[8];
		short light;
		short special;
		short tag;
	};

	struct Doom64Data
	{
		short    f_height;
		short    c_height;
		uint16_t f_tex;
		uint16_t c_tex;
		uint16_t color[5];
		short    special;
		short    tag;
		uint16_t flags;
	};

	// UDMF property id strings
	static const string PROP_TEX_FLOOR;
	static const string PROP_TEX_CEILING;
	static const string PROP_HEIGHT_FLOOR;
	static const string PROP_HEIGHT_CEILING;
	static const string PROP_LIGHT;
	static const string PROP_SPECIAL;
	static const string PROP_ID;
	static const string PROP_LIGHT_FLOOR;
	static const string PROP_LIGHT_CEILING;
	static const string PROP_LIGHT_FLOOR_ABSOLUTE;
	static const string PROP_LIGHT_CEILING_ABSOLUTE;
	static const string PROP_LIGHT_COLOR;
	static const string PROP_FADE_COLOR;
	static const string PROP_PAN_X_FLOOR;
	static const string PROP_PAN_Y_FLOOR;
	static const string PROP_SCALE_X_FLOOR;
	static const string PROP_SCALE_Y_FLOOR;
	static const string PROP_SCALE_X_CEILING;
	static const string PROP_SCALE_Y_CEILING;
	static const string PROP_ROTATION_FLOOR;
	static const string PROP_ROTATION_CEILING;

	MapSector(SLADEMap* parent = nullptr);
	MapSector(string_view f_tex, string_view c_tex, SLADEMap* parent = nullptr);
	~MapSector() = default;

	void copy(MapObject* s) override;

	const Surface& floor() const { return floor_; }
	const Surface& ceiling() const { return ceiling_; }
	short          light() const { return light_; }
	short          special() const { return special_; }
	short          id() const { return id_; }

	string stringProperty(const string& key) override;
	int    intProperty(const string& key) override;

	void setStringProperty(const string& key, const string& value) override;
	void setFloatProperty(const string& key, double value) override;
	void setIntProperty(const string& key, int value) override;

	void setFloorTexture(string_view tex);
	void setCeilingTexture(string_view tex);
	void setFloorHeight(short height);
	void setCeilingHeight(short height);
	void setFloorPlane(const Plane& p);
	void setCeilingPlane(const Plane& p);

	template<SurfaceType p> short   planeHeight();
	template<SurfaceType p> Plane plane();
	template<SurfaceType p> void    setPlane(const Plane& plane);

	fpoint2_t         point(Point point) override;
	void              resetBBox() { bbox_.reset(); }
	const BBox&     boundingBox();
	vector<MapSide*>& connectedSides() { return connected_sides_; }
	void              resetPolygon() { poly_needsupdate_ = true; }
	Polygon2D*        polygon();
	bool              isWithin(fpoint2_t point);
	double            distanceTo(fpoint2_t point, double maxdist = -1);
	bool              getLines(vector<MapLine*>& list);
	bool              getVertices(vector<MapVertex*>& list);
	bool              getVertices(vector<MapObject*>& list);
	uint8_t           lightAt(int where = 0);
	void              changeLight(int amount, int where = 0);
	ColRGBA            colourAt(int where = 0, bool fullbright = false);
	ColRGBA            fogColour();
	long              geometryUpdatedTime() const { return geometry_updated_; }

	void connectSide(MapSide* side);
	void disconnectSide(MapSide* side);

	void updateBBox();

	void writeBackup(Backup* backup) override;
	void readBackup(Backup* backup) override;

	operator Debuggable() const
	{
		if (!this)
			return Debuggable("<sector NULL>");

		return Debuggable(fmt::format("<sector {}>", index_));
	}

private:
	// Basic data
	Surface floor_;
	Surface ceiling_;
	short   light_   = 0;
	short   special_ = 0;
	short   id_      = 0;

	// Internal info
	vector<MapSide*> connected_sides_;
	BBox           bbox_;
	Polygon2D        polygon_;
	bool             poly_needsupdate_ = true;
	long             geometry_updated_ = 0;
	fpoint2_t        text_point_;

	void setGeometryUpdated();
	void findTextPoint();
};

// Note: these MUST be inline, or the linker will complain
template<> inline short MapSector::planeHeight<MapSector::Floor>()
{
	return floor_.height;
}
template<> inline short MapSector::planeHeight<MapSector::Ceiling>()
{
	return ceiling_.height;
}
template<> inline Plane MapSector::plane<MapSector::Floor>()
{
	return floor_.plane;
}
template<> inline Plane MapSector::plane<MapSector::Ceiling>()
{
	return ceiling_.plane;
}
template<> inline void MapSector::setPlane<MapSector::Floor>(const Plane& plane)
{
	setFloorPlane(plane);
}
template<> inline void MapSector::setPlane<MapSector::Ceiling>(const Plane& plane)
{
	setCeilingPlane(plane);
}
