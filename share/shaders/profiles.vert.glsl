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

layout (location = 0) in vec3 position;
layout (location = 1) in vec2 uv;

readonly layout(std430) buffer InstanceMatrices
{
    mat4 bInstanceMatrices[];
};

readonly layout(std430) buffer CornerMatrices
{
    mat4 bCornerMatrices[];
};

out GeometryData {
    vec3 position_ws;
    flat uint tileEdge;
} geo;

#include "include/uniform/camera.inc.glsl"
uniform mat4 uModelMatrix = mat4(1);
uniform uint uInstanceStart = 0u;
uniform bool uUseInstancing;
uniform bool uUseCornerMatrices = false;
uniform float uThickness = 0.5;

mat4 mixMat4(mat4 a, mat4 b, float t) {
	return a * (1 - t) + b * t;
}

void main() {
	if (uUseCornerMatrices) {
		uint faceId = uInstanceStart + gl_InstanceID;
		mat4 m0 = bCornerMatrices[4 * faceId + 0];
		mat4 m1 = bCornerMatrices[4 * faceId + 1];
		mat4 m2 = bCornerMatrices[4 * faceId + 2];
		mat4 m3 = bCornerMatrices[4 * faceId + 3];

		mat4 m = mixMat4(
			mixMat4(m0, m1, position.x + 0.5),
			mixMat4(m3, m2, position.x + 0.5),
			position.y + 0.5
		);

		m = uModelMatrix * m;

		vec4 position_ws = m * vec4(0.0, 0.0, position.z * uThickness, 1.0);

		gl_Position = uCamera.projectionMatrix * uCamera.viewMatrix * position_ws;
		
		geo.position_ws = position_ws.xyz;
	} else {
		mat4 modelMatrix = uModelMatrix;
		if (uUseInstancing) {
			mat4 instanceMatrix = bInstanceMatrices[uInstanceStart + gl_InstanceID];
			modelMatrix = modelMatrix * instanceMatrix;
		}

		vec4 position_ws = modelMatrix * vec4(position, 1.0);
		gl_Position = uCamera.projectionMatrix * uCamera.viewMatrix * position_ws;
		
		geo.position_ws = position_ws.xyz;
	}

	geo.tileEdge = uint(uv.x);
}

