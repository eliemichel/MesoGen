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
#include "QuadObject.h"
#include <Texture.h>

#include <utils/reflutils.h>

#include <glm/gtx/transform.hpp>

QuadObject::QuadObject(float size)
	: m_drawCall("mesh", GL_TRIANGLE_STRIP, false /* indexed */)
{
	m_drawCall.loadQuad();
	setSize(size);
}

QuadObject::~QuadObject() {
	m_drawCall.destroy();
}

void QuadObject::render(const Camera &camera) const {
	const auto& shader = *m_drawCall.shader;
	autoSetUniforms(shader, properties());

	if (auto texture = m_texture.lock()) {
		shader.setUniform("uHasTexture", true);
		glBindTextureUnit(0, texture->raw());
		shader.setUniform("uTexture", 0);
	}
	else {
		shader.setUniform("uHasTexture", false);
	}

	m_drawCall.render(camera);
}

void QuadObject::setPosition(const glm::vec3& position) {
	properties().modelMatrix[3] = glm::vec4(position, 1.0f);
}

void QuadObject::setSize(const glm::vec3& size) {
	for (int k = 0; k < 3; ++k) {
		properties().modelMatrix[k][k] = size[k];
	}
}

void QuadObject::setTexture(std::weak_ptr<Texture> texture) {
	m_texture = texture;
}

void QuadObject::unsetTexture() {
	m_texture.reset();
}
