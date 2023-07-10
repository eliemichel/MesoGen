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
#include "MacrosurfaceData.h"
#include "BMesh.h"
#include "BMeshIo.h"

#include <DrawCall.h>
#include <libwfc.h>

using glm::vec3;

MacrosurfaceData::MacrosurfaceData() {
	m_meshDrawCall = std::make_shared<DrawCall>(glad_glCreateProgram ? "shaded-mesh" : "", GL_TRIANGLES, true);
}

bool MacrosurfaceData::loadMesh() {
	m_bmesh = bmesh::loadObj(properties().modelFilename, m_meshDrawCall.get());
	if (!m_bmesh) return false;

	m_slotTopology = asSlotTopology(*m_bmesh);

	m_version.increment();
	return true;
}

void MacrosurfaceData::loadBMesh(std::shared_ptr<bmesh::BMesh> bm) {
	if (m_meshDrawCall) {
		drawCallFromBMesh(*m_meshDrawCall, *bm);
	}
	m_slotTopology = asSlotTopology(*bm);
	m_bmesh = bm;
	m_version.increment();
}

void MacrosurfaceData::generateGrid() {
	m_bmesh = std::make_shared<bmesh::BMesh>();
	auto& bm = *m_bmesh;

	auto& dim = properties().dimensions;

	bm.AddVertices((dim.x + 1) * (dim.y + 1));

	// Setup vertices
	for (int j = 0; j < dim.y + 1; ++j) {
		for (int i = 0; i < dim.x + 1; ++i) {
			bm.vertices[j * (dim.x + 1) + i]->position = vec3(
				static_cast<float>(i) / dim.x - 0.5f,
				static_cast<float>(j) / dim.y - 0.5f, 0.0f) * 5.0f;
		}
	}

	// Add faces
	for (int j = 0; j < dim.y; ++j) {
		for (int i = 0; i < dim.x; ++i) {
			bm.AddFace(std::vector<int>{
				j * (dim.x + 1) + (i + 1),
				(j + 1) * (dim.x + 1) + (i + 1),
				(j + 1) * (dim.x + 1) + i,
				j * (dim.x + 1) + i
			});
		}
	}

	// Fix loops
	for (auto& l : bm.loops) {
		l->normal = vec3(0, 0, 1);
	}

	loadBMesh(m_bmesh);
}

void MacrosurfaceData::disableDrawCall() {
	m_meshDrawCall.reset();
}

bool MacrosurfaceData::ready() const {
	return m_slotTopology != nullptr;
}
