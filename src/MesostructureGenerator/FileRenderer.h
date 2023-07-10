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
#include <ReflectionAttributes.h>
#include <NonCopyable.h>
#include <Camera.h>

#include <refl.hpp>
#include <glm/glm.hpp>

#include <filesystem>
#include <memory>

class TurntableCamera;
class RenderTarget;
class App;

class FileRenderer : public NonCopyable {
public:
	FileRenderer(App& app);

	// Render current frame
	bool renderStill() const;

	// Render a 360 shot around the object
	bool renderTurntable() const;

	// Render an animation where the camera is driven by the content of cameraAnimation
	bool renderAnimation() const;

	// Render the different PBR maps, to be used as a material
	bool renderPbrMaps() const;

	// Call this whenever the properties change, to sync the camera settings (poor
	// design here, the camera setting should be exposed in another object).
	void updateModel();

public:
	struct Properties {
		std::string prefix = "render/frame";
		glm::ivec2 resolution = { 1920, 1080 };
		int framePerTurn = 360;
		bool overwrite = false;
		Camera::ProjectionType cameraProjectionType = Camera::ProjectionType::Orthographic;
		float cameraFov = 45.0f;
		std::string cameraAnimation = "anim/camera.bin"; // a dump of camera matrices, typically generated from Blender
		bool exportDepth = false;
		bool DEBUG = false;
	};
	const Properties& properties() const { return m_properties; }
	Properties& properties() { return m_properties; }

private:
	struct SavedState {
		GLsizei gbufferWidth;
		GLsizei gbufferHeight;
		bool deferredRenderingExportLinearDepth;
	};

	SavedState beforeRenderToFile() const;
	bool renderToFile(int frame) const;
	bool pbrRenderToFile(int frame) const; // export G-Buffer maps instead of beauty render
	void afterRenderToFile(const SavedState& state) const;
	// This is more or less a copy of App::render
	void render(bool pbrMaps) const;
	std::filesystem::path resolvePath(int frame) const;
	std::filesystem::path resolvePath(const std::string& frameName, const std::string& extension = ".png") const;

	RenderTarget& renderTarget() const;
	RenderTarget& pbrRenderTarget() const;

private:
	Properties m_properties;
	std::unique_ptr<TurntableCamera> m_camera;
	mutable std::unique_ptr<RenderTarget> m_renderTarget;
	mutable std::unique_ptr<RenderTarget> m_pbrRenderTarget;
	App& m_app; // this is a bit HACKY!
};

#define _ ReflectionAttributes::
REFL_TYPE(FileRenderer::Properties)
REFL_FIELD(prefix)
REFL_FIELD(resolution, _ Range(0, 4096))
REFL_FIELD(framePerTurn, _ Range(0, 3600))
REFL_FIELD(overwrite)
REFL_FIELD(cameraProjectionType)
REFL_FIELD(cameraFov, _ Range(0, 180))
REFL_FIELD(cameraAnimation)
REFL_FIELD(exportDepth)
REFL_FIELD(DEBUG)
REFL_END
#undef _
