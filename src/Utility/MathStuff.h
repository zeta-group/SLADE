#pragma once

#include "Structs.h"

#define PI 3.1415926535897932384

namespace MathStuff
{
double    clamp(double val, double min, double max);
int       floor(double val);
int       ceil(double val);
int       round(double val);
double    distance(const fpoint2_t& p1, const fpoint2_t& p2);
double    distance3d(const fpoint3_t& p1, const fpoint3_t& p2);
double    lineSide(const fpoint2_t& point, const fseg2_t& line);
fpoint2_t closestPointOnLine(const fpoint2_t& point, const fseg2_t& line);
double    distanceToLine(const fpoint2_t& point, const fseg2_t& line);
double    distanceToLineFast(const fpoint2_t& point, const fseg2_t& line);
bool      linesIntersect(const fseg2_t& line1, const fseg2_t& line2, fpoint2_t& out);
double    distanceRayLine(
	   const fpoint2_t& ray_origin,
	   const fpoint2_t& ray_dir,
	   const fpoint2_t& seg1,
	   const fpoint2_t& seg2);
double    angle2DRad(const fpoint2_t& p1, const fpoint2_t& p2, const fpoint2_t& p3);
fpoint2_t rotatePoint(const fpoint2_t& origin, const fpoint2_t& point, double angle);
fpoint3_t rotateVector3D(const fpoint3_t& vector, const fpoint3_t& axis, double angle);
double    degToRad(double angle);
double    radToDeg(double angle);
fpoint2_t vectorAngle(double angle_rad);
double    distanceRayPlane(const fpoint3_t& ray_origin, const fpoint3_t& ray_dir, const Plane& plane);
bool      boxLineIntersect(const frect_t& box, const fseg2_t& line);
Plane     planeFromTriangle(const fpoint3_t& p1, const fpoint3_t& p2, const fpoint3_t& p3);
} // namespace MathStuff
