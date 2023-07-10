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
#include "CsgEngine.h"
#include "CsgCompiler.h"

#include <glm/glm.hpp>
#include <variant>
#include <algorithm>
#include <limits>

using namespace Model::Csg2D;
using glm::vec2;
using glm::mat2;

vec2 CsgEngine::center(const Csg2D& csg) const {
	if (auto* disc = std::get_if<Disc>(&csg)) {
		return center(*disc);
	}
	else if (auto* box = std::get_if<Box>(&csg)) {
		return center(*box);
	}
	else if (auto* unionOp = std::get_if<Union>(&csg)) {
		return center(*unionOp);
	}
	else if (auto* differenceOp = std::get_if<Difference>(&csg)) {
		return center(*differenceOp);
	}
	else {
		throw EvalError("Variant not supported by center: " + std::to_string(csg.index()));
	}
}

float CsgEngine::distance(const Csg2D& a, const Csg2D& b) const {
	CsgCompiler compiler;
	CsgCompiler::Options opts;
	opts.maxSegmentLength = 1e-2f;
	std::vector<vec2> pointsA = compiler.compileGlm(a, opts);
	std::vector<vec2> pointsB = compiler.compileGlm(b, opts);
	float minSqDistance = 1e8f;

	for (int i = 1; i < pointsA.size(); ++i) {
		const vec2& ptA0 = pointsA[i - 1];
		const vec2& ptA1 = pointsA[i];
		vec2 sA = ptA1 - ptA0;

		for (int j = 1; j < pointsB.size(); ++j) {
			const vec2& ptB0 = pointsB[j - 1];
			const vec2& ptB1 = pointsB[j];
			vec2 sB = ptB1 - ptB0;

			mat2 M(sA, sB);
			vec2 t = inverse(M) * (ptB0 - ptA0);
			float tA = glm::clamp(t.x, 0.0f, 1.0f);
			float tB = glm::clamp(-t.y, 0.0f, 1.0f);

			vec2 diff = (ptA0 + tA * sA) - (ptB0 + tB * sB);
			float sqDistance = dot(diff, diff);
			minSqDistance = std::min(minSqDistance, sqDistance);
		}
	}
	return sqrt(minSqDistance);
}

//-----------------------------------------------------------------------------

vec2 CsgEngine::center(const Disc& disc) const {
	return disc.center;
}

vec2 CsgEngine::center(const Box& box) const {
	return box.center;
}

vec2 CsgEngine::center(const Union& unionOp) const {
	if (!unionOp.lhs && !unionOp.rhs) {
		throw EvalError("Empty union is not allowed");
	}
	if (!unionOp.lhs) return center(*unionOp.rhs);
	if (!unionOp.rhs) return center(*unionOp.lhs);

	vec2 lhs = center(*unionOp.lhs);
	vec2 rhs = center(*unionOp.rhs);

	return (lhs + rhs) * 0.5f;
}

vec2 CsgEngine::center(const Difference& differenceOp) const {
	if (!differenceOp.lhs || !differenceOp.rhs) {
		throw EvalError("Empty member in difference is not allowed");
	}

	vec2 lhs = center(*differenceOp.lhs);
	//vec2 rhs = center(*differenceOp.rhs);

	return lhs;
}
