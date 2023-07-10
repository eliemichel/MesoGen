# Copyright (c) 2020 -- Élie Michel <elie.michel@telecom-paris.fr>
# 
# Permission is hereby granted, free of charge, to any person obtaining a copy
# of this software and associated documentation files (the “Software”), to
# deal in the Software without restriction, including without limitation the
# rights to use, copy, modify, merge, publish, distribute, sublicense, and/or
# sell copies of the Software, and to permit persons to whom the Software is
# furnished to do so, subject to the following conditions:
# 
# The above copyright notice and this permission notice shall be included in
# all copies or substantial portions of the Software.
#
# The Software is provided “as is”, without warranty of any kind, express or
# implied, including but not limited to the warranties of merchantability,
# fitness for a particular purpose and non-infringement. In no event shall the
# authors or copyright holders be liable for any claim, damages or other
# liability, whether in an action of contract, tort or otherwise, arising
# from, out of or in connection with the software or the use or other dealings
# in the Software.

cmake_minimum_required(VERSION 3.0)
project(modernglad)

###########

set(MODERN_GLAD_INCLUDE_DIR ${CMAKE_CURRENT_BINARY_DIR}/include)
set(MODERN_GLAD_HEADER ${MODERN_GLAD_INCLUDE_DIR}/glad/modernglad.h)
set(GENERATOR_SCRIPT ${CMAKE_CURRENT_SOURCE_DIR}/generator/modernglad.py)

###########

find_package(PythonInterp REQUIRED)

add_custom_command(
	OUTPUT ${MODERN_GLAD_HEADER}
	COMMAND ${PYTHON_EXECUTABLE} ${GENERATOR_SCRIPT} ${MODERN_GLAD_HEADER}
	DEPENDS ${GENERATOR_SCRIPT}
	COMMENT "Generating ModernGLAD"
)
add_custom_target(modernglad-generate-files DEPENDS ${MODERN_GLAD_HEADER})
set_source_files_properties(${MODERN_GLAD_HEADER} PROPERTIES GENERATED TRUE)

file(WRITE ${CMAKE_CURRENT_BINARY_DIR}/null.c "")
add_library(modernglad STATIC ${MODERN_GLAD_HEADER} ${CMAKE_CURRENT_BINARY_DIR}/null.c)
target_include_directories(modernglad PUBLIC ${MODERN_GLAD_INCLUDE_DIR})
target_link_libraries(modernglad PUBLIC glad)
add_dependencies(modernglad modernglad-generate-files)
