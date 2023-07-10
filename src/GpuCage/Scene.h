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

#include <RuntimeObject.h>
#include <DrawCall.h>
#include <ReflectionAttributes.h>
#include <refl.hpp>
#include <glm/glm.hpp>

#include <vector>
#include <string>

class Scene : public RuntimeObject {
public:
	Scene();
	~Scene();
	Scene(const Scene& scene) = delete;
	Scene& operator=(const Scene&) = delete;

	void render(const Camera& camera) const;

	void loadMesh();

	glm::vec3 handlePosition(int i) const;
	void setHandlePosition(int i, const glm::vec3& position);

	glm::vec2 handleScreenPosition(int i, const Camera& camera) const;
	void setHandleScreenPosition(int i, const glm::vec2& screenPosition, const glm::vec3& initialWorldPosition, const Camera& camera);

	int getHandleNear(const glm::vec2& mouse, const Camera& camera);

	void resetCage();

private:
	void initCageDrawCall();
	void updateCageDrawCall();
	void resetHandleData();

public:
	struct Properties {
		std::string modelFilename = "test.obj";
		float handleSize = 20.0f; // diameter, in pixels
		bool raymarching = false;
	};
	const Properties& properties() const { return m_properties; }
	Properties& properties() { return m_properties; }

private:
	Properties m_properties;

	DrawCall m_meshDrawCall;
	DrawCall m_cageDrawCall;
	DrawCall m_handleDrawCall;
	DrawCall m_raymarchingDrawCall;
	GLuint m_cageSsbo;

	std::vector<GLfloat> m_handleData;
};

#define _ ReflectionAttributes::
REFL_TYPE(Scene::Properties)
REFL_FIELD(modelFilename, _ MaximumSize(1024))
REFL_FIELD(handleSize, _ Range(0, 30))
REFL_FIELD(raymarching)
REFL_END
#undef _
