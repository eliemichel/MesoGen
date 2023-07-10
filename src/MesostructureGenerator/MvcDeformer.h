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

#include <glm/glm.hpp>

#include <vector>

/**
 * Compute Mean Value Coordinate based deformation
 */
class MvcDeformer {
public:
	MvcDeformer(
		const std::vector<glm::vec3>& corners,
		const std::vector<glm::uvec3>& cageTriangles,
		const glm::uvec4& permutation
	);

	glm::vec3 tileToWorld(const glm::vec3& position) const;
	//glm::vec3 worldToTile(const glm::vec3& position) const;

public:
	void computeMeanValueCoordinates(const glm::vec3& position, std::vector<float>& cageWeights, float eps = 1e-5) const;

private:
	std::vector<glm::vec3> m_corners;
	std::vector<glm::uvec3> m_cageTriangles;
	glm::mat3 m_transform;

	glm::uvec4 m_permutation;

	// Cached memory
	mutable std::vector<float> m_cageWeights;

private:
	static const std::vector<glm::vec3> s_restCorners;
};
