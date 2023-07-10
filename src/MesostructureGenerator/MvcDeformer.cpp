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
#include "MvcDeformer.h"

using glm::uvec3;
using glm::mat3;
using glm::vec4;
using glm::vec3;
using glm::vec2;
using glm::mat2;
using glm::mat4;

const std::vector<vec3> MvcDeformer::s_restCorners{
	{+1, -1, -1},
	{+1, +1, -1},
	{-1, +1, -1},
	{-1, -1, -1},

	{+1, -1, +1},
	{+1, +1, +1},
	{-1, +1, +1},
	{-1, -1, +1}
};

MvcDeformer::MvcDeformer(
	const std::vector<vec3>& corners,
	const std::vector<uvec3>& cageTriangles,
	const glm::uvec4& permutation
)
	: m_corners(corners)
	, m_cageTriangles(cageTriangles)
	, m_permutation(permutation)
{
	m_corners = std::vector<vec3>{
		corners[permutation[0]],
		corners[permutation[1]],
		corners[permutation[2]],
		corners[permutation[3]],
		corners[4 + permutation[0]],
		corners[4 + permutation[1]],
		corners[4 + permutation[2]],
		corners[4 + permutation[3]],
	};
}

vec3 MvcDeformer::tileToWorld(const vec3& position) const {
	vec3 position_ts = position;
	vec3 position_sts = position_ts / vec3(0.5, 0.5, 0.25) - vec3(0.0, 0.0, 1.0);

	computeMeanValueCoordinates(position_sts, m_cageWeights);

	vec3 position_ws = vec3(0.0);
	assert(m_cageWeights.size() == m_corners.size());
	for (int i = 0; i < m_corners.size(); ++i) {
		position_ws += m_cageWeights[i] * m_corners[i];
	}

	return position_ws;
}

// ----------------------------------------------------------------------------

vec3 getBarycentricCoords(const vec3 &p, const vec3& a, const vec3& b, const vec3& c) {
	vec3 v0 = b - a, v1 = c - a, v2 = p - a;
	float d00 = dot(v0, v0);
	float d01 = dot(v0, v1);
	float d11 = dot(v1, v1);
	float d20 = dot(v2, v0);
	float d21 = dot(v2, v1);
	float denom = d00 * d11 - d01 * d01;
	float v = (d11 * d20 - d01 * d21) / denom;
	float w = (d00 * d21 - d01 * d20) / denom;
	float u = 1.0f - v - w;
	return vec3(u, v, w);
}

void MvcDeformer::computeMeanValueCoordinates(const vec3 &position, std::vector<float> & cageWeights, float eps) const {
	cageWeights.resize(s_restCorners.size());
	for (int i = 0; i < cageWeights.size(); ++i) { cageWeights[i] = 0; }

	for (int i = 0; i < m_cageTriangles.size(); ++i) {
		uvec3 indices = m_cageTriangles[i];

		mat3 tri = mat3(
			s_restCorners[indices.x],
			s_restCorners[indices.y],
			s_restCorners[indices.z]
		);

		// eq 14
		mat3 relTri = tri - mat3(position, position, position);
		vec3 N0 = cross(relTri[2], relTri[1]);
		vec3 N1 = cross(relTri[0], relTri[2]);
		vec3 N2 = cross(relTri[1], relTri[0]);

		float l0 = length(relTri[0]);
		float l1 = length(relTri[1]);
		float l2 = length(relTri[2]);

		bool inPlane = determinant(relTri) < eps;
		//bool inPlane = abs(l0) < eps || abs(l1) < eps || abs(l2) < eps;
		if (inPlane) {
			vec3 bcoords = getBarycentricCoords(position, tri[0], tri[1], tri[2]);
			bool inTriangle = bcoords.x >= 0 && bcoords.x <= 1 && bcoords.y >= 0 && bcoords.y <= 1 && bcoords.z <= 1;
			if (inTriangle) {
				for (int j = 0; j < cageWeights.size(); ++j) { cageWeights[j] = 0; }
				cageWeights[indices.x] = bcoords.x;
				cageWeights[indices.y] = bcoords.y;
				cageWeights[indices.z] = bcoords.z;
				return;
			}
		}

		float th0 = acos(dot(relTri[1], relTri[2]) / (l1 * l2));
		float th1 = acos(dot(relTri[2], relTri[0]) / (l2 * l0));
		float th2 = acos(dot(relTri[0], relTri[1]) / (l0 * l1));

		vec3 m = -0.5f * (th0 * normalize(N0) + th1 * normalize(N1) + th2 * normalize(N2));

		mat3 N = mat3(N0, N1, N2);
		vec3 mN = transpose(N) * m;

		float w0 = mN[0] / dot(relTri[0], N0);
		float w1 = mN[1] / dot(relTri[1], N1);
		float w2 = mN[2] / dot(relTri[2], N2);

		cageWeights[indices.x] += w0;
		cageWeights[indices.y] += w1;
		cageWeights[indices.z] += w2;
	}

	// Normalize
	float sum = 0;
	for (int i = 0; i < cageWeights.size(); ++i) {
		sum += cageWeights[i];
	}
	float normalizer = 1.0f / sum;
	for (int i = 0; i < cageWeights.size(); ++i) {
		cageWeights[i] *= normalizer;
	}
}
