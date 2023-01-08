#pragma once

#include <cmath>

#define M_PI 3.14159265358979323846


inline float crossent(float predicted, float actual)
{
	return -actual * log2(predicted);
}


inline float clamp(float val, float min, float max)
{
	return std::fmin(std::fmax(val, min), max);
}


inline float factor_eps(float a, float b, float eps)
{
	float dist = a - b;
	if (dist > eps) {
		return 1.f;
	} else if (dist < -eps) {
		return 0.f;
	} else {
		return (dist + eps) / (2.f * eps);
	}
}


inline float factor_eps_top(float a, float b, float eps)
{
	float dist = a - b;
	if (dist > eps) {
		return 1.f;
	} else if (dist < 0.f) {
		return 0.f;
	} else {
		return dist / eps;
	}
}


inline float gaussian_blur(int x, int y, float std_dev)
{
	float a = 1.f / (2.f * M_PI * std_dev * std_dev);
	float b = (x*x + y*y) / (2.f * std_dev * std_dev);
	return clamp(a * exp(-b), 0.f, 1.f);
}


inline float logisticize(float f)
{
	float sig_ff = 0.1f;
	float sig_f = 1.f / (1.f - 2.f * sig_ff);
	f -= 0.5f;
	f *= 4.38f;
	f = (1.f / (1.f + exp(-f))) * sig_f - sig_ff;
	return clamp(f, 0.f, 1.f);
}


inline float logisticize_full(float f)
{
	f -= 0.5f;
	f *= 12.f;
	f = (1.f / (1.f + exp(-f)));
	return clamp(f, 0.f, 1.f);
}


inline int modulo(int a, int b)
{
	return (a%b + b) % b;
}
