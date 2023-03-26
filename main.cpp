
// ----------------------------------------------------------------------------
// PBR TYLER
// ----------------------------------------------------------------------------
// 
// Takes 2x by 2x PBR texture renders and turns them into 1x by 1x seamless
// textures.
// 
// The source map is cut into 4 pieces:
// - base source
// - corners source
// - up/down edge source
// - left/right edge source
// 
// Corner and edge pieces are cut through the middle where the picture is
// seamless and put it where the seams are. Having 4x the source might be
// excessive but it guarantees no repetition of the source material.
// 
// Required maps:
// - _d - diffuse colour (RGBA) - Alpha currently not tested.
// - _n - normal map (RGB)
// - _hrm - hrm map (RGB) - Channels accordingly: Height, Roughness, Metalness.
// 
// No ambient occlusion currently - assuming diffuse colour already has one
// baked in.
// 
// Format is PNG.
// 
// Usage:
// ./pbrtyler -i <input_path> -o <output_path>
// 
// Example:
// ./pbrtyler -i C:\source\tex -o C:\destination\tex
// 
// Input dir:
// - tex_d.png
// - tex_n.png
// - tex_hrm.png
// 
// Suffixes like _d.png will be added for you.
// 
// ----------------------------------------------------------------------------



// ----------------------------------------------------------------------------
// TODO
// ----------------------------------------------------------------------------
//
// - add low frequency height to break up the join lines
// - include alpha in the factor compute
// 
// ----------------------------------------------------------------------------



#include <string>
#include <exception>

#include "FastNoiseLite.h"

#include "argument_reader.h"
#include "log.h"
#include "loader.h"
#include "maptools.h"
#include "types.h"

using namespace std;



// ----------------------------------------------------------------------------
// GLOBALS
// ----------------------------------------------------------------------------

string input;
string output;
bool blur;
float influence_power = 0.125f;
float height_noise_factor = 0.8f;
float height_epsilon = 0.03f;

MapTools mt;
unsigned int src_w = 0U;
unsigned int src_h = 0U;
unsigned int w;
unsigned int h;


PBRMap src;

PBRMap base;
vector<float> fac_base;

PBRMap sc1;
vector<float> fac_sc1;

PBRMap sc2;
vector<float> fac_sc2;

PBRMap sc3;
vector<float> fac_sc3;

PBRMap dst;
vector<float> fac_dst;

// We can get away with a single map cause no overlap.
PBRMap corners;
vector<float> fac_corners;

// All edges to be blended here.
PBRMap edges;
vector<float> fac_edges;

// No need for ud, copy those straight to edges map.
PBRMap edges_lr;
vector<float> fac_edges_lr;



// ----------------------------------------------------------------------------
// FUNCTIONS
// ----------------------------------------------------------------------------

void get_filenames(int argc, char** argv)
{
	OP("- Get arguments.");

	input = get_argument_value("-i", argc, argv);
	output = get_argument_value("-o", argc, argv);
	blur = !get_argument_flag("-noblur", argc, argv);
	if (get_argument_flag("-sharpness", argc, argv)) {
		influence_power = stof(get_argument_value("-sharpness", argc, argv));
	}
	if (get_argument_flag("-noise", argc, argv)) {
		height_noise_factor = stof(get_argument_value("-noise", argc, argv));
	}
	if (get_argument_flag("-epsilon", argc, argv)) {
		height_epsilon = stof(get_argument_value("-epsilon", argc, argv));
	}
}


int read_source_maps()
{
	OP("- Read source maps.");

	try {
		load_pbr(input, src, src_w, src_h);
		w = src_w / 2;
		h = src_h / 2;
		mt.src_w = src_w;
		mt.src_h = src_h;
		mt.w = w;
		mt.h = h;
		mt.hnf = height_noise_factor;
		mt.he = height_epsilon;
		OP("w=[" << w << "] h=[" << h << "]");
	} catch (std::exception e) {
		OP("Could not load source maps.");
		return 1;
	}

	return 0;
}


void reserve_maps()
{
	OP("- Reserve maps.");
	reserve_pbr(base, w, h);
	reserve_pbr(sc1, w, h);
	reserve_pbr(sc2, w, h);
	reserve_pbr(sc3, w, h);
	reserve_pbr(dst, w, h);
	reserve_pbr(corners, w, h);
	reserve_pbr(edges, w, h);
	reserve_pbr(edges_lr, w, h);
}


void create_influence_maps()
{
	OP("- Create influence maps.");
	
	mt.influence_map_base(fac_base, influence_power);

	mt.influence_map_corner(fac_sc1, influence_power);
	mt.influence_map_edge(fac_sc2, influence_power);
	mt.influence_map_edge(fac_sc3, influence_power);

	mt.influence_map_empty(fac_corners);
	mt.influence_map_empty(fac_edges);
	mt.influence_map_empty(fac_edges_lr);

	mt.influence_map_empty(fac_dst);
}


void split_sources()
{
	OP("- Split into working sources.");
	mt.copy_from_wide_map(src, base, 0, 0);
	mt.copy_from_wide_map(src, sc1, w, 0);
	mt.copy_from_wide_map(src, sc2, 0, h);
	mt.copy_from_wide_map(src, sc3, w, h);
	mt.copy_from_wide_map(src, dst, w/2, h/2);
	free_pbr(src);
}


void copy_corners()
{
	OP("- Copy corners to corners temp.");
	Rect2 src_ul = Rect2(0, 0, w/2, h/2);
	Rect2 src_ur = Rect2(w/2, 0, w/2, h/2);
	Rect2 src_dr = Rect2(w/2, h/2, w/2, h/2);
	Rect2 src_dl = Rect2(0, h/2, w/2, h/2);
	Vec2 dst_ul = Vec2(0, 0);
	Vec2 dst_ur = Vec2(w/2, 0);
	Vec2 dst_dr = Vec2(w/2, h/2);
	Vec2 dst_dl = Vec2(0, h/2);
	mt.copy_chunk(sc1, corners, fac_sc1, fac_corners, src_dr, dst_ul);
	mt.copy_chunk(sc1, corners, fac_sc1, fac_corners, src_dl, dst_ur);
	mt.copy_chunk(sc1, corners, fac_sc1, fac_corners, src_ul, dst_dr);
	mt.copy_chunk(sc1, corners, fac_sc1, fac_corners, src_ur, dst_dl);
	free_pbr(sc1);
}


void copy_edges()
{
	OP("- Copy edges to u,d edge temp.");
	Rect2 src_u = Rect2(0, 0, w, h/2);
	Rect2 src_d = Rect2(0, h/2, w, h/2);
	Rect2 src_l = Rect2(0, 0, w/2, h);
	Rect2 src_r = Rect2(w/2, 0, w/2, h);
	Vec2 dst_ul = Vec2(0, 0);
	Vec2 dst_ur = Vec2(w/2, 0);
	Vec2 dst_dr = Vec2(w/2, h/2);
	Vec2 dst_dl = Vec2(0, h/2);
	mt.copy_chunk(sc2, edges, fac_sc2, fac_edges, src_u, dst_dl);
	mt.copy_chunk(sc2, edges, fac_sc2, fac_edges, src_d, dst_ul);
	free_pbr(sc2);

	OP("- Copy edges to l,r temp.");
	mt.copy_chunk(sc3, edges_lr, fac_sc3, fac_edges_lr, src_l, dst_ur);
	mt.copy_chunk(sc3, edges_lr, fac_sc3, fac_edges_lr, src_r, dst_ul);

	free_pbr(sc3);
}


void apply_height_noise()
{
	OP("- Apply height noise.");
	FastNoiseLite ns;
	ns.SetNoiseType(FastNoiseLite::NoiseType_OpenSimplex2);
	ns.SetFrequency(1.5f / (float)w);
	ns.SetFractalGain(0.5);
	ns.SetFractalLacunarity(2.f);
	ns.SetFractalOctaves(8
	);
	
	std::srand(std::time(0));

	ns.SetSeed(std::rand());
	mt.apply_fac_noise(fac_base, ns, height_noise_factor);
	ns.SetSeed(std::rand());
	mt.apply_fac_noise(fac_sc1, ns, height_noise_factor);
	ns.SetSeed(std::rand());
	mt.apply_fac_noise(fac_sc2, ns, height_noise_factor);
	ns.SetSeed(std::rand());
	mt.apply_fac_noise(fac_sc3, ns, height_noise_factor);
}


void apply_seams_fix()
{
	OP("- Blend edges temp to corner temp.");
	mt.blend_map_4_way(dst,
		base, edges, edges_lr, corners,
		fac_base, fac_edges, fac_edges_lr, fac_corners,
		blur
	);
}


void save_output()
{
	OP("- Save output.");
	save_pbr(output, dst, w, h);
}


void clean_up()
{
	OP("- Clean up.");
	free_pbr(dst);
}


// ----------------------------------------------------------------------------
// MAIN
// ----------------------------------------------------------------------------

int main(int argc, char** argv)
{
	OP("\n\n");
	OP("--------------------------------------------------------------------");
	OP("- PBR TYLER v0.1");
	OP("--------------------------------------------------------------------");


	// Inputs.
	get_filenames(argc, argv);

	// Source maps.
	int status = read_source_maps();
	if (status != 0) return status;

	// Setup working memory.
	reserve_maps();
	create_influence_maps();
	split_sources();
	apply_height_noise();

	// Perform ops.
	copy_corners();
	copy_edges();
	apply_seams_fix();

	// Save.
	save_output();

	// Finish.
	clean_up();

	
	OP("- PBR TYLER end.");
	OP("--------------------------------------------------------------------");
	OP("\n\n");
	return 0;
}

