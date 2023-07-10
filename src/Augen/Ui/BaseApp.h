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

class Window;
class Dialog;
struct GLFWwindow;

#include <string>
#include <vector>
#include <memory>

/**
 * Abstract class, define application specific behaviors in App
 */
class BaseApp {
public:
	BaseApp(std::shared_ptr<Window> window);
	~BaseApp();
	BaseApp(BaseApp&) = delete;
	void operator=(BaseApp&) = delete;

	virtual void update() { updateGui(); }
	virtual void render() const { renderGui(); }

	float width() const { return m_windowWidth; }
	float height() const { return m_windowHeight; }

	// Call these resp. before and after loading the scene
	void beforeLoading();
	void afterLoading();

	bool showPanel() const { return m_showPanel; }
	void setShowPanel(bool showPanel) { m_showPanel = showPanel; }

public:
	// For tools that don't use the main Gui
	static void Init(const Window& window);
	static void NewFrame();
	static void DrawFrame();
	static void Shutdown();

protected:
	// event callbacks
	virtual void setupDialogs() {}

	virtual void onResize(int width, int height);
	virtual void onMouseButton(int button, int action, int mods);
	virtual void onKey(int key, int scancode, int action, int mods);
	virtual void onCursorPosition(double x, double y);
	virtual void onScroll(double xoffset, double yoffset);

protected:
	std::weak_ptr<Window> getWindow() const { return m_window; }
	bool guiHasFocus() const { return m_guiFocus; }
	void clearDialogs() { m_dialogGroups.clear(); }
	void renderGui() const;
	void resize();

private:
	void setupCallbacks();
	void updateGui();

	void onResizeInternal(int width, int height);
	void onMouseButtonInternal(int, int action, int);
	void onCursorPositionInternal(double x, double);

private:
	// Raw static callbacks that routes to non statick equivalent
	static void KeyCallback(GLFWwindow* window, int key, int scancode, int action, int mode);
	static void CursorPositionCallback(GLFWwindow* window, double x, double y);
	static void MouseButtonCallback(GLFWwindow* window, int button, int action, int mods);
	static void WindowSizeCallback(GLFWwindow* window, int width, int height);
	static void ScrollCallback(GLFWwindow* window, double xoffset, double yoffset);

private:
	struct DialogGroup {
		std::string title;
		std::vector<std::shared_ptr<Dialog>> dialogs;
		bool enabled = false;
	};

protected:
	template<typename DialogType, typename ControllerType>
	std::shared_ptr<DialogType> addDialogGroup(std::string title, ControllerType controller)
	{
		DialogGroup group;
		group.title = title;
		auto dialog = std::make_shared<DialogType>();
		dialog->setController(controller);
		group.dialogs.push_back(dialog);
		m_dialogGroups.push_back(group);
		return dialog;
	}
	std::vector<DialogGroup> & dialogGroups() { return m_dialogGroups; }

private:
	std::weak_ptr<Window> m_window;
	std::vector<DialogGroup> m_dialogGroups;

	float m_startTime;

	float m_windowWidth;
	float m_windowHeight;
	bool m_showPanel = true;
	float m_panelWidth = 300.f;

	bool m_guiFocus;
	bool m_isMouseMoveStarted;
	int m_isControlPressed = 0;
};
