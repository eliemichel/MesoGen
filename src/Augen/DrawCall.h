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

#include "ShaderProgram.h"
#include "ShaderPool.h"

#include <limits>
#include <memory>

class Camera;

struct DrawCall {
	GLsizei elementCount = 0;
	GLuint vao = std::numeric_limits<GLuint>::max();
	GLuint vbo = std::numeric_limits<GLuint>::max();
	GLuint ebo = std::numeric_limits<GLuint>::max();
	std::shared_ptr<ShaderProgram> shader;
	GLenum topology = GL_TRIANGLES;
	bool indexed = true;
	GLenum elementType = GL_UNSIGNED_INT;
	bool restart = false;
	GLuint restartIndex = 0;

	DrawCall(
		const std::string& shaderName,
		GLenum topology = GL_TRIANGLES,
		bool indexed = true,
		GLenum elementType = GL_UNSIGNED_INT
	);
	~DrawCall();
	DrawCall(const DrawCall&) = delete;
	DrawCall& operator=(const DrawCall&) = delete;

	/**
	 * Destroy OpenGL objects if the draw call is valid (non null element count)
	 */
	void destroy();

	void render(const Camera& camera, GLint first = 0, GLsizei size = 0, GLsizei instanceCount = 1) const;
	void render(GLint first = 0, GLsizei size = 0, GLsizei instanceCount = 1) const;

	// Functions that populate the draw call
	// (caller must then manually call destroy() to clean up gl objects)
	void loadFromVectors(const std::vector<GLfloat>& pointData, bool dynamic = false);
	void loadFromVectors(const std::vector<GLfloat>& pointData, const std::vector<GLuint> & elementData, bool dynamic = false);
	void loadFromVectorsWithNormal(const std::vector<GLfloat>& pointData, const std::vector<GLfloat>& normalData, bool dynamic = false);
	void loadFromVectorsWithNormal(const std::vector<GLfloat>& pointData, const std::vector<GLfloat>& normalData, const std::vector<GLuint>& elementData, bool dynamic = false);
	void loadFromVectorsWithNormalAndUVs(const std::vector<GLfloat>& pointData, const std::vector<GLfloat>& normalData, const std::vector<GLfloat>& uvData, bool dynamic = false);
	void loadFromVectorsWithNormalAndUVs(const std::vector<GLfloat>& pointData, const std::vector<GLfloat>& normalData, const std::vector<GLfloat>& uvData, const std::vector<GLuint>& elementData, bool dynamic = false);
	void loadQuad();

	/**
	 * Check that the element buffer is consistent with the vbo size
	 */
	bool checkIntegrity(GLuint vertexCount) const;
};
