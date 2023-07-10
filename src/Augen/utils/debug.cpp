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


#include "utils/debug.h"

#include <iostream>

using namespace std;

void enableGlDebug() {
#ifdef GLAD_DEBUG
	glad_set_pre_callback(openglPreFunction);
	// don't use the callback for glClear
	glad_debug_glClear = glad_glClear;
#endif // GLAD_DEBUG
	// Enable opengl debug output when building in debug mode
	glEnable(GL_DEBUG_OUTPUT_SYNCHRONOUS);
	glDebugMessageCallback(openglCallbackFunction, nullptr);
	GLuint unusedIds = 0;
	glDebugMessageControl(GL_DONT_CARE, GL_DONT_CARE, GL_DONT_CARE, 0, &unusedIds, true);
}

void APIENTRY openglCallbackFunction(
	GLenum source,
	GLenum type,
	GLuint id,
	GLenum severity,
	GLsizei /*length*/,
	const GLchar* message,
	const void* /*userParam*/) {

	if (id == 131185 || id == 131154 || id == 131186 || id == 7) {
		return;
	}

	if (severity == GL_DEBUG_SEVERITY_LOW && type == GL_DEBUG_TYPE_PERFORMANCE) {
		return;
	}

	cout << "---------------------opengl-callback-start------------" << endl;
	cout << "message: " << message << endl;
	cout << "type: ";
	switch (type) {
	case GL_DEBUG_TYPE_ERROR:
		cout << "ERROR";
		break;
	case GL_DEBUG_TYPE_DEPRECATED_BEHAVIOR:
		cout << "DEPRECATED_BEHAVIOR";
		break;
	case GL_DEBUG_TYPE_UNDEFINED_BEHAVIOR:
		cout << "UNDEFINED_BEHAVIOR";
		break;
	case GL_DEBUG_TYPE_PORTABILITY:
		cout << "PORTABILITY";
		break;
	case GL_DEBUG_TYPE_PERFORMANCE:
		cout << "PERFORMANCE";
		break;
	case GL_DEBUG_TYPE_OTHER:
		cout << "OTHER";
		break;
	}
	cout << endl;
	cout << "source: ";
	switch (source) {
	case GL_DEBUG_SOURCE_API:
		cout << "API";
		break;
	case GL_DEBUG_SOURCE_WINDOW_SYSTEM:
		cout << "WINDOW_SYSTEM";
		break;
	case GL_DEBUG_SOURCE_SHADER_COMPILER:
		cout << "SHADER_COMPILER";
		break;
	case GL_DEBUG_SOURCE_THIRD_PARTY:
		cout << "THIRD_PARTY";
		break;
	case GL_DEBUG_SOURCE_APPLICATION:
		cout << "APPLICATION";
		break;
	case GL_DEBUG_SOURCE_OTHER:
		cout << "OTHER";
		break;
	}
	cout << endl;

	cout << "id: " << id << endl;
	cout << "severity: ";
	switch (severity) {
	case GL_DEBUG_SEVERITY_LOW:
		cout << "LOW";
		break;
	case GL_DEBUG_SEVERITY_MEDIUM:
		cout << "MEDIUM";
		break;
	case GL_DEBUG_SEVERITY_HIGH:
		cout << "HIGH";
		break;
	}
	cout << endl;
	if (type != GL_DEBUG_TYPE_OTHER && type != GL_DEBUG_TYPE_PERFORMANCE && type != GL_DEBUG_TYPE_DEPRECATED_BEHAVIOR && id != 131218) {
		cout << "break point..." << endl;
		exit(1);
	}
	cout << "---------------------opengl-callback-end--------------" << endl;
}

#ifdef GLAD_DEBUG
void openglPreFunction(const char *name, void *funcptr, int len_args, ...) {
    cout << "GLAD trace: " << name << " (" << len_args << " arguments)" << endl;
}
#endif
