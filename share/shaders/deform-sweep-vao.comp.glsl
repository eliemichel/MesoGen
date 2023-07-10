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
layout(local_size_x = 8, local_size_y = 8, local_size_z = 1) in;

layout(std430) buffer SweepVbo { vec4 bSweepVbo[]; };

readonly layout(std430) buffer SweepInfos { uint bSweepInfo[]; };

readonly layout(std430) buffer SweepControlPoint0 { vec4 bSweepControlPoint0[]; };
readonly layout(std430) buffer SweepControlPoint1 { vec4 bSweepControlPoint1[]; };
readonly layout(std430) buffer SweepControlPoint2 { vec4 bSweepControlPoint2[]; };
readonly layout(std430) buffer SweepControlPoint3 { vec4 bSweepControlPoint3[]; };

uniform uvec2 uSize;
uniform uint uSweepControlPointOffset;
uniform uint uNormalBufferStartOffset;
uniform float uProfileScale;

#include "include/bezier.inc.glsl"

void main() {
	uvec2 p = gl_GlobalInvocationID.xy;
	if (p.x >= uSize.x || p.y >= uSize.y) return;
	uint vertexIndex = p.x + p.y * uSize.x;

	// READ
	vec3 position = bSweepVbo[vertexIndex].xyz;
	vec3 normal = bSweepVbo[uNormalBufferStartOffset + vertexIndex].xyz;

	// TRANSFORM
	vec3 up = vec3(0, 0, 1);

	// Get path
	vec3 p0 = bSweepControlPoint0[uSweepControlPointOffset].xyz;
	vec3 p1 = p0 + bSweepControlPoint1[uSweepControlPointOffset].xyz;
	float t = position.z;

	uint sweepInfo = bSweepInfo[uSweepControlPointOffset];
	bool isCap = (sweepInfo & 0x1) != 0;
	bool flipSweepProfile = (sweepInfo & 0x2) != 0;

	// Local frame
	vec3 origin, dp;
	float radius = 1.0;
	if (isCap) {
		origin = mix(p0, p1, pow(t, 0.5) * 0.125);
		dp = p1 - p0;
		radius = 1 - pow(t, 2.0);
	} else {
		vec3 p3 = bSweepControlPoint3[uSweepControlPointOffset].xyz;
		vec3 p2 = p3 + bSweepControlPoint2[uSweepControlPointOffset].xyz;

		origin = bezier(p0, p1, p2, p3, t);
		dp = dbezier(p0, p1, p2, p3, t);
	}
	//vec3 X = normalize(vec3(1,0,0));
	vec3 X = normalize(dp);
	vec3 Y = normalize(cross(up, X));
	vec3 Z = normalize(cross(X, Y));

	float flip = flipSweepProfile ? -1.0 : 1.0;
	Y = flip * Y;

	vec3 position_local = position;
	// replace with circle for debug
	/*
	float th = uv.x * 2 * 3.14159266;
	position_local.x = cos(th) * uProfileScale * 0.1;
	position_local.y = sin(th) * uProfileScale * 0.1;
	*/

	position_local.y *= 2;
	vec3 radial = (Y * position_local.x + Z * position_local.y) * uProfileScale;
	vec3 position_ws = origin + radial * radius;
	vec3 normal_ws = normalize(normal.x * Y + normal.y * Z);

	//position_ws = position + p0;
	//normal_ws = normal;

	// WRITE
	bSweepVbo[vertexIndex] = vec4(position_ws, 1.0);
	bSweepVbo[uNormalBufferStartOffset + vertexIndex] = vec4(normal_ws, 0.0);
}

