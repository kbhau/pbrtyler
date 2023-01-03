#pragma once

#include "functions.h"
#include "log.h"
#include "types.h"


inline float mix(float a, float b, float f)
{
	return b * f + (1.f - f) * a;
}

inline Col4 mix(Col4 a, Col4 b, float f)
{
	Col4 c;
	c.r = b.r * f + (1.f - f) * a.r;
	c.g = b.g * f + (1.f - f) * a.g;
	c.b = b.b * f + (1.f - f) * a.b;
	c.a = b.a * f + (1.f - f) * a.a;
	return c;
}

inline Vec3 mix(Vec3 a, Vec3 b, float f)
{
	Vec3 c;
	c.x = b.x * f + (1.f - f) * a.x;
	c.y = b.y * f + (1.f - f) * a.y;
	c.z = b.z * f + (1.f - f) * a.z;
	return c;
}


float blend_factor(float f1, float f2, float h1, float h2)
{
	if (f1 == 1.f) {
		return 1.f;
	}

	if (f1 == 0.f) {
		return 0.f;
	}

	float ha1 = h1 * f1 + (1.f - f1) * (f1 * h1);
	float ha2 = h2 * f2 + (1.f - f2) * (f2 * h2);
	return factor_eps(ha1, ha2, 0.01f);
}


void copy_pixel(
	PBRMap& src,
	PBRMap& dst,
	std::vector<float>& src_f,
	std::vector<float>& dst_f,
	int src_i,
	int dst_i,
	float blend_f = 1.f,
	bool copy_fac = true
) {
	if (blend_f <= 0.f) {
		return;
	}

	if (blend_f == 1.f) {
		if (copy_fac) {
			dst_f[dst_i] = src_f[src_i];
		}
		dst.d[dst_i] = src.d[src_i];
		dst.n[dst_i] = src.n[src_i];
		dst.h[dst_i] = src.h[src_i];
		dst.r[dst_i] = src.r[src_i];
		dst.m[dst_i] = src.m[src_i];
	} else {
		if (copy_fac) {
			dst_f[dst_i] = mix(dst_f[dst_i], src_f[src_i], blend_f);
		}
		dst.d[dst_i] = mix(dst.d[dst_i], src.d[src_i], blend_f);
		dst.n[dst_i] = mix(dst.n[dst_i], src.n[src_i], blend_f);
		dst.h[dst_i] = mix(dst.h[dst_i], src.h[src_i], blend_f);
		dst.r[dst_i] = mix(dst.r[dst_i], src.r[src_i], blend_f);
		dst.m[dst_i] = mix(dst.m[dst_i], src.m[src_i], blend_f);
	}
}
