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
//////////////////////////////////////////////////////
// Z-Buffer utils
// Requires a "include/uniform/camera.inc.glsl" and "include/utils.inc.glsl"

// CHange these uniforms if using glDepthRange()
uniform float uDepthRangeMin = 0.0f;
uniform float uDepthRangeMax = 1.0f;

float linearizeDepth_perspective(float near, float far, float logDepth) {
	float in01 = (logDepth - uDepthRangeMin) / (uDepthRangeMax - uDepthRangeMin);
	return (2.0 * near * far) / (far + near - (in01 * 2.0 - 1.0) * (far - near));
}

float linearizeDepth_orthographic(float near, float far, float logDepth) {
	return near + logDepth * (far - near);
}

float linearizeDepth(in mat4 projectionMatrix, float logDepth) {
	float near = getNearDistance(projectionMatrix);
	float far = getFarDistance(projectionMatrix);
	return isOrthographic(projectionMatrix)
		? linearizeDepth_orthographic(near, far, logDepth)
		: linearizeDepth_perspective(near, far, logDepth);
}

float linearizeDepth01(in mat4 projectionMatrix, float logDepth) {
	float near = getNearDistance(projectionMatrix);
	float far = getFarDistance(projectionMatrix);
	float depth = isOrthographic(projectionMatrix)
		? linearizeDepth_orthographic(near, far, logDepth)
		: linearizeDepth_perspective(near, far, logDepth);
	return (depth - near) / (far - near);
}

float unlinearizeDepth_perspective(float near, float far, float linearDepth) {
	float in01 = (far + near - (2.0 * near * far) / linearDepth) / (far - near) * 0.5 + 0.5;
	return uDepthRangeMin + in01 * (uDepthRangeMax - uDepthRangeMin);
}

float unlinearizeDepth_orthographic(float near, float far, float linearDepth) {
	return (linearDepth - near) / (far - near);
}

float unlinearizeDepth(in mat4 projectionMatrix, float linearDepth) {
	float near = getNearDistance(projectionMatrix);
	float far = getFarDistance(projectionMatrix);
	return isOrthographic(projectionMatrix)
		? unlinearizeDepth_orthographic(near, far, linearDepth)
		: unlinearizeDepth_perspective(near, far, linearDepth);
}

