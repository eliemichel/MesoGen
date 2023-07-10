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
#pragma once

#include "Model.h"

#include <glm/glm.hpp>
#include <Clipper/clipper.hpp>

#include <string>
#include <exception>

class CsgCompiler {
public:
	struct Options {
		/**
		 * Use Clipper to compute true booleans
		 */
		bool useBooleans = false;

		/**
		 * Length of segments when using discretization
		 */
		float maxSegmentLength = 1e-3f;
	};

public:
	class CompileError : public std::exception {
	public:
		CompileError(const std::string& message) : m_message(message) {}
		const char* what() const throw () { return m_message.c_str(); }

	private:
		std::string m_message;
	};

public:
    /**
     * Compile a CSG into a GLSL snippet
     */
	std::string compileGlsl(const Model::Csg2D::Csg2D& csg) const;

	/**
	 * Compile a CSG into a GLSL snippet using Clipper
	 */
	ClipperLib::Paths compileClipperPaths(const Model::Csg2D::Csg2D& csg, const Options& opts) const;

	/**
	 * Compile a CSG into a GLSL snippet using Clipper, return as a flatten path using glm
	 */
	std::vector<glm::vec2> compileGlm(const Model::Csg2D::Csg2D& csg, const Options& opts) const;

private:
	ClipperLib::Paths compileClipperPaths(const Model::Csg2D::Disc& disc, const Options& opts) const;
	ClipperLib::Paths compileClipperPaths(const Model::Csg2D::Box& box, const Options& opts) const;
	ClipperLib::Paths compileClipperPaths(const Model::Csg2D::Union& unionOp, const Options& opts) const;
	ClipperLib::Paths compileClipperPaths(const Model::Csg2D::Difference& differenceOp, const Options& opts) const;

	/**
	 * Split segments that are longer than twice the threshold value.
	 * Also remove points that are too close (less than 1/10 of the value)
	 */
	static std::vector<glm::vec2> LimitSegmentSize(const std::vector<glm::vec2>& profile, float maxSegmentLength);
};
