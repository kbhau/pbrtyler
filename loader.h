#pragma once

#include <exception>
#include <string>
#include <vector>

#include "lodepng.h"

#include "log.h"
#include "types.h"


// Normal [0, 1]
inline unsigned char byt(float val)
{
	return round(val * 255.f);
}
inline float flt(unsigned char val)
{
	return (float)val / 255.f;
}

// Vector [-1, 1]
inline unsigned char bytv(float val)
{
	return round((val + 1.f) * 0.5 * 255.f);
}
inline float fltv(unsigned char val)
{
	return ((float)val / 255.f) * 2.f - 1.f;
}
inline Vec3 normalize(Vec3 v)
{
	float len = sqrt(v.x*v.x + v.y*v.y + v.z*v.z);
	v.x /= len;
	v.y /= len;
	v.z /= len;
	return v;
}


void reserve_pbr(
	PBRMap& pbr,
	unsigned int w,
	unsigned int h
) {
	auto size = w*h;
	pbr.d = std::vector<Col4>(size, Col4{ 0.f, 0.f, 0.f, 1.f });
	pbr.n = std::vector<Vec3>(size, Vec3{ 0.f, 0.f, 0.f });
	pbr.h = std::vector<float>(size, 0.f);
	pbr.hn = std::vector<float>(size, 0.f);
	pbr.r = std::vector<float>(size, 0.f);
	pbr.m = std::vector<float>(size, 0.f);
}


void free_pbr(PBRMap& pbr)
{
	pbr.d.clear();
	pbr.n.clear();
	pbr.h.clear();
	pbr.r.clear();
	pbr.m.clear();
}


void read_file(
	std::string filename,
	std::vector<unsigned char>& bytes,
	unsigned int& w,
	unsigned int& h
) {
	unsigned error = lodepng::decode(bytes, w, h, filename);
	if (error) {
		OP("Decoder error " << error << ": " << lodepng_error_text(error));
		OP("Filename=[" << filename << "]");
		throw std::exception("Decoder error.");
	}
	// 4 bytes per pixel, ordered RGBARGBA
}


void read_col3(
	std::string filename,
	std::vector<Col3>& pixels,
	unsigned int& w,
	unsigned int& h
) {
	std::vector<unsigned char> bytes;
	read_file(filename, bytes, w, h);

	auto size = w * h;
	pixels.resize(size);
	for (int i=0; i<size; ++i) {
		pixels[i].r = flt(bytes[i*4]);
		pixels[i].g = flt(bytes[i*4 + 1]);
		pixels[i].b = flt(bytes[i*4 + 2]);		
	}
}


void read_col4(
	std::string filename,
	std::vector<Col4>& pixels,
	unsigned int& w,
	unsigned int& h
) {
	std::vector<unsigned char> bytes;
	read_file(filename, bytes, w, h);

	auto size = w * h;
	pixels.resize(size);
	for (int i=0; i<size; ++i) {
		pixels[i].r = flt(bytes[i*4]);
		pixels[i].g = flt(bytes[i*4 + 1]);
		pixels[i].b = flt(bytes[i*4 + 2]);
		pixels[i].a = flt(bytes[i*4 + 3]);
	}
}


void read_float(
	std::string filename,
	std::vector<float>& pixels,
	unsigned int& w,
	unsigned int& h
) {
	std::vector<unsigned char> bytes;
	read_file(filename, bytes, w, h);

	auto size = w * h;
	pixels.resize(size);
	for (int i=0; i<size; ++i) {
		pixels[i] = flt(bytes[i*4]);
	}
}


void read_vec3(
	std::string filename,
	std::vector<Vec3>& pixels,
	unsigned int& w,
	unsigned int& h
) {
	std::vector<unsigned char> bytes;
	read_file(filename, bytes, w, h);

	auto size = w * h;
	pixels.resize(size);
	for (int i=0; i<size; ++i) {
		pixels[i].x = fltv(bytes[i*4]);
		pixels[i].y = fltv(bytes[i*4 + 1]);
		pixels[i].z = fltv(bytes[i*4 + 2]);
	}
}


void load_pbr(
	std::string _filename,
	PBRMap& pbr,
	unsigned int& w,
	unsigned int& h
) {
	OP("Load PBR begin.");
	std::string filename(_filename);

	// diffuse
	read_col4(std::string(filename).append("_d.png").c_str(), pbr.d, w, h);

	// normal
	read_vec3(std::string(filename).append("_n.png").c_str(), pbr.n, w, h);

	// hrm
	std::vector<Col4> hrm;
	read_col4(std::string(filename).append("_hrm.png").c_str(), hrm, w, h);
	auto size = w * h;
	pbr.h.resize(size);
	pbr.r.resize(size);
	pbr.m.resize(size);
	for (int i=0; i<size; ++i) {
		pbr.h[i] = hrm[i].r;
		pbr.r[i] = hrm[i].g;
		pbr.m[i] = hrm[i].b;
	}

	OP("Load PBR end.");
}


void write_file(
	std::string filename,
	std::vector<unsigned char>& bytes,
	unsigned int& w,
	unsigned int& h
) {
	// 4 bytes per pixel, ordered RGBARGBA
	unsigned error = lodepng::encode(filename, bytes, w, h);
	if (error) {
		OP("Encoder error " << error << ": " << lodepng_error_text(error));
		OP("Filename=[" << filename << "]");
		throw std::exception("Encoder error.");
	}
}


void write_col3(
	std::string filename,
	std::vector<Col3>& pixels,
	unsigned int w,
	unsigned int h
) {
	std::vector<unsigned char> bytes;
	auto size = w * h;
	bytes.resize(size * 4);
	for (int i=0; i<size; ++i) {
		bytes[i*4] = byt(pixels[i].r);
		bytes[i*4+1] = byt(pixels[i].g);
		bytes[i*4+2] = byt(pixels[i].b);
		bytes[i*4+3] = byt(1.0);
	}
	write_file(filename, bytes, w, h);
}


void write_col4(
	std::string filename,
	std::vector<Col4>& pixels,
	unsigned int w,
	unsigned int h
) {
	std::vector<unsigned char> bytes;
	auto size = w * h;
	bytes.resize(size * 4);
	for (int i=0; i<size; ++i) {
		bytes[i*4] = byt(pixels[i].r);
		bytes[i*4+1] = byt(pixels[i].g);
		bytes[i*4+2] = byt(pixels[i].b);
		bytes[i*4+3] = byt(pixels[i].a);
	}
	write_file(filename, bytes, w, h);
}


void write_float(
	std::string filename,
	std::vector<float>& pixels,
	unsigned int w,
	unsigned int h
) {
	std::vector<unsigned char> bytes;
	auto size = w * h;
	bytes.resize(size * 4);
	for (int i=0; i<size; ++i) {
		bytes[i*4] = byt(pixels[i]);
		bytes[i*4+1] = byt(pixels[i]);
		bytes[i*4+2] = byt(pixels[i]);
		bytes[i*4+3] = byt(1.0);
	}
	write_file(filename, bytes, w, h);
}


void write_vec3(
	std::string filename,
	std::vector<Vec3>& pixels,
	unsigned int w,
	unsigned int h
) {
	std::vector<unsigned char> bytes;
	auto size = w * h;
	bytes.resize(size * 4);
	Vec3 v;
	for (int i=0; i<size; ++i) {
		v = normalize(pixels[i]);
		bytes[i*4] = bytv(pixels[i].x);
		bytes[i*4+1] = bytv(pixels[i].y);
		bytes[i*4+2] = bytv(pixels[i].z);
		bytes[i*4+3] = byt(1.0);
	}
	write_file(filename, bytes, w, h);
}


void save_pbr(
	std::string _filename,
	PBRMap& pbr,
	unsigned int w,
	unsigned int h
) {
	OP("Save PBR begin.");
	std::string filename(_filename);

	// diffuse
	write_col4(std::string(filename).append("_d.png").c_str(), pbr.d, w, h);

	// normal
	write_vec3(std::string(filename).append("_n.png").c_str(), pbr.n, w, h);

	// hrm
	std::vector<Col3> hrm;
	auto size = w * h;
	hrm.resize(size);
	for (int i=0; i<size; ++i) {
		hrm[i].r = pbr.h[i];
		hrm[i].g = pbr.r[i];
		hrm[i].b = pbr.m[i];
	}
	write_col3(std::string(filename).append("_hrm.png").c_str(), hrm, w, h);

	OP("Create RHM map.");
	//read_float(filename.append("_h.png").c_str(), pbr.h, w, h);
	OP("Save PBR end.");
}
