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
#include "QuadObject.h"
#include "Model.h"
#include "TileVariantList.h"

#include <RuntimeObject.h>
#include <ReflectionAttributes.h>
#include <NonCopyable.h>
#include <libwfc.h>

#include <refl.hpp>
#include <glm/glm.hpp>

#include <memory>
#include <string>
#include <limits>
#include <utility>

class TilesetController;
namespace bmesh {
struct BMesh;
}
class MacrosurfaceData;
class MesostructureData;
class MacrosurfaceRenderer;
class MesostructureRenderer;
class TurntableCamera;

/**
 * The graphics object that is responsible for turning around the macrosurface.
 * NB: This class is poorly named I know.
 */
class OutputObject : public RuntimeObject, public NonCopyable {
public:
	enum class GeneratorAlgorithm {
		ModelSynthesis,
		WaveFunctionCollapse,
	};

public:
	OutputObject(
		std::shared_ptr<MacrosurfaceData> macrosurfaceData,
		std::shared_ptr<MesostructureData> mesostructureData
	);
	
	void render(const Camera& camera) const override;

	void update() override;

	bool onMousePress(bool forceHover = false);
	void onMouseRelease();
	void onCursorPosition(float x, float y, const Camera& camera);

	void setController(std::shared_ptr<TilesetController> controller);

	TurntableCamera& camera() { return *m_camera; }

private:
	/**
	 * Return a matrix representing the transform applied to the viewed
	 * macro/mesostructure object.
	 */
	glm::mat4 contentModelMatrix() const;

public:
	struct Properties {
		float displaySize = 1.0f;
		float displayMargin = 0.0f;
	};
	const Properties& properties() const { return m_properties; }
	Properties& properties() { return m_properties; }

private:
	Properties m_properties;
	std::weak_ptr<TilesetController> m_controller;
	
	// TODO: replace with weak pointers
	std::shared_ptr<MacrosurfaceData> m_macrosurfaceData;
	std::shared_ptr<MesostructureData> m_mesostructureData;

	// This camera is not used for rendering, it is only used for orbiting around the object
	std::shared_ptr<TurntableCamera> m_camera;
	bool m_hover = false;

	mutable QuadObject m_quad; // used for rendering
};

#define _ ReflectionAttributes::
REFL_TYPE(OutputObject::Properties)
REFL_FIELD(displaySize, _ Range(0, 2))
REFL_FIELD(displayMargin, _ Range(0, 1))
REFL_END
#undef _
