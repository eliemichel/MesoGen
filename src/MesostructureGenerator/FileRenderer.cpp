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
#include "FileRenderer.h"
#include "Ui/App.h"
#include "RuntimeObject.h"
#include "OutputObject.h"
#include "MacrosurfaceData.h"

#include <Logger.h>
#include <TurntableCamera.h>
#include <RenderTarget.h>
#include <ScopedFramebufferOverride.h>

#include <glm/glm.hpp>
#include <glm/ext.hpp>
#include <stb_image_write.h>

#include <fstream>
#include <filesystem>
namespace fs = std::filesystem;

constexpr float PI = 3.141592653589793238462643383279502884f;

FileRenderer::FileRenderer(App& app)
	: m_app(app)
{
	m_camera = std::make_unique<TurntableCamera>();
}

bool FileRenderer::renderStill() const {
	auto state = beforeRenderToFile();
	bool status = renderToFile(0);
	afterRenderToFile(state);
	return status;
}

bool FileRenderer::renderTurntable() const {
	auto state = beforeRenderToFile();
	bool status = true;

	auto& outputCamera = m_app.outputSlots().camera();
	glm::quat initialOrientation = outputCamera.orientation();

	for (int frame = 0; frame < properties().framePerTurn; ++frame) {
		float theta = frame / (float)properties().framePerTurn * 2 * PI;
		outputCamera.setOrientation(initialOrientation * glm::angleAxis(theta, glm::vec3(0.f, 0.f, 1.f)));
		if (!renderToFile(frame)) {
			status = false;
			break;
		}
	}

	outputCamera.setOrientation(initialOrientation);
	afterRenderToFile(state);
	return status;
}

bool FileRenderer::renderAnimation() const {
	// 1. Load camera animation
	const std::string& path = properties().cameraAnimation;
	std::ifstream file(path, std::ios::binary | std::ios::ate);
	if (!file.is_open()) {
		ERR_LOG << "Could not open viewMatrix buffer file: " << path;
		return false;
	}
	std::streamsize size = file.tellg() / sizeof(float);
	if (size % 16 != 0) {
		ERR_LOG << "viewMatrix buffer size must be a multiple of 16 (in file " << path << ")";
		return false;
	}
	file.seekg(0, std::ios::beg);
	std::shared_ptr<float[]> buffer(new float[size]);
	if (!file.read(reinterpret_cast<char*>(buffer.get()), size * sizeof(float))) {
		ERR_LOG << "Could not read viewMatrix buffer from file: " << path;
		return false;
	}

	int startFrame = 0;
	int endFrame = startFrame + static_cast<int>(size) / 16 - 1;
	auto setFrame = [startFrame, endFrame, buffer, this](int frame) {
		if (frame >= startFrame && frame < endFrame) {
			glm::mat4 viewMatrix = glm::make_mat4(buffer.get() + 16 * (frame - startFrame));

			glm::mat4 M = m_app.macrosurfaceData()->modelMatrix();
			glm::mat4 V = viewMatrix * inverse(M);

			m_camera->setViewMatrix(V);
			m_camera->updateUbo();
		}
	};

	// 2. Render
	auto state = beforeRenderToFile();
	bool status = true;

	for (int frame = startFrame; frame <= endFrame; ++frame) {
		setFrame(frame);
		if (!renderToFile(frame)) {
			status = false;
			break;
		}
	}
	return status;
}

bool FileRenderer::renderPbrMaps() const {
	auto& deferredRendering = m_app.deferredRendering();

	// Save state
	bool extraState = deferredRendering.properties().enabled;
	deferredRendering.properties().enabled = true;
	auto state = beforeRenderToFile();

	// Render
	bool status = pbrRenderToFile(0);

	// Restore state
	afterRenderToFile(state);
	deferredRendering.properties().enabled = extraState;

	return status;
}

void FileRenderer::updateModel() {
	auto& mainCamera = m_app.camera();
	mainCamera.setProjectionType(properties().cameraProjectionType);
	mainCamera.setFov(properties().cameraFov);
	mainCamera.updateUbo();
}

FileRenderer::SavedState FileRenderer::beforeRenderToFile() const {
	const auto& props = properties();

	GLsizei width = static_cast<GLsizei>(props.resolution.x);
	GLsizei height = static_cast<GLsizei>(props.resolution.y);

	//m_camera->setViewMatrix(m_app.camera().viewMatrix());
	m_camera->setOrientation(m_app.camera().orientation());
	m_camera->setCenter(m_app.camera().center());
	m_camera->setZoom(m_app.camera().getZoom());
	//theta = dy * m_sensitivity;

	m_camera->setOrthographicScale(m_app.camera().orthographicScale());
	m_camera->setFov(m_app.camera().fov());
	m_camera->setProjectionType(m_app.camera().projectionType());
	m_camera->setFarDistance(m_app.camera().farDistance());
	m_camera->setNearDistance(m_app.camera().nearDistance());
	m_camera->setResolution(width, height);

	auto& gbuffer = m_app.gbuffer();
	int fac = m_app.deferredRendering().properties().msaaFactor;
	SavedState state;
	state.gbufferWidth = gbuffer.width();
	state.gbufferHeight = gbuffer.height();
	gbuffer.setResolution(fac * width, fac * height);

	auto& deferredRenderingProps = m_app.deferredRendering().properties();
	state.deferredRenderingExportLinearDepth = deferredRenderingProps.exportLinearDepth;
	deferredRenderingProps.exportLinearDepth = true;

	return state;
}

void FileRenderer::afterRenderToFile(const SavedState& state) const {
	auto& gbuffer = m_app.gbuffer();
	gbuffer.setResolution(state.gbufferWidth, state.gbufferHeight);

	auto& deferredRendering = m_app.deferredRendering();
	deferredRendering.properties().exportLinearDepth = state.deferredRenderingExportLinearDepth;
}

bool FileRenderer::renderToFile(int frame) const {
	const auto& props = properties();
	std::vector<fs::path> paths = {
		resolvePath(frame),
		resolvePath(std::to_string(frame) + "_depth", ".hdr"),
	};

	if (exists(paths[0]) && !props.overwrite) return false;
	if (props.exportDepth && exists(paths[1]) && !props.overwrite) return false;
	LOG << "Rendering to " << paths[0];

	std::vector<unsigned char> imageData(4 * props.resolution.x * props.resolution.y);
	std::vector<float> depthData;

	GLsizei width = static_cast<GLsizei>(props.resolution.x);
	GLsizei height = static_cast<GLsizei>(props.resolution.y);
	renderTarget().setResolution(width, height);

	{
		ScopedFramebufferOverride framebufferSettings;
		renderTarget().bind();
		glViewport(0, 0, width, height);

		render(false /* pbrMaps */);

		glNamedFramebufferReadBuffer(renderTarget().framebuffer().raw(), GL_COLOR_ATTACHMENT0);
		glReadnPixels(0, 0, width, height, GL_RGBA, GL_UNSIGNED_BYTE, static_cast<GLsizei>(imageData.size()), imageData.data());

		if (props.exportDepth) {
			depthData.resize(props.resolution.x * props.resolution.y);
			//glNamedFramebufferReadBuffer(renderTarget().framebuffer().raw(), GL_NONE);
			//glReadnPixels(0, 0, width, height, GL_DEPTH_COMPONENT, GL_FLOAT, static_cast<GLsizei>(depthData.size() * sizeof(float)), depthData.data());
			glNamedFramebufferReadBuffer(renderTarget().framebuffer().raw(), GL_COLOR_ATTACHMENT1);
			glReadnPixels(0, 0, width, height, GL_RED, GL_FLOAT, static_cast<GLsizei>(depthData.size() * sizeof(float)), depthData.data());
		}
	}

	// Save buffer to file
	stbi_flip_vertically_on_write(1);
	int success = stbi_write_png(
		paths[0].string().c_str(),
		props.resolution.x,
		props.resolution.y,
		4,
		imageData.data(),
		4 * props.resolution.x
	);
	if (!success) return false;

	if (props.exportDepth) {
		// Save buffer to file
		stbi_flip_vertically_on_write(1);
		int success = stbi_write_hdr(
			paths[1].string().c_str(),
			props.resolution.x,
			props.resolution.y,
			1,
			depthData.data()
		);
		if (!success) return false;
	}

	return true;
}

bool FileRenderer::pbrRenderToFile(int frame) const {
	const auto& props = properties();
	std::vector<fs::path> paths = {
		resolvePath(std::to_string(frame) + "_baseColor"),
		resolvePath(std::to_string(frame) + "_normal"),
		resolvePath(std::to_string(frame) + "_metallicRoughness"),
	};
	for (const auto& p : paths) {
		if (exists(p) && !props.overwrite) return false;
	}
	LOG << "Rendering to " << paths[0];

	std::vector<unsigned char> imageData(4 * props.resolution.x * props.resolution.y);

	GLsizei width = static_cast<GLsizei>(props.resolution.x);
	GLsizei height = static_cast<GLsizei>(props.resolution.y);

	pbrRenderTarget().setResolution(width, height);

	{
		ScopedFramebufferOverride framebufferSettings;
		pbrRenderTarget().bind();
		glViewport(0, 0, width, height);

		render(true /* pbrMaps */);

		for (int i = 0; i < paths.size(); ++i) {
			glNamedFramebufferReadBuffer(pbrRenderTarget().framebuffer().raw(), GL_COLOR_ATTACHMENT0 + i);
			glReadnPixels(0, 0, width, height, GL_RGBA, GL_UNSIGNED_BYTE, static_cast<GLsizei>(imageData.size()), imageData.data());

			// Save buffer to file
			stbi_flip_vertically_on_write(1);
			int success = stbi_write_png(
				paths[i].string().c_str(),
				props.resolution.x,
				props.resolution.y,
				4,
				imageData.data(),
				4 * props.resolution.x
			);

			if (!success) return false;
		}
	}

	return true;
}

void FileRenderer::render(bool pbrMaps) const {
	auto& gbuffer = m_app.gbuffer();
	auto& deferredRendering = m_app.deferredRendering();
	bool useDeferredRendering = deferredRendering.properties().enabled;

	{
		ScopedFramebufferOverride framebufferSettings;
		if (useDeferredRendering) {
			gbuffer.bind();
			glViewport(0, 0, gbuffer.width(), gbuffer.height());
		}

		glClearColor(0, 0, 0, 0);
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
		glEnable(GL_DEPTH_TEST);

		for (const auto& obj : m_app.objects()) {
			obj->render(*m_camera);
		}
	}

	glClearColor(1.0, 1.0, 1.0, 1.0);
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
	glDisable(GL_DEPTH_TEST);

	if (useDeferredRendering) {
		deferredRendering.render(gbuffer, *m_camera, pbrMaps);
	}
}

fs::path FileRenderer::resolvePath(int frame) const {
	return resolvePath(std::to_string(frame));
}

fs::path FileRenderer::resolvePath(const std::string& frameName, const std::string& extension) const {
	fs::path base = properties().prefix + frameName + extension;
	create_directories(base.parent_path());
	return fs::absolute(base);
}

RenderTarget& FileRenderer::renderTarget() const {
	if (!m_renderTarget) {
		std::vector<ColorLayerInfo> colorLayerInfos = {
			{GL_RGBA32F, GL_COLOR_ATTACHMENT0},
			{GL_R32F, GL_COLOR_ATTACHMENT1} // for depth
		};
		m_renderTarget = std::make_unique<RenderTarget>(1, 1, colorLayerInfos);
	}
	return *m_renderTarget;
}

RenderTarget& FileRenderer::pbrRenderTarget() const {
	if (!m_pbrRenderTarget) {
		std::vector<ColorLayerInfo> colorLayerInfos = {
			{GL_RGBA32F, GL_COLOR_ATTACHMENT0},
			{GL_RGBA32F, GL_COLOR_ATTACHMENT1},
			{GL_RGBA32F, GL_COLOR_ATTACHMENT2}
		};
		m_pbrRenderTarget = std::make_unique<RenderTarget>(1, 1, colorLayerInfos);
	}
	return *m_pbrRenderTarget;
}
