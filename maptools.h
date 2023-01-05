#pragma once

#include <cmath>
#include <vector>

#include "functions.h"
#include "log.h"
#include "pixeltools.h"
#include "types.h"


class MapTools
{
public:
	unsigned int src_w;
	unsigned int src_h;
	unsigned int w;
	unsigned int h;

	inline int _i(int x, int y)
	{
		return y * w + x;
	}

	inline int _ii(int x, int y, int x_offset, int y_offset)
	{
		return (y + y_offset) * src_w + (x + x_offset);
	}


	std::vector<float> blur_map(
		std::vector<float>& map
	) {
		OP("Blur map.");

		std::vector<float> out;
		out.resize(map.size());

		for (int y=0; y<h; ++y)
		for (int x=0; x<w; ++x) {
			int i = _i(x, y);
			for (int yy=-2; yy<=2; ++yy)
			for (int xx=-2; xx<=2; ++xx) {
				int j = _i(modulo(x+xx, w), modulo(y+yy, h));
				out[i] += map[j] * gaussian_blur(xx, yy, 0.83f);
			}
		}

		return out;
	}


	void blend_map(
		PBRMap& src,
		PBRMap& dst,
		std::vector<float>& src_f,
		std::vector<float>& dst_f
	) {
		OP("Blend map begin.");
		
		OP("Create blend map.");
		std::vector<float> bm;
		bm.resize(w*h);
		for (int y=0; y<h; ++y)
		for (int x=0; x<w; ++x) {
			int i = _i(x, y);
			bm[i] = blend_factor(src_f[i], dst_f[i], src.h[i], dst_f[i]);
		}
		bm = blur_map(bm);

		OP("Mix in pixels.");
		for (int y=0; y<h; ++y)
		for (int x=0; x<w; ++x) {
			int i = _i(x, y);
			copy_pixel(src, dst, src_f, dst_f, i, i, bm[i]);
		}

		OP("Blend map end.");
	}


	void copy_chunk(
		PBRMap& src,
		PBRMap& dst,
		std::vector<float>& src_f,
		std::vector<float>& out_f,
		Rect2 from,
		Vec2 to
	) {
		OP("Copy chunk raw begin.");
		OP("from=[" << from.x << "][" << from.y << "][" << from.w << "][" << from.h << "]");
		OP("to=[" << to.x << "][" << to.y << "]");

		// Coords.
		int sx;
		int sy;
		int dx;
		int dy;

		// Indexes.
		int si;
		int di;

		// Copy pixels.
		for (int ry=0; ry<from.h; ++ry) {
			sy = from.y + ry;
			dy = to.y + ry;
			for (int rx=0; rx<from.w; ++rx) {
				sx = from.x + rx;
				dx = to.x + rx;
				si = _i(sx, sy);
				di = _i(dx, dy);

				copy_pixel(src, dst, src_f, out_f, si, di);
			}
		}

		OP("Copy chunk raw end.");
	}


	void copy_from_wide_map(PBRMap& src, PBRMap& dst, int x_offset, int y_offset)
	{
		OP("Copy from wide map begin.");
		std::vector<float> placeholder;
		for (int y=0; y<h; ++y)
		for (int x=0; x<w; ++x) {
			copy_pixel(
				src,
				dst,
				placeholder,
				placeholder,
				_ii(x, y, x_offset, y_offset),
				_i(x, y),
				1.f,
				false
			);
		}
		OP("Copy from wide map end.");
	}


	inline int distance_from_center_box(int x, int y)
	{
		return std::max(
			std::abs((int)(x - w/2)),
			std::abs((int)(y - h/2))
		);
	}


	inline int distance_from_center_manhattan(int x, int y)
	{
		return
			std::abs((int)(x - w/2))
			+ std::abs((int)(y - h/2));
	}


	inline float distance_from_center_radial(int x, int y)
	{
		return sqrt(
			pow(std::abs((int)(x - w/2)), 2.f)
			+ pow(std::abs((int)(y - h/2)), 2.f));
	}


	void influence_map_base(std::vector<float>& map)
	{
		int dist;
		int mind = w / 32;
		int maxd = w / 2;
		float fac;
		map.resize(w*h);
		for (int y=0; y<h; ++y)
		for (int x=0; x<w; ++x) {
			dist = distance_from_center_radial(x, y);
			fac = 1.f - clamp(dist / (float)(maxd - mind), 0.f, 1.f);
			map[_i(x, y)] = sqrt(fac);
		}
	}


	void influence_map_corner(std::vector<float>& map)
	{
		int dist;
		int mind = w / 32;
		int maxd = w / 2;
		float fac;
		map.resize(w*h);
		for (int y=0; y<h; ++y)
		for (int x=0; x<w; ++x) {
			dist = distance_from_center_radial(x, y);
			fac = 1.f - clamp(dist / (float)(maxd - mind), 0.f, 1.f);
			map[_i(x, y)] = sqrt(fac);
		}
	}


	void influence_map_edge(std::vector<float>& map)
	{
		float dist;
		float mind = w / 32;
		float maxd = w / 2;
		float fac;
		map.resize(w*h);
		for (int y=0; y<h; ++y)
		for (int x=0; x<w; ++x) {
			dist = distance_from_center_radial(x, y);
			fac = 1.f - clamp(dist / (float)(maxd - mind), 0.f, 1.f);
			map[_i(x, y)] = sqrt(fac);
		}
	}


	void influence_map_tmp(std::vector<float>& map)
	{
		float dist;
		float mind = w / 4;
		float maxd = w * 7 / 8;
		float fac;
		map.resize(w*h);
		for (int y=0; y<h; ++y)
		for (int x=0; x<w; ++x) {
			dist = distance_from_center_box(x, y);
			fac = clamp(dist / (float)(maxd - mind), 0.f, 1.f);
			map[_i(x, y)] = pow(fac, 1.f);
		}
	}


	void influence_map_empty(std::vector<float>& map)
	{
		map = std::vector<float>(w*h, 0.0);
	}
};
