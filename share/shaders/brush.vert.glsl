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

out GeometryData {
    vec3 position_ws;
} geo;

#include "include/uniform/camera.inc.glsl"
uniform mat4 uModelMatrix = mat4(1);

uniform vec2 uBrushSize = vec2(1.0, 2.0);
uniform float uBrushScale = 0.1;
uniform float uBrushAngle = 0.0;

void main() {
	float th = uBrushAngle / 180 * 3.14159266;
	float c = cos(th), s = sin(th);
	vec2 p = mat2(c, s, -s, c) * (position.xy * uBrushSize * uBrushScale * 0.5);
	vec4 position_ws = uModelMatrix * vec4(p, 0.0, 1.0);

	gl_Position = uCamera.projectionMatrix * uCamera.viewMatrix * position_ws;
		
	geo.position_ws = position_ws.xyz;
}

