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
#include "objutils.h"

#include <DrawCall.h>
#include <Logger.h>
#include <tiny_obj_loader.h>

#include <vector>

bool ObjUtils::LoadDrawCallFromMesh(const std::string& filename, DrawCall& drawCall) {
	tinyobj::attrib_t attrib;
	std::vector<tinyobj::shape_t> shapes;
	std::vector<tinyobj::material_t> materials;

	std::string warn;
	std::string err;

	bool ret = tinyobj::LoadObj(&attrib, &shapes, &materials, &warn, &err, filename.c_str(), 0, false);

	if (!warn.empty()) {
		WARN_LOG << warn;
	}

	if (!err.empty()) {
		ERR_LOG << err;
	}

	if (!ret) {
		return false;
	}

	// Mesh draw call
	std::vector<GLfloat> pointData;
	std::vector<GLfloat> normalData;
	std::vector<GLuint> elementData;
	for (size_t s = 0; s < shapes.size(); s++) {
		// Loop over faces(polygon)
		size_t index_offset = 0;
		for (size_t f = 0; f < shapes[s].mesh.num_face_vertices.size(); f++) {
			int fv = shapes[s].mesh.num_face_vertices[f];
			if (fv != 4) {
				ERR_LOG << "Only Quad meshes are supported! (found a face of " << fv << " corners)";
				continue;
			}

			tinyobj::index_t verts[4];
			for (size_t v = 0; v < fv; v++) {
				verts[v] = shapes[s].mesh.indices[index_offset + v];
			}
			index_offset += fv;

			tinyobj::index_t indices[] = {
				verts[0],
				verts[1],
				verts[2],
				verts[0],
				verts[2],
				verts[3],
			};

			for (const auto& idx : indices) {
				for (int k = 0; k < 3; ++k) {
					pointData.push_back(static_cast<GLfloat>(attrib.vertices[3 * idx.vertex_index + k]));
					normalData.push_back(static_cast<GLfloat>(attrib.normals[3 * idx.normal_index + k]));
				}
				elementData.push_back(static_cast<GLuint>(elementData.size()));
			}
		}
	}

	drawCall.loadFromVectorsWithNormal(pointData, normalData, elementData);
	return true;
}
