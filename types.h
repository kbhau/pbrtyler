#pragma once

#include <vector>

struct Col3
{
	float r;
	float g;
	float b;
};

struct Col4
{
	float r;
	float g;
	float b;
	float a;
};

struct Vec2
{
	float x;
	float y;

	Vec2(float _x, float _y) : x(_x), y(_y) {}
};

struct Vec3
{
	float x;
	float y;
	float z;
};

struct Rect2
{
	float x;
	float y;
	float w;
	float h;

	Rect2(float _x, float _y, float _w, float _h) : x(_x), y(_y), w(_w), h(_h) {}
};

union U4
{
	unsigned char bytes[4];

	Col3 col3;
	Col4 col4;
	Vec3 vec3;
};

class PBRMap
{
public:
	std::vector<Col4> d;
	std::vector<Vec3> n;
	std::vector<float> r;
	std::vector<float> h;
	std::vector<float> hn;
	std::vector<float> m;
};
