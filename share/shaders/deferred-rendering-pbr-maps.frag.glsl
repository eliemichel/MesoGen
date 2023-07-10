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
#version 450 core
#include "sys:defines"

layout (location = 0) out vec4 out_baseColor;
layout (location = 1) out vec4 out_normal;
layout (location = 2) out vec4 out_metallicRoughness;

in GeometryData {
	vec2 uv;
} geo;

#define IN_GBUFFER
#include "include/gbuffer.inc.glsl"
#include "include/utils.inc.glsl"
#include "include/zbuffer.inc.glsl"

#include "include/uniform/camera.inc.glsl"
#include "include/uniform/lighting.inc.glsl"

uniform int uMsaaFactor;
uniform uvec2 uResolution; // of the gbuffer

//-----------------------------------------------------------------------------

void main() {
    ivec2 texel = ivec2(gl_FragCoord.xy * uMsaaFactor);
	vec3 camera_ws = uCamera.inverseViewMatrix[3].xyz;

    GFragment aa_fragment;
    initGFragment(aa_fragment);

    GFragment fragment;
	for (int i = 0 ; i < uMsaaFactor ; ++i) {
		for (int j = 0 ; j < uMsaaFactor ; ++j) {
			autoUnpackGFragmentAt(texel + ivec2(i, j), fragment);
            addToGFragment(aa_fragment, fragment);
		}
	}

	multiplyGFragment(aa_fragment, 1.0 / (uMsaaFactor * uMsaaFactor));

    autoUnpackGFragmentAt(texel, aa_fragment);
    
    out_baseColor = vec4(aa_fragment.baseColor, 1.0);
    out_normal = vec4(normalize(aa_fragment.normal) * 0.5 + 0.5, 1.0);
    out_metallicRoughness = vec4(aa_fragment.metallic, aa_fragment.roughness, 0.0, 1.0);
}
