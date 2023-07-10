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


#include "Camera.h"
#include "Logger.h"
#include "RenderTarget.h"
#include "ColorLayerInfo.h"

#include <glm/glm.hpp>
#include <glm/gtx/euler_angles.hpp>

#include <iostream>
#include <algorithm>

Camera::Camera()
	: m_isMouseRotationStarted(false)
	, m_isMouseZoomStarted(false)
	, m_isMousePanningStarted(false)
	, m_isLastMouseUpToDate(false)
	, m_fov(45.0f)
	, m_orthographicScale(1.0f)
	, m_freezeResolution(false)
	, m_projectionType(ProjectionType::Perspective)
	, m_extraRenderTargets(static_cast<int>(ExtraRenderTargetOption::_Count))
{
	initUbo();
	setResolution(800, 600);
}

Camera::~Camera() {
	glDeleteBuffers(1, &m_ubo);
}

void Camera::update(float time) {
	m_position = glm::vec3(3.f * cos(time), 3.f * sin(time), 0.f);
	m_uniforms.viewMatrix = glm::lookAt(
		m_position,
		glm::vec3(0.0f, 0.0f, 0.0f),
		glm::vec3(0.0f, 0.0f, 1.0f));
}

void Camera::initUbo() {
	glCreateBuffers(1, &m_ubo);
	glNamedBufferStorage(m_ubo, static_cast<GLsizeiptr>(sizeof(CameraUbo)), NULL, GL_DYNAMIC_STORAGE_BIT);
}
void Camera::updateUbo() {
	
	m_uniforms.inverseViewMatrix = inverse(m_uniforms.viewMatrix);
	m_uniforms.resolution = resolution();
	glNamedBufferSubData(m_ubo, 0, static_cast<GLsizeiptr>(sizeof(CameraUbo)), &m_uniforms);
}

void Camera::setResolution(glm::vec2 resolution)
{
	if (!m_freezeResolution) {
		glm::vec4 rect = properties().viewRect;
		m_uniforms.resolution = resolution * glm::vec2(rect.z, rect.w);
	}

	updateProjectionMatrix();

	// Resize extra framebuffers
	GLsizei width = static_cast<GLsizei>(m_uniforms.resolution.x);
	GLsizei height = static_cast<GLsizei>(m_uniforms.resolution.y);
	for (auto& target : m_extraRenderTargets) {
		if (target) target->setResolution(width, height);
	}

	updateUbo();
}

void Camera::setFreezeResolution(bool freeze)
{
	m_freezeResolution = freeze;
	if (m_freezeResolution) {
		GLsizei width = static_cast<GLsizei>(m_uniforms.resolution.x);
		GLsizei height = static_cast<GLsizei>(m_uniforms.resolution.y);
		const std::vector<ColorLayerInfo> colorLayerInfos = { { GL_RGBA32F,  GL_COLOR_ATTACHMENT0 } };
		m_renderTarget = std::make_shared<RenderTarget>(width, height, colorLayerInfos);
	} else {
		m_renderTarget = nullptr;
	}
}

void Camera::setFov(float fov)
{
	m_fov = fov;
	updateProjectionMatrix();
}

float Camera::focalLength() const
{
	return 1.0f / (2.0f * glm::tan(glm::radians(fov()) / 2.0f));
}

void Camera::setNearDistance(float distance)
{
	m_uniforms.uNear = distance;
	updateProjectionMatrix();
}

void Camera::setFarDistance(float distance)
{
	m_uniforms.uFar = distance;
	updateProjectionMatrix();
}


void Camera::setOrthographicScale(float orthographicScale)
{
	m_orthographicScale = orthographicScale;
	updateProjectionMatrix();
}

void Camera::setProjectionType(ProjectionType projectionType)
{
	m_projectionType = projectionType;
	updateProjectionMatrix();
}

void Camera::updateMousePosition(float x, float y)
{
	if (m_isLastMouseUpToDate) {
		if (m_isMouseRotationStarted) {
			updateDeltaMouseRotation(m_lastMouseX, m_lastMouseY, x, y);
		}
		if (m_isMouseZoomStarted) {
			updateDeltaMouseZoom(m_lastMouseX, m_lastMouseY, x, y);
		}
		if (m_isMousePanningStarted) {
			updateDeltaMousePanning(m_lastMouseX, m_lastMouseY, x, y);
		}
	}
	m_lastMouseX = x;
	m_lastMouseY = y;
	m_isLastMouseUpToDate = true;
}

void Camera::updateProjectionMatrix()
{
	switch (m_projectionType) {
	case ProjectionType::Perspective:
		if (m_uniforms.resolution.x > 0.0f && m_uniforms.resolution.y > 0.0f) {
			m_uniforms.projectionMatrix = glm::perspectiveFov(glm::radians(m_fov), m_uniforms.resolution.x, m_uniforms.resolution.y, m_uniforms.uNear, m_uniforms.uFar);
			m_uniforms.uRight = m_uniforms.uNear / m_uniforms.projectionMatrix[0][0];
			m_uniforms.uLeft = -m_uniforms.uRight;
			m_uniforms.uTop = m_uniforms.uNear / m_uniforms.projectionMatrix[1][1];
			m_uniforms.uBottom = -m_uniforms.uTop;
		}
		break;
	case ProjectionType::Orthographic:
	{
		float ratio = m_uniforms.resolution.x > 0.0f ? m_uniforms.resolution.y / m_uniforms.resolution.x : 1.0f;
		m_uniforms.projectionMatrix = glm::ortho(-m_orthographicScale, m_orthographicScale, -m_orthographicScale * ratio, m_orthographicScale * ratio, m_uniforms.uNear, m_uniforms.uFar);
		m_uniforms.uRight = 1.0f / m_uniforms.projectionMatrix[0][0];
		m_uniforms.uLeft = -m_uniforms.uRight;
		m_uniforms.uTop = 1.0f / m_uniforms.projectionMatrix[1][1];
		m_uniforms.uBottom = -m_uniforms.uTop;
		break;
	}
	}
}

Ray Camera::mouseRay(glm::vec2 mousePosition) const
{
	Ray ray_cs;
	glm::vec2 normalizedMousePosition = (mousePosition - 0.5f * resolution()) / resolution().y * glm::vec2(1,-1);
	if (projectionType() == ProjectionType::Perspective) {
		ray_cs.direction = normalize(glm::vec3(
			normalizedMousePosition,
			-focalLength()
		));
		ray_cs.direction.y = -ray_cs.direction.y;
	} else {
		float ratio = m_uniforms.resolution.x > 0.0f ? m_uniforms.resolution.y / m_uniforms.resolution.x : 1.0f;
		ray_cs.origin = glm::vec3(
			normalizedMousePosition * 2.0f * orthographicScale() * ratio,
			0
		);
		ray_cs.direction = glm::vec3(0, 0, -1);
	}

	Ray ray_ws;
	ray_ws.origin = m_uniforms.inverseViewMatrix * glm::vec4(ray_cs.origin, 1.0f);
	ray_ws.direction = glm::mat3(m_uniforms.inverseViewMatrix) * ray_cs.direction;
	return ray_ws;
}

glm::vec2 Camera::projectPoint(const glm::vec3& point) const
{
	glm::vec4 ndc = projectionMatrix() * viewMatrix() * glm::vec4(point, 1.0f);
	return (glm::vec2(ndc.x, -ndc.y) / ndc.w * 0.5f + 0.5f) * resolution();
}

glm::vec3 Camera::projectSphere(const glm::vec3& center, float radius) const
{
	// http://www.iquilezles.org/www/articles/sphereproj/sphereproj.htm
	glm::vec3 o = (viewMatrix() * glm::vec4(center, 1.0f));

	glm::mat2 R = glm::mat2(o.x, o.y, -o.y, o.x);

	float r2 = radius * radius;
	float fl = focalLength();
	float ox2 = o.x * o.x;
	float oy2 = o.y * o.y;
	float oz2 = o.z * o.z;
	float fp = fl * fl * r2 * (ox2 + oy2 + oz2 - r2) / (oz2 - r2);
	float outerRadius = sqrt(abs(fp / (r2 - oz2)));

	glm::vec2 circleCenter = glm::vec2(o.x, o.y) * o.z * fl / (oz2 - r2);

	float pixelRadius = outerRadius * m_uniforms.resolution.y;
	glm::vec2 pixelCenter = circleCenter * m_uniforms.resolution.y * glm::vec2(-1.0f, 1.0f) + m_uniforms.resolution * 0.5f;

	return glm::vec3(pixelCenter, pixelRadius);
}

float Camera::linearDepth(float zbufferDepth) const
{
	return (2.0f * m_uniforms.uNear * m_uniforms.uFar) / (m_uniforms.uFar + m_uniforms.uNear - (zbufferDepth * 2.0f - 1.0f) * (m_uniforms.uFar - m_uniforms.uNear));
}

///////////////////////////////////////////////////////////////////////////////
// Framebuffer pool
///////////////////////////////////////////////////////////////////////////////

std::shared_ptr<RenderTarget> Camera::getExtraRenderTarget(ExtraRenderTargetOption option) const
{
	// TODO: add mutex
	auto& target = m_extraRenderTargets[static_cast<int>(option)];
	// Lazy construction
	if (!target) {
		int width = static_cast<int>(m_uniforms.resolution.x);
		int height = static_cast<int>(m_uniforms.resolution.y);
		std::vector<ColorLayerInfo> colorLayerInfos;
		using Opt = ExtraRenderTargetOption;
		switch (option)
		{
		case Opt::Rgba32fDepth:
			colorLayerInfos = std::vector<ColorLayerInfo>{ { GL_RGBA32F,  GL_COLOR_ATTACHMENT0 } };
			break;
		case Opt::TwoRgba32fDepth:
		case Opt::LinearGBufferDepth:
			colorLayerInfos = std::vector<ColorLayerInfo>{
				{ GL_RGBA32F,  GL_COLOR_ATTACHMENT0 },
				{ GL_RGBA32F,  GL_COLOR_ATTACHMENT1 }
			};
			break;
		case Opt::LeanLinearGBufferDepth:
			colorLayerInfos = std::vector<ColorLayerInfo>{
				{ GL_RGBA32F,  GL_COLOR_ATTACHMENT0 },
				{ GL_RGBA32F,  GL_COLOR_ATTACHMENT1 },
				{ GL_RGBA32F,  GL_COLOR_ATTACHMENT2 }
			};
			break;
		case Opt::Depth:
			colorLayerInfos = std::vector<ColorLayerInfo>{};
			break;
		case Opt::GBufferDepth:
			colorLayerInfos = std::vector<ColorLayerInfo>{
				{ GL_RGBA32F,  GL_COLOR_ATTACHMENT0 },
				{ GL_RGBA32UI,  GL_COLOR_ATTACHMENT1 },
				{ GL_RGBA32UI,  GL_COLOR_ATTACHMENT2 }
			};
			break;
		}
		target = std::make_shared<RenderTarget>(width, height, colorLayerInfos);
	}
	return target;
}

void Camera::releaseExtraRenderTarget(std::shared_ptr<RenderTarget>) const
{
	// TODO: release mutexes
}
