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
layout(local_size_x = 8, local_size_y = 8, local_size_z = 8) in;

readonly layout(std430) buffer SweepInfos { uint bSweepInfo[]; };

readonly layout(std430) buffer SweepControlPoint0 { vec4 bSweepControlPoint0[]; };
readonly layout(std430) buffer SweepControlPoint1 { vec4 bSweepControlPoint1[]; };
readonly layout(std430) buffer SweepControlPoint2 { vec4 bSweepControlPoint2[]; };
readonly layout(std430) buffer SweepControlPoint3 { vec4 bSweepControlPoint3[]; };

readonly layout(std430) buffer MacrosurfaceNormals { mat4 bMacrosurfaceNormals[]; };
readonly layout(std430) buffer MacrosurfaceCorners { mat4 bMacrosurfaceCorners[]; };

readonly layout(std430) buffer SweepControlPointOffset { uint bSweepControlPointOffset[]; };

readonly layout(std430) buffer SweepVbo { vec4 bSweepVbo[]; };

writeonly layout(std430) buffer OutputPoints { vec4 bOutputPoints[]; };
writeonly layout(std430) buffer OutputNormal { vec4 bOutputNormal[]; };
writeonly layout(std430) buffer OutputUv { vec4 bOutputUv[]; };

uniform mat4 uModelMatrix = mat4(1);
uniform uint uSlotStartIndex = 0u;
uniform uint uSweepIndex = 0u; // within a given slot
uniform float uProfileScale = 1.0;
uniform float uTime = 0.0;

#include "include/bezier.inc.glsl"
#include "mesostructure/vertex-deform.inc.glsl"

uniform uint uPointCount;
uniform uint uNormalBufferStartOffset;
uniform uint uUvBufferStartOffset;
uniform uint uInstanceCount = 2;

void main() {
	uint instanceId = gl_GlobalInvocationID.y;
	uint vertexIndex = gl_GlobalInvocationID.x;
	if (instanceId >= uInstanceCount || vertexIndex >= uPointCount) return;
	uint outputIndex = instanceId * uPointCount + vertexIndex;

	vec3 position = bSweepVbo[vertexIndex].xyz;
	vec3 normal = bSweepVbo[uNormalBufferStartOffset + vertexIndex].xyz;
	vec2 uv = bSweepVbo[uUvBufferStartOffset + vertexIndex].xy;

	// DEBUG - passthrough
	/*
	bOutputPoints[outputIndex].xyz = position.xyz;
	bOutputNormal[outputIndex].xyz = normal.xyz;
	bOutputUv[outputIndex].xy = uv.xy;
	return;
	//*/

	// <COMMON>
	uint slotIndex = uSlotStartIndex + instanceId;
	vec3 position_ws, normal_ws, up;
	vertexDeform(slotIndex, position, normal, uv.xy, position_ws, normal_ws, uv, up);
	// </COMMON>
	
	bOutputPoints[outputIndex].xyz = position_ws.xyz;
	bOutputNormal[outputIndex].xyz = normal_ws.xyz;
	bOutputUv[outputIndex].xy = uv.xy;
}
