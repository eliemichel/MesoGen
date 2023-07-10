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

readonly layout(std430) buffer SweepInfos { uint bSweepInfo[]; };

readonly layout(std430) buffer SweepControlPoint0 { vec4 bSweepControlPoint0[]; };
readonly layout(std430) buffer SweepControlPoint1 { vec4 bSweepControlPoint1[]; };
readonly layout(std430) buffer SweepControlPoint2 { vec4 bSweepControlPoint2[]; };
readonly layout(std430) buffer SweepControlPoint3 { vec4 bSweepControlPoint3[]; };

readonly layout(std430) buffer MacrosurfaceNormals { mat4 bMacrosurfaceNormals[]; };
readonly layout(std430) buffer MacrosurfaceCorners { mat4 bMacrosurfaceCorners[]; };

readonly layout(std430) buffer SweepControlPointOffset { uint bSweepControlPointOffset[]; };


out GeometryData {
	sample vec3 position_ws;
	sample vec3 normal_ws;
	sample vec2 uv;
	sample vec3 macro_normal_ws;
} geo;

#include "include/uniform/camera.inc.glsl"
uniform mat4 uModelMatrix = mat4(1);
uniform uint uSlotStartIndex = 0u;
uniform uint uSweepIndex = 0u; // within a given slot
uniform float uProfileScale = 1.0;
uniform mat4 uGeoTransform = mat4(1);
uniform float uTime = 0.0;
uniform float uOffset = 0.25;
uniform float uThickness = 0.5;

#include "include/bezier.inc.glsl"
#include "mesostructure/vertex-deform.inc.glsl"

void main() {
	uint slotIndex = uSlotStartIndex + gl_InstanceID;
	mat4 extraTransform = transpose(mat4(
		1.0, 0.0, 0.0, 0.0,
		0.0, 1.0, 0.0, 0.0,
		0.0, 0.0, uThickness, uOffset,
		0.0, 0.0, 0.0, 1.0
	));
	vec3 tr_position = (extraTransform * uGeoTransform * vec4(position, 1.0)).xyz;
	vec3 tr_normal = mat3(extraTransform) * mat3(uGeoTransform) * normal;
	geometryVertexDeform(slotIndex, tr_position, tr_normal, uv.xy, geo.position_ws, geo.normal_ws, geo.uv, geo.macro_normal_ws);
	geo.normal_ws = mat3(uModelMatrix) * geo.normal_ws;
	geo.position_ws = (uModelMatrix * vec4(geo.position_ws, 1.0)).xyz;

	gl_Position = uCamera.projectionMatrix * uCamera.viewMatrix * vec4(geo.position_ws, 1.0);
}

