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

#include <string>
#include <exception>

/**
 * Class used for CPU evaluation of 2D CSG
 * (for GPU, see CsgCOmpiler)
 */
class CsgEngine {
	using Shape = Model::Shape;
	using Csg2D = Model::Csg2D::Csg2D;
	using Disc = Model::Csg2D::Disc;
	using Box = Model::Csg2D::Box;
	using Union = Model::Csg2D::Union;
	using Difference = Model::Csg2D::Difference;

public:
	class EvalError : public std::exception {
	public:
		EvalError(const std::string& message) : m_message(message) {}
		const char* what() const throw () { return m_message.c_str(); }

	private:
		std::string m_message;
	};

public:
	glm::vec2 center(const Csg2D& csg) const;

	float distance(const Csg2D& a, const Csg2D& b) const;

private:
	glm::vec2 center(const Disc& disc) const;
	glm::vec2 center(const Box& box) const;
	glm::vec2 center(const Union& unionOp) const;
	glm::vec2 center(const Difference& differenceOp) const;
};

namespace Model {
namespace Csg2D {

inline glm::vec2 center(const Model::Csg2D::Csg2D& csg) {
	return CsgEngine().center(csg);
}

inline float distance(const Model::Csg2D::Csg2D& a, const Model::Csg2D::Csg2D& b) {
	return CsgEngine().distance(a, b);
}

}
}
