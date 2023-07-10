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

#define USE_MSAA
#define USE_PERMUTATION

layout (location = 0) in vec3 position;
layout (location = 1) in vec3 normal;
layout (location = 2) in vec3 uv;

readonly layout(std430) buffer MacrosurfaceNormals { mat4 bMacrosurfaceNormals[]; };
readonly layout(std430) buffer MacrosurfaceCorners { mat4 bMacrosurfaceCorners[]; };

out GeometryData {
	sample vec3 position_ws;
	sample vec3 normal_ws;
	sample vec3 uv;
} geo;

#include "include/uniform/camera.inc.glsl"
uniform mat4 uModelMatrix = mat4(1);
uniform uint uSlotStartIndex = 0u;
uniform uint uSweepIndex = 0u; // within a given slot
uniform float uProfileScale = 1.0;
uniform float uOffset = 0.0;
uniform float uThickness = 1.0;
uniform float uTime = 0.0;

#include "include/bezier.inc.glsl"
#include "include/mvc2.inc.glsl"

void main() {
	uint slotIndex = uSlotStartIndex + gl_InstanceID;
	vec3 position_ws = position;
	vec3 normal_ws = normal;

	vec3 position_ns = (position - vec3(0,0,0.5)) * vec3(2,2,2) * 1.00001;
	vec3 normal_ns = normal;

	mat4 cageBaseCorners = bMacrosurfaceCorners[slotIndex];
	mat4 cageNormals = bMacrosurfaceNormals[slotIndex];

	mat4 cageBottomCorners = cageBaseCorners + uOffset * cageNormals;
	mat4 cageTopCorners = cageBaseCorners + (uOffset + max(uThickness, 1e-4) * 0.5) * cageNormals;

	vec3[8] cagePoints;
	cagePoints[0] = cageBottomCorners[0].xyz;
	cagePoints[1] = cageBottomCorners[1].xyz;
	cagePoints[2] = cageBottomCorners[2].xyz;
	cagePoints[3] = cageBottomCorners[3].xyz;
	cagePoints[4] = cageTopCorners[0].xyz;
	cagePoints[5] = cageTopCorners[1].xyz;
	cagePoints[6] = cageTopCorners[2].xyz;
	cagePoints[7] = cageTopCorners[3].xyz;

	float[8] cageWeights;
	float[8] dpCageWeights;
	computeMeanValueCoordinates(position_ns, cageWeights);
	computeMeanValueCoordinates(position_ns + 1e-1 * normal_ns, dpCageWeights);

	position_ws = vec3(0);
	vec3 dp = vec3(0);
	for (int i = 0 ; i < 8 ; ++i) {
		position_ws += cagePoints[i] * cageWeights[i];
		dp += cagePoints[i] * dpCageWeights[i];
	}
	normal_ws = normalize(dp - position_ws);

	gl_Position = uCamera.projectionMatrix * uCamera.viewMatrix * uModelMatrix * vec4(position_ws, 1.0);
	
	geo.position_ws = position_ws.xyz;
	geo.normal_ws = normal_ws;
	geo.uv = uv;
}

