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
#include "Bezier.h"

using glm::vec3;

std::vector<vec3> Bezier::InsertBezier(const vec3& a, const vec3& b, const vec3& c, const vec3& d, std::vector<GLfloat>& pointData, std::vector<GLuint>& elementData, GLuint restartIndex) {
	GLuint idx = static_cast<GLuint>(pointData.size() / 4);

	std::vector<vec3> line;

	for (float t = 0; t < 1.05f; t += 0.05f) {
		vec3 Bab = a + t * (b - a);
		vec3 Bbc = b + t * (c - b);
		vec3 Bcd = c + t * (d - c);

		vec3 Babc = Bab + t * (Bbc - Bab);
		vec3 Bbcd = Bbc + t * (Bcd - Bbc);

		vec3 p = Babc + t * (Bbcd - Babc);

		pointData.push_back(p.x);
		pointData.push_back(p.y);
		pointData.push_back(p.z);
		pointData.push_back(t);

		elementData.push_back(idx++);

		line.push_back(p);
	}

	elementData.push_back(restartIndex);
	return line;
}

vec3 Bezier::EvalBezier(const vec3& a, const vec3& b, const vec3& c, const vec3& d, float t) {
	vec3 Bab = a + t * (b - a);
	vec3 Bbc = b + t * (c - b);
	vec3 Bcd = c + t * (d - c);

	vec3 Babc = Bab + t * (Bbc - Bab);
	vec3 Bbcd = Bbc + t * (Bcd - Bbc);

	return Babc + t * (Bbcd - Babc);
}

vec3 Bezier::EvalBezierDerivative(const vec3& a, const vec3& b, const vec3& c, const vec3& d, float t) {
	vec3 u = 3.0f * (b - a);
	vec3 v = 3.0f * (c - b);
	vec3 w = 3.0f * (d - c);

	vec3 Buv = u + t * (v - u);
	vec3 Bvw = v + t * (w - v);

	return Buv + t * (Bvw - Buv);
}

