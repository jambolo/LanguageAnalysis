cmake_minimum_required(VERSION 3.23)
project(SubtlexImporterLib LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

add_library(SubtlexImporter STATIC 
    SubtlexImporter.cpp
    SubtlexImporter.h
    DatasetImporter.h
)
target_include_directories(SubtlexImporter PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
