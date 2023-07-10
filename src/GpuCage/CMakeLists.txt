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

###############################################################################
# Main executable

set(SRC
	Ui/App.h
	Ui/App.cpp
	Ui/SceneDialog.h
	Ui/SceneDialog.cpp
	Ui/GlobalTimerDialog.h
	Ui/GlobalTimerDialog.cpp

	utils/objutils.h
	utils/objutils.cpp

	Scene.h
	Scene.cpp
)

set(LIBS
	Augen
	tinyobjloader
)

# Most of the program is defined as a library, for tests to pick what they need
add_library(GpuCageLibrary STATIC ${SRC})
target_include_directories(GpuCageLibrary PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
target_link_libraries(GpuCageLibrary PUBLIC ${LIBS})
target_treat_warnings_as_errors(GpuCageLibrary)

# Executable
add_executable(GpuCage main.cpp)
target_link_libraries(GpuCage LINK_PRIVATE GpuCageLibrary)
target_treat_warnings_as_errors(GpuCage)

# Installation
set(INSTALL_DIR "${PROJECT_SOURCE_DIR}/releases/${CMAKE_PROJECT_NAME}-v${CMAKE_PROJECT_VERSION}")
get_filename_component(INSTALL_DIR "${INSTALL_DIR}" ABSOLUTE)
install(TARGETS GpuCage DESTINATION ${INSTALL_DIR})

# For IDE projects
group_source_by_folder(${SRC})
