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
#include "TilesetRenderer.h"
#include "Model.h"
#include "TilesetController.h"
#include "utils/streamutils.h"
#include "CsgCompiler.h"
#include "MesostructureRenderer.h"
#include "SweepSsboItem.h"
#include "MesostructureData.h"
#include "MacrosurfaceData.h"
#include "BMesh.h"

#include <GlobalTimer.h>
#include <TurntableCamera.h>
#include <ScopedFramebufferOverride.h>
#include <Logger.h>
#include <Ray.h>
#include <ShaderProgram.h>
#include <RenderTarget.h>
#include <Texture.h>
#include <utils/reflutils.h>

#include <magic_enum.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtx/quaternion.hpp>
#include <glm/gtx/matrix_decompose.hpp>
#include <variant>

constexpr float PI = 3.141592653589793238462643383279502884f;

#define GET_MODEL_OR_RETURN() \
auto controllerPtr = m_controller.lock(); if (!controllerPtr) return; \
auto& controller = *controllerPtr; \
auto modelPtr = controller.lockModel(); if (!modelPtr) return; \
auto& model = *modelPtr;

using glm::vec2;
using glm::vec3;
using glm::vec4;
using glm::mat3;
using glm::mat4;

TilesetRenderer::TilesetRenderer()
	: m_pathDrawCall("tile25d-paths", GL_LINE_STRIP, false /* indexed */)
	, m_shapesDrawCall("profiles", GL_LINE_STRIP, true /* indexed */)
{
	m_macrosurfaceData = std::make_shared<MacrosurfaceData>();
	m_mesostructureData = std::make_shared<MesostructureData>();
	m_mesostructureData->macrosurface = m_macrosurfaceData;
	m_mesostructureRenderer = std::make_shared<MesostructureRenderer>();
	m_mesostructureRenderer->setMesostructureData(m_mesostructureData);

	auto& props = m_mesostructureRenderer->properties();
	props.offset = 0;
	props.thickness = 1;
	props.profileScale = 1;
	props.wireframe = true;
	props.wireframeDiagonal = false;
	props.benchmark = false;
}

void TilesetRenderer::setController(std::shared_ptr<TilesetController> controller) {
	m_controller = controller;
	m_mesostructureData->model = controller->model().lock();
	rebuild();
}

void TilesetRenderer::update() {
	auto& rendererProps = m_mesostructureRenderer->properties();
	rendererProps.wireframe = properties().drawCage;
}

void TilesetRenderer::renderTile(
	int tileIndex,
	const Camera& camera,
	const mat4& modelMatrix
) const {
	GET_MODEL_OR_RETURN();
	auto mesostructureRenderer = m_mesostructureRenderer; if (!mesostructureRenderer) return;
	GLfloat lineScale = 2; // must be synced with Properties::msaaFactor in DeferredRendering.h

	const auto& props = properties();
	if (!props.enable) return;

	glEnable(GL_DEPTH_TEST);
	glDepthMask(GL_TRUE);

	const Model::TileType& tile = *model.tileTypes[tileIndex];

	m_macrosurfaceData->setModelMatrix(modelMatrix);
	mesostructureRenderer->render(camera, tileIndex);

	// Draw shapes on faces
	if (properties().drawCrossSections) {
		mat4 colors = mat4(0);
		for (int k = 0; k < 4; ++k) {
			auto tileType = tile.edges[k].type.lock();
			if (tileType) {
				colors[k] = vec4(tileType->color, 1);
			}
		}
		const auto& slice = m_shapesSlices[tileIndex];
		if (slice.size > 1) {
			const auto& shader = *m_shapesDrawCall.shader;
			autoSetUniforms(shader, props);
			shader.setUniform("uModelMatrix", modelMatrix);
			shader.setUniform("uInstanceStart", 0u);
			shader.setUniform("uColors", colors);
			glLineWidth(2 * lineScale);
			m_shapesDrawCall.render(camera, slice.start, slice.size, 1);
		}
	}

	// Draw edge type markers
	if (properties().drawLabels) {
		for (int tileEdgeIndex = 0; tileEdgeIndex < Model::TileEdge::Count; ++tileEdgeIndex) {
			renderEdgeTypeMarker(tile, tileEdgeIndex, camera, modelMatrix);
		}
	}

	// Draw path highlights
	glDisable(GL_DEPTH_TEST);
	if (properties().drawTrajectories) {
		const auto& shader = *m_pathDrawCall.shader;
		autoSetUniforms(shader, props);
		shader.setUniform("uModelMatrix", modelMatrix);
		shader.setUniform("uUseCornerMatrices", false);
		shader.setUniform("uInstanceStart", 0u);
		shader.setUniform("uUseInstancing", false);

		glLineWidth(3 * lineScale);
		for (const auto& path : m_paths[tileIndex]) {
			if (path.hover) {
				shader.setUniform("uColor", vec4(1.0f, 0.5f, 0.0f, 1.0f));
				m_pathDrawCall.render(camera, path.slice.start, path.slice.size, 1);
			}
			else if (path.active) {
				shader.setUniform("uColor", vec4(1.0f, 0.1f, 0.2f, 1.0f));
				m_pathDrawCall.render(camera, path.slice.start, path.slice.size, 1);
			}
		}
	}

	// Draw potential paths
	if (properties().drawTrajectories) {
		const auto& slice = m_pathSlices[tileIndex];
		if (slice.size > 1) {
			const auto& shader = *m_pathDrawCall.shader;
			autoSetUniforms(shader, props);
			shader.setUniform("uModelMatrix", modelMatrix);
			shader.setUniform("uInstanceStart", 0u);
			shader.setUniform("uColor", vec4(0.0f, 0.0f, 0.0f, 1.0f));
			glLineWidth(1 * lineScale);
			m_pathDrawCall.render(camera, slice.start, slice.size, 1);
		}
	}
}

//-----------------------------------------------------------------------------
// PRIVATE

void TilesetRenderer::renderEdgeTypeMarker(const Model::TileType& tile, int tileEdgeIndex, const Camera& camera, const mat4& modelMatrix) const {
	if (tileEdgeIndex >= 4) return;
	auto& tileEdge = tile.edges[tileEdgeIndex];
	auto edgeType = tileEdge.type.lock();
	if (!edgeType) return;

	const auto& tr = modelMatrix * glm::mat4(Model::TileEdge::Transforms[tileEdgeIndex]);

	m_quad.properties().color =
		edgeType->isExplicit
		? vec4(edgeType->color, 1.0f)
		: vec4(0.5f, 0.5f, 0.5f, 1.0f);

	mat4 xz(mat3(
		1, 0, 0,
		0, 0, 1,
		0, 1, 0
	));

	{
		vec3 position(0.0f, 0.0f, 0.55f);
		vec3 size(0.3f, 1.0f, 0.05f);
		m_quad.properties().modelMatrix = tr * glm::translate(mat4(1), position) * glm::scale(mat4(1), size) * xz;
		m_quad.render(camera);
	}

	{
		vec3 position(tileEdge.transform.flipped ? -0.2f : 0.2f, 0.0f, 0.55f);
		vec3 size(0.05f, 1.0f, 0.05f);
		m_quad.properties().modelMatrix = tr * glm::translate(mat4(1), position) * glm::scale(mat4(1), size) * xz;
		m_quad.render(camera);
	}
}

void TilesetRenderer::rebuild() {
	//auto timer = GlobalTimer::Start("TilesetRenderer::rebuild");
	m_mesostructureRenderer->onModelChange();
	rebuildMockMesostructure();
	rebuildPotentialPaths();
	rebuildShapesDrawCall();
	//GlobalTimer::Stop(timer);
}

void TilesetRenderer::rebuildPotentialPaths() {
	GET_MODEL_OR_RETURN();

	m_pathSlices.resize(0);
	m_paths.resize(0);

	std::vector<GLfloat> pointData;
	std::vector<GLuint> elementData;
	constexpr GLuint RESTART = std::numeric_limits<GLuint>::max();

	std::array<std::vector<vec4>, 4> controlPoints;
	for (auto& vec : controlPoints) {
		vec.reserve(model.tileTypes.size());
	}

	for (const auto& tile : model.tileTypes) {
		std::vector<Path> tilePaths;
		auto potentialPaths = TilesetController::GetPotentialPaths(*tile);
		GLuint start = static_cast<GLuint>(elementData.size());

		tilePaths.reserve(potentialPaths.size());
		for (const auto& pathGeometry : potentialPaths) {
			GLuint substart = static_cast<GLuint>(elementData.size());
			GLuint offset = static_cast<GLuint>(pointData.size() / 4);
			for (const auto& x : pathGeometry.pointData) {
				pointData.push_back(x);
			}

			Path path;
			size_t pointCount = pathGeometry.pointData.size() / 4;
			path.points.reserve(pointCount);
			for (GLuint i = 0; i < pointCount; ++i) {
				elementData.push_back(offset + i);

				path.points.push_back(vec3(
					pathGeometry.pointData[4 * i + 0],
					pathGeometry.pointData[4 * i + 1],
					pathGeometry.pointData[4 * i + 2]
				));
			}
			elementData.push_back(RESTART);
			GLuint subend = static_cast<GLuint>(elementData.size());

			path.slice.start = substart;
			path.slice.size = subend - substart - 1;
			path.model = pathGeometry.model;
			path.active = controller.hasTileInnerPath(*tile, path.model);
			tilePaths.push_back(path);

			if (path.active) {
				for (int k = 0; k < 4; ++k) {
					controlPoints[k].push_back(vec4(pathGeometry.controlPoints[k], 1));
				}
			}
		}

		GLuint end = static_cast<GLuint>(elementData.size());
		m_pathSlices.push_back(PathSlice{ start, end - start });
		m_paths.push_back(tilePaths);
	}

	m_pathDrawCall.destroy();
	if (!pointData.empty()) {
		m_pathDrawCall.loadFromVectors(pointData, elementData);
		glVertexArrayVertexBuffer(m_pathDrawCall.vao, 0, m_pathDrawCall.vbo, 0, 4 * sizeof(GLfloat));
		m_pathDrawCall.restart = true;
		m_pathDrawCall.restartIndex = RESTART;
	}
}

void TilesetRenderer::rebuildMockMesostructure() {
	auto& mesostructure = *m_mesostructureData;
	size_t n = mesostructure.model->tileTypes.size();
	if (n == 0) return;

	auto& macrosurface = *m_macrosurfaceData;
	if (!macrosurface.bmesh() || macrosurface.bmesh()->faceCount() != n) {
		auto bm = std::make_shared<bmesh::BMesh>();
		bm->AddVertices(4);
		bm->vertices[0]->position = vec3(+0.5, -0.5, 0);
		bm->vertices[1]->position = vec3(+0.5, +0.5, 0);
		bm->vertices[2]->position = vec3(-0.5, +0.5, 0);
		bm->vertices[3]->position = vec3(-0.5, -0.5, 0);
		for (int i = 0; i < n; ++i) {
			bm->AddFace(std::vector<int>{0, 1, 2, 3});
		}
		for (auto& l : bm->loops) {
			l->normal = vec3(0, 0, 1);
		}
		macrosurface.loadBMesh(bm);
		macrosurface.setModelMatrix(mat4(1));
	}

	auto adapter = TilesetController::ConvertModelToSignedWangRuleset(*mesostructure.model);
	mesostructure.tileVariantList = std::move(adapter.tileVariantList);
	mesostructure.slotAssignments.resize(n);
	int prevTileIndex = -1;
	for (int i = 0, j = 0; i < n; ++j) {
		int tileIndex = mesostructure.tileVariantList->variantToTile(j).tileIndex;
		if (tileIndex != prevTileIndex) {
			mesostructure.slotAssignments[i++] = j;
			prevTileIndex = tileIndex;
		}
	}

	mesostructure.slotAssignmentsVersion.increment();
	if (mesostructure.tileVersions.size() != n) {
		mesostructure.tileVersions.resize(n);
		mesostructure.tileListVersion.increment();
	}

	m_mesostructureRenderer->update();
}

void TilesetRenderer::rebuildShapesDrawCall() {
	GET_MODEL_OR_RETURN();
	constexpr GLuint RESTART = std::numeric_limits<GLuint>::max();

	std::vector<GLfloat> pointData;
	std::vector<GLfloat> uvData;
	std::vector<GLuint> elementData;

	m_shapesSlices.clear();

	CsgCompiler compiler;
	CsgCompiler::Options opts;
	opts.maxSegmentLength = 1e-3f;

	GLuint element = 0;

	for (const auto& tile : model.tileTypes) {
		GLuint start = static_cast<GLuint>(elementData.size());

		// Add shapes
		for (auto& shapeAccessor : TilesetController::GetTileShapes(*tile)) {

			auto& csg = shapeAccessor.shape().csg;
			std::vector<vec2> points = compiler.compileGlm(csg, opts);
			for (vec2& pt : points) {
				if (shapeAccessor.tileEdge().transform.flipped) pt.x = 1 - pt.x;
				vec3 wpt = shapeAccessor.transform() * vec4(pt.x - 0.5f, pt.y, 0.5, 1.0);
				pointData.push_back(wpt.x);
				pointData.push_back(wpt.y);
				pointData.push_back(wpt.z);
				uvData.push_back(static_cast<GLfloat>(shapeAccessor.tileEdgeIndex()));
				uvData.push_back(0);
				uvData.push_back(0);
				elementData.push_back(element);
				++element;
			}

			elementData.push_back(RESTART);
		}

		GLuint end = static_cast<GLuint>(elementData.size());
		m_shapesSlices.push_back(PathSlice{ start, end - start });
	}

	m_shapesDrawCall.loadFromVectorsWithNormal(pointData, uvData, elementData);
	m_shapesDrawCall.restart = true;
	m_shapesDrawCall.restartIndex = RESTART;
}
