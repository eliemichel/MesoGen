# This projects puts together two different header only libraries, namely refl-cpp and magic_enum

cmake_minimum_required(VERSION 3.0)
project(refl-cpp)

add_library(refl-cpp INTERFACE)
target_include_directories(refl-cpp INTERFACE include)
