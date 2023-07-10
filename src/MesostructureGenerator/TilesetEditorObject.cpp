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
#include "TilesetEditorObject.h"
#include "Model.h"
#include "TilesetController.h"
#include "utils/streamutils.h"
#include "CsgCompiler.h"
#include "TilesetRenderer.h"
#include "MesostructureRenderer.h"
#include "DrawCall.h"

#include <GlobalTimer.h>
#include <TurntableCamera.h>
#include <ScopedFramebufferOverride.h>
#include <Logger.h>
#include <Ray.h>
#include <ShaderProgram.h>
#include <utils/reflutils.h>

#include <magic_enum.hpp>
#include <glm/gtc/matrix_transform.hpp>
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

TilesetEditorObject::TilesetEditorObject()
	: m_circleCursorDrawCall("brush", GL_LINE_STRIP)
	, m_squareCursorDrawCall("brush", GL_LINE_STRIP)
{
	auto camera = std::make_shared<TurntableCamera>();
	camera->setProjectionType(Camera::ProjectionType::Orthographic);
	camera->setOrientation(glm::quat(sqrt(2.f) / 2.f, -sqrt(2.f) / 2.f, 0.f, 0.f) * glm::angleAxis(PI / 2, vec3(1, 0, 0)));
	camera->setOrthographicScale(1.5f);
	camera->setResolution(1, 1);
	m_camera = camera;

	m_cursor.properties().color = vec4(1.0f, 0.5f, 0.0f, 1.0f);
	rebuildCursorDrawCalls();
}

void TilesetEditorObject::setController(std::weak_ptr<TilesetController> controller) {
	m_controller = controller;
	rebuild();
}

void TilesetEditorObject::setRenderer(std::weak_ptr<TilesetRenderer> renderer) {
	m_renderer = renderer;
	rebuild();
}

void TilesetEditorObject::setMesostructureRenderer(std::weak_ptr<MesostructureRenderer> mesostructureRenderer) {
	m_mesostructureRenderer = mesostructureRenderer;
	rebuild();
}

void TilesetEditorObject::update() {
	if (auto renderer = m_renderer.lock()) {
		renderer->update();
	}
	if (auto controller = m_controller.lock()) {
		if (controller->hasChangedLately()) {
			rebuild();
		}
	}
}

void TilesetEditorObject::render(const Camera& camera) const {
	auto timer = GlobalTimer::Start("TilesetEditorObject::render");
	GET_MODEL_OR_RETURN();

	glEnable(GL_DEPTH_TEST);

	if (auto renderer = m_renderer.lock()) {
		for (int i = 0; i < model.tileTypes.size(); ++i) {
			glm::mat4 modelMatrix = glm::translate(glm::mat4(1), GetTileCenter(i)) * m_camera->viewMatrix();
				renderer->renderTile(i, camera, modelMatrix);
		}
	}

	if (m_mouseHit.hitTileEdge.isValid) {
		switch (properties().brush) {
		case Brush::Circle:
			setCursorUniforms(*m_circleCursorDrawCall.shader);
			m_circleCursorDrawCall.render(camera);
			break;
		case Brush::Square:
			setCursorUniforms(*m_squareCursorDrawCall.shader);
			m_squareCursorDrawCall.render(camera);
			break;
		}
	}
	GlobalTimer::Stop(timer);
}

void TilesetEditorObject::setCursorUniforms(const ShaderProgram& shader) const {
	autoSetUniforms(shader, properties());
	shader.setUniform("uModelMatrix", m_brushMatrix);

	vec3 brushColor;
	switch (properties().operation) {
	case Operation::Add:
		brushColor = vec3(0.0, 1.0, 0.5);
		break;
	case Operation::Substract:
		brushColor = vec3(1.0, 0.5, 0.2);
		break;
	case Operation::Remove:
		brushColor = vec3(1.0, 0.2, 0.4);
		break;
	}
	shader.setUniform("uColor", vec4(brushColor, 1.0));
}

void TilesetEditorObject::onMouseScroll(float scroll) {
	auto& props = properties();
	props.brushSize *= std::pow(2.0f, scroll * props.brushSizeSensitivity);
	m_cursor.setSize(properties().brushScale);
}

void TilesetEditorObject::onMousePress() {
	if (m_mouseHit.hitPath.isValid) {
		onMousePressOnPath();
	}
	else if (m_mouseHit.hitTileEdge.isValid) {
		onMousePressOnInterface();
	}
	else {
		m_camera->startMouseRotation();
	}
}

void TilesetEditorObject::onMouseRelease() {
	m_camera->stopMouseRotation();
}

void TilesetEditorObject::onCursorPosition(float x, float y, const Camera &camera) {
	m_camera->updateMousePosition(x, y);

	auto renderer = m_renderer.lock(); if (!renderer) return;

	Ray ray = camera.mouseRay(vec2(x, y));

	float minLambda = std::numeric_limits<float>::max();
	float hitLambda = minLambda;
	m_mouseHit.hitTileEdge.isValid = false;
	m_mouseHit.hitPath.isValid = false;

	// Intersect with paths
	testPathHit(renderer->paths(), ray, minLambda, hitLambda);

	if (!m_mouseHit.hitPath.isValid) {
		// Intersect with tiles
		testInterfaceHit(ray, minLambda, hitLambda);
	}
}

vec3 TilesetEditorObject::GetTileCenter(int tileIndex) {
	return vec3(tileIndex * 1.5f, 0, 0);
}

//-----------------------------------------------------------------------------
// PRIVATE

void TilesetEditorObject::rebuild() {
	if (auto renderer = m_renderer.lock()) {
		renderer->rebuild();
	}
}

void TilesetEditorObject::rebuildCursorDrawCalls() {
	{
		std::vector<GLfloat> pointData;
		for (int i = 0; i < 65; ++i) {
			GLfloat theta = static_cast<GLfloat>(i) / 64 * 2 * 3.14159266f;
			pointData.push_back(cos(theta));
			pointData.push_back(sin(theta));
			pointData.push_back(0);
		}
		m_circleCursorDrawCall.loadFromVectors(pointData);
	}

	{
		static const std::vector<GLfloat> pointData = {
			-1.0, -1.0, 0.0,
			+1.0, -1.0, 0.0,
			+1.0, +1.0, 0.0,
			-1.0, +1.0, 0.0,
			-1.0, -1.0, 0.0,
		};
		m_squareCursorDrawCall.loadFromVectors(pointData);
	}
}

void TilesetEditorObject::testPathHit(std::vector<std::vector<TilesetRenderer::Path>>& paths, const Ray& ray, float& minLambda, float& hitLambda) {
	float minDistance = std::numeric_limits<float>::max();

	for (int tileIndex = 0; tileIndex < paths.size(); ++tileIndex) {
		mat4 modelMatrix = glm::translate(glm::mat4(1), GetTileCenter(tileIndex)) * m_camera->viewMatrix();
		Ray localRay = transform(ray, inverse(modelMatrix));

		int pathIndex = -1;
		if (tileIndex >= paths.size()) continue;
		for (auto& path : paths[tileIndex]) {
			++pathIndex;
			path.hover = false;

			float hitLocalLambda, hitGamma;
			hitLambda = minLambda;
			float localMinDistance = minDistance;
			for (int i = 1; i < path.points.size(); ++i) {
				float distance = intersectLine(localRay, path.points[i], path.points[i - 1], hitLocalLambda, hitGamma);
				bool hit = distance < properties().lineThickness;
				if (hit && distance < localMinDistance && hitGamma >= 0 && hitGamma <= 1) {
					hitLambda = hitLocalLambda;
					localMinDistance = distance;
				}
			}

			// If closer to screen or closer to the true locatino of the line
			if (hitLambda < minLambda || localMinDistance < minDistance) {
				m_mouseHit.hitPath.isValid = true;
				minLambda = hitLambda;
				minDistance = localMinDistance;
				m_mouseHit.hitPath.tileIndex = tileIndex;
				m_mouseHit.hitPath.pathIndex = pathIndex;
			}
		}
	}

	if (m_mouseHit.hitPath.isValid) {
		auto& path = paths[m_mouseHit.hitPath.tileIndex][m_mouseHit.hitPath.pathIndex];
		path.hover = true;
	}
}

void TilesetEditorObject::testInterfaceHit(const Ray& ray, float& minLambda, float& hitLambda) {
	GET_MODEL_OR_RETURN();
	vec3 hitPoint, bestHitPoint(0);
	for (int i = 0; i < model.tileTypes.size(); ++i) {
		mat4 modelMatrix = glm::translate(glm::mat4(1), GetTileCenter(i)) * m_camera->viewMatrix();
		Ray localRay = transform(ray, inverse(modelMatrix));

		if (intersectPlane(localRay, vec3(0, 0, 1), 0, hitPoint, hitLambda)) {
			if (hitLambda < minLambda && std::abs(hitPoint.x) <= 0.5f && std::abs(hitPoint.y) <= 0.5f) {
				minLambda = hitLambda;
				bestHitPoint = hitPoint;
			}
		}

		for (int direction = 0; direction < 4; ++direction) { // Eventually replace 4 with Model::TileEdge::Count but hit procedure does not generalize well to Pos/Neg Z interfaces
			const mat3& tr = Model::TileEdge::Transforms[direction];
			const Model::TileEdge& tileEdge = model.tileTypes[i]->edges[direction];
			if (!tileEdge.type.lock()) continue;
			const Model::EdgeTransform& edgeTransform = tileEdge.transform;
			if (intersectPlane(transform(localRay, inverse(tr)), vec3(0, 0, 1), 0.5f, hitPoint, hitLambda)) {
				if (hitLambda < minLambda && std::abs(hitPoint.x) <= 0.5f && hitPoint.y >= 0 && hitPoint.y <= 0.5f) {
					minLambda = hitLambda;
					bestHitPoint = tr * hitPoint;
					m_mouseHit.hitTileEdge.tileEdge = magic_enum::enum_value<Model::TileEdge::Direction>(direction);
					m_mouseHit.hitTileEdge.isValid = true;
					m_mouseHit.hitTileEdge.tileIndex = i;
					m_mouseHit.hitTileEdge.localCoord =
						edgeTransform.flipped
						? vec2(0.5 - hitPoint.x, hitPoint.y * 2)
						: vec2(0.5 + hitPoint.x, hitPoint.y * 2);
				}
			}
		}
	}

	if (m_mouseHit.hitTileEdge.isValid) {
		int i = m_mouseHit.hitTileEdge.tileIndex;
		glm::mat4 modelMatrix = glm::translate(glm::mat4(1), GetTileCenter(i)) * m_camera->viewMatrix();
		m_cursor.setSize(properties().brushScale);
		vec3 brushPosition = bestHitPoint;
		mat3 brushOrientation = Model::TileEdge::Transforms[static_cast<int>(m_mouseHit.hitTileEdge.tileEdge)];
		m_brushMatrix = modelMatrix * glm::translate(mat4(1.0), brushPosition) * mat4(brushOrientation);
		m_cursor.setPosition(modelMatrix * vec4(brushPosition, 1));
	}
}

void TilesetEditorObject::onMousePressOnInterface() {
	GET_MODEL_OR_RETURN();

	const auto& hit = m_mouseHit.hitTileEdge;

	int tileEdgeIndex = static_cast<int>(hit.tileEdge);
	const auto& tileEdge = model.tileTypes[hit.tileIndex]->edges[tileEdgeIndex];
	auto edgeType = tileEdge.type.lock();
	if (edgeType) {
		float brushAngle = properties().brushAngle;
		if (tileEdge.transform.flipped) {
			brushAngle = -brushAngle;
		}

		auto shape = std::make_unique<Model::Shape>();
		switch (properties().brush) {
		case Brush::Circle:
			*shape = Model::Shape{
				Model::Csg2D::Disc {
					hit.localCoord,
					properties().brushSize * properties().brushScale,
					brushAngle * 3.14159266f / 180
				}
			};
			break;
		case Brush::Square:
			*shape = Model::Shape{
				Model::Csg2D::Box{
					hit.localCoord,
					properties().brushSize * properties().brushScale,
					brushAngle * 3.14159266f / 180
				}
			};
			break;
		default:
			shape.reset();
		}
		if (shape) {
			shape->color = glm::vec4(1.0f);
			shape->roughness = 0.1f;
			shape->metallic = 0.0f;
			switch (properties().operation) {
			case Operation::Add:
				controller.insertEdgeShape(*edgeType, *shape);
				break;
			case Operation::Substract:
				controller.subtractEdgeShape(*edgeType, *shape);
				break;
			case Operation::Remove:
				controller.removeEdgeShape(*edgeType, *shape);
				break;
			}
		}
	}
}

void TilesetEditorObject::onMousePressOnPath() {
	GET_MODEL_OR_RETURN();
	auto renderer = m_renderer.lock(); if (!renderer) return;

	const auto& hit = m_mouseHit.hitPath;
	auto& path = renderer->paths()[hit.tileIndex][hit.pathIndex];
	auto& tileType = model.tileTypes[hit.tileIndex];

	if (path.active) {
		controller.removeTileInnerPath(*tileType, path.model);
	}
	else {
		controller.addTileInnerPath(*tileType, path.model);
	}
}
