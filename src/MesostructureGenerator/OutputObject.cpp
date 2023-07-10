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
#include "OutputObject.h"
#include "Model.h"
#include "TilesetController.h"
#include "BMesh.h"
#include "BMeshIo.h"
#include "utils/glutils.h"
#include "MacrosurfaceData.h"
#include "MesostructureData.h"

// MS
#include "Synthesizer.h"

#include <libwfc.h>

#include <utils/streamutils.h>
#include <Logger.h>
#include <TurntableCamera.h>
#include <Ray.h>

#include <Eigen/Core>
#include <Eigen/LU>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <magic_enum.hpp>
#include <chrono>

using glm::vec2;
using glm::vec3;
using glm::vec4;
using glm::mat4;

constexpr float PI = 3.141592653589793238462643383279502884f;

//-----------------------------------------------------------------------------

OutputObject::OutputObject(
	std::shared_ptr<MacrosurfaceData> macrosurfaceData,
	std::shared_ptr<MesostructureData> mesostructureData
)
	: m_macrosurfaceData(macrosurfaceData)
	, m_mesostructureData(mesostructureData)
{
	auto camera = std::make_shared<TurntableCamera>();
	camera->setProjectionType(Camera::ProjectionType::Orthographic);
	camera->setOrientation(glm::quat(sqrt(2.f) / 2.f, -sqrt(2.f) / 2.f, 0.f, 0.f) * glm::angleAxis(PI / 2, vec3(1, 0, 0)));
	camera->setOrthographicScale(1.5f);
	camera->setResolution(1, 1);
	m_camera = camera;

	m_quad.properties().color = vec4(0.9f, 0.9f, 0.9f, 1.0f);
}

mat4 OutputObject::contentModelMatrix() const {
	float size = properties().displaySize;
	float halfSize = size / 2;
	vec3 center(halfSize * 5, -1.5f - halfSize * 5, 0);
	return glm::translate(mat4(1), center) * m_camera->viewMatrix();
}

void OutputObject::render(const Camera& camera) const {
	if (m_hover) {
		glDepthMask(GL_FALSE);
		m_quad.render(camera);
		glDepthMask(GL_TRUE);
	}

	const auto& props = m_macrosurfaceData->properties();
	m_macrosurfaceData->setModelMatrix(
		glm::scale(
			glm::translate(mat4(1), vec3(props.viewportPosition, 0)),
			vec3(props.viewportScale)
		) * m_camera->viewMatrix()
	);
	//m_macrosurfaceData->setModelMatrix(contentModelMatrix());
}

void OutputObject::update() {
}

bool OutputObject::onMousePress(bool forceHover) {
	if (!m_hover && !forceHover) return false;
	m_camera->startMouseRotation();
	return true;
}

void OutputObject::onMouseRelease() {
	m_camera->stopMouseRotation();
}

void OutputObject::onCursorPosition(float x, float y, const Camera& camera) {
	m_camera->updateMousePosition(x, y);

	float size = properties().displaySize;

	Ray ray = camera.mouseRay(vec2(x, y));
	vec3 hitPoint = ray.origin - (ray.origin.z / ray.direction.z) * ray.direction;

	float margin = size / 2 + 0.02f;

	size_t W = 5;
	size_t H = 5;

	vec3 minTileCenter(-margin, -((W - 1) * size) - 1.5f - margin, 0);
	vec3 maxTileCenter((H - 1) * size + margin, -1.5f + margin, 0);
	m_hover = (
		hitPoint.x >= minTileCenter.x &&
		hitPoint.y >= minTileCenter.y &&
		hitPoint.x <= maxTileCenter.x &&
		hitPoint.y <= maxTileCenter.y
	);
	m_hover = false; // deactivated behavior

	m_quad.setPosition((minTileCenter + maxTileCenter) * 0.5f);
	m_quad.setSize(maxTileCenter - minTileCenter);
}

void OutputObject::setController(std::shared_ptr<TilesetController> controller) {
	m_controller = controller;
	auto model = controller->lockModel(); if (!model) return;
	m_mesostructureData->model = model;
}
