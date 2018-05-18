#pragma once


class SAction;
class SBrush
{
public:
	// Constructor
	SBrush(string_view name);
	// Destructor
	~SBrush() = default;

	// Returns an action ready to be inserted in a menu or toolbar
	SAction getAction();

	// Returns the brush's name ("pgfx_brush_xyz")
	const string& getName() const { return name_; }

	// Returns the brush's icon name ("brush_xyz")
	const string& getIcon() const { return icon_; }

	// Returns intensity of how much this pixel is affected by the brush; [0, 0] is the brush's center
	uint8_t getPixel(int x, int y) const;

private:
	// The cursor graphic
	SImage    image_;
	string    name_;
	string    icon_;
	Vec2<int> center_;
};

class SBrushManager
{
public:
	// Constructor
	SBrushManager();
	// Destructor
	~SBrushManager();

	// Get a brush from its name
	SBrush* get(string_view name);

	// Add a brush
	void add(SBrush* brush);

	// Single-instance implementation
	static SBrushManager* getInstance()
	{
		if (!instance_)
			instance_ = new SBrushManager();
		return instance_;
	}

	// Init brushes
	static bool initBrushes();

private:
	// The collection of SBrushes
	vector<SBrush*> brushes_;

	// Single-instance implementation
	static SBrushManager* instance_;
};

#define theBrushManager SBrushManager::getInstance()
