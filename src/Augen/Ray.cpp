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
#include "Ray.h"

using glm::vec3;
using glm::vec4;
using glm::mat3;

Ray RayUtils::transform(const Ray& ray, const glm::mat4& matrix) {
	return Ray(
		matrix * vec4(ray.origin, 1.0f),
		mat3(matrix) * ray.direction
	);
}

bool RayUtils::intersectPlane(
	const Ray& ray,
	const glm::vec3& planeNormal,
	float planeOrigin,
	glm::vec3& hitPoint,
	float& hitLambda
) {
	float denom = dot(ray.direction, planeNormal);
	if (std::abs(denom) < 1e-6) return false;
	hitLambda = (planeOrigin - dot(ray.origin, planeNormal)) / denom;
	hitPoint = ray.origin + hitLambda * ray.direction;
	return hitLambda >= 0;
}

float RayUtils::intersectLine(
	const Ray& ray,
	const glm::vec3& a,
	const glm::vec3& b,
	float& hitLambda,
	float& hitGamma
) {
	Ray line(a, b - a);
	vec3 n = cross(ray.direction, line.direction);
	float ln2 = dot(n, n);
	float ln = std::sqrt(ln2);
	hitLambda = dot(cross(line.origin - ray.origin, line.direction), n) / ln2;
	hitGamma = dot(cross(line.origin - ray.origin, ray.direction), n) / ln2;
	float distance = abs(dot(ray.origin - line.origin, n) / ln);
	return distance;
}
