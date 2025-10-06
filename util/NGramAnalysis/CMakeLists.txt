cmake_minimum_required(VERSION 3.23)
project(NGramAnalysis)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

add_executable(ngram_analyzer main.cpp)

target_link_libraries(ngram_analyzer PRIVATE CLI11::CLI11 nlohmann_json::nlohmann_json SubtlexImporter)
target_include_directories(ngram_analyzer PRIVATE ${CMAKE_SOURCE_DIR}/lib)
