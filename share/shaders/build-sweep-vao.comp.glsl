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
#version 450
layout(local_size_x = 8, local_size_y = 8, local_size_z = 1) in;

writeonly layout(std430) buffer SweepVbo { vec4 bSweepVbo[]; };
writeonly layout(std430) buffer SweepEbo { uint bSweepEbo[]; };

uniform uvec2 uSize;
uniform uint uRestartIndex;
uniform uint uNormalBufferStartOffset;
uniform uint uUvBufferStartOffset;
uniform vec2 uStartPosition;
uniform vec2 uEndPosition;
uniform bool uIsCap;
uniform bool uStartFlip;
uniform bool uEndFlip;

uniform bool uAddHalfTurn;
uniform int uAssignmentOffset;
uniform bool uFlipEndProfile;

uniform sampler1D uStartProfile;
uniform sampler1D uEndProfile;

vec2 computeProfile(float radialAbscissa, float curveAbscissa) {
	vec2 startFlip = vec2(uStartFlip ? 1 : -1, 1);
	vec2 endFlip = vec2(uEndFlip ? -1 : 1, 1);

	float startRa = radialAbscissa;
	if (uStartFlip) startRa = 1 - startRa;

	float endRa = radialAbscissa;
	//if (uFlipEndProfile) endRa = 1 - endRa;
	if (!uEndFlip) endRa = 1 - endRa;

	if (uStartFlip != uEndFlip) endRa += 0.5;
	//if (uFlipEndProfile) endRa += 0.5;

	vec2 profile = (texture(uStartProfile, startRa).xy - uStartPosition * vec2(1.0, 0.5)) * startFlip;
	if (!uIsCap) {
		endRa += (uAddHalfTurn ? 0.0 : 0.5) + float(uAssignmentOffset) / 100;
		profile = mix(
			profile,
			(texture(uEndProfile, endRa).xy - uEndPosition * vec2(1.0, 0.5)) * endFlip,
			curveAbscissa
		);
	}
	return profile;
}

void main() {
	uvec2 p = gl_GlobalInvocationID.xy;
	if (p.x >= uSize.x || p.y >= uSize.y) return;
	float u = float(p.x) / float(uSize.x - 1);
	float t = float(p.y) / float(uSize.y - 1);

	float profileT = smoothstep(0.0, 1.0, t);

	vec2 profile = computeProfile(u, profileT);

	// DEBUG
	//float th = u * 2 * 3.14159266;
	//profile = vec2(cos(th), sin(th)) * 0.1;
	
	float s = u < 0.5 ? 1 : 1;
	vec2 dprofile = s * normalize(computeProfile(u + s * 1e-3, profileT) - profile);
	vec3 normal = vec3(-dprofile.y, dprofile.x, 0);

	uint vertexIndex = p.x + p.y * uSize.x;
	bSweepVbo[vertexIndex] = vec4(profile, t, 0.0);
	bSweepVbo[uNormalBufferStartOffset + vertexIndex] = vec4(normal, 0.0);
	bSweepVbo[uUvBufferStartOffset + vertexIndex] = vec4(u, t, 0.0, 0.0);

	if (p.y < uSize.y - 1) {
		uint elementIndex = 2 * p.x + p.y * (2 * uSize.x + 1);
		bSweepEbo[elementIndex] = vertexIndex;
		bSweepEbo[elementIndex+1] = vertexIndex + uSize.x;

		if (p.x == uSize.x - 1) {
			bSweepEbo[elementIndex+2] = uRestartIndex;
		}
	}
}
