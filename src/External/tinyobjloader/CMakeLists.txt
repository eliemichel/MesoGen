#Tiny Object Loader Cmake configuration file.
#This configures the Cmake system with multiple properties, depending
#on the platform and configuration it is set to build in.
project(tinyobjloader)
cmake_minimum_required(VERSION 2.8.11...3.0)
set(TINYOBJLOADER_SOVERSION 1)
set(TINYOBJLOADER_VERSION 1.0.4)

#optional double precision support
option(TINYOBJLOADER_USE_DOUBLE "Build library with double precision instead of single (float)" OFF)

if(TINYOBJLOADER_USE_DOUBLE)
  set(LIBRARY_NAME ${PROJECT_NAME}_double)
else()
  set(LIBRARY_NAME ${PROJECT_NAME})
endif()


#Folder Shortcuts
set(TINYOBJLOADEREXAMPLES_DIR ${CMAKE_CURRENT_SOURCE_DIR}/examples)

set(tinyobjloader-Source
  ${CMAKE_CURRENT_SOURCE_DIR}/tiny_obj_loader.h
  ${CMAKE_CURRENT_SOURCE_DIR}/tiny_obj_loader.cc
  )

add_library(${LIBRARY_NAME} ${tinyobjloader-Source})

if(BUILD_SHARED_LIBS)
  set_target_properties(${LIBRARY_NAME} PROPERTIES
    SOVERSION ${TINYOBJLOADER_SOVERSION}
    WINDOWS_EXPORT_ALL_SYMBOLS ON
  )
endif()

if(TINYOBJLOADER_USE_DOUBLE)
  target_compile_definitions(${LIBRARY_NAME} PUBLIC TINYOBJLOADER_USE_DOUBLE)
endif()

set_target_properties(${LIBRARY_NAME} PROPERTIES VERSION ${TINYOBJLOADER_VERSION})

target_include_directories(${LIBRARY_NAME} INTERFACE
  $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}>
  $<INSTALL_INTERFACE:${TINYOBJLOADER_INCLUDE_DIR}>
  )

export(TARGETS ${LIBRARY_NAME} FILE ${PROJECT_NAME}-targets.cmake)
