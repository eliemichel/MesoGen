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
#include "Scene.h"
#include "utils/objutils.h"
#include "utils/streamutils.h"

#include <GlobalTimer.h>
#include <Logger.h>
#include <Camera.h>

#include <glm/glm.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <vector>

using glm::vec2;
using glm::vec3;
using glm::vec4;

Scene::Scene()
	: m_meshDrawCall("GpuCage/cage-mesh", GL_TRIANGLES, true)
	, m_cageDrawCall("mesh", GL_LINES, true)
	, m_handleDrawCall("GpuCage/handle", GL_POINTS, true)
	, m_raymarchingDrawCall("GpuCage/raymarching", GL_TRIANGLES, true)
{
	m_meshDrawCall.shader->load();
	m_cageDrawCall.shader->load();
	m_handleDrawCall.shader->load();
	m_raymarchingDrawCall.shader->load();
	m_raymarchingDrawCall.loadQuad();
	initCageDrawCall();
}

Scene::~Scene() {
	glDeleteBuffers(1, &m_cageSsbo);
}

void Scene::render(const Camera& camera) const {
	m_meshDrawCall.shader->setUniform("uColorMultiplier", 0.3f);
	m_meshDrawCall.shader->setUniform("uColorGamma", 0.6f);
	m_meshDrawCall.shader->bindStorageBlock("CagePoints", m_cageSsbo);

	{
		ScopedTimer timer("MeshDrawCall");
		m_meshDrawCall.render(camera);
	}

	m_cageDrawCall.render(camera);

	m_handleDrawCall.shader->setUniform("uColor1", vec4(1, 0, 0, 0));
	m_handleDrawCall.render(camera);

	m_meshDrawCall.shader->bindStorageBlock("CagePoints", m_cageSsbo);
	if (properties().raymarching) {
		ScopedTimer timer("Raymarching");
		m_raymarchingDrawCall.render(camera);
	}
}

void Scene::loadMesh() {
	ObjUtils::LoadDrawCallFromMesh(properties().modelFilename, m_meshDrawCall);
}

glm::vec3 Scene::handlePosition(int i) const {
	return glm::make_vec3(&m_handleData[3 * i]);
}

void Scene::setHandlePosition(int i, const vec3& position) {
	memcpy(&m_handleData[3 * i], glm::value_ptr(position), 3 * sizeof(GLfloat));
	updateCageDrawCall();
}

vec2 Scene::handleScreenPosition(int i, const Camera& camera) const {
	return camera.projectPoint(handlePosition(i));
}

void Scene::setHandleScreenPosition(int i, const vec2& screenPosition, const vec3& initialWorldPosition, const Camera& camera) {
	glm::mat4 M = camera.projectionMatrix() * camera.viewMatrix();
	vec4 ndc = M * glm::vec4(initialWorldPosition, 1.0f);
	vec2 normalizedInitialScreenPosition = (glm::vec2(ndc.x, -ndc.y) / ndc.w * 0.5f + 0.5f);

	vec2 normalizedScreenPosition = screenPosition / camera.resolution();
	vec2 dp = normalizedScreenPosition - normalizedInitialScreenPosition;

	vec3 deltaWorldPosition = inverse(glm::mat3(M)) * vec3(2.0f * dp * vec2(1, -1) * ndc.w, 0);

	setHandlePosition(i, initialWorldPosition + deltaWorldPosition);
}

int Scene::getHandleNear(const vec2& mouse, const Camera& camera) {
	int best = -1;
	float sqRadius = properties().handleSize * properties().handleSize / 4;
	for (int i = 0; i < 8; ++i) {
		vec2 p = handleScreenPosition(i, camera);
		vec2 dp = p - mouse;
		float sqDistance = dot(dp, dp);
		if (sqDistance < sqRadius) {
			best = i;
			sqRadius = sqDistance;
		}
	}
	return best;
}

void Scene::resetCage() {
	resetHandleData();
	updateCageDrawCall();
}

//-----------------------------------------------------------------------------

void Scene::initCageDrawCall() {
	resetHandleData();

	const std::vector<GLuint> elementData = {
		0, 1,
		1, 2,
		2, 3,
		3, 0,

		4, 5,
		5, 6,
		6, 7,
		7, 4,

		0, 4,
		1, 5,
		2, 6,
		3, 7,
	};

	m_cageDrawCall.loadFromVectors(m_handleData, elementData, true);

	glCreateBuffers(1, &m_cageSsbo);
	glNamedBufferStorage(m_cageSsbo, 8 * sizeof(vec4), nullptr, GL_DYNAMIC_STORAGE_BIT);
	updateCageDrawCall();
}

void Scene::resetHandleData() {
	m_handleData = {
		-1, -1, -1,
		+1, -1, -1,
		+1, +1, -1,
		-1, +1, -1,

		-1, -1, +1,
		+1, -1, +1,
		+1, +1, +1,
		-1, +1, +1,
	};
}

void Scene::updateCageDrawCall() {
	glNamedBufferSubData(m_cageDrawCall.vbo, 0, m_handleData.size() * sizeof(GLfloat), m_handleData.data());

	static std::vector<vec4> alignedHandleData(8);
	for (int i = 0; i < 8; ++i) {
		alignedHandleData[i] = vec4(m_handleData[3 * i + 0], m_handleData[3 * i + 1], m_handleData[3 * i + 2], 1.0f);
	}

	glNamedBufferSubData(m_cageSsbo, 0, alignedHandleData.size() * sizeof(vec4), alignedHandleData.data());
}
