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
#include "utils/streamutils.h"
#include <Ray.h>

#include <catch2/catch_test_macros.hpp>

using glm::vec3;

TEST_CASE("ray plane instersection", "[Ray]") {
	SECTION("hit case") {
		Ray ray(vec3(0, 0, 0), vec3(1, 1, 1));

		vec3 hitPoint;
		float hitLambda;

		vec3 planeNormal(0, 0, 1);
		float planeOffset = 2;

		bool hit = intersectPlane(
			ray,
			planeNormal,
			planeOffset,
			hitPoint,
			hitLambda
		);

		REQUIRE(hit);
		REQUIRE(ray.origin + hitLambda * ray.direction == hitPoint);
		REQUIRE(dot(hitPoint, planeNormal) == planeOffset);
	}

	SECTION("no hit case") {
		Ray ray(vec3(5, 8, 0), vec3(2, -1, 1));

		vec3 hitPoint;
		float hitLambda;

		vec3 planeNormal(0, 1, 0);
		float planeOffset = 12;

		bool hit = intersectPlane(
			ray,
			planeNormal,
			planeOffset,
			hitPoint,
			hitLambda
		);

		REQUIRE(!hit);
	}
}

TEST_CASE("ray line instersection", "[Ray]") {
	SECTION("simple hit case") {
		Ray ray(vec3(0, 0, 0), vec3(1, 1, 0));
		vec3 a(0, 1, 0);
		vec3 b(1, 0, 0);
		float thickness = 1e-5f;

		float hitLambda, hitGamma;

		float distance = intersectLine(ray, a, b, hitLambda, hitGamma);

		vec3 rayPoint = ray.origin + hitLambda * ray.direction;
		vec3 linePoint = a + hitGamma * (b - a);

		INFO("hitLambda = " << hitLambda << " (expecting " << 0.5f << ")");
		REQUIRE(hitLambda == 0.5f);

		INFO("hitGamma = " << hitGamma << " (expecting " << 0.5f << ")");
		REQUIRE(hitGamma == 0.5f);

		REQUIRE(distance <= thickness);

		float checkDistance = length(rayPoint - linePoint);
		INFO("rayPoint = " << rayPoint);
		INFO("linePoint = " << linePoint);
		REQUIRE(checkDistance == distance);

	}

	SECTION("another hit case") {
		Ray ray(vec3(0, 0, 0), vec3(1, 2, 0));
		vec3 a(1, 0, 0);
		vec3 b(0, 8, 0);
		float thickness = 1e-5f;

		float hitLambda, hitGamma;

		float distance = intersectLine(ray, a, b, hitLambda, hitGamma);

		vec3 rayPoint = ray.origin + hitLambda * ray.direction;
		vec3 linePoint = a + hitGamma * (b - a);

		REQUIRE(distance <= thickness);

		float checkDistance = length(rayPoint - linePoint);
		INFO("rayPoint = " << rayPoint);
		INFO("linePoint = " << linePoint);
		REQUIRE(checkDistance == distance);
	}

	SECTION("no hit case") {
		Ray ray(vec3(0, 0, 0), vec3(1, 2, 0));
		vec3 a(1, 0, 0);
		vec3 b(0, 8, 1);
		float thickness = 0.01f;

		float hitLambda, hitGamma;
		
		float distance = intersectLine(ray, a, b, hitLambda, hitGamma);

		vec3 rayPoint = ray.origin + hitLambda * ray.direction;
		vec3 linePoint = a + hitGamma * (b - a);
		float checkDistance = length(rayPoint - linePoint);

		REQUIRE(checkDistance == distance);

		INFO("rayPoint = " << rayPoint);
		INFO("linePoint = " << linePoint);
		REQUIRE(distance > thickness);
	}
}
