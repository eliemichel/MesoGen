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

cmake_minimum_required(VERSION 3.7)
project(
	MesostructureGenerator
	VERSION 0.5
	DESCRIPTION "A tileset creation tool"
    LANGUAGES CXX
)

include(cmake/utils.cmake)
enable_cpp20()
enable_multiprocessor_compilation()
enable_testing()
set_property(GLOBAL PROPERTY USE_FOLDERS ON)

# Options
if (CMAKE_INSTALL_PREFIX_INITIALIZED_TO_DEFAULT)
	set(DEFAULT_INSTALL_DIR "${PROJECT_SOURCE_DIR}/releases/${CMAKE_PROJECT_NAME}-v${CMAKE_PROJECT_VERSION}")
	get_filename_component(DEFAULT_INSTALL_DIR "${DEFAULT_INSTALL_DIR}" ABSOLUTE)
    set(CMAKE_INSTALL_PREFIX "${DEFAULT_INSTALL_DIR}" CACHE PATH "default install path" FORCE)
endif()

option(DEV_MODE "Build in dev mode, use share directory from source tree rather than installation tree" ON)

# Build
add_subdirectory(src)
add_subdirectory(tests)

# Installation
install(TARGETS MesostructureGenerator DESTINATION ${CMAKE_INSTALL_PREFIX})
install(DIRECTORY ${PROJECT_SOURCE_DIR}/share DESTINATION ${CMAKE_INSTALL_PREFIX})

