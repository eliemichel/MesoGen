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

/**
 * Get the UP vector local to a given interface
 */
vec3 getUpDirection(uint slotIndex, uint interfaceIndex) {
	if (interfaceIndex < 4) {
		return (
			bMacrosurfaceNormals[slotIndex][interfaceIndex].xyz +
			bMacrosurfaceNormals[slotIndex][(interfaceIndex + 1) % 4].xyz
		);
	} else if (interfaceIndex == 4) {
		mat4 topCorners = bMacrosurfaceCorners[slotIndex] + bMacrosurfaceNormals[slotIndex];
		return (
			topCorners[1].xyz - topCorners[2].xyz +
			topCorners[0].xyz - topCorners[3].xyz
		);
	} else {
		mat4 bottomCorners = bMacrosurfaceCorners[slotIndex];
		return (
			bottomCorners[2].xyz - bottomCorners[1].xyz +
			bottomCorners[3].xyz - bottomCorners[0].xyz
		);
	}
}

/**
 * Deform the vertices of sweeps 
 */
void vertexDeform(
	uint slotIndex,
	in vec3 position,
	in vec3 normal,
	in vec2 uv,
	out vec3 position_out,
	out vec3 normal_out,
	out vec2 uv_out,
	out vec3 up_out
) {

	uint sweepOffset = bSweepControlPointOffset[slotIndex];

	// Get path
	uint sweepInstanceIndex = sweepOffset + uSweepIndex;
	vec3 p0 = bSweepControlPoint0[sweepInstanceIndex].xyz;
	vec3 p1 = p0 + bSweepControlPoint1[sweepInstanceIndex].xyz;
	float t = position.z;

	uint sweepInfo = bSweepInfo[sweepInstanceIndex];
	bool isCap = (sweepInfo & 0x1) != 0;
	bool flipSweepProfile = (sweepInfo & 0x2) != 0;

	uint startInterfaceIndex = (sweepInfo & (0xff << 2)) >> 2;
	uint endInterfaceIndex = (sweepInfo & (0xff << 10)) >> 10;

	vec3 up = getUpDirection(slotIndex, startInterfaceIndex);
	up_out = up;

	// Local frame
	vec3 origin, dp;
	float radius = 1.0;
	if (isCap) {
		origin = mix(p0, p1, pow(t, 0.5) * 0.125);
		dp = p1 - p0;
		radius = 1 - pow(t, 2.0);
	} else {
		vec3 p3 = bSweepControlPoint3[sweepInstanceIndex].xyz;
		vec3 p2 = p3 + bSweepControlPoint2[sweepInstanceIndex].xyz;

		origin = bezier(p0, p1, p2, p3, t);
		dp = dbezier(p0, p1, p2, p3, t);

		up = mix(up, getUpDirection(slotIndex, endInterfaceIndex), t);
	}
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

	// Animation!
	//float profileScale = uProfileScale + 0.3 * sin(5 * uTime + 3 * origin.z - 1 * origin.x);
	//float profileScale = uProfileScale + 0.3 * sin(3 * uTime + 1 * origin.z - .3 * origin.x);
	//float profileScale = uProfileScale * (1 + 0.7 * sin(3 * origin.z - 1.3));
	float profileScale = uProfileScale;

	vec3 radial = (Y * position_local.x + Z * position_local.y) * profileScale;
	position_out = origin + radial * radius;
	normal_out = normalize(normal.x * Y + normal.y * Z);
	//normal_out = normalize(radial);

	// Test sphere
	//if (isCap) {
	//	position_out = vec3(cos(uv.y * 3.141592), cos(uv.x * 2 * 3.14159266) * sin(uv.y * 3.141592), sin(uv.x * 2 * 3.14159266) * sin(uv.y * 3.141592)) * 0.5;
	//	normal_out = normalize(position_out);
	//}

	//position_out = position * vec3(uProfileScale, uProfileScale, 1);
	//position_out.x += float(slotIndex) * 0.5f;

	uv_out = uv;
}

/**
 * Deform vertices of additional inner geometry
 */
void geometryVertexDeform(
	uint slotIndex,
	in vec3 position,
	in vec3 normal,
	in vec2 uv,
	out vec3 position_out,
	out vec3 normal_out,
	out vec2 uv_out,
	out vec3 up_out
) {

	mat4 macroNormals = bMacrosurfaceNormals[slotIndex];
	mat4 bottomCorners = bMacrosurfaceCorners[slotIndex];
	mat4 topCorners = bottomCorners + macroNormals;

	position_out = mix(
		mix(
			mix(
				bottomCorners[3],
				bottomCorners[0],
				position.x
			),
			mix(
				bottomCorners[2],
				bottomCorners[1],
				position.x
			),
			position.y
		),
		mix(
			mix(
				topCorners[3],
				topCorners[0],
				position.x
			),
			mix(
				topCorners[2],
				topCorners[1],
				position.x
			),
			position.y
		),
		position.z
	).xyz;

	vec3 N = normalize(mix(
		mix(
			macroNormals[3],
			macroNormals[0],
			position.x
		),
		mix(
			macroNormals[2],
			macroNormals[1],
			position.x
		),
		position.y
	).xyz);

	vec3 T = normalize(mix(
		normalize(bottomCorners[0] - bottomCorners[3]),
		normalize(bottomCorners[1] - bottomCorners[2]),
		position.y
	).xyz);

	vec3 B = normalize(mix(
		normalize(bottomCorners[2] - bottomCorners[3]),
		normalize(bottomCorners[1] - bottomCorners[0]),
		position.x
	).xyz);

	//vec3 B = normalize(cross(N, T));
	//T = normalize(cross(B, N));

	normal_out = mat3(T, B, N) * normal;

	uv_out = uv;
	up_out = N;//(macroNormals[0] + macroNormals[1] + macroNormals[2] + macroNormals[3]).xyz * 0.25;
}
