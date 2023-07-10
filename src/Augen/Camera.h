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

#include <OpenGL>

#include <refl.hpp>
#include <glm/glm.hpp>

#include <memory>
#include <string>
#include <vector>

#include "utils/macros.h"
#include "Ray.h"

class RenderTarget;

class Camera {
public:
	enum class ProjectionType {
		Perspective,
		Orthographic,
	};
	struct OutputSettings {
		bool autoOutputResolution = true;
		int width;
		int height;
	};
	enum class ExtraRenderTargetOption {
		Rgba32fDepth = 0,
		TwoRgba32fDepth = 1,
		Depth = 2,
		GBufferDepth = 3, // attachements to hold a g-buffer (see gbuffer.inc.glsl plus a depth buffer
		LinearGBufferDepth = 4, // same with linear g-buffer
		LeanLinearGBufferDepth = 5, // same with linear g-buffer + (pseudo) lean maps
		_Count,
	};

public:
	Camera();
	~Camera();
	Camera(const Camera&) = delete;
	Camera & operator=(const Camera&) = delete;

	virtual void update(float time);

	GLuint ubo() const { return m_ubo; }

	inline glm::vec3 position() const { return m_position; }

	inline glm::mat4 viewMatrix() const { return m_uniforms.viewMatrix; }
	
	inline glm::mat4 projectionMatrix() const { return m_uniforms.projectionMatrix; }
	ProjectionType projectionType() const { return m_projectionType; }
	void setProjectionType(ProjectionType projectionType);

	inline glm::vec2 resolution() const { return m_uniforms.resolution; }
	void setResolution(glm::vec2 resolution);
	inline void setResolution(int w, int h) { setResolution(glm::vec2(static_cast<float>(w), static_cast<float>(h))); }
	void setFreezeResolution(bool freeze);

	float fov() const { return m_fov; }
	float focalLength() const;
	void setFov(float fov);

	float nearDistance() const { return m_uniforms.uNear; }
	void setNearDistance(float distance);

	float farDistance() const { return m_uniforms.uFar; }
	void setFarDistance(float distance);

	void setOrthographicScale(float orthographicScale);
	float orthographicScale() const { return m_orthographicScale; }

	float panningSensitivity() const { return m_panningSensitivity; }
	void setPanningSensitivity(float panningSensitivity) { m_panningSensitivity = panningSensitivity; }

	OutputSettings & outputSettings() { return m_outputSettings; }
	const OutputSettings & outputSettings() const { return m_outputSettings; }

	std::shared_ptr<RenderTarget> renderTarget() const { return m_renderTarget; }

	inline void startMouseRotation() { m_isMouseRotationStarted = true; m_isLastMouseUpToDate = false; }
	inline void stopMouseRotation() { m_isMouseRotationStarted = false; }

	inline void startMouseZoom() { m_isMouseZoomStarted = true; m_isLastMouseUpToDate = false; }
	inline void stopMouseZoom() { m_isMouseZoomStarted = false; }

	inline void zoom(float factor) { updateDeltaMouseZoom(0, 0, 0, factor); }
	
	inline void startMousePanning() { m_isMousePanningStarted = true; m_isLastMouseUpToDate = false; }
	inline void stopMousePanning() { m_isMousePanningStarted = false; }

	inline void tiltLeft() { tilt(-0.1f); }
	inline void tiltRight() { tilt(0.1f); }

	void updateMousePosition(float x, float y);

	virtual void setCenter(glm::vec3 center) { UNUSED(center); }

	Ray mouseRay(glm::vec2 mousePosition) const;

	/**
	 * Get a framebuffer that has the same resolution as the camera, to be used
	 * as intermediate step in render. Once you're done with it, release the
	 * framebuffer with releaseExtraFramebuffer(). It is safe to call this
	 * every frame.
	 * TODO: implement a proper dynamic framebuffer pool mechanism
	 */
	std::shared_ptr<RenderTarget> getExtraRenderTarget(ExtraRenderTargetOption option = ExtraRenderTargetOption::Rgba32fDepth) const;
	void releaseExtraRenderTarget(std::shared_ptr<RenderTarget>) const;

	glm::vec2 projectPoint(const glm::vec3& point) const;

	/**
	 * Bounding circle of the projected sphere (which is an ellipsis).
	 * xy is the center, z is the radius, all in pixels
	 */
	glm::vec3 projectSphere(const glm::vec3& center, float radius) const;

	/**
	 * Convert depth value from Z-buffer to a linear depth in camera space units
	 */
	float linearDepth(float zbufferDepth) const;

	public:
		struct Properties {
			// xy: offset, relative to screen size
			// zw: size, relative to screen size
			// e.g. for fullscreen: (0, 0, 1, 1)
			glm::vec4 viewRect = glm::vec4(0, 0, 1, 1);

			bool displayInViewport = true;
			bool controlInViewport = true; // receive mouse input
		};
		const Properties& properties() const { return m_properties; }
		Properties & properties(){ return m_properties; }

protected:
	/**
	* Called when mouse moves and rotation has started
	* x1, y1: old mouse position
	* x2, y2: new mouse position
	*/
	virtual void updateDeltaMouseRotation(float x1, float y1, float x2, float y2) {
		UNUSED(x1); UNUSED(y1); UNUSED(x2); UNUSED(y2);
	}

	/**
	* Called when mouse moves and zoom has started
	* x1, y1: old mouse position
	* x2, y2: new mouse position
	*/
	virtual void updateDeltaMouseZoom(float x1, float y1, float x2, float y2) {
		UNUSED(x1); UNUSED(y1); UNUSED(x2); UNUSED(y2);
	}

	/**
	* Called when mouse moves and panning has started
	* x1, y1: old mouse position
	* x2, y2: new mouse position
	*/
	virtual void updateDeltaMousePanning(float x1, float y1, float x2, float y2) {
		UNUSED(x1); UNUSED(y1); UNUSED(x2); UNUSED(y2);
	}

	virtual void tilt(float theta) { UNUSED(theta); }

protected:
	void initUbo();
public:
	void updateUbo();

	inline void setViewMatrix(glm::mat4 matrix) { m_uniforms.viewMatrix = matrix; }  // use with caution
	inline void setProjectionMatrix(glm::mat4 matrix) { m_uniforms.projectionMatrix = matrix; }  // use with caution

private:
	// Memory layout on GPU, matches shaders/include/uniform/camera.inc.glsl
	struct CameraUbo {
		glm::mat4 viewMatrix;
		glm::mat4 projectionMatrix;
		glm::mat4 inverseViewMatrix;
		glm::vec2 resolution;
		float uNear = 0.001f;
		float uFar = 1000.f;
		float uLeft;
		float uRight;
		float uTop;
		float uBottom;
	};

private:
	void updateProjectionMatrix();

protected:
	// Core data
	Properties m_properties;
	CameraUbo m_uniforms;
	GLuint m_ubo;

	// Other data, used to build matrices but not shared with gpu
	glm::vec3 m_position;
	float m_fov;
	float m_orthographicScale;

	// Move related attributes
	bool m_isMouseRotationStarted, m_isMouseZoomStarted, m_isMousePanningStarted;
	bool m_isLastMouseUpToDate;
	float m_lastMouseX, m_lastMouseY;
	float m_panningSensitivity = 1.0f;

	bool m_freezeResolution;

	// When resolution is freezed, this target framebuffer is allocated at the
	// fixed resolution and can be bound by the render pipeline
	std::shared_ptr<RenderTarget> m_renderTarget;

	mutable std::vector<std::shared_ptr<RenderTarget>> m_extraRenderTargets; // lazy initialized

	OutputSettings m_outputSettings;
	ProjectionType m_projectionType;
};

#define _ ReflectionAttributes::
REFL_TYPE(Camera::Properties)
REFL_FIELD(viewRect)
REFL_FIELD(displayInViewport)
REFL_FIELD(controlInViewport)
REFL_END
#undef _
