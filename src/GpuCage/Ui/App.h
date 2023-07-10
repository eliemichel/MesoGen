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

#include <Ui/BaseApp.h>
#include <Ray.h>

#include <Camera.h>
#include <RenderTarget.h>
#include <memory>

class Scene;

class App : public BaseApp
{
public:
	App(std::shared_ptr<Window> window);

	void update() override;
	void render() const override;

protected:
	void setupDialogs() override;

	void onResize(int width, int height) override;
	void onMouseButton(int button, int action, int mods) override;
	void onKey(int key, int scancode, int action, int mods) override;
	void onCursorPosition(double x, double y) override;
	void onScroll(double xoffset, double yoffset) override;

private:
	std::shared_ptr<Scene> m_scene;
	std::unique_ptr<Camera> m_camera;
	std::unique_ptr<RenderTarget> m_renderTarget;
	glm::vec2 m_mouse;
	int m_grabbedHandle = -1;
	glm::vec2 m_grabOffset;
	glm::vec3 m_handleStart;
};
