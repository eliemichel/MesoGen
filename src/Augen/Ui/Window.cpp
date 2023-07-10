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

#include "Window.h"
#include "utils/debug.h"
#include "Logger.h"

#include <GLFW/glfw3.h>

#include <iostream>


Window::Window(int width, int height, const char *title)
	: m_window(nullptr)
{
	// Initialize GLFW, the library responsible for window management
	if (!glfwInit()) {
		ERR_LOG << "Failed to init GLFW";
		return;
	}

	// Before creating the window, set some option flags
	glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
	glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 5);
	glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
	glfwWindowHint(GLFW_RESIZABLE, GL_TRUE);
	glfwWindowHint(GLFW_SAMPLES, 4);
#ifndef NDEBUG
	// Enable opengl debug output when building in debug mode
	glfwWindowHint(GLFW_OPENGL_DEBUG_CONTEXT, true);
#endif // !NDEBUG

	// Create the window
	m_window = glfwCreateWindow(width, height, title, nullptr, nullptr);
	if (!m_window) {
		ERR_LOG << "Failed to open window";
		glfwTerminate();
		return;
	}

	// Load the OpenGL context in the GLFW window using GLAD OpenGL wrangler
	glfwMakeContextCurrent(m_window);
	if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress)) {
		ERR_LOG << "Failed to initialize OpenGL context";
		glfwTerminate();
		return;
	}

#ifndef NDEBUG
	enableGlDebug();
#endif // !NDEBUG

	LOG << "Running over OpenGL " << GLVersion.major << "." << GLVersion.minor;
}

Window::~Window()
{
	glfwDestroyWindow(m_window);
	glfwTerminate();
}

GLFWwindow * Window::glfw() const
{
	return m_window;
}

bool Window::isValid() const
{
	return m_window != nullptr;
}

void Window::pollEvents() const
{
	glfwPollEvents();
}

void Window::swapBuffers() const
{
	glfwSwapBuffers(m_window);
}

bool Window::shouldClose() const
{
	return glfwWindowShouldClose(m_window);
}
