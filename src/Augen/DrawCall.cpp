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
#include "DrawCall.h"
#include "Camera.h"
#include "Logger.h"

DrawCall::DrawCall(
	const std::string& shaderName,
	GLenum topology,
	bool indexed,
	GLenum elementType
)
	: shader(ShaderPool::GetShader(shaderName))
	, topology(topology)
	, indexed(indexed)
	, elementType(elementType)
{}

DrawCall::~DrawCall() {
	destroy();
}

void DrawCall::destroy() {
	constexpr GLuint invalid = std::numeric_limits<GLuint>::max();

	if (vao != invalid) {
		glDeleteVertexArrays(1, &vao);
		vao = invalid;
	}

	if (vbo != invalid) {
		glDeleteBuffers(1, &vbo);
		vbo = invalid;
	}

	if (ebo != invalid) {
		glDeleteBuffers(1, &ebo);
		ebo = invalid;
	}

	elementCount = 0;
}

void DrawCall::render(const Camera& camera, GLint first, GLsizei size, GLsizei instanceCount) const {
	if (!shader) return;
	shader->use();
	shader->bindUniformBlock("Camera", camera.ubo());

	render(first, size, instanceCount);
}

void DrawCall::render(GLint first, GLsizei size, GLsizei instanceCount) const {
	if (elementCount == 0) return;
	if (size == 0) size = elementCount;

	if (restart) {
		glEnable(GL_PRIMITIVE_RESTART);
		glPrimitiveRestartIndex(restartIndex);
	}

	glBindVertexArray(vao);
	if (indexed) {
		size_t sizeOfElement = 0;
		switch (elementType) {
		case GL_UNSIGNED_INT:
			sizeOfElement = sizeof(GLuint);
			break;
		case GL_UNSIGNED_BYTE:
			sizeOfElement = sizeof(GLubyte);
			break;
		default:
			assert(false);
		}
		
		glDrawElementsInstanced(
			topology,
			size,
			elementType,
			reinterpret_cast<void*>(first * sizeOfElement),
			instanceCount
		);
	}
	else {
		glDrawArraysInstanced(topology, first, size, instanceCount);
	}
	glBindVertexArray(0);

	if (restart) {
		glDisable(GL_PRIMITIVE_RESTART);
	}
}

void DrawCall::loadFromVectors(const std::vector<GLfloat> & pointData, bool dynamic) {
	if (pointData.empty()) return;
	indexed = false;
	restart = false;

	elementCount = static_cast<GLsizei>(pointData.size() / 3);

	glCreateBuffers(1, &vbo);
	glNamedBufferData(vbo, pointData.size() * sizeof(GLfloat), pointData.data(), dynamic ? GL_DYNAMIC_DRAW : GL_STATIC_DRAW);

	glCreateVertexArrays(1, &vao);

	glVertexArrayAttribFormat(vao, 0, 3, GL_FLOAT, GL_FALSE, 0);
	glVertexArrayAttribBinding(vao, 0, 0);
	glVertexArrayVertexBuffer(vao, 0, vbo, 0, 3 * sizeof(GLfloat));
	glEnableVertexArrayAttrib(vao, 0);
}

void DrawCall::loadFromVectors(const std::vector<GLfloat>& pointData, const std::vector<GLuint>& elementData, bool dynamic) {
	if (pointData.empty()) return;
	loadFromVectors(pointData, dynamic);

	indexed = true;
	elementCount = static_cast<GLsizei>(elementData.size());

	glCreateBuffers(1, &ebo);
	glNamedBufferData(ebo, elementData.size() * sizeof(GLuint), elementData.data(), dynamic ? GL_DYNAMIC_DRAW : GL_STATIC_DRAW);

	glVertexArrayElementBuffer(vao, ebo);
}

void DrawCall::loadFromVectorsWithNormal(const std::vector<GLfloat>& pointData, const std::vector<GLfloat>& normalData, bool dynamic) {
	if (pointData.empty()) return;
	(void)dynamic; // it's always dynamic actually
	indexed = false;
	restart = false;

	elementCount = static_cast<GLsizei>(pointData.size() / 3);
	assert(normalData.size() == pointData.size());

	glCreateBuffers(1, &vbo);
	glNamedBufferStorage(vbo, pointData.size() * sizeof(GLfloat) * 2, nullptr, GL_DYNAMIC_STORAGE_BIT);
	
	glCreateVertexArrays(1, &vao);

	GLintptr offset = 0;

	glNamedBufferSubData(vbo, offset, pointData.size() * sizeof(GLfloat), pointData.data());
	glVertexArrayAttribFormat(vao, 0, 3, GL_FLOAT, GL_FALSE, 0);
	glVertexArrayAttribBinding(vao, 0, 0);
	glVertexArrayVertexBuffer(vao, 0, vbo, offset, 3 * sizeof(GLfloat));
	glEnableVertexArrayAttrib(vao, 0);
	offset += pointData.size() * sizeof(GLfloat);

	glNamedBufferSubData(vbo, offset, normalData.size() * sizeof(GLfloat), normalData.data());
	glVertexArrayAttribFormat(vao, 1, 3, GL_FLOAT, GL_FALSE, 0);
	glVertexArrayAttribBinding(vao, 1, 1);
	glVertexArrayVertexBuffer(vao, 1, vbo, offset, 3 * sizeof(GLfloat));
	glEnableVertexArrayAttrib(vao, 1);
}

void DrawCall::loadFromVectorsWithNormal(const std::vector<GLfloat>& pointData, const std::vector<GLfloat>& normalData, const std::vector<GLuint>& elementData, bool dynamic) {
	if (pointData.empty()) return;
	loadFromVectorsWithNormal(pointData, normalData, dynamic);

	indexed = true;
	elementCount = static_cast<GLsizei>(elementData.size());

	glCreateBuffers(1, &ebo);
	glNamedBufferData(ebo, elementData.size() * sizeof(GLuint), elementData.data(), dynamic ? GL_DYNAMIC_DRAW : GL_STATIC_DRAW);

	glVertexArrayElementBuffer(vao, ebo);
}

void DrawCall::loadFromVectorsWithNormalAndUVs(const std::vector<GLfloat>& pointData, const std::vector<GLfloat>& normalData, const std::vector<GLfloat>& uvData, bool dynamic) {
	if (pointData.empty()) return;
	(void)dynamic; // it's always dynamic actually
	indexed = false;
	restart = false;

	elementCount = static_cast<GLsizei>(pointData.size() / 3);
	assert(normalData.size() == pointData.size());
	GLint uvCount = 0;
	if (uvData.size() == pointData.size()) {
		uvCount = 3;
	}
	else {
		uvCount = 2;
		assert(uvData.size() / 2 == pointData.size() / 3);
	}

	glCreateBuffers(1, &vbo);
	glNamedBufferStorage(vbo, (pointData.size() + normalData.size() + uvData.size()) * sizeof(GLfloat), nullptr, GL_DYNAMIC_STORAGE_BIT);

	glCreateVertexArrays(1, &vao);

	GLintptr offset = 0;
	GLuint attributeIndex = 0;

	glNamedBufferSubData(vbo, offset, pointData.size() * sizeof(GLfloat), pointData.data());
	glVertexArrayAttribFormat(vao, attributeIndex, 3, GL_FLOAT, GL_FALSE, 0);
	glVertexArrayAttribBinding(vao, attributeIndex, attributeIndex);
	glVertexArrayVertexBuffer(vao, attributeIndex, vbo, offset, 3 * sizeof(GLfloat));
	glEnableVertexArrayAttrib(vao, attributeIndex);
	offset += pointData.size() * sizeof(GLfloat);
	++attributeIndex;

	glNamedBufferSubData(vbo, offset, normalData.size() * sizeof(GLfloat), normalData.data());
	glVertexArrayAttribFormat(vao, attributeIndex, 3, GL_FLOAT, GL_FALSE, 0);
	glVertexArrayAttribBinding(vao, attributeIndex, attributeIndex);
	glVertexArrayVertexBuffer(vao, attributeIndex, vbo, offset, 3 * sizeof(GLfloat));
	glEnableVertexArrayAttrib(vao, attributeIndex);
	offset += normalData.size() * sizeof(GLfloat);
	++attributeIndex;

	glNamedBufferSubData(vbo, offset, uvData.size() * sizeof(GLfloat), uvData.data());
	glVertexArrayAttribFormat(vao, attributeIndex, uvCount, GL_FLOAT, GL_FALSE, 0);
	glVertexArrayAttribBinding(vao, attributeIndex, attributeIndex);
	glVertexArrayVertexBuffer(vao, attributeIndex, vbo, offset, uvCount * sizeof(GLfloat));
	glEnableVertexArrayAttrib(vao, attributeIndex);
}

void DrawCall::loadFromVectorsWithNormalAndUVs(const std::vector<GLfloat>& pointData, const std::vector<GLfloat>& normalData, const std::vector<GLfloat>& uvData, const std::vector<GLuint>& elementData, bool dynamic) {
	if (pointData.empty()) return;
	loadFromVectorsWithNormalAndUVs(pointData, normalData, uvData, dynamic);

	indexed = true;
	elementCount = static_cast<GLsizei>(elementData.size());

	glCreateBuffers(1, &ebo);
	glNamedBufferData(ebo, elementData.size() * sizeof(GLuint), elementData.data(), dynamic ? GL_DYNAMIC_DRAW : GL_STATIC_DRAW);

	glVertexArrayElementBuffer(vao, ebo);
}

void DrawCall::loadQuad() {
	static const std::vector<GLfloat> pointData = {
		// Position
		+0.5f, -0.5f, 0,
		+0.5f, +0.5f, 0,
		-0.5f, -0.5f, 0,
		-0.5f, +0.5f, 0,

		// UV
		1, 0,
		1, 1,
		0, 0,
		0, 1,
	};

	topology = GL_TRIANGLE_STRIP;
	indexed = false;
	restart = false;

	elementCount = 4;

	glCreateBuffers(1, &vbo);
	glNamedBufferData(vbo, pointData.size() * sizeof(GLfloat), pointData.data(), GL_STATIC_DRAW);

	glCreateVertexArrays(1, &vao);

	GLintptr offset = 0;

	glVertexArrayAttribFormat(vao, 0, 3, GL_FLOAT, GL_FALSE, 0);
	glVertexArrayAttribBinding(vao, 0, 0);
	glVertexArrayVertexBuffer(vao, 0, vbo, offset, 3 * sizeof(GLfloat));
	glEnableVertexArrayAttrib(vao, 0);
	offset += 3 * sizeof(GLfloat) * elementCount;

	glVertexArrayAttribFormat(vao, 1, 2, GL_FLOAT, GL_FALSE, 0);
	glVertexArrayAttribBinding(vao, 1, 1);
	glVertexArrayVertexBuffer(vao, 1, vbo, offset, 2 * sizeof(GLfloat));
	glEnableVertexArrayAttrib(vao, 1);
}

bool DrawCall::checkIntegrity(GLuint vertexCount) const {
	glMemoryBarrier(GL_SHADER_STORAGE_BARRIER_BIT);

	if (indexed) {
		std::vector<GLuint> elementData(elementCount);
		glGetNamedBufferSubData(ebo, 0, sizeof(GLuint) * elementCount, elementData.data());
		for (int i = 0; i < elementCount; ++i) {
			bool valid = elementData[i] < vertexCount || (restart && elementData[i] == restartIndex);
			if (!valid) {
				ERR_LOG << "Invalid element data at index #" << i << ": " << elementData[i];
				return false;
			}
		}
	}

	return true;
}
