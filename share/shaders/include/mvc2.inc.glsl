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

/*/
const vec3[] cRestCagePoints = vec3[](
	vec3(-1, -1, -1),
	vec3(+1, -1, -1),
	vec3(+1, +1, -1),
	vec3(-1, +1, -1),

	vec3(-1, -1, +1),
	vec3(+1, -1, +1),
	vec3(+1, +1, +1),
	vec3(-1, +1, +1)
);
/*/
const vec3[] cRestCagePoints = vec3[](
	vec3(+1, -1, -1),
	vec3(+1, +1, -1),
	vec3(-1, +1, -1),
	vec3(-1, -1, -1),

	vec3(+1, -1, +1),
	vec3(+1, +1, +1),
	vec3(-1, +1, +1),
	vec3(-1, -1, +1)
);
//*/

//*/
uniform uvec3[] uCageTriangles = uvec3[](
	uvec3(0, 2, 1),
	uvec3(0, 3, 2),

	uvec3(4, 5, 6),
	uvec3(4, 6, 7),

	uvec3(0, 1, 5),
	uvec3(0, 5, 4),

	uvec3(1, 2, 6),
	uvec3(1, 6, 5),

	uvec3(2, 3, 7),
	uvec3(2, 7, 6),

	uvec3(3, 0, 4),
	uvec3(3, 4, 7)
);
/*/
const uvec3[] uCageTriangles = uvec3[](
	uvec3(3, 1, 0),
	uvec3(3, 2, 1),

	uvec3(7, 4, 5),
	uvec3(7, 5, 6),

	uvec3(3, 0, 4),
	uvec3(3, 4, 7),

	uvec3(0, 1, 5),
	uvec3(0, 5, 4),

	uvec3(1, 2, 6),
	uvec3(1, 6, 5),

	uvec3(2, 3, 3),
	uvec3(2, 3, 6)
);
//*/

const float eps = 1e-5;

void computeMeanValueCoordinates(in vec3 position, out float cageWeights[8]) {
	for (int i = 0 ; i < 8 ; ++i) { cageWeights[i] = 0; }

	for (int i = 0 ; i < 12 ; ++i) {
		uvec3 indices = uCageTriangles[i];

		mat3 tri = mat3(
			cRestCagePoints[indices.x].xyz,
			cRestCagePoints[indices.y].xyz,
			cRestCagePoints[indices.z].xyz
		);

		// eq 14
		mat3 relTri = tri - mat3(position, position, position);
		vec3 N0 = cross(relTri[2], relTri[1]);
		vec3 N1 = cross(relTri[0], relTri[2]);
		vec3 N2 = cross(relTri[1], relTri[0]);

		float l0 = length(relTri[0]);
		float l1 = length(relTri[1]);
		float l2 = length(relTri[2]);

		//bool inPlane = determinant(relTri) < eps; //abs(z) < eps;
		bool inPlane = abs(l0) < eps || abs(l1) < eps || abs(l2) < eps; //abs(z) < eps;
		if (inPlane) {
			vec3 u = tri[1] - tri[0];
			vec3 v = tri[2] - tri[0];
			vec3 n = normalize(cross(u, v));
			float z = dot(position, n) - dot(tri[0], n);
			vec3 projected = position - n * z;

			vec3 Z = n;
			vec3 Y = normalize(cross(Z, u));
			vec3 X = normalize(cross(Y, Z));
			mat3 toLocalFrame = transpose(mat3(X, Y, Z));

			vec2 weights = inverse(mat2((toLocalFrame * u).xy, (toLocalFrame * v).xy)) * (toLocalFrame * projected).xy;
			bool inTriangle = weights.x >= 0 && weights.x <= 1 && weights.y >= 0 && weights.y <= 1 && weights.x + weights.y <= 1;
			if (inTriangle) {
				for (int j = 0 ; j < 8 ; ++j) { cageWeights[j] = 0; }
				cageWeights[indices.x] = weights.x;
				cageWeights[indices.y] = weights.y;
				cageWeights[indices.z] = 1 - weights.x - weights.y;
				return;
			}
		}

		float th0 = acos(dot(relTri[1], relTri[2]) / (l1 * l2));
		float th1 = acos(dot(relTri[2], relTri[0]) / (l2 * l0));
		float th2 = acos(dot(relTri[0], relTri[1]) / (l0 * l1));

		vec3 m = -0.5 * (th0 * normalize(N0) + th1 * normalize(N1) + th2 * normalize(N2));

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
	for (int i = 0 ; i < 8 ; ++i) {
		sum += cageWeights[i];
	}
	float normalizer = 1.0 / sum;
	for (int i = 0 ; i < 8 ; ++i) {
		cageWeights[i] *= normalizer;
	}
}

void computeInverseMeanValueCoordinates(in vec3 position, in vec3 cagePoints[8], out float cageWeights[8]) {
	for (int i = 0 ; i < 8 ; ++i) { cageWeights[i] = 0; }

	for (int i = 0 ; i < 12 ; ++i) {
		uvec3 indices = uCageTriangles[i];

		mat3 tri = mat3(
			cagePoints[indices.x].xyz,
			cagePoints[indices.y].xyz,
			cagePoints[indices.z].xyz
		);

		// eq 14
		mat3 relTri = tri - mat3(position, position, position);
		vec3 N0 = cross(relTri[2], relTri[1]);
		vec3 N1 = cross(relTri[0], relTri[2]);
		vec3 N2 = cross(relTri[1], relTri[0]);

		float l0 = length(relTri[0]);
		float l1 = length(relTri[1]);
		float l2 = length(relTri[2]);

		bool inPlane = abs(l0) < eps || abs(l1) < eps || abs(l2) < eps; //abs(z) < eps;
		if (inPlane) {
			vec3 u = tri[1] - tri[0];
			vec3 v = tri[2] - tri[0];
			vec3 n = normalize(cross(u, v));
			float z = dot(position, n) - dot(tri[0], n);
			vec3 projected = position - n * z;

			vec3 Z = n;
			vec3 Y = normalize(cross(Z, u));
			vec3 X = normalize(cross(Y, Z));
			mat3 toLocalFrame = transpose(mat3(X, Y, Z));

			vec2 weights = inverse(mat2((toLocalFrame * u).xy, (toLocalFrame * v).xy)) * (toLocalFrame * projected).xy;
			bool inTriangle = weights.x >= 0 && weights.x <= 1 && weights.y >= 0 && weights.y <= 1 && weights.x + weights.y <= 1;
			if (inTriangle) {
				for (int j = 0 ; j < 8 ; ++j) { cageWeights[j] = 0; }
				cageWeights[indices.x] = weights.x;
				cageWeights[indices.y] = weights.y;
				cageWeights[indices.z] = 1 - weights.x - weights.y;
				break;
			}
		}

		float th0 = acos(dot(relTri[1], relTri[2]) / (l1 * l2));
		float th1 = acos(dot(relTri[2], relTri[0]) / (l2 * l0));
		float th2 = acos(dot(relTri[0], relTri[1]) / (l0 * l1));

		vec3 m = -0.5 * (th0 * normalize(N0) + th1 * normalize(N1) + th2 * normalize(N2));

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
	for (int i = 0 ; i < 8 ; ++i) {
		sum += cageWeights[i];
	}
	float normalizer = 1.0 / sum;
	for (int i = 0 ; i < 8 ; ++i) {
		cageWeights[i] *= normalizer;
	}
}
