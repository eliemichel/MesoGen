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
# Tests (only enabled if this project is the toplevel one)
if(CMAKE_PROJECT_NAME STREQUAL PROJECT_NAME AND BUILD_TESTING)

	add_subdirectory(External/Catch2)
	set_property(TARGET Catch2 PROPERTY FOLDER "External")
	set_property(TARGET Catch2WithMain PROPERTY FOLDER "External")
	list(APPEND CMAKE_MODULE_PATH ${CMAKE_CURRENT_SOURCE_DIR}/External/Catch2/extras)

	include(Catch)
	
	add_executable(TestLibwfc test-libwfc.cpp)
	target_link_libraries(TestLibwfc PRIVATE Catch2::Catch2WithMain)
	target_link_libraries(TestLibwfc PRIVATE libwfc)
	set_property(TARGET TestLibwfc PROPERTY FOLDER "Tests")
	catch_discover_tests(TestLibwfc)

	#add_executable(TestTileDag2d
	#	test-tiledag2d.cpp
	#	test-tiledag25d.cpp
	#	test-mesh-based-slot-topology.cpp
	#	test-tileset04.cpp
	#)
	#target_link_libraries(TestTileDag2d PRIVATE Catch2::Catch2WithMain)
	#target_link_libraries(TestTileDag2d PRIVATE libwfc TileDag2dLibrary)
	#set_property(TARGET TestTileDag2d PROPERTY FOLDER "Tests")
	#target_compile_definitions(TestTileDag2d PRIVATE -DSHARE_DIR="${PROJECT_SOURCE_DIR}/share")
	#catch_discover_tests(TestTileDag2d)

	add_executable(TestTileset04 test-tileset04.cpp)
	target_link_libraries(TestTileset04 PRIVATE Catch2::Catch2WithMain)
	target_link_libraries(TestTileset04 PRIVATE libwfc MesostructureGeneratorLibrary)
	set_property(TARGET TestTileset04 PROPERTY FOLDER "Tests")
	target_compile_definitions(TestTileset04 PRIVATE -DSHARE_DIR="${PROJECT_SOURCE_DIR}/share")
	catch_discover_tests(TestTileset04)

	add_executable(TestTileset05 test-tileset05.cpp)
	target_link_libraries(TestTileset05 PRIVATE Catch2::Catch2WithMain)
	target_link_libraries(TestTileset05 PRIVATE libwfc MesostructureGeneratorLibrary)
	set_property(TARGET TestTileset05 PROPERTY FOLDER "Tests")
	target_compile_definitions(TestTileset05 PRIVATE -DSHARE_DIR="${PROJECT_SOURCE_DIR}/share")
	catch_discover_tests(TestTileset05)

	add_executable(TestTileset06 test-tileset06.cpp)
	target_link_libraries(TestTileset06 PRIVATE Catch2::Catch2WithMain)
	target_link_libraries(TestTileset06 PRIVATE libwfc MesostructureGeneratorLibrary)
	set_property(TARGET TestTileset06 PROPERTY FOLDER "Tests")
	target_compile_definitions(TestTileset06 PRIVATE -DSHARE_DIR="${PROJECT_SOURCE_DIR}/share")
	catch_discover_tests(TestTileset06)

	add_executable(TestAugen test-augen.cpp)
	target_link_libraries(TestAugen PRIVATE Catch2::Catch2WithMain)
	target_link_libraries(TestAugen PRIVATE Augen MesostructureGeneratorLibrary)
	set_property(TARGET TestAugen PROPERTY FOLDER "Tests")
	catch_discover_tests(TestAugen)

	add_executable(TestTileSuggestion test-tile-suggestion.cpp)
	target_link_libraries(TestTileSuggestion PRIVATE Augen MesostructureGeneratorLibrary argparse)
	set_property(TARGET TestTileSuggestion PROPERTY FOLDER "Tests")
	target_compile_definitions(TestTileSuggestion PRIVATE -DDATA_DIR="${PROJECT_SOURCE_DIR}/tests/data")

	add_executable(TestLibwfcFastSuperposition
		test-libwfc-fast-superposition.cpp
		../src/MesostructureGenerator/BMesh.cpp
		../src/MesostructureGenerator/BMeshIO.cpp
		../src/MesostructureGenerator/Model.cpp
		../src/MesostructureGenerator/Serialization.cpp
		../src/MesostructureGenerator/Serialization/SerializationModel.cpp
		../src/MesostructureGenerator/TileVariantList.cpp
		../src/MesostructureGenerator/TilesetController.cpp
		../src/MesostructureGenerator/VersionCounter.cpp
		../src/MesostructureGenerator/CsgEngine.cpp
		../src/MesostructureGenerator/CsgCompiler.cpp
		../src/MesostructureGenerator/Bezier.cpp
	)
	target_link_libraries(TestLibwfcFastSuperposition PRIVATE Catch2::Catch2WithMain)
	target_link_libraries(TestLibwfcFastSuperposition PRIVATE libwfc Augen tinyobjloader RapidJSON polyclipping)
	set_property(TARGET TestLibwfcFastSuperposition PROPERTY FOLDER "Tests")
	target_compile_definitions(TestLibwfcFastSuperposition PRIVATE -DSHARE_DIR="${PROJECT_SOURCE_DIR}/share")
	target_include_directories(TestLibwfcFastSuperposition PRIVATE ../src/MesostructureGenerator)
	catch_discover_tests(TestLibwfcFastSuperposition)

endif()
