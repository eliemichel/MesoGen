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
#include "CsgCompiler.h"

#include <Logger.h>

#include <Clipper/clipper.hpp>
#include <glm/glm.hpp>
#include <variant>
#include <sstream>

using glm::vec2;
using glm::mat2;

using namespace Model::Csg2D;

using namespace ClipperLib;

Paths CsgCompiler::compileClipperPaths(const Csg2D & csg, const Options& opts) const {
	if (auto* disc = std::get_if<Disc>(&csg)) {
		return compileClipperPaths(*disc, opts);
	}
	else if (auto* box = std::get_if<Box>(&csg)) {
		return compileClipperPaths(*box, opts);
	}
	else if (auto* unionOp = std::get_if<Union>(&csg)) {
		return compileClipperPaths(*unionOp, opts);
	}
	else if (auto* differenceOp = std::get_if<Difference>(&csg)) {
		return compileClipperPaths(*differenceOp, opts);
	}
	else {
		throw CompileError("Variant not supported by compileGlslToClipperPaths: " + std::to_string(csg.index()));
	}
}

std::vector<vec2> CsgCompiler::compileGlm(const Model::Csg2D::Csg2D& csg, const Options& opts) const {
	ClipperLib::Paths clipper = compileClipperPaths(csg, opts);
	std::vector<vec2> profile;
	for (auto segment : clipper) {
		for (auto pt : segment) {
			profile.push_back(vec2(pt.X, pt.Y) * opts.maxSegmentLength / 100.0f);
		}
	}
	return LimitSegmentSize(profile, opts.maxSegmentLength);
}

Paths CsgCompiler::compileClipperPaths(const Model::Csg2D::Disc& disc, const Options &opts) const {
	constexpr float TWO_PI = 2 * 3.14159266f;
	float step = opts.maxSegmentLength; // length of segments
	float scale = 100 / step; // applied before discretization to int coords

	vec2 center = disc.center * vec2(1.0f, 0.5f);
	float c = cos(disc.angle);
	float s = sin(disc.angle);
	mat2 rot = mat2(c, s, -s, c);
	float perimeter = TWO_PI * sqrt((disc.size.x * disc.size.x + disc.size.y * disc.size.y) * 0.5f) * 0.5f;
	int stepCount = static_cast<int>(floor(perimeter / step));
	Path path;
	for (int i = 0; i < stepCount + 1; ++i) {
		float theta = (TWO_PI * i) / stepCount;
		vec2 p(cos(theta), sin(theta));
		p = center + rot * (disc.size * 0.5f * p);
		IntPoint pt(
			static_cast<cInt>(scale * p.x),
			static_cast<cInt>(scale * p.y)
		);
		path.push_back(pt);
	}
	return Paths{ path };
}

Paths CsgCompiler::compileClipperPaths(const Model::Csg2D::Box& box, const Options& opts) const {
	constexpr float TWO_PI = 2 * 3.14159266f;
	float step = opts.maxSegmentLength; // length of segments
	float scale = 100 / step; // applied before discretization to int coords

	static const std::vector<vec2> points = {
		{ -0.5f, -0.5f },
		{ +0.5f, -0.5f },
		{ +0.5f, +0.5f },
		{ -0.5f, +0.5f },
		{ -0.5f, -0.5f }
	};

	float c = cos(box.angle);
	float s = sin(box.angle);
	mat2 rot = mat2(c, s, -s, c);
	vec2 center = box.center * vec2(1.0f, 0.5f);

	Path path;
	for (vec2 p : points) {
		p = center + rot * (box.size * p);
		path.push_back(IntPoint(
			static_cast<cInt>(scale * p.x),
			static_cast<cInt>(scale * p.y)
		));
	}
	return Paths{ path };
}
	
Paths CsgCompiler::compileClipperPaths(const Model::Csg2D::Union& unionOp, const Options& opts) const {
	if (!unionOp.lhs && !unionOp.rhs) return Paths();
	if (!unionOp.lhs) return compileClipperPaths(*unionOp.rhs, opts);
	if (!unionOp.rhs) return compileClipperPaths(*unionOp.lhs, opts);
	
	Clipper clipper;
	clipper.AddPaths(compileClipperPaths(*unionOp.rhs, opts), PolyType::ptSubject, true /* closed */);
	clipper.AddPaths(compileClipperPaths(*unionOp.lhs, opts), PolyType::ptClip, true /* closed */);

	Paths solution;
	bool success = clipper.Execute(ClipType::ctUnion, solution, PolyFillType::pftEvenOdd);
	if (!success) {
		ERR_LOG << "Could not compile profile shape!";
	}

	return solution;
}

Paths CsgCompiler::compileClipperPaths(const Model::Csg2D::Difference& differenceOp, const Options& opts) const {
	if (!differenceOp.lhs && !differenceOp.rhs) return Paths();
	if (!differenceOp.lhs) return compileClipperPaths(*differenceOp.rhs, opts);
	if (!differenceOp.rhs) return compileClipperPaths(*differenceOp.lhs, opts);

	Clipper clipper;
	clipper.AddPaths(compileClipperPaths(*differenceOp.lhs, opts), PolyType::ptSubject, true /* closed */);
	clipper.AddPaths(compileClipperPaths(*differenceOp.rhs, opts), PolyType::ptClip, true /* closed */);

	Paths solution;
	bool success = clipper.Execute(ClipType::ctDifference, solution, PolyFillType::pftEvenOdd);
	if (!success) {
		ERR_LOG << "Could not compile profile shape!";
	}

	return solution;
}

std::vector<vec2> CsgCompiler::LimitSegmentSize(const std::vector<vec2>& profile, float maxSegmentLength) {
	std::vector<vec2> outputProfile;
	if (profile.empty()) return profile;
	float minLength = maxSegmentLength / 10;
	vec2 prevP = profile[profile.size() - 1];
	for (vec2 p : profile) {
		float length = distance(prevP, p);
		if (length < minLength) continue;
		int stepCount = std::max(static_cast<int>(ceil(length / maxSegmentLength / 2)), 1);
		for (int i = 0; i < stepCount; ++i) {
			float fac = static_cast<float>(i + 1) / stepCount;
			outputProfile.push_back(mix(prevP, p, fac));
		}
		prevP = p;
	}
	return outputProfile;
}
