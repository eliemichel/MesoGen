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
#include "BMeshIo.h"
#include "BMesh.h"

#include <libwfc.h>
#include <DrawCall.h>
#include <Logger.h>

namespace bmesh {

std::shared_ptr<bmesh::BMesh> loadObj(const std::string& objFilename, DrawCall *drawCall) {
	tinyobj::attrib_t attrib;
	std::vector<tinyobj::shape_t> shapes;
	std::vector<tinyobj::material_t> materials;

	std::string warn;
	std::string err;

	bool ret = tinyobj::LoadObj(&attrib, &shapes, &materials, &warn, &err, objFilename.c_str(), 0, false);
	if (!warn.empty()) WARN_LOG << warn;
	if (!err.empty()) ERR_LOG << err;

	if (!ret) {
		return nullptr;
	}

	if (attrib.normals.empty()) {
		ERR_LOG << "Expecting an OBJ with normal information!";
		return nullptr;
	}

	// Topology
	auto bm = std::make_shared<bmesh::BMesh>();
	bm->FromTinyObj(attrib, shapes);

	if (nullptr != drawCall) {
		//if (!drawCallFromTinyObj(*drawCall, attrib, shapes)) {
		if (!drawCallFromBMesh(*drawCall, *bm)) {
			return nullptr;
		}
	}

	return bm;
}

std::shared_ptr<libwfc::MeshSlotTopology> asSlotTopology(const BMesh & bm) {
	std::vector<libwfc::MeshSlotTopology::Face> faces;
	for (int i = 0; i < bm.faces.size(); ++i) {
		bmesh::Face* f = bm.faces[i];
		assert(f->index == i);
		std::vector<bmesh::Face*> neighbors = bm.NeighborFaces(f);

		if (neighbors.size() != 4) {
			continue;
		}

		libwfc::MeshSlotTopology::Face faceTopo;
		faceTopo.index = i;
		for (const auto& neighborFace : neighbors) {
			faceTopo.neighbors.push_back(neighborFace ? neighborFace->index : -1);

			auto neighborRelation = libwfc::MeshSlotTopology::RelationEnum::Neighbor0;
			if (neighborFace) {
				int j = 0;
				for (auto neighborNeighborFace : bm.NeighborFaces(neighborFace)) {
					if (neighborNeighborFace == f) {
						neighborRelation = static_cast<libwfc::MeshSlotTopology::RelationEnum>(j);
						break;
					}
					++j;
				}
			}
			faceTopo.neighborRelations.push_back(neighborRelation);
		}
		faces.push_back(faceTopo);
	}

	return std::make_shared<libwfc::MeshSlotTopology>(faces);
}

bool drawCallFromBMesh(DrawCall& drawCall, const bmesh::BMesh& bm) {
	// Mesh draw call
	std::vector<GLfloat> pointData;
	std::vector<GLfloat> normalData;
	std::vector<GLuint> elementData;
	size_t index_offset = 0;
	for (auto face : bm.faces) {
		auto loops = bm.NeighborLoops(face);
		size_t fv = loops.size();
		if (fv != 4 && fv != 3) {
			ERR_LOG << "Only Quad & Tri meshes are supported! (found a face of " << fv << " corners)";
			return false;
		}

		index_offset += fv;

		std::vector<bmesh::Loop*> triloops;
		if (fv == 4) {
			triloops = {
				loops[0],
				loops[1],
				loops[2],
				loops[0],
				loops[2],
				loops[3],
			};
		} else{
			triloops = loops;
		}

		for (const auto& l : triloops) {
			for (int k = 0; k < 3; ++k) {
				pointData.push_back(static_cast<GLfloat>(l->vertex->position[k]));
				normalData.push_back(static_cast<GLfloat>(l->normal[k]));
			}
			elementData.push_back(static_cast<GLuint>(elementData.size()));
		}
	}
	drawCall.loadFromVectorsWithNormal(pointData, normalData, elementData);
	return true;
}

bool drawCallFromTinyObj(DrawCall& drawCall, const tinyobj::attrib_t& attrib, const std::vector<tinyobj::shape_t>& shapes) {
	// Mesh draw call
	std::vector<GLfloat> pointData;
	std::vector<GLfloat> normalData;
	std::vector<GLuint> elementData;
	size_t index_offset = 0;
	for (size_t s = 0; s < shapes.size(); s++) {
		// Loop over faces(polygon)
		for (size_t f = 0; f < shapes[s].mesh.num_face_vertices.size(); f++) {
			int fv = shapes[s].mesh.num_face_vertices[f];
			if (fv != 4) {
				ERR_LOG << "Only Quad meshes are supported! (found a face of " << fv << " corners)";
				return false;
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

} // namespace bmesh
