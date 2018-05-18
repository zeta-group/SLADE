
#pragma once

#include <cmath>
using std::max;
using std::min;

// This is basically a collection of handy little structs I use a lot,
// with some useful functions for each of them

// 2D Vector
template<typename T> struct Vec2
{
	T x, y;

	// static T zero = T(); // TODO: figure out a nice way to do this

	Vec2<T>() : x{}, y{} {}
	Vec2<T>(T X, T Y) : x{ X }, y{ Y } {}

	void set(T X, T Y)
	{
		x = X;
		y = Y;
	}
	void set(const Vec2<T>& v)
	{
		x = v.x;
		y = v.y;
	}

	Vec2<T> operator+(const Vec2<T>& v) const { return { v.x + x, v.y + y }; }
	Vec2<T> operator-(const Vec2<T>& v) const { return { x - v.x, y - v.y }; }
	Vec2<T> operator*(T num) const { return { x * num, y * num }; }
	Vec2<T> operator/(T num) const
	{
		if (num == 0)
			return Vec2<T>{};

		return Vec2<T>{ x / num, y / num };
	}
	bool     operator==(const Vec2<T>& rhs) const { return (x == rhs.x && y == rhs.y); }
	bool     operator!=(const Vec2<T>& rhs) const { return (x != rhs.x || y != rhs.y); }
	Vec2<T>& operator=(const Vec2<T>& rhs)
	{
		x = rhs.x;
		y = rhs.y;
		return *this;
	}

	double magnitude() const { return std::sqrt((x * x) + (y * y)); }

	Vec2<T> normalized() const
	{
		auto mag = magnitude();
		if (mag == 0)
			return {};

		return { T(x / mag), T(y / mag) };
	}

	void normalize()
	{
		auto mag = magnitude();
		if (mag == 0)
		{
			x = 0;
			y = 0;
		}
		else
		{
			x /= mag;
			y /= mag;
		}
	}

	double distanceTo(const Vec2<T> other) const
	{
		T dist_x = other.x - x;
		T dist_y = other.y - y;

		return std::sqrt((dist_x * dist_x) + (dist_y * dist_y));
	}

	// aka "Manhattan" distance -- just the sum of the vertical and horizontal
	// distance, and an upper bound on the true distance
	T taxicabDistanceTo(const Vec2<T>& other) const
	{
		T dist;
		if (other.x < x)
			dist = x - other.x;
		else
			dist = other.x - x;
		if (other.y < y)
			dist += y - other.y;
		else
			dist += other.y - y;

		return dist;
	}

	T dot(const Vec2<T>& other) const { return x * other.x + y * other.y; }

	T cross(const Vec2<T>& other) const { return (x * other.y) - (y * other.x); }
};

typedef Vec2<int> point2_t;
#define POINT_OUTSIDE point2_t(-1, -1)
typedef Vec2<double> fpoint2_t;

// 3D Vector
template<typename T> struct Vec3
{
	T x, y, z;

	Vec3<T>() : x{}, y{}, z{} {}
	Vec3<T>(T x, T y, T z) : x{ x }, y{ y }, z{ z } {}
	Vec3<T>(const Vec2<T>& p, T z = {}) : x{ p.x }, y{ p.y }, z{ z } {}

	void set(T x, T y, T z)
	{
		this->x = x;
		this->y = y;
		this->z = z;
	}
	void set(const Vec3<T>& p)
	{
		x = p.x;
		y = p.y;
		z = p.z;
	}

	Vec3<T>& operator=(const Vec3<T>& rhs)
	{
		x = rhs.x;
		y = rhs.y;
		z = rhs.z;
		return *this;
	}

	double magnitude() const { return sqrt((x * x) + (y * y) + (z * z)); }

	T dot(const Vec3<T>& vec) const { return x * vec.x + y * vec.y + z * vec.z; }

	Vec3<T> normalized() const
	{
		auto mag = magnitude();
		if (mag == 0)
			return {};
		else
			return { T(x / mag), T(y / mag), T(z / mag) };
	}

	void normalize()
	{
		double mag = magnitude();
		if (mag == 0)
			set(0, 0, 0);
		else
		{
			x /= mag;
			y /= mag;
			z /= mag;
		}
	}

	double distanceTo(const Vec3<T>& point) const
	{
		auto dist_x = point.x - x;
		auto dist_y = point.y - y;
		auto dist_z = point.z - z;

		return sqrt((dist_x * dist_x) + (dist_y * dist_y) + (dist_z * dist_z));
	}

	Vec3<T> operator+(const Vec3<T>& point) const { return { point.x + x, point.y + y, point.z + z }; }

	Vec3<T> operator-(const Vec3<T>& point) const { return { x - point.x, y - point.y, z - point.z }; }

	Vec3<T> operator*(T num) const { return { x * num, y * num, z * num }; }

	Vec3<T> operator/(T num) const
	{
		if (num == 0)
			return {};
		else
			return { x / num, y / num, z / num };
	}

	Vec3<T> cross(const Vec3<T>& p2) const
	{
		Vec3<T> cross_product;

		cross_product.x = ((y * p2.z) - (z * p2.y));
		cross_product.y = ((z * p2.x) - (x * p2.z));
		cross_product.z = ((x * p2.y) - (y * p2.x));

		return cross_product;
	}

	Vec2<T> get2d() const { return { x, y }; }
};
typedef Vec3<double> fpoint3_t;


// ColRGBA: A 32-bit colour definition
// TODO: Possibly move this out to its own file? (along with HSL/LAB colour types)
struct ColRGBA
{
	uint8_t r = 0, g = 0, b = 0, a = 0;
	int16_t index = -1; // -1=not indexed
	char    blend = -1; // 0=normal, 1=additive

	// Constructors
	ColRGBA() = default;
	ColRGBA(uint8_t r, uint8_t g, uint8_t b, uint8_t a = 255, char blend = -1, int16_t index = -1) :
		r{ r },
		g{ g },
		b{ b },
		a{ a },
		index{ index },
		blend{ blend }
	{
	}
	ColRGBA(const ColRGBA& c) : r{ c.r }, g{ c.g }, b{ c.b }, a{ c.a }, index{ c.index }, blend{ c.blend } {}

	// Functions
	void set(uint8_t r, uint8_t g, uint8_t b, uint8_t a = 255, char blend = -1, int16_t index = -1)
	{
		this->r     = r;
		this->g     = g;
		this->b     = b;
		this->a     = a;
		this->blend = blend;
		this->index = index;
	}

	void set(const ColRGBA& colour)
	{
		r     = colour.r;
		g     = colour.g;
		b     = colour.b;
		a     = colour.a;
		blend = colour.blend;
		index = colour.index;
	}

	float fr() const { return (float)r / 255.0f; }
	float fg() const { return (float)g / 255.0f; }
	float fb() const { return (float)b / 255.0f; }
	float fa() const { return (float)a / 255.0f; }

	double dr() const { return (double)r / 255.0; }
	double dg() const { return (double)g / 255.0; }
	double db() const { return (double)b / 255.0; }
	double da() const { return (double)a / 255.0; }

	bool equals(const ColRGBA& rhs, bool alpha = false, bool index = false) const
	{
		bool col_equal = (r == rhs.r && g == rhs.g && b == rhs.b);

		if (index)
			col_equal &= (this->index == rhs.index);
		if (alpha)
			return col_equal && (a == rhs.a);
		else
			return col_equal;
	}

	// Amplify/fade colour components by given amounts
	ColRGBA amp(int R, int G, int B, int A) const
	{
		int nr = r + R;
		int ng = g + G;
		int nb = b + B;
		int na = a + A;

		if (nr > 255)
			nr = 255;
		if (nr < 0)
			nr = 0;
		if (ng > 255)
			ng = 255;
		if (ng < 0)
			ng = 0;
		if (nb > 255)
			nb = 255;
		if (nb < 0)
			nb = 0;
		if (na > 255)
			na = 255;
		if (na < 0)
			na = 0;

		return { (uint8_t)nr, (uint8_t)ng, (uint8_t)nb, (uint8_t)na, blend, -1 };
	}

	// Amplify/fade colour components by factors
	ColRGBA ampf(float fr, float fg, float fb, float fa) const
	{
		int nr = (int)(r * fr);
		int ng = (int)(g * fg);
		int nb = (int)(b * fb);
		int na = (int)(a * fa);

		if (nr > 255)
			nr = 255;
		if (nr < 0)
			nr = 0;
		if (ng > 255)
			ng = 255;
		if (ng < 0)
			ng = 0;
		if (nb > 255)
			nb = 255;
		if (nb < 0)
			nb = 0;
		if (na > 255)
			na = 255;
		if (na < 0)
			na = 0;

		return { (uint8_t)nr, (uint8_t)ng, (uint8_t)nb, (uint8_t)na, blend, -1 };
	}

	void write(uint8_t* ptr) const
	{
		if (ptr)
		{
			ptr[0] = r;
			ptr[1] = g;
			ptr[2] = b;
			ptr[3] = a;
		}
	}

	// Returns a copy of this colour as greyscale (using 'common' component coefficients)
	ColRGBA greyscale() const
	{
		uint8_t l = r * 0.3 + g * 0.59 + b * 0.11;
		return { l, l, l, a, blend };
	}

	// Some basic colours
	static const ColRGBA WHITE;
	static const ColRGBA BLACK;
	static const ColRGBA RED;
	static const ColRGBA GREEN;
	static const ColRGBA BLUE;
	static const ColRGBA YELLOW;
	static const ColRGBA PURPLE;
	static const ColRGBA CYAN;
};

// Convert ColRGBA <-> wxColor
#define WXCOL(rgba) wxColor(rgba.r, rgba.g, rgba.b, rgba.a)
#define COLWX(wxcol) wxcol.Red(), wxcol.Green(), wxcol.Blue()

// ColHSL: Represents a colour in HSL format, generally used for calculations
struct ColHSL
{
	double h = 0., s = 0., l = 0.;

	ColHSL() = default;
	ColHSL(double h, double s, double l) : h{ h }, s{ s }, l{ l } {}
};

// ColLAB: Represents a colour in CIE-L*a*b format, generally used for calculations
struct ColLAB
{
	double l = 0., a = 0., b = 0.;

	ColLAB() = default;
	ColLAB(double l, double a, double b) : l{ l }, a{ a }, b{ b } {}
};

// Rectangle
template<typename T> struct Rect
{
	Vec2<T> tl, br;

	// Constructors
	Rect() = default;
	Rect(const Vec2<T>& tl, const Vec2<T>& br) : tl{ tl }, br{ br } {}
	Rect(T x1, T y1, T x2, T y2) : tl{ x1, y1 }, br{ x2, y2 } {}
	Rect(T x, T y, T width, T height, bool center)
	{
		if (center)
		{
			tl.set(x - (width * 0.5), y - (height * 0.5));
			br.set(x + (width * 0.5), y + (height * 0.5));
		}
		else
		{
			tl.set(x, y);
			br.set(x + width, y + height);
		}
	}

	// Functions
	void set(const Vec2<T>& tl, const Vec2<T>& br)
	{
		this->tl = tl;
		this->br = br;
	}

	void set(int x1, int y1, int x2, int y2)
	{
		tl.set(x1, y1);
		br.set(x2, y2);
	}

	void set(const Rect<T>& rect)
	{
		tl.set(rect.tl);
		br.set(rect.br);
	}

	T x1() const { return tl.x; }
	T y1() const { return tl.y; }
	T x2() const { return br.x; }
	T y2() const { return br.y; }

	T left() const { return std::min<T>(tl.x, br.x); }
	T top() const { return std::min<T>(tl.y, br.y); }
	T right() const { return std::max<T>(br.x, tl.x); }
	T bottom() const { return std::max<T>(br.y, tl.y); }

	T width() const { return br.x - tl.x; }
	T height() const { return br.y - tl.y; }

	T awidth() const { return std::max<T>(br.x, tl.x) - std::min<T>(tl.x, br.x); }
	T aheight() const { return std::max<T>(br.y, tl.y) - std::min<T>(tl.y, br.y); }

	Vec2<T> middle() const { return { left() + T(awidth() * 0.5), top() + T(aheight() * 0.5) }; }

	void expand(T x, T y)
	{
		if (tl.x < br.x)
		{
			tl.x -= x;
			br.x += x;
		}
		else
		{
			tl.x += x;
			br.x -= x;
		}

		if (tl.y < br.y)
		{
			tl.y -= y;
			br.y += y;
		}
		else
		{
			tl.y += y;
			br.y -= y;
		}
	}

	double length() const
	{
		double dist_x = br.x - tl.x;
		double dist_y = br.y - tl.y;

		return sqrt(dist_x * dist_x + dist_y * dist_y);
	}

	bool contains(Vec2<T> point) const
	{
		return (point.x >= left() && point.x <= right() && point.y >= top() && point.y <= bottom());
	}
};

typedef Rect<int>    rect_t;
typedef Rect<double> frect_t;

// Rectangle is not really any different from a 2D segment, but using it to
// mean that can be confusing, so here's an alias.
template<typename T> using Segment = Rect<T>;
typedef Rect<double> fseg2_t;


// Plane: A 3d plane
struct Plane
{
	double a = 0., b = 0., c = 0., d = 0.;

	Plane() = default;
	Plane(double a, double b, double c, double d) : a{ a }, b{ b }, c{ c }, d{ d } {}

	// Construct a flat plane (perpendicular to the z axis) at the given height.
	static Plane flat(double height) { return { 0., 0., 1., height }; }

	bool operator==(const Plane& rhs) const { return a == rhs.a && b == rhs.b && c == rhs.c && d == rhs.d; }
	bool operator!=(const Plane& rhs) const { return !(*this == rhs); }

	void set(double a, double b, double c, double d)
	{
		this->a = a;
		this->b = b;
		this->c = c;
		this->d = d;
	}

	Vec3<double> normal() const
	{
		Vec3<double> norm(a, b, c);
		norm.normalize();
		return norm;
	}

	void normalize()
	{
		Vec3<double> vec(a, b, c);
		double       mag = vec.magnitude();
		a /= mag;
		b /= mag;
		c /= mag;
		d /= mag;
	}

	double heightAt(const Vec2<double>& point) const { return heightAt(point.x, point.y); }
	double heightAt(double x, double y) const { return ((-a * x) + (-b * y) + d) / c; }
};


// BBox: A simple bounding box with related functions
struct BBox
{
	Vec2<double> min;
	Vec2<double> max;

	BBox() = default;

	void reset()
	{
		min.set(0., 0.);
		max.set(0., 0.);
	}

	void extend(double x, double y)
	{
		// Init bbox if it has been reset last
		if (min.x == 0 && min.y == 0 && max.x == 0 && max.y == 0)
		{
			min.set(x, y);
			max.set(x, y);
			return;
		}

		// Extend to fit the point [x,y]
		if (x < min.x)
			min.x = x;
		if (x > max.x)
			max.x = x;
		if (y < min.y)
			min.y = y;
		if (y > max.y)
			max.y = y;
	}

	bool pointWithin(double x, double y) const { return (x >= min.x && x <= max.x && y >= min.y && y <= max.y); }
	bool contains(const Vec2<double>& point) const { return pointWithin(point.x, point.y); }
	bool isWithin(const Vec2<double>& bmin, const Vec2<double>& bmax) const
	{
		return (min.x >= bmin.x && max.x <= bmax.x && min.y >= bmin.y && max.y <= bmax.y);
	}

	bool isValid() const { return ((max.x - min.x > 0) && (max.y - min.y) > 0); }

	Vec2<double> size() const { return { max.x - min.x, max.y - min.y }; }
	double       width() const { return max.x - min.x; }
	double       height() const { return max.y - min.y; }

	Vec2<double> mid() const { return { midX(), midY() }; }
	double       midX() const { return min.x + ((max.x - min.x) * 0.5); }
	double       midY() const { return min.y + ((max.y - min.y) * 0.5); }

	Segment<double> leftSide() const { return { min.x, min.y, min.x, max.y }; }
	Segment<double> rightSide() const { return { max.x, min.y, max.x, max.y }; }
	Segment<double> bottomSide() const { return { min.x, max.y, max.x, max.y }; }
	Segment<double> topSide() const { return { min.x, min.y, max.x, min.y }; }
};

// Formerly key_value_t
typedef std::pair<string, string> StringPair;


// TODO: Move these out to their own file

// patch_header_t: The header of a Doom-format gfx image
struct patch_header_t
{
	short width;
	short height;
	short left;
	short top;
};

// oldpatch_header_t: The header of an alpha/beta Doom-format gfx image
struct oldpatch_header_t
{
	uint8_t width;
	uint8_t height;
	int8_t  left;
	int8_t  top;
};

// jagpic_header_t: The header of a Jaguar Doom-format gfx image
struct jagpic_header_t
{
	short width;
	short height;
	short depth;
	short palshift;
	char  padding[8];
};

// psxpic_header_t: The header of a PSX Doom-format gfx image
struct psxpic_header_t
{
	short left;
	short top;
	short width;
	short height;
};

// rottpatch_header_t: The header of a rott-format gfx image
struct rottpatch_header_t
{
	short origsize;
	short width;
	short height;
	short left;
	short top;
	// short	translevel; // Not all of them have that
};

// imgz_header: The header of a ZDoom imgz image
struct imgz_header_t
{
	uint8_t  magic[4];
	uint16_t width;
	uint16_t height;
	int16_t  left;
	int16_t  top;
	uint8_t  compression;
	uint8_t  reserved[11];
};


// Platform-independent macros to read values from 8-bit arrays or MemChunks
#define READ_L16(a, i) (a[i] + (a[i + 1] << 8))
#define READ_L24(a, i) (a[i] + (a[i + 1] << 8) + (a[i + 2] << 16))
#define READ_L32(a, i) (a[i] + (a[i + 1] << 8) + (a[i + 2] << 16) + (a[i + 3] << 24))
#define READ_B16(a, i) (a[i + 1] + (a[i] << 8))
#define READ_B24(a, i) (a[i + 2] + (a[i + 1] << 8) + (a[i] << 16))
#define READ_B32(a, i) (a[i + 3] + (a[i + 2] << 8) + (a[i + 1] << 16) + (a[i] << 24))
