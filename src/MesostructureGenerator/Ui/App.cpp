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
#include "QuadObject.h"
#include "TilesetEditorObject.h"
#include "TilesetController.h"
#include "TilesetRenderer.h"
#include "OutputObject.h"
#include "Serialization.h"
#include "Serialization/SerializationScene.h"
#include "TilingSolver.h"
#include "MacrosurfaceData.h"
#include "MesostructureData.h"
#include "MacrosurfaceRenderer.h"
#include "MesostructureRenderer.h"
#include "MesostructureMvcRenderer.h"
#include "TilingSolverRenderer.h"
#include "MesostructureExporter.h"
#include "FileRenderer.h"

#include "ModelDialog.h"
#include "OutputDialog.h"
#include "HelpDialog.h"
#include "TilesetRendererDialog.h"
#include "MacrosurfaceRendererDialog.h"
#include "MesostructureRendererDialog.h"
#include "MacrosurfaceDialog.h"
#include "TilingSolverDialog.h"
#include "TilingSolverRendererDialog.h"
#include "MesostructureExporterDialog.h"
#include "TilesetEditorDialog.h"
#include "GlobalTimerDialog.h"
#include "SimpleDialog.h"
#include "RenderDialog.h"

#include <ReflectionAttributes.h>
#include <Logger.h>
#include <TurntableCamera.h>
#include <Ui/Window.h>
#include <GroupObject.h>
#include <ShaderPool.h>
#include <GlobalTimer.h>
#include <RenderTarget.h>
#include <ScopedFramebufferOverride.h>

#include <imgui.h>
#include <GLFW/glfw3.h>
#include <glm/gtc/quaternion.hpp>
#include <refl.hpp>
#include <algorithm>

constexpr float PI = 3.141592653589793238462643383279502884f;

#define UNUSED(x) (void)x

App::App(std::shared_ptr<Window> window)
	: BaseApp(window)
{
	m_tileset = std::make_shared<Model::Tileset>();
	m_tilesetController = std::make_shared<TilesetController>();
	m_tilesetController->setModel(m_tileset);

	m_macrosurfaceData = std::make_shared<MacrosurfaceData>();
	m_mesostructureData = std::make_shared<MesostructureData>();
	m_mesostructureData->macrosurface = m_macrosurfaceData;

	m_outputSlots = std::make_shared<OutputObject>(m_macrosurfaceData, m_mesostructureData);
	m_outputSlots->setController(m_tilesetController);
	m_objects.push_back(m_outputSlots);

	m_macrosurfaceRenderer = std::make_shared<MacrosurfaceRenderer>();
	m_macrosurfaceRenderer->setData(m_macrosurfaceData);
	m_objects.push_back(m_macrosurfaceRenderer);

	m_mesostructureRenderer = std::make_shared<MesostructureRenderer>();
	m_mesostructureRenderer->setMesostructureData(m_mesostructureData);
	m_objects.push_back(m_mesostructureRenderer);

	m_tilesetController->setMesostructureData(m_mesostructureData);

	m_tilesetRenderer = std::make_shared<TilesetRenderer>();
	m_tilesetRenderer->setController(m_tilesetController);

	m_tilesetEditor = std::make_shared<TilesetEditorObject>();
	m_tilesetEditor->setController(m_tilesetController);
	m_tilesetEditor->setRenderer(m_tilesetRenderer);
	m_objects.push_back(m_tilesetEditor);

	m_cursorObject = std::make_shared<QuadObject>(0.01f);
	m_cursorObject->properties().color = glm::vec4(0, 0, 0, 1);
	m_objects.push_back(m_cursorObject);

	m_tilingSolver = std::make_shared<TilingSolver>();
	m_tilingSolver->setMesostructureData(m_mesostructureData);
	m_tilingSolver->setController(m_tilesetController);

	m_tilingSolverRenderer = std::make_shared<TilingSolverRenderer>(m_tilingSolver->tilingSolverData(), m_mesostructureData);
	m_objects.push_back(m_tilingSolverRenderer);

	m_mesostructureExporter = std::make_shared<MesostructureExporter>(m_mesostructureData, m_mesostructureRenderer);

	m_mesostructureMvcRenderer = std::make_shared<MesostructureMvcRenderer>(m_mesostructureRenderer);
	m_mesostructureRenderer->setMvcRenderer(m_mesostructureMvcRenderer);

	m_fileRenderer = std::make_shared<FileRenderer>(*this);

	auto camera = std::make_shared<TurntableCamera>();
	camera->setProjectionType(Camera::ProjectionType::Orthographic);
	camera->setOrientation(glm::quat(sqrt(2.f) / 2.f, -sqrt(2.f) / 2.f, 0.f, 0.f) * glm::angleAxis(PI / 2, glm::vec3(1, 0, 0)));
	camera->setOrthographicScale(1.5f);
	m_camera = camera;

	m_gbuffer = std::make_unique<RenderTarget>(1, 1, std::vector<ColorLayerInfo>{
		{ GL_RGBA32F, GL_COLOR_ATTACHMENT0 },
		{ GL_RGBA32UI, GL_COLOR_ATTACHMENT1 },
		{ GL_RGBA32UI, GL_COLOR_ATTACHMENT2 },
	});
	m_deferredRendering = std::make_unique<DeferredRendering>();

	setupDialogs();
	resize();
}

void App::update() {
	BaseApp::update();

	if (m_tilesetController->hasChangedLately()) {
		m_mesostructureRenderer->onModelChange();
	}

	for (const auto& obj : m_objects) {
		obj->update();
	}

	if (m_tilesetController->hasChangedLately()) {
		m_tilesetRenderer->rebuild();
	}

	m_tilesetController->resetChangedEvent();
}

void App::render() const {
	GlobalTimer::StartFrame();
	bool useDeferredRendering = m_deferredRendering->properties().enabled;

	{
		ScopedFramebufferOverride framebufferSettings;
		if (useDeferredRendering) {
			m_gbuffer->bind();
			glViewport(0, 0, m_gbuffer->width(), m_gbuffer->height());
		}

		glClearColor(0, 0, 0, 0);
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
		glDisable(GL_DEPTH_TEST);

		for (const auto& obj : m_objects) {
			obj->render(*m_camera);
		}

	}

	glClearColor(1.0, 1.0, 1.0, 1.0);
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
	glDisable(GL_DEPTH_TEST);

	if (useDeferredRendering) {
		m_deferredRendering->render(*m_gbuffer, *m_camera);
	}

	GlobalTimer::StopFrame();

	renderGui();
}

//-----------------------------------------------------------------------------

void App::deserialize(const Document& document, const Value& json) {
	DESER_ASSERT(json, Object);
	DESER_ASSERT_HAS_MEMBER(json, "version");
	int version = json["version"].GetInt();
	if (version != 1) {
		throw Serialization::DeserializationError("Unsupported scene file version: " + std::to_string(version) + " (expected 1)");
	}

	Serialization::autoDeserialize(m_tilesetController->properties(), document, json["tilesetController"]);
	Serialization::autoDeserialize(m_tilesetEditor->properties(), document, json["tilesetEditor"]);
	Serialization::autoDeserialize(m_macrosurfaceData->properties(), document, json["macrosurfaceData"]);
	Serialization::autoDeserialize(m_tilingSolver->properties(), document, json["tilingSolver"]);
	Serialization::autoDeserialize(m_macrosurfaceRenderer->properties(), document, json["macrosurfaceRenderer"]);
	Serialization::autoDeserialize(m_mesostructureRenderer->properties(), document, json["mesostructureRenderer"]);
	Serialization::autoDeserialize(m_mesostructureExporter->properties(), document, json["mesostructureExporter"]);
	Serialization::autoDeserialize(m_fileRenderer->properties(), document, json["fileRenderer"]);
}

void App::serialize(Document& document, Value& json) const {
	auto& allocator = document.GetAllocator();
	json.SetObject();
	json.AddMember("version", 1, allocator);
	json.AddMember("tilesetController", Serialization::autoSerialize(m_tilesetController->properties(), document), allocator);
	json.AddMember("tilesetEditor", Serialization::autoSerialize(m_tilesetEditor->properties(), document), allocator);
	json.AddMember("macrosurfaceData", Serialization::autoSerialize(m_macrosurfaceData->properties(), document), allocator);
	json.AddMember("tilingSolver", Serialization::autoSerialize(m_tilingSolver->properties(), document), allocator);
	json.AddMember("macrosurfaceRenderer", Serialization::autoSerialize(m_macrosurfaceRenderer->properties(), document), allocator);
	json.AddMember("mesostructureRenderer", Serialization::autoSerialize(m_mesostructureRenderer->properties(), document), allocator);
	json.AddMember("mesostructureExporter", Serialization::autoSerialize(m_mesostructureExporter->properties(), document), allocator);
	json.AddMember("fileRenderer", Serialization::autoSerialize(m_fileRenderer->properties(), document), allocator);
}

//-----------------------------------------------------------------------------

void App::setupDialogs() {
	clearDialogs();
	addDialogGroup<SimpleDialog>("Simple", this);
	addDialogGroup<ModelDialog>("Model", m_tilesetController);
	addDialogGroup<TilesetEditorDialog>("Interface Editor", m_tilesetEditor);
	addDialogGroup<MacrosurfaceDialog>("Macrosurface", m_macrosurfaceData);
	addDialogGroup<TilingSolverDialog>("Generator", m_tilingSolver);
	addDialogGroup<MacrosurfaceRendererDialog>("Macrosurface Renderer", m_macrosurfaceRenderer);
	addDialogGroup<MesostructureRendererDialog>("Mesostructure Renderer", m_mesostructureRenderer);
	//addDialogGroup<TilingSolverRendererDialog>("Generator Debug Renderer", m_tilingSolverRenderer);
	addDialogGroup<MesostructureExporterDialog>("Export", m_mesostructureExporter);
	addDialogGroup<RenderDialog>("Render", m_fileRenderer);
	addDialogGroup<GlobalTimerDialog>("Timing", GlobalTimer::GetInstance());
	addDialogGroup<HelpDialog>("Help", nullptr);

	dialogGroups()[0].enabled = true;
}

void App::onResize(int width, int height) {
	m_camera->setResolution(width, height);
	int fac = m_deferredRendering->properties().msaaFactor;
	m_gbuffer->setResolution(fac * width, fac * height);
}

void App::onMouseButton(int button, int action, int mods) {
	if (guiHasFocus()) return;

	switch (button) {
	case GLFW_MOUSE_BUTTON_LEFT:
		if ((mods & GLFW_MOD_ALT) != 0) {
			if (action == GLFW_PRESS) {
				m_camera->startMousePanning();
			}
			else if (action == GLFW_RELEASE) {
				m_camera->stopMousePanning();
			}
			break;
		}
		else if ((mods & GLFW_MOD_SHIFT) != 0) {
			if (action == GLFW_PRESS) {
				m_outputSlots->onMousePress(true /* force hover */);
			}
			else if (action == GLFW_RELEASE) {
				m_outputSlots->onMouseRelease();
			}
			break;
		}
		else {
			if (action == GLFW_PRESS) {
				if (!m_outputSlots->onMousePress()) {
					m_tilesetEditor->onMousePress();
				}
			}
			else if (action == GLFW_RELEASE) {
				m_outputSlots->onMouseRelease();
				m_tilesetEditor->onMouseRelease();
			}
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

	glm::vec2 mouseScreenPosition(
		static_cast<float>(x),
		static_cast<float>(y)
	);

	m_camera->updateMousePosition(mouseScreenPosition.x, mouseScreenPosition.y);
	m_outputSlots->onCursorPosition(mouseScreenPosition.x, mouseScreenPosition.y, *m_camera);
	m_tilesetEditor->onCursorPosition(mouseScreenPosition.x, mouseScreenPosition.y, *m_camera);
	
	m_mouseRay = m_camera->mouseRay(mouseScreenPosition);

	if (hitGround(m_mouseRay, m_mousePosition)) {
		m_cursorObject->setPosition(m_mousePosition);

		afterUpdateMousePosition();
	}
}

void App::onScroll(double xoffset, double yoffset) {
	if (guiHasFocus()) return;

	UNUSED(xoffset);

	bool holdingControl = false;
	if (auto window = getWindow().lock()) {
		holdingControl = (
			glfwGetKey(window->glfw(), GLFW_KEY_LEFT_CONTROL) == GLFW_PRESS ||
			glfwGetKey(window->glfw(), GLFW_KEY_RIGHT_CONTROL) == GLFW_PRESS
		);
	}

	if (holdingControl) {
		m_tilesetEditor->onMouseScroll(static_cast<float>(yoffset));
	}
	else {
		m_camera->zoom(-2 * static_cast<float>(yoffset));
	}

	afterUpdateMousePosition();
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

		case GLFW_KEY_C:
			printOutputCameraMatrix();
			break;

		case GLFW_KEY_F5:
			ShaderPool::ReloadShaders();
			break;
		}
	}
}

//-----------------------------------------------------------------------------

void App::afterUpdateMousePosition() {
	//m_tileEditor->onMouseMove(m_mousePosition);
}

//-----------------------------------------------------------------------------

bool App::hitGround(const Ray& ray, glm::vec3& hit) {
	if (std::abs(ray.direction.z) < 1e-5) return false;

	float t = -ray.origin.z / ray.direction.z;
	if (t < 0) return false;

	hit = ray.origin + t * ray.direction;
	return true;
}

void App::saveModel() {
	Serialization::serializeTo(*m_tileset, "tileset.json");
}

void App::loadModel() {
	Serialization::deserializeFrom(*m_tileset, "tileset.json");
	m_tilesetController->setDirty();
}

void App::printOutputCameraMatrix() {
	glm::mat4 P, V;
	P = camera().projectionMatrix();
	V = camera().viewMatrix() * m_macrosurfaceData->modelMatrix();

	static auto printMatrix = [](const glm::mat4& M) {
		std::cout << "  (" << M[0][0] << ", " << M[1][0] << ", " << M[2][0] << ", " << M[3][0] << ")," << std::endl;
		std::cout << "  (" << M[0][1] << ", " << M[1][1] << ", " << M[2][1] << ", " << M[3][1] << ")," << std::endl;
		std::cout << "  (" << M[0][2] << ", " << M[1][2] << ", " << M[2][2] << ", " << M[3][2] << ")," << std::endl;
		std::cout << "  (" << M[0][3] << ", " << M[1][3] << ", " << M[2][3] << ", " << M[3][3] << ")," << std::endl;
	};

	std::cout << "Output Camera Matrix:" << std::endl;
	std::cout << "perspective = Matrix((" << std::endl;
	printMatrix(P);
	std::cout << "))" << std::endl;
	std::cout << "view = Matrix((" << std::endl;
	printMatrix(V);
	std::cout << "))" << std::endl;
}
