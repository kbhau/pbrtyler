#pragma once

#include <algorithm>
#include <cmath>
#include <vector>

#include "FastNoiseLite.h"

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
	float hnf;
	float he;

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
	)
	{
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
		std::vector<float>& dst_f,
		bool blur
	)
	{
		OP("Blend map begin.");

		OP("Create blend map.");
		std::vector<float> bm;
		bm.resize(w*h);
		for (int y=0; y<h; ++y)
			for (int x=0; x<w; ++x) {
				int i = _i(x, y);
				bm[i] = blend_factor(src_f[i], dst_f[i], src.h[i], dst_f[i]);
			}
		if (blur) {
			bm = blur_map(bm);
		}

		OP("Mix in pixels.");
		for (int y=0; y<h; ++y)
			for (int x=0; x<w; ++x) {
				int i = _i(x, y);
				copy_pixel(src, dst, src_f, dst_f, i, i, bm[i]);
			}

		OP("Blend map end.");
	}

	struct idedFloat
	{
		int k;
		float v;
	};

	void blend_map_4_way(
		PBRMap& dst,	// pre filled as base
		PBRMap& s1,
		PBRMap& s2,
		PBRMap& s3,
		PBRMap& s4,
		std::vector<float>& f1,
		std::vector<float>& f2,
		std::vector<float>& f3,
		std::vector<float>& f4,
		bool blur
	)
	{
		OP("4 way blend map begin.");

		OP("Reserving maps.");
		std::vector<float> bm[4];
		for (int i=0; i<4; ++i) {
			bm[i].resize(w*h, 0.f);
		}
		std::vector<idedFloat> ha;
		ha.resize(4);

		std::vector< std::vector<idedFloat> > hb;
		hb.resize(w*h);

		OP("Compute blend factors.");
		for (int y=0; y<h; ++y)
			for (int x=0; x<w; ++x) {
				int i = _i(x, y);

				ha[0].k = 0;
				ha[1].k = 1;
				ha[2].k = 2;
				ha[3].k = 3;
				ha[0].v = height_factor(f1[i], s1.h[i]);
				ha[1].v = height_factor(f2[i], s2.h[i]);
				ha[2].v = height_factor(f3[i], s3.h[i]);
				ha[3].v = height_factor(f4[i], s4.h[i]);

				std::sort(
					ha.begin(),
					ha.end(),
					[](idedFloat a, idedFloat b)
					{
						return a.v > b.v;
					}
				);

				// Top layer factor most important.
				// Then factor underneath work in (1 - top factor).

				for (int j=0; j<4; ++j) {
					float sum_prev = 0.f;
					for (int k=0; k<j; ++k) {
						sum_prev += bm[ha[k].k][i];
					}
					if (j < 3) {
						bm[ha[j].k][i] = (1.f - sum_prev)
							* sqrt(sqrt(factor_eps(ha[j].v, ha[j+1].v, he)));
					} else {
						bm[ha[j].k][i] = (1.f - sum_prev)
							* sqrt(sqrt(factor_eps(ha[j].v, 0.f, he)));
					}
				}

				hb[i] = ha;
			}

		// Blur maps.
		if (blur) {
			for (int i=0; i<4; ++i) {
				OP("Blur blend map " << i+1);
				bm[i] = blur_map(bm[i]);
			}
		}

		// Normalize factors - need to sum to 1.
		for (int y=0; y<h; ++y)
		for (int x=0; x<w; ++x) {
			int i = _i(x, y);
			float len = 0.f;
			for (int j=0; j<4; ++j) {
				len += bm[j][i];
			}
			for (int j=0; j<4; ++j) {
				bm[j][i] /= len;
			}
		}

		OP("Mix in pixels.");
		PBRMap* ss[4] = {
			&s1, &s2, &s3, &s4
		};
		for (int y=0; y<h; ++y)
		for (int x=0; x<w; ++x) {
			int i = _i(x, y);
			copy_pixel(*ss[0], dst, f1, f1, i, i, 1.f, false);
			for (int j=3; j>=0; --j) {
				int k = hb[i][j].k;
				float v = hb[i][j].v;
				copy_pixel(*ss[k], dst, f1, f1, i, i,
					bm[k][i],
					false
				);
			}
		}

		OP("4 way blend map end.");
	}


	void copy_chunk(
		PBRMap& src,
		PBRMap& dst,
		std::vector<float>& src_f,
		std::vector<float>& out_f,
		Rect2 from,
		Vec2 to
	)
	{
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


	void influence_map_base(std::vector<float>& map, float fac_power)
	{
		int dist;
		int mind = w / 8;
		int maxd = w / 2;
		float fac;
		map.resize(w*h);
		for (int y=0; y<h; ++y)
			for (int x=0; x<w; ++x) {
				dist = distance_from_center_radial(x, y);
				fac = 1.f - clamp(dist / (float)(maxd - mind), 0.f, 1.f);
				map[_i(x, y)] = pow(fac, fac_power);
			}
	}


	void influence_map_corner(std::vector<float>& map, float fac_power)
	{
		int dist;
		int mind = w / 8;
		int maxd = w / 2;
		float fac;
		map.resize(w*h);
		for (int y=0; y<h; ++y)
			for (int x=0; x<w; ++x) {
				dist = distance_from_center_radial(x, y);
				fac = 1.f - clamp(dist / (float)(maxd - mind), 0.f, 1.f);
				map[_i(x, y)] = pow(fac, fac_power);
			}
	}


	void influence_map_edge(std::vector<float>& map, float fac_power)
	{
		float dist;
		float mind = w / 8;
		float maxd = w / 2;
		float fac;
		map.resize(w*h);
		for (int y=0; y<h; ++y)
			for (int x=0; x<w; ++x) {
				dist = distance_from_center_radial(x, y);
				fac = 1.f - clamp(dist / (float)(maxd - mind), 0.f, 1.f);
				map[_i(x, y)] = pow(fac, fac_power);
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


	void apply_fac_noise(
		std::vector<float>& map,
		FastNoiseLite& noise,
		float noise_factor
	)
	{
		for (int y=0; y<h; ++y)
		for (int x=0; x<w; ++x) {
			int i = _i(x, y);
			float f = clamp(
				map[i] * (1.f - noise_factor)
				+ noise_factor
				* (noise.GetNoise((float)x, (float)y) * 0.5f + 0.5f),
				0.f,
				1.f
			);
			map[i] = 1.f * map[i] + (1.f - map[i]) * f;
		}
	};


	void apply_height_noise(
		PBRMap& map,
		FastNoiseLite& noise,
		float height_noise_factor
	) {
		for (int y=0; y<h; ++y)
		for (int x=0; x<w; ++x) {
			int i = _i(x, y);
			map.hn[i] = clamp(
				map.h[i] * (1.f - height_noise_factor)
					+ height_noise_factor
						* (noise.GetNoise((float)x, (float)y) * 0.5f + 0.5f),
				0.f,
				1.f
			);
		}
	};
};