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

#include "RuntimeObject.h"
#include "QuadObject.h"

#include <NonCopyable.h>
#include <ReflectionAttributes.h>
#include <refl.hpp>
#include <glm/glm.hpp>

#include <vector>
#include <memory>

class TilingSolverData;
class TilingSolverRenderData;
class MesostructureData;

/**
 * Renders debug data about the tiling solver
 */
class TilingSolverRenderer : public RuntimeObject, public NonCopyable {
public:
	struct PositionedText {
		glm::vec2 position;
		std::string text;
	};

public:
	TilingSolverRenderer(std::weak_ptr<TilingSolverData> solverData, std::weak_ptr<MesostructureData> mesostructureData);

	void render(const Camera& camera) const override;

	void update() override;

	const std::vector<PositionedText>& texts() const { return m_texts; }

public:
	struct Properties {
		bool enabled = false;
		bool drawFrame = true;
		bool drawStates = true;
	};
	const Properties& properties() const { return m_properties; }
	Properties& properties() { return m_properties; }

public:
	Properties m_properties;
	std::weak_ptr<TilingSolverData> m_solverData;
	std::weak_ptr<MesostructureData> m_mesostructureData;

	/**
	 * Cached data used only for rendering
	 */
	std::shared_ptr<TilingSolverRenderData> m_renderData;

	/**
	 * The render() method actually fills data here instead of drawing, and
	 * ImGui is used then in TilingSOlverRendererDialog.
	 */
	mutable std::vector<PositionedText> m_texts;

	mutable QuadObject m_quad; // used for rendering
};

#define _ ReflectionAttributes::
REFL_TYPE(TilingSolverRenderer::Properties)
REFL_FIELD(enabled)
REFL_FIELD(drawFrame)
REFL_FIELD(drawStates)
REFL_END
#undef _
