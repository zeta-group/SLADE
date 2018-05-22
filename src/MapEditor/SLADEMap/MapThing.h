#pragma once

#include "MapObject.h"

class MapThing : public MapObject
{
	friend class SLADEMap;

public:
	struct DoomData
	{
		short x;
		short y;
		short angle;
		short type;
		short flags;
	};

	struct HexenData
	{
		short   tid;
		short   x;
		short   y;
		short   z;
		short   angle;
		short   type;
		short   flags;
		uint8_t special;
		uint8_t args[5];
	};

	struct Doom64Data
	{
		short x;
		short y;
		short z;
		short angle;
		short type;
		short flags;
		short tid;
	};

	// UDMF property id strings
	static const string PROP_TYPE;
	static const string PROP_X;
	static const string PROP_Y;
	static const string PROP_ANGLE;

	MapThing(SLADEMap* parent = nullptr) : MapObject{ Type::Thing, parent } {}
	MapThing(double x, double y, short type, SLADEMap* parent = nullptr);
	~MapThing() = default;

	const Vec2<double>& position() const { return position_; }
	double              xPos() const { return position_.x; }
	double              yPos() const { return position_.y; }
	void                setPosition(double x, double y) { position_.set(x, y); }

	fpoint2_t point(Point point) override;

	int type() const { return type_; }
	int angle() const { return angle_; }

	int    intProperty(const string& key) override;
	double floatProperty(const string& key) override;
	void   setIntProperty(const string& key, int value) override;
	void   setFloatProperty(const string& key, double value) override;

	void copy(MapObject* c) override;

	void setAnglePoint(fpoint2_t point);

	void writeBackup(Backup* backup) override;
	void readBackup(Backup* backup) override;

	operator Debuggable() const
	{
		if (!this)
			return Debuggable("<thing NULL>");

		return Debuggable(fmt::format("<thing {}>", index_));
	}

private:
	// Basic data
	int          type_ = 1;
	Vec2<double> position_;
	int          angle_ = 0;
};
