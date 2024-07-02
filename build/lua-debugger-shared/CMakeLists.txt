cmake_minimum_required(VERSION 3.14)

project(cpl_debugger_shared)

add_library(cpl_debugger_shared STATIC)

target_include_directories(cpl_debugger_shared PUBLIC ${debugger_shared_SOURCE_DIR})

target_sources(cpl_debugger_shared PRIVATE shme.cpp)

target_link_libraries(cpl_debugger_shared)