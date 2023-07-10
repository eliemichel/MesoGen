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

readonly layout(std430) buffer MacrosurfaceNormals { mat4 bMacrosurfaceNormals[]; };
readonly layout(std430) buffer MacrosurfaceCorners { mat4 bMacrosurfaceCorners[]; };


#include "include/uniform/camera.inc.glsl"
uniform mat4 uModelMatrix = mat4(1);
uniform uint uSlotStartIndex = 0u;
uniform bool uUseInstancing;

uniform float uThickness = 0.5;
uniform float uOffset = 0.5;

void main() {
	uint slotIndex = 0;
	mat4 cornerPositions = mat4(
		 0.5, -0.5, 0.0, 1.0,
		 0.5,  0.5, 0.0, 1.0,
		-0.5,  0.5, 0.0, 1.0,
		-0.5, -0.5, 0.0, 1.0
	);
	mat4 normals = mat4(
		0.0, 0.0, 1.0, 0.0,
		0.0, 0.0, 1.0, 0.0,
		0.0, 0.0, 1.0, 0.0,
		0.0, 0.0, 1.0, 0.0
	);

	if (uUseInstancing) {
		slotIndex = uSlotStartIndex + gl_InstanceID;
		cornerPositions = bMacrosurfaceCorners[slotIndex];
		normals = bMacrosurfaceNormals[slotIndex];
	}

	cornerPositions += uOffset * normals;
	normals *= uThickness;

	// TODO: use look-up rather than mixes
	float u = position.x + 0.5;
	float v = position.y + 0.5;
	float margin = 0.0001;
	u = mix(margin, 1 - margin, u);
	v = mix(margin, 1 - margin, v);
	vec3 position_ws = mix(
		mix(cornerPositions[3].xyz, cornerPositions[0].xyz, u),
		mix(cornerPositions[2].xyz, cornerPositions[1].xyz, u),
		v
	) + position.z * mix(
		mix(normals[3].xyz, normals[0].xyz, u),
		mix(normals[2].xyz, normals[1].xyz, u),
		v
	);

	gl_Position = uCamera.projectionMatrix * uCamera.viewMatrix * uModelMatrix * vec4(position_ws, 1.0);
}

