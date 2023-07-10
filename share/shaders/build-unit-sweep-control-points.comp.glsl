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
layout(local_size_x = 8, local_size_y = 1, local_size_z = 1) in;

struct SweepBuildingInfo {
	uint startTileEdge;
	uint startProfile;
	uint endTileEdge;
	uint endProfile;

	vec2 startCenter;
	vec2 endCenter;

	uint flags; // IS_CAP, START_FLIP, END_FLIP
	uint _pad0;
	uint _pad1;
	uint _pad2;
};
readonly layout(std430) buffer SweepBuildingInfos { SweepBuildingInfo bSweepBuildingInfo[]; };

struct SweepIndexInfo {
	uint slotIndex;
	uint tileTypeIndex;
	uint sweepIndex;
	bool flip;
};
readonly layout(std430) buffer SweepIndexInfos { SweepIndexInfo bSweepIndexInfo[]; };

writeonly layout(std430) buffer SweepControlPoint0 { vec4 bSweepControlPoint0[]; };
writeonly layout(std430) buffer SweepControlPoint1 { vec4 bSweepControlPoint1[]; };
writeonly layout(std430) buffer SweepControlPoint2 { vec4 bSweepControlPoint2[]; };
writeonly layout(std430) buffer SweepControlPoint3 { vec4 bSweepControlPoint3[]; };

const mat4 baseCorners = mat4(
	+0.5, -0.5, 0, 0,
	+0.5, +0.5, 0, 0,
	-0.5, +0.5, 0, 0,
	-0.5, -0.5, 0, 0
);
const mat4 normals = mat4(
	0, 0, 1, 0,
	0, 0, 1, 0,
	0, 0, 1, 0,
	0, 0, 1, 0
);

const float SQRT2 = 1.4142135623730951f;

const vec3[] cTileEdgeUp = vec3[](
	// Pos X
	vec3(1, 0, 0),
	// Pos Y
	vec3(0, 1, 0),
	// Neg X
	vec3(-1, 0, 0),
	// Neg Y
	vec3(0, -1, 0)
);

uniform uint uSweepCount;

uniform float uOffset = 0.25;
uniform float uThickness = 0.5;
uniform float uTangentMultiplier = 1.0;

// Normal of a quad
vec3 computeQuadNormal(vec3 a, vec3 b, vec3 c, vec3 d) {
	vec3 center = 0.25f * (a + b + c + d);

	return normalize(
		cross(a - center, b - center) +
		cross(b - center, c - center) +
		cross(c - center, d - center) +
		cross(d - center, a - center)
	);
}

/**
 * @param center position given in [0,1]² of the sweep origin within its tile edge
 */
void computeSweepTangent(
	uint tileEdgeIndex,
	vec2 center,
	bool flip,
	in mat4 bottomCorners,
	in mat4 topCorners,
	out vec3 origin,
	out vec3 tangent
) {
	vec3 c00 = bottomCorners[tileEdgeIndex].xyz;
	vec3 c01 = bottomCorners[(tileEdgeIndex + 1) % 4].xyz;
	vec3 c10 = topCorners[tileEdgeIndex].xyz;
	vec3 c11 = topCorners[(tileEdgeIndex + 1) % 4].xyz;

	if (flip) center.x = 1 - center.x;

	origin = mix(
		mix(c00, c01, center.x),
		mix(c10, c11, center.x),
		center.y
	);

	tangent = -computeQuadNormal(c00, c01, c11, c10);
}

void computeSweepHandleSizes(in vec3 p0, in vec3 p1, in vec3 t0, in vec3 t1, uint k0, uint k1, out float handle0, out float handle1) {
	float distance = distance(p0, p1);
	
	float circleRadius = k0 == k1 ? distance : distance * SQRT2 / 2;
	float handleOffsetBudget = circleRadius * 8 * (SQRT2 - 1) / 3;

	float alpha0 = acos(dot(normalize(p1 - p0), t0));
	float alpha1 = acos(dot(normalize(p0 - p1), t1));
	if (abs(alpha0 + alpha1) < 1e-4) {
		alpha0 = 1;
		alpha1 = 1;
	}

	handle0 = handleOffsetBudget * alpha0 / (alpha0 + alpha1);
	handle1 = handleOffsetBudget * alpha1 / (alpha0 + alpha1);
}

/*
uint transformEdgeDirection(uint tileEdgeIndex, in TileTransform tileTransform) {
	if (tileTransform.flipX && tileEdgeIndex % 2 == 0) tileEdgeIndex = (tileEdgeIndex + 2) % 4;
	if (tileTransform.flipY && tileEdgeIndex % 2 == 1) tileEdgeIndex = (tileEdgeIndex + 2) % 4;
	tileEdgeIndex = (tileEdgeIndex + tileTransform.orientation) % 4;

	return tileEdgeIndex;
}
*/

void main() {
	uint sweepInstanceIndex = gl_GlobalInvocationID.x;
	if (sweepInstanceIndex >= uSweepCount) return;

	SweepIndexInfo indices = bSweepIndexInfo[sweepInstanceIndex];
	uint slotIndex = indices.slotIndex;
	uint tileTypeIndex = indices.tileTypeIndex;
	uint sweepIndex = indices.sweepIndex;

	SweepBuildingInfo info = bSweepBuildingInfo[sweepIndex];
	bool isCap = (info.flags & 0x1) != 0;
	bool startFlip = (info.flags & 0x2) != 0;
	bool endFlip = (info.flags & 0x4) != 0;

	//TileTransform tileTransform = bTileTransform[slotIndex];
	bool flip = indices.flip;//tileTransform.flipX != tileTransform.flipY;
	if (flip) {
		//startFlip = !startFlip;
		//endFlip = !endFlip;
	}

	mat4 bottomCorners = baseCorners;
	mat4 topCorners = baseCorners + normals;

	//uint startTileEdge = transformEdgeDirection(info.startTileEdge, tileTransform);
	uint startTileEdge = info.startTileEdge;

	vec3 startOrigin, startTangent;
	vec3 endOrigin, endTangent;
	computeSweepTangent(startTileEdge, info.startCenter, startFlip, bottomCorners, topCorners, startOrigin, startTangent);
	endTangent = -endTangent;

	if (!isCap) {
		//int endTileEdge = transformEdgeDirection(info.endTileEdge, tileTransform);
		uint endTileEdge = info.endTileEdge;
		computeSweepTangent(endTileEdge, info.endCenter, endFlip, bottomCorners, topCorners, endOrigin, endTangent);

		float startHandle, endHandle;
		computeSweepHandleSizes(startOrigin, endOrigin, startTangent, endTangent, startTileEdge, endTileEdge, startHandle, endHandle);

		startTangent *= startHandle * uTangentMultiplier;
		endTangent *= endHandle * uTangentMultiplier;
	}

	if (flip) {
		startTangent = -startTangent;
		endTangent = -endTangent;
	}

	////////////////////////////////////////////////////////
	// Write output

	//startOrigin = vec3(-0.5, 0, 0);
	//startTangent = vec3(1, 0, 0);
	//endTangent = vec3(-1, 0, 0);
	//endOrigin = vec3(0.5, 0, 1);
	//if (isCap) {
	//	endOrigin = vec3(0, 0, -10);
	//	bSweepControlPoint3[sweepInstanceIndex] = vec4(endOrigin, 1.0);
	//}

	bSweepControlPoint0[sweepInstanceIndex] = vec4(startOrigin, 1.0);
	bSweepControlPoint1[sweepInstanceIndex] = vec4(startTangent, 0.0);

	if (!isCap) {
		bSweepControlPoint2[sweepInstanceIndex] = vec4(endTangent, 0.0);
		bSweepControlPoint3[sweepInstanceIndex] = vec4(endOrigin, 1.0);
	}

}
