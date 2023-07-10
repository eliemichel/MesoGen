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

#include <NonCopyable.h>
#include <ReflectionAttributes.h>

#include <refl.hpp>

#include <memory>

struct DrawCall;
class RenderTarget;
class Camera;

class DeferredRendering : public NonCopyable {
public:
	DeferredRendering();
	~DeferredRendering();
	// If pbrMaps is true, export to multiple attachments meant to be saved as PBR
	// material maps instead of drawing a single beauty pass (this option ids used
	// by the PBR map export of FileRenderer).
	void render(const RenderTarget& gbuffer, const Camera& camera, bool pbrMaps = false) const;

public:
	struct Properties {
		bool enabled = true;
		int msaaFactor = 2; // must not change at runtime, and must be synced with lineScale in TilesetRenderer::renderTile and in MesostructureRenderer::render
		bool exportLinearDepth = false; // only used for exporting, in the depth buffer of the output framebuffer
	};
	const Properties& properties() const { return m_properties; }
	Properties& properties() { return m_properties; }

private:
	Properties m_properties;
	std::shared_ptr<DrawCall> m_drawCall;
	std::shared_ptr<DrawCall> m_pbrMapsDrawCall;
};

#define _ ReflectionAttributes::
REFL_TYPE(DeferredRendering::Properties)
REFL_FIELD(enabled)
REFL_FIELD(msaaFactor, _ Range(0, 4))
REFL_FIELD(exportLinearDepth)
REFL_END
#undef _
