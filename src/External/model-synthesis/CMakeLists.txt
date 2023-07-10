cmake_minimum_required(VERSION 3.7...3.21)
project(ModelSynthesis
	VERSION 1.0
	DESCRIPTION "Model Synthesis Implementation"
	LANGUAGES CXX
)

###########################################################
# Reusable part of the MS implementation

add_library(ModelSynthesisLibrary STATIC
	src/parseInput/InputSettings.cpp
	src/parseInput/InputSettings.h
	src/parseInput/parseInput.cpp
	src/parseInput/parseInput.h
	src/parseInput/parseOverlapping.cpp
	src/parseInput/parseOverlapping.h
	src/parseInput/parseSimpleTiled.cpp
	src/parseInput/parseSimpleTiled.h
	src/parseInput/parseTiledModel.cpp
	src/parseInput/parseTiledModel.h

	src/propagator/Propagator.cpp
	src/propagator/Propagator.h
	src/propagator/PropagatorAc3.cpp
	src/propagator/PropagatorAc3.h
	src/propagator/PropagatorAc4.cpp
	src/propagator/PropagatorAc4.h

	src/third_party/lodepng/lodepng.h
	src/third_party/lodepng/lodepng.cpp
	src/third_party/xmlParser.h
	src/third_party/xmlParser.cpp

	src/OutputGenerator.h
	src/OutputGenerator.cpp
	src/synthesizer.h
	src/synthesizer.cpp
)
target_compile_definitions(ModelSynthesisLibrary PUBLIC UNICODE)
target_include_directories(ModelSynthesisLibrary PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/src)

###########################################################
# Demonstrator

add_executable(ModelSynthesisDemo
	"Model Synthesis.cpp"
)
target_link_libraries(ModelSynthesisDemo PRIVATE ModelSynthesisLibrary)
