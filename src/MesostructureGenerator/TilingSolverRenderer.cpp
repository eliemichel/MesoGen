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
#include "TilingSolverRenderer.h"
#include "TilingSolverRenderData.h"
#include "TilingSolverData.h"
#include "MesostructureData.h"
#include "MacrosurfaceData.h"
#include "TileVariantList.h"
#include "BMesh.h"

#include <Camera.h>

#include <sstream>

using glm::vec2;
using glm::vec3;
using glm::vec4;

TilingSolverRenderer::TilingSolverRenderer(std::weak_ptr<TilingSolverData> solverData, std::weak_ptr<MesostructureData> mesostructureData)
	: m_solverData(solverData)
	, m_mesostructureData(mesostructureData)
	, m_renderData(std::make_shared<TilingSolverRenderData>())
{}

void TilingSolverRenderer::render(const Camera& camera) const {
	if (!properties().enabled) return;

	auto solverData = m_solverData.lock(); if (!solverData) return;
	auto mesostructure = m_mesostructureData.lock(); if (!mesostructure) return;
	if (!mesostructure->isValid()) return;
	auto macrosurface = mesostructure->macrosurface.lock();
	if (!solverData->solver) return;
	auto& solver = *solverData->solver;

	size_t slotCount = solver.slots().size();
	const auto& bmesh = *macrosurface->bmesh();
	if (bmesh.faces.size() != slotCount) return;

	m_texts.clear();
	for (int i = 0; i < slotCount; ++i) {
		vec3 p = bmesh.Center(bmesh.faces[i]);

		if (properties().drawStates) {
			const auto& superposition = solver.slots()[i];
			std::ostringstream ss;
			ss << "Slot #" << i << "\n";
			for (auto x : superposition) {
				ss << mesostructure->tileVariantList->variantRepr(x) << "\n";
			}

			vec3 q = macrosurface->modelMatrix() * vec4(p, 1.0f);

			m_texts.push_back(PositionedText{
				camera.projectPoint(q),
				ss.str()
				});
		}

		if (properties().drawStates) {
			auto verts = bmesh.NeighborVertices(bmesh.faces[i]);

			vec3 r = 0.1f * p + 0.9f * 0.5f * (verts[0]->position + verts[1]->position);
			float size = 0.1f;
			m_quad.properties().color = vec4(0.9f, 0.9f, 0.9f, 1.0f);
			m_quad.properties().modelMatrix = macrosurface->modelMatrix() * glm::mat4(
				glm::vec4(size, 0, 0, 0),
				glm::vec4(0, size, 0, 0),
				glm::vec4(0, 0, size, 0),
				glm::vec4(r, 1.0f)
			);
			m_quad.render(camera);

			r = 0.1f * p + 0.9f * 0.5f * (verts[1]->position + verts[2]->position);
			size = 0.05f;
			m_quad.properties().color = vec4(0.9f, 0.9f, 0.9f, 1.0f);
			m_quad.properties().modelMatrix = macrosurface->modelMatrix() * glm::mat4(
				glm::vec4(size, 0, 0, 0),
				glm::vec4(0, size, 0, 0),
				glm::vec4(0, 0, size, 0),
				glm::vec4(r, 1.0f)
			);
			m_quad.render(camera);


			int variant = mesostructure->slotAssignments[i];
			if (variant < 0) continue;
			auto transformedTile = mesostructure->tileVariantList->variantToTile(variant);
			if (transformedTile.tileIndex < 0 || transformedTile.tileIndex >= mesostructure->tileCount()) continue;

			glm::mat4 tr = TilesetController::TileTransformAsMat2(transformedTile.transform);
			const auto& tileType = *mesostructure->model->tileTypes[transformedTile.tileIndex];
			auto dir0 = TilesetController::TransformEdgeDirection(tileType, Model::TileEdge::Direction::PosX, transformedTile.transform);
			auto dir1 = TilesetController::TransformEdgeDirection(tileType, Model::TileEdge::Direction::PosY, transformedTile.transform);

			int edge0 = static_cast<int>(dir0.direction);
			r = 0.1f * p + 0.9f * 0.5f * (verts[edge0]->position + verts[(edge0 + 1) % 4]->position);
			//r = p + 0.9f * vec3(tr * vec4(0.5f * (verts[0]->position + verts[1]->position) - p, 1));
			size = 0.05f;
			m_quad.properties().color = vec4(1.0f, 0.0f, 0.0f, 1.0f);
			m_quad.properties().modelMatrix = macrosurface->modelMatrix() * glm::mat4(
				glm::vec4(size, 0, 0, 0),
				glm::vec4(0, size, 0, 0),
				glm::vec4(0, 0, size, 0),
				glm::vec4(r + vec3(0, 0, .01f), 1.0f)
			);
			m_quad.render(camera);

			int edge1 = static_cast<int>(dir1.direction);
			r = 0.1f * p + 0.9f * 0.5f * (verts[edge1]->position + verts[(edge1 + 1) % 4]->position);
			//r = p + 0.9f * vec3(tr * vec4(0.5f * (verts[1]->position + verts[2]->position) - p, 1));
			size = 0.025f;
			m_quad.properties().color = vec4(1.0f, 0.0f, 0.0f, 1.0f);
			m_quad.properties().modelMatrix = macrosurface->modelMatrix() * glm::mat4(
				glm::vec4(size, 0, 0, 0),
				glm::vec4(0, size, 0, 0),
				glm::vec4(0, 0, size, 0),
				glm::vec4(r + vec3(0, 0, .01f), 1.0f)
			);
			m_quad.render(camera);
		}
	}
}

void TilingSolverRenderer::update() {
	if (auto solverData = m_solverData.lock()) {
		m_renderData->sync(*solverData);
	}
}
