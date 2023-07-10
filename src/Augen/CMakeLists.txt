# This file is part of MesoGen, the reference implementation of:
#
#   Michel, Élie and Boubekeur, Tamy (2023).
#   MesoGen: Designing Procedural On-Surface Stranded Mesostructures,
#   ACM Transaction on Graphics (SIGGRAPH '23 Conference Proceedings),
#   https://doi.org/10.1145/3588432.3591496
#
# Copyright (c) 2020 - 2023 -- Télécom Paris (Élie Michel <emichel@adobe.com>)
# 
# Permission is hereby granted, free of charge, to any person obtaining a copy
# of this software and associated documentation files (the "Software"), to
# deal in the Software without restriction, including without limitation the
# rights to use, copy, modify, merge, publish, distribute, sublicense, and/or
# sell copies of the Software, and to permit persons to whom the Software is
# furnished to do so, subject to the following conditions:
# 
# The above copyright notice and this permission notice shall be included in
# all copies or substantial portions of the Software.
#
# The Software is provided "as is", without warranty of any kind, express or
# implied, including but not limited to the warranties of merchantability,
# fitness for a particular purpose and non-infringement. In no event shall the
# authors or copyright holders be liable for any claim, damages or other
# liability, whether in an action of contract, tort or otherwise, arising
# from, out of or in connection with the software or the use or other dealings
# in the Software.

set(SRC
	utils/debug.h
	utils/debug.cpp
	utils/fileutils.h
	utils/fileutils.cpp
	utils/guiutils.h
	utils/guiutils.cpp
	utils/reflutils.h
	utils/reflutils.cpp
	utils/strutils.h
	utils/strutils.cpp
	utils/streamutils.h
	
	Ui/BaseApp.h
	Ui/BaseApp.cpp
	Ui/Dialog.h
	Ui/Window.h
	Ui/Window.cpp

	Camera.h
	Camera.cpp
	ColorLayerInfo.h
	DrawCall.h
	DrawCall.cpp
	Framebuffer.h
	Framebuffer.cpp
	GlobalTimer.h
	GlobalTimer.cpp
	GroupObject.h
	GroupObject.cpp
	Logger.h
	Logger.cpp
	NonCopyable.h
	Ray.h
	Ray.cpp
	ReflectionAttributes.h
	RenderTarget.h
	RenderTarget.cpp
	RuntimeObject.h
	ScopedFramebufferOverride.h
	Shader.h
	Shader.cpp
	ShaderPool.h
	ShaderPool.cpp
	ShaderPreprocessor.h
	ShaderPreprocessor.cpp
	ShaderProgram.h
	ShaderProgram.cpp
	Texture.h
	Texture.cpp
	TinyTimer.h
	TurntableCamera.h
	TurntableCamera.cpp
)

set(LIBS
	modernglad
	refl-cpp
	glfw
	glm
	imgui
)

if(CMAKE_COMPILER_IS_GNUCC)
	list(APPEND LIBS stdc++fs)
endif(CMAKE_COMPILER_IS_GNUCC)

add_library(Augen STATIC ${SRC})
target_include_directories(Augen PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
target_link_libraries(Augen LINK_PUBLIC ${LIBS})

target_treat_warnings_as_errors(Augen)
if (MSVC)
	target_compile_options(Augen PUBLIC /wd4127) # forces using "if constexpr"
endif (MSVC)

target_compile_definitions(Augen PUBLIC -DNOMINMAX)
if (DEV_MOD)
	target_compile_definitions(Augen PRIVATE -DSHARE_DIR="${PROJECT_SOURCE_DIR}/share")
else (DEV_MOD)
	target_compile_definitions(Augen PRIVATE -DSHARE_DIR="./share")
endif (DEV_MOD)

# For IDE projects
group_source_by_folder(${SRC})
