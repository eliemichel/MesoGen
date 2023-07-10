/**
 * This file is part of MesoGen, the reference implementation of:
 *
 *   Michel, Élie and Boubekeur, Tamy (2023).
 *   MesoGen: Designing Procedural On-Surface Stranded Mesostructures,
 *   ACM Transaction on Graphics (SIGGRAPH '23 Conference Proceedings),
 *   https://doi.org/10.1145/3588432.3591496
 *
 * Copyright (c) 2020 - 2023 -- Télécom Paris (Élie Michel <emichel@adobe.com>)
 * 
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to
 * deal in the Software without restriction, including without limitation the
 * rights to use, copy, modify, merge, publish, distribute, sublicense, and/or
 * sell copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 * 
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * The Software is provided "as is", without warranty of any kind, express or
 * implied, including but not limited to the warranties of merchantability,
 * fitness for a particular purpose and non-infringement. In no event shall the
 * authors or copyright holders be liable for any claim, damages or other
 * liability, whether in an action of contract, tort or otherwise, arising
 * from, out of or in connection with the software or the use or other dealings
 * in the Software.
 */
//////////////////////////////////////////////////////
// G-Buffer related functions
//
// define either OUT_GBUFFER or IN_GBUFFER before including this file,
// to respectivaly write or read to gbuffer.

struct GFragment {
	vec3 baseColor;
	vec3 normal;
	vec3 ws_coord;
	uint material_id;
	float roughness;
	float metallic;
	vec3 emission;
	float alpha;
	uint count;
};

#define gbuffer0Sampler2D sampler2D
#define gbuffer1Sampler2D usampler2D
#define gbuffer2Sampler2D usampler2D

#define gbuffer0Type vec4
#define gbuffer1Type uvec4
#define gbuffer2Type uvec4

//////////////////////////////////////////////////////
// Reserved material IDs

const uint noMaterial = 0;
const uint skyboxMaterial = 1;
const uint standardMaterial = 2;
const uint forwardBaseColorMaterial = 3;
const uint forwardNormalMaterial = 4;

//////////////////////////////////////////////////////
// GFragment methods

void initGFragment(out GFragment fragment) {
    fragment.baseColor = vec3(0.0);
    fragment.material_id = standardMaterial;
    fragment.normal = vec3(0.0);
    fragment.ws_coord = vec3(0.0);
    fragment.roughness = 0.8;
    fragment.metallic = 0.0;
    fragment.emission = vec3(0.0);
    fragment.alpha = 0.0;
    fragment.count = 0;
}

/**
 * Apply mix to all components of a GFragment
 */
GFragment LerpGFragment(in GFragment ga, in GFragment gb, float t) {
	GFragment g;
	g.baseColor = mix(ga.baseColor, gb.baseColor, t);
	g.ws_coord = mix(ga.ws_coord, gb.ws_coord, t);
	g.metallic = mix(ga.metallic, gb.metallic, t);
	g.roughness = mix(ga.roughness, gb.roughness, t);
	g.emission = mix(ga.emission, gb.emission, t);
	g.alpha = mix(ga.alpha, gb.alpha, t);
	g.material_id = ga.material_id; // cannot be interpolated

	// Normal interpolation
	float wa = (1 - t) * ga.alpha;
	float wb = t * gb.alpha;

	// Avoid trouble with NaNs in normals
	if (wa == 0) ga.normal = vec3(0.0);
	if (wb == 0) gb.normal = vec3(0.0);

	g.normal = normalize(wa * ga.normal + wb * gb.normal);

	// Naive interpolation
	//g.normal = normalize(mix(ga.normal, gb.normal, t));
	return g;
}

void addToGFragment(inout GFragment ga, in GFragment gb) {
	ga.baseColor += gb.baseColor;
	ga.material_id += gb.material_id;
	ga.normal += gb.normal;
	ga.ws_coord += gb.ws_coord;
	ga.roughness += gb.roughness;
	ga.metallic += gb.metallic;
	ga.emission += gb.emission;
	ga.alpha += gb.alpha;
	ga.count += gb.count;
}

void multiplyGFragment(inout GFragment g, float fac) {
	g.baseColor *= fac;
	g.material_id = uint(g.material_id * fac);
	g.normal *= fac;
	g.ws_coord *= fac;
	g.roughness *= fac;
	g.metallic *= fac;
	g.emission *= fac;
	g.alpha *= fac;
	g.count = uint(g.count * fac);
}

void unpackGFragment(
	in sampler2D gbuffer0,
	in usampler2D gbuffer1,
	in usampler2D gbuffer2,
	in ivec2 coords, 
	out GFragment fragment)
{
	vec4 data0 = texelFetch(gbuffer0, coords, 0);
	uvec4 data1 = texelFetch(gbuffer1, coords, 0);
	uvec4 data2 = texelFetch(gbuffer2, coords, 0);
	vec2 tmp;

	tmp = unpackHalf2x16(data1.y);
	fragment.baseColor = vec3(unpackHalf2x16(data1.x), tmp.x);
	fragment.normal = normalize(vec3(tmp.y, unpackHalf2x16(data1.z)));
	fragment.ws_coord = data0.xyz;
	fragment.material_id = data1.w;
	fragment.roughness = data0.w;

	tmp = unpackHalf2x16(data2.y);
	//fragment.lean2.xyz = vec3(unpackHalf2x16(data2.x), tmp.x);
	fragment.metallic = tmp.y;
	fragment.count = data2.w;
}

void packGFragment(
	in GFragment fragment,
	out vec4 gbuffer_color0,
	out uvec4 gbuffer_color1,
	out uvec4 gbuffer_color2)
{
	gbuffer_color0 = vec4(fragment.ws_coord, fragment.roughness);
	gbuffer_color1 = uvec4(
		packHalf2x16(fragment.baseColor.xy),
		packHalf2x16(vec2(fragment.baseColor.z, fragment.normal.x)),
		packHalf2x16(fragment.normal.yz),
		fragment.material_id
	);
	gbuffer_color2 = uvec4(
		0,//packHalf2x16(fragment.lean2.xy),
		packHalf2x16(vec2(0.0, fragment.metallic)),
		packHalf2x16(vec2(0.0, 0.0)),
		fragment.count
	);
}

/////////////////////////////////////////////////////////////////////
#ifdef OUT_GBUFFER

layout (location = 0) out vec4 gbuffer_color0;
layout (location = 1) out uvec4 gbuffer_color1;
layout (location = 2) out uvec4 gbuffer_color2;

void autoPackGFragment(in GFragment fragment) {
	packGFragment(fragment, gbuffer_color0, gbuffer_color1, gbuffer_color2);
}

#endif // OUT_GBUFFER


/////////////////////////////////////////////////////////////////////
#ifdef IN_GBUFFER

layout (binding = 0) uniform sampler2D uGbuffer0;
layout (binding = 1) uniform usampler2D uGbuffer1;
layout (binding = 2) uniform usampler2D uGbuffer2;
layout (binding = 3) uniform sampler2D uZbuffer;

void autoUnpackGFragment(inout GFragment fragment) {
	unpackGFragment(uGbuffer0, uGbuffer1, uGbuffer2, ivec2(gl_FragCoord.xy), fragment);
}

void autoUnpackGFragmentAt(ivec2 coord, inout GFragment fragment) {
	unpackGFragment(uGbuffer0, uGbuffer1, uGbuffer2, coord, fragment);
}

#endif // IN_GBUFFER
