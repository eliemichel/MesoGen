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
#include "App.h"
#include "Scene.h"
#include "Ui/SceneDialog.h"
#include "Ui/GlobalTimerDialog.h"

#include <GlobalTimer.h>
#include <Logger.h>
#include <TurntableCamera.h>
#include <Ui/Window.h>
#include <ShaderPool.h>
#include <RenderTarget.h>
#include <ScopedFramebufferOverride.h>

#include <imgui.h>
#include <GLFW/glfw3.h>

#define UNUSED(x) (void)x

App::App(std::shared_ptr<Window> window)
	: BaseApp(window)
{
	m_scene = std::make_shared<Scene>();

	auto camera = std::make_unique<TurntableCamera>();
	m_camera = std::move(camera);

	std::vector<ColorLayerInfo> renderLayers = {
		{GL_RGBA32F, GL_COLOR_ATTACHMENT0},
		{GL_RGBA32F, GL_COLOR_ATTACHMENT1},
	};
	m_renderTarget = std::make_unique<RenderTarget>(1, 1, renderLayers);

	setupDialogs();
	resize();
}

void App::update() {
	m_scene->update();
	BaseApp::update();
}

void App::render() const {
	GlobalTimer::StartFrame();

	{
		ScopedFramebufferOverride lock;
		m_renderTarget->bind();
		glClearColor(1, 1, 1, 1);
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
		glEnable(GL_DEPTH_TEST);

		m_scene->render(*m_camera);
	}

	GLint width = static_cast<GLint>(m_camera->resolution().x);
	GLint height = static_cast<GLint>(m_camera->resolution().y);
	glBlitNamedFramebuffer(
		m_renderTarget->framebuffer().raw(),
		0,
		0, 0,
		width, height,
		0, 0,
		width, height,
		GL_COLOR_BUFFER_BIT,
		GL_NEAREST
	);

	renderGui();
	GlobalTimer::StopFrame();
}

//-----------------------------------------------------------------------------

void App::setupDialogs() {
	clearDialogs();
	addDialogGroup<SceneDialog>("Scene", m_scene);
	addDialogGroup<GlobalTimerDialog>("Timer", GlobalTimer::GetInstance());
}

void App::onResize(int width, int height) {
	m_camera->setResolution(width, height);
	m_renderTarget->setResolution(width, height);
}

void App::onMouseButton(int button, int action, int /*mods*/) {
	if (guiHasFocus()) return;

	switch (button) {
	case GLFW_MOUSE_BUTTON_LEFT:
		if (action == GLFW_PRESS) {
			m_grabbedHandle = m_scene->getHandleNear(m_mouse, *m_camera);
			if (m_grabbedHandle != -1) {
				m_grabOffset = m_scene->handleScreenPosition(m_grabbedHandle, *m_camera) - m_mouse;
				m_handleStart = m_scene->handlePosition(m_grabbedHandle);
			}
			else {
				m_camera->startMouseRotation();
			}
		}
		else if (action == GLFW_RELEASE) {
			m_camera->stopMouseRotation();
			m_grabbedHandle = -1;
		}
		break;
	case GLFW_MOUSE_BUTTON_MIDDLE:
		if (action == GLFW_PRESS) {
			m_camera->startMousePanning();
		}
		else if (action == GLFW_RELEASE) {
			m_camera->stopMousePanning();
		}
		break;
	case GLFW_MOUSE_BUTTON_RIGHT:
		if (action == GLFW_PRESS) {
			m_camera->startMouseZoom();
		}
		else if (action == GLFW_RELEASE) {
			m_camera->stopMouseZoom();
		}
		break;
	}
}

void App::onCursorPosition(double x, double y) {
	if (guiHasFocus()) return;

	m_mouse.x = static_cast<float>(x);
	m_mouse.y = static_cast<float>(y);

	m_camera->updateMousePosition(m_mouse.x, m_mouse.y);

	if (m_grabbedHandle != -1) {
		m_scene->setHandleScreenPosition(m_grabbedHandle, m_mouse + m_grabOffset, m_handleStart, *m_camera);
	}
}

void App::onScroll(double xoffset, double yoffset) {
	if (guiHasFocus()) return;

	UNUSED(xoffset);

	m_camera->zoom(static_cast<float>(yoffset));
}

void App::onKey(int key, int scancode, int action, int mods) {
	if (action == GLFW_PRESS && key == GLFW_KEY_ESCAPE) {
		if (auto window = getWindow().lock()) {
			glfwSetWindowShouldClose(window->glfw(), GL_TRUE);
		}
	}

	if (guiHasFocus()) return;

	UNUSED(scancode);
	UNUSED(mods);

	if (action == GLFW_PRESS) {
		switch (key) {
		case GLFW_KEY_P:
			setShowPanel(!showPanel());
			break;
		case GLFW_KEY_R:
			m_scene->resetCage();
			break;
		case GLFW_KEY_F5:
			ShaderPool::ReloadShaders();
			break;
		}
	}
}

//-----------------------------------------------------------------------------
