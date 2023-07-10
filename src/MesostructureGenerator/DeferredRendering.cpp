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
#include "DeferredRendering.h"

#include <RenderTarget.h>
#include <ShaderProgram.h>
#include <DrawCall.h>
#include <Texture.h>
#include <Camera.h>
#include <utils/reflutils.h>
#include <GLFW/glfw3.h>

#include <glm/glm.hpp>

DeferredRendering::DeferredRendering() {
	m_drawCall = std::make_unique<DrawCall>("deferred-rendering", GL_TRIANGLES, false /* indexed */);
	m_drawCall->loadFromVectors({
		0.0f, 0.0f, 0.0f,
		2.0f, 0.0f, 0.0f,
		0.0f, 2.0f, 0.0f,
	});
	m_pbrMapsDrawCall = std::make_unique<DrawCall>("deferred-rendering-pbr-maps", GL_TRIANGLES, false /* indexed */);
	m_pbrMapsDrawCall->loadFromVectors({
		0.0f, 0.0f, 0.0f,
		2.0f, 0.0f, 0.0f,
		0.0f, 2.0f, 0.0f,
	});
}

DeferredRendering::~DeferredRendering() {
	m_drawCall->destroy();
	m_pbrMapsDrawCall->destroy();
}

void DeferredRendering::render(const RenderTarget& gbuffer, const Camera& camera, bool pbrMaps) const {
	auto& dc = pbrMaps ? *m_pbrMapsDrawCall : *m_drawCall;
	auto& shader = *dc.shader;

	shader.use();
	autoSetUniforms(shader, properties());

	shader.setUniform("uResolution", glm::uvec2(gbuffer.width(), gbuffer.height()));
	shader.setUniform("uTime", (float)glfwGetTime());
	
	gbuffer.colorTexture(0).bind(0);
	shader.setUniform("uGbuffer0", 0);

	gbuffer.colorTexture(1).bind(1);
	shader.setUniform("uGbuffer1", 1);

	gbuffer.colorTexture(2).bind(2);
	shader.setUniform("uGbuffer2", 2);

	gbuffer.depthTexture().bind(3);
	shader.setUniform("uZbuffer", 3);

	dc.render(camera);
}
