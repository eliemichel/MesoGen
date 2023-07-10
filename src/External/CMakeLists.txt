# Copyright (c) 2017 - 2020 -- Élie Michel <elie.michel@exppad.com>
# This is private research work, DO NOT SHARE without the explicit
# consent of the authors.

###############################################################################
# Submodules
###############################################################################

# Glad (v0.1.27), providing headers for OpenGL procedures
# Here it is configured to link to OpenGL 4.5 Core profile, but you can change
# the two following lines to get another one. If you do so, you must also
# change the window creation flags.
set(GLAD_PROFILE "core" CACHE STRING "" FORCE)
set(GLAD_API "gl=4.5" CACHE STRING "" FORCE)
set(GLAD_EXTENSIONS "GL_NV_mesh_shader" CACHE STRING "" FORCE)
add_subdirectory(glad)
set_property(TARGET glad PROPERTY FOLDER "External")
set_property(TARGET glad-generate-files PROPERTY FOLDER "External")

add_subdirectory(modernglad)
set_property(TARGET modernglad PROPERTY FOLDER "External")
set_property(TARGET modernglad-generate-files PROPERTY FOLDER "External")

# GLFW, for openning the window and getting mouse/keyboard events
set(GLFW_BUILD_DOCS OFF CACHE BOOL "" FORCE)
set(GLFW_BUILD_TESTS OFF CACHE BOOL "" FORCE)
set(GLFW_BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)
set(GLFW_INSTALL OFF CACHE BOOL "" FORCE)
add_subdirectory(glfw)
set_property(TARGET glfw PROPERTY FOLDER "External")

# Dear ImGui
add_library(imgui STATIC 
	imgui/imgui.h
	imgui/imgui.cpp
	imgui/imgui_widgets.cpp
	imgui/imgui_demo.cpp
	imgui/imgui_draw.cpp
	imgui/imgui_tables.cpp
	imgui/backends/imgui_impl_glfw.h
	imgui/backends/imgui_impl_glfw.cpp
	imgui/backends/imgui_impl_opengl3.h
	imgui/backends/imgui_impl_opengl3.cpp
	imgui/misc/cpp/imgui_stdlib.cpp
)
target_include_directories(imgui PUBLIC imgui imgui/backends)
add_definitions(-DIMGUI_IMPL_OPENGL_LOADER_GLAD)
target_link_libraries(imgui PUBLIC glad glfw)
set_property(TARGET imgui PROPERTY FOLDER "External")

###############################################################################
# Repackaged third party libraries
###############################################################################

add_subdirectory(refl-cpp)

# glm
add_subdirectory(glm)
# Deactivate the warning "C4201: nonstandard extension used: nameless struct/union" triggered by glm
target_compile_options(glm INTERFACE /wd4201)

# Merell's model synthesis
if (USE_MODEL_SYNTHESIS)
	add_subdirectory(model-synthesis)
	set_property(TARGET ModelSynthesisLibrary PROPERTY FOLDER "External")
	set_property(TARGET ModelSynthesisDemo PROPERTY FOLDER "External")
endif (USE_MODEL_SYNTHESIS)

# RapidJSON
add_library(RapidJSON INTERFACE)
target_include_directories(RapidJSON INTERFACE rapidjson/include)

# TinyObjLoader
add_subdirectory(tinyobjloader)
set_property(TARGET tinyobjloader PROPERTY FOLDER "External")

# Eigen
add_library(Eigen INTERFACE)
target_include_directories(Eigen INTERFACE eigen-3.3.9)

# Clipper, for boolean vector graphics
set(BUILD_SHARED_LIBS OFF CACHE BOOL "" FORCE)
add_subdirectory(Clipper)
set_property(TARGET polyclipping PROPERTY FOLDER "External")

add_subdirectory(argparse)
add_subdirectory(stb)
