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


#include <OpenGL>

#include "Logger.h"
#include "BaseApp.h"
#include "Window.h"
#include "Dialog.h"

#include <GLFW/glfw3.h>
#include <imgui.h>
#include "imgui_impl_glfw.h"
#include "imgui_impl_opengl3.h"

#include <iostream>

using namespace std;

//-----------------------------------------------------------------------------

BaseApp::BaseApp(std::shared_ptr<Window> window)
	: m_window(window)
{
	setupCallbacks();

	Init(*window);

	int width, height;
	glfwGetFramebufferSize(window->glfw(), &width, &height);
	onResizeInternal(width, height);
	onResize(width, height);
}

BaseApp::~BaseApp() {
	Shutdown();
}

void BaseApp::beforeLoading()
{
	NewFrame();
	ImGui::SetNextWindowSize(ImVec2(m_windowWidth, m_windowHeight));
	ImGui::SetNextWindowPos(ImVec2(0.f, 0.f));
	ImGui::Begin("Reload", nullptr,
		ImGuiWindowFlags_NoResize |
		ImGuiWindowFlags_NoMove |
		ImGuiWindowFlags_NoCollapse |
		ImGuiWindowFlags_NoTitleBar);
	ImGui::Text(" ");
	ImGui::Text(" ");
	ImGui::Text("Loading scene...");
	ImGui::End();
	DrawFrame();
	if (auto window = m_window.lock()) {
		glfwSwapBuffers(window->glfw());
	}
}

void BaseApp::afterLoading()
{
	if (auto window = m_window.lock()) {
		int width, height;
		glfwGetFramebufferSize(window->glfw(), &width, &height);
		onResize(width, height);
	}

	m_startTime = static_cast<float>(glfwGetTime());
	
	setupDialogs();
}

void BaseApp::updateGui() {
	ImGuiIO& io = ImGui::GetIO();
	NewFrame();
	ImVec4 clear_color = ImColor(114, 144, 154);
	static float f = 0.0f;
	ImGui::SetNextWindowPos(ImVec2(0.f, 0.f));
	ImGui::SetNextWindowSize(ImVec2(420.f * io.FontGlobalScale, 0.f));
	ImGui::Begin("Framerate", nullptr,
		ImGuiWindowFlags_NoResize |
		ImGuiWindowFlags_NoMove |
		ImGuiWindowFlags_NoCollapse |
		ImGuiWindowFlags_NoTitleBar);
	ImGui::Text("Application average %.3f ms/frame (%.1f FPS)", 1000.0f / ImGui::GetIO().Framerate, ImGui::GetIO().Framerate);
	ImGui::End();

	// Handles
	int dialogId = 0;
	for (auto & dg : m_dialogGroups) {
		ImGui::PushID(dialogId++);
		for (auto d : dg.dialogs) {
			d->drawHandles(0, 0, m_windowWidth, m_windowHeight);
		}
		ImGui::PopID();
	}

	m_panelWidth = 300 * io.FontGlobalScale;

	if (m_showPanel) {
		ImGui::SetNextWindowSize(ImVec2(m_panelWidth, m_windowHeight));
		ImGui::SetNextWindowPos(ImVec2(m_windowWidth - m_panelWidth, 0.f));
		ImGui::Begin("Dialogs", nullptr,
			ImGuiWindowFlags_NoResize |
			ImGuiWindowFlags_NoMove |
			ImGuiWindowFlags_NoCollapse |
			ImGuiWindowFlags_NoTitleBar);

		// Outliner
		ImGui::PushID(0);

		dialogId = 0;
		bool pressed;
		for (auto & dg : m_dialogGroups) {
			ImGui::PushID(dialogId);
			dialogId += static_cast<int>(dg.dialogs.size());
			pressed = ImGui::Selectable(dg.title.c_str(), &dg.enabled);
			if (pressed && m_isControlPressed == 0) {
				// Select exclusive
				for (auto & other : m_dialogGroups) {
					other.enabled = &other == &dg;
				}
			}
			ImGui::PopID();
		}

		ImGui::PopID();
		
		// Visible dialogs
		ImGui::PushID(1);

		dialogId = 0;
		for (auto & dg : m_dialogGroups) {
			if (dg.enabled) {
				for (auto d : dg.dialogs) {
					ImGui::PushID(dialogId++);
					d->draw();
					ImGui::PopID();
				}
			}
			else {
				dialogId += static_cast<int>(dg.dialogs.size());
			}
		}

		ImGui::PopID();

		ImGui::End();
	}
}

void BaseApp::renderGui() const {
	glBindFramebuffer(GL_FRAMEBUFFER, 0);
	DrawFrame();
}

void BaseApp::resize() {
	onResize(
		static_cast<int>(m_windowWidth),
		static_cast<int>(m_windowHeight)
	);
}

void BaseApp::onResize(int, int) {}
void BaseApp::onMouseButton(int, int, int) {}
void BaseApp::onKey(int, int, int, int) {}
void BaseApp::onCursorPosition(double, double) {}
void BaseApp::onScroll(double, double) {}

void BaseApp::onResizeInternal(int width, int height) {
	m_windowWidth = static_cast<float>(width);
	m_windowHeight = static_cast<float>(height);

	if (auto window = m_window.lock()) {
		int fbWidth, fbHeight;
		glfwGetFramebufferSize(window->glfw(), &fbWidth, &fbHeight);
		glViewport(0, 0, fbWidth, fbHeight);
	}
}

void BaseApp::onMouseButtonInternal(int, int action, int) {
	if (!guiHasFocus()) {
		m_isMouseMoveStarted = action == GLFW_PRESS;
	}
}

void BaseApp::onCursorPositionInternal(double x, double) {
	double limit = static_cast<double>(m_windowWidth) - static_cast<double>(m_panelWidth);
	m_guiFocus = x >= limit && m_showPanel && !m_isMouseMoveStarted;
}

//-----------------------------------------------------------------------------

void BaseApp::Init(const Window& window)
{
	IMGUI_CHECKVERSION();
	ImGui::CreateContext();
	ImGuiIO& io = ImGui::GetIO(); (void)io;
	ImGui_ImplGlfw_InitForOpenGL(window.glfw(), true);
	ImGui_ImplOpenGL3_Init("#version 150");
	ImGui::GetStyle().WindowRounding = 0.0f;
}

void BaseApp::NewFrame()
{
	ImGui_ImplOpenGL3_NewFrame();
	ImGui_ImplGlfw_NewFrame();
	ImGui::NewFrame();
}

void BaseApp::DrawFrame()
{
	ImGui::Render();
	ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
}

void BaseApp::Shutdown()
{
	ImGui_ImplOpenGL3_Shutdown();
	ImGui_ImplGlfw_Shutdown();
	ImGui::DestroyContext();
}

//-----------------------------------------------------------------------------

void BaseApp::KeyCallback(GLFWwindow* window, int key, int scancode, int action, int mode) {
	BaseApp* app = static_cast<BaseApp*>(glfwGetWindowUserPointer(window));
	app->onKey(key, scancode, action, mode);
}

void BaseApp::CursorPositionCallback(GLFWwindow* window, double x, double y) {
	BaseApp* app = static_cast<BaseApp*>(glfwGetWindowUserPointer(window));
	app->onCursorPositionInternal(x, y);
	app->onCursorPosition(x, y);
}

void BaseApp::MouseButtonCallback(GLFWwindow* window, int button, int action, int mods) {
	BaseApp* app = static_cast<BaseApp*>(glfwGetWindowUserPointer(window));
	app->onMouseButtonInternal(button, action, mods);
	app->onMouseButton(button, action, mods);
}

void BaseApp::WindowSizeCallback(GLFWwindow* window, int width, int height) {
	BaseApp* app = static_cast<BaseApp*>(glfwGetWindowUserPointer(window));
	app->onResizeInternal(width, height);
	app->onResize(width, height);
}

void BaseApp::ScrollCallback(GLFWwindow* window, double xoffset, double yoffset) {
	BaseApp* app = static_cast<BaseApp*>(glfwGetWindowUserPointer(window));
	app->onScroll(xoffset, yoffset);
}

void BaseApp::setupCallbacks()
{
	if (auto window = m_window.lock()) {
		glfwSetWindowUserPointer(window->glfw(), static_cast<void*>(this));
		glfwSetKeyCallback(window->glfw(), KeyCallback);
		glfwSetCursorPosCallback(window->glfw(), CursorPositionCallback);
		glfwSetMouseButtonCallback(window->glfw(), MouseButtonCallback);
		glfwSetWindowSizeCallback(window->glfw(), WindowSizeCallback);
		glfwSetScrollCallback(window->glfw(), ScrollCallback);
	}
}

