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
	Serialization/SerializationModel.h
	Serialization/SerializationModel.cpp
	Serialization/SerializationScene.h
	Serialization/SerializationScene.cpp

	Ui/App.h
	Ui/App.cpp
	Ui/HelpDialog.h
	Ui/HelpDialog.cpp
	Ui/GlobalTimerDialog.h
	Ui/GlobalTimerDialog.cpp
	Ui/MacrosurfaceDialog.h
	Ui/MacrosurfaceDialog.cpp
	Ui/MacrosurfaceRendererDialog.h
	Ui/MacrosurfaceRendererDialog.cpp
	Ui/MesostructureExporterDialog.h
	Ui/MesostructureExporterDialog.cpp
	Ui/MesostructureRendererDialog.h
	Ui/MesostructureRendererDialog.cpp
	Ui/ModelDialog.h
	Ui/ModelDialog.cpp
	Ui/OutputDialog.h
	Ui/OutputDialog.cpp
	Ui/RenderDialog.h
	Ui/RenderDialog.cpp
	Ui/SimpleDialog.h
	Ui/SimpleDialog.cpp
	Ui/TilesetEditorDialog.h
	Ui/TilesetEditorDialog.cpp
	Ui/TilesetRendererDialog.h
	Ui/TilesetRendererDialog.cpp
	Ui/TilingSolverDialog.h
	Ui/TilingSolverDialog.cpp
	Ui/TilingSolverRendererDialog.h
	Ui/TilingSolverRendererDialog.cpp

	utils/glutils.h
	utils/miscutils.h

	Bezier.h
	Bezier.cpp
	BMesh.h
	BMesh.cpp
	BMeshIo.h
	BMeshIo.cpp
	CsgCompiler.h
	CsgCompiler.cpp
	CsgEngine.h
	CsgEngine.cpp
	DeferredRendering.h
	DeferredRendering.cpp
	FileRenderer.h
	FileRenderer.cpp
	io.h
	io.cpp
	MacrosurfaceData.h
	MacrosurfaceData.cpp
	MacrosurfaceRenderer.h
	MacrosurfaceRenderer.cpp
	MesostructureData.h
	MesostructureData.cpp
	MesostructureExporter.h
	MesostructureExporter.cpp
	MesostructureRenderer.h
	MesostructureRenderer.cpp
	MesostructureMvcRenderer.h
	MesostructureMvcRenderer.cpp
	MesostructureRenderOptions.h
	MvcDeformer.h
	MvcDeformer.cpp
	Model.h
	Model.cpp
	QuadObject.h
	QuadObject.cpp
	OutputObject.h
	OutputObject.cpp
	Serialization.h
	Serialization.cpp
	Ssbo.h
	Ssbo.cpp
	SweepSsboItem.h
	SweepSsboItem.cpp
	TilesetController.h
	TilesetController.cpp
	TilesetEditorObject.h
	TilesetEditorObject.cpp
	TilesetRenderer.h
	TilesetRenderer.cpp
	TileSuggestion.h
	TileSuggestion.cpp
	TileVariantList.h
	TileVariantList.cpp
	TilingSolver.h
	TilingSolver.cpp
	TilingSolverData.h
	TilingSolverData.cpp
	TilingSolverRenderData.h
	TilingSolverRenderData.cpp
	TilingSolverRenderer.h
	TilingSolverRenderer.cpp
	VersionCounter.h
	VersionCounter.cpp
)

set(LIBS
	Augen
	libwfc
	RapidJSON
	tinyobjloader
	Eigen
	polyclipping
	stb_image_write
)

if (USE_MODEL_SYNTHESIS)
	list(APPEND SRC TilingSolver_solveGridMs.cpp)
	list(APPEND LIBS ModelSynthesisLibrary)
endif ()

# Most of the program is defined as a library, for tests to pick what they need
add_library(MesostructureGeneratorLibrary STATIC ${SRC})
target_include_directories(MesostructureGeneratorLibrary PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
target_link_libraries(MesostructureGeneratorLibrary PUBLIC ${LIBS})
target_treat_warnings_as_errors(MesostructureGeneratorLibrary)
target_compile_options(MesostructureGeneratorLibrary PRIVATE "/Wv:18")
if (USE_MODEL_SYNTHESIS)
	target_compile_definitions(MesostructureGeneratorLibrary PRIVATE USE_MODEL_SYNTHESIS)
endif ()

target_compile_definitions(MesostructureGeneratorLibrary PRIVATE RAPIDJSON_HAS_STDSTRING=1)

# Executable
add_executable(MesostructureGenerator main.cpp)
target_link_libraries(MesostructureGenerator LINK_PRIVATE MesostructureGeneratorLibrary)
target_treat_warnings_as_errors(MesostructureGenerator)

# For IDE projects
group_source_by_folder(${SRC})
