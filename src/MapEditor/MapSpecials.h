#pragma once

#include "SLADEMap/MapLine.h"
#include "SLADEMap/MapSector.h"
#include "SLADEMap/MapThing.h"

class SLADEMap;
class ArchiveEntry;

class MapSpecials
{
public:
	void reset();

	void processMapSpecials(SLADEMap* map) const;
	void processLineSpecial(MapLine* line) const;

	bool getTagColour(int tag, ColRGBA* colour);
	bool getTagFadeColour(int tag, ColRGBA* colour);
	bool tagColoursSet() const;
	bool tagFadeColoursSet() const;
	void updateTaggedSectors(SLADEMap* map);

	// ZDoom
	void processZDoomMapSpecials(SLADEMap* map) const;
	void processZDoomLineSpecial(MapLine* line) const;
	void processACSScripts(ArchiveEntry* entry);

private:
	struct SectorColour
	{
		int    tag    = 0;
		ColRGBA colour = ColRGBA::WHITE;
	};

	typedef std::map<MapVertex*, double> VertexHeightMap;

	vector<SectorColour> sector_colours_;
	vector<SectorColour> sector_fadecolours_;

	void processZDoomSlopes(SLADEMap* map) const;
	void processEternitySlopes(SLADEMap* map) const;

	template<MapSector::SurfaceType>
	void applyPlaneAlign(MapLine* line, MapSector* sector, MapSector* model_sector) const;
	template<MapSector::SurfaceType> void applyLineSlopeThing(SLADEMap* map, MapThing* thing) const;
	template<MapSector::SurfaceType> void applySectorTiltThing(SLADEMap* map, MapThing* thing) const;
	template<MapSector::SurfaceType> void applyVavoomSlopeThing(SLADEMap* map, MapThing* thing) const;
	template<MapSector::SurfaceType>
	void applyVertexHeightSlope(MapSector* target, vector<MapVertex*>& vertices, VertexHeightMap& heights) const;
};
