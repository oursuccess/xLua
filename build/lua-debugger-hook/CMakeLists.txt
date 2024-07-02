cmake_minimum_required(VERSION 3.14)

project(cpl_hook)
set(CMAKE_CXX_STANDARD 20)

add_library(cpl_hook SHARED)

add_dependencies(cpl_hook 
	xlua
	uv_a
	cpl_debugger_shared
	cpl_debugger
)

target_include_directories(cpl_hook 
	PUBLIC 
	${LUA_SRC_PATH}
	${CMAKE_SOURCE_DIR}/lua-debugger-shared
	${cpl_debugger_SOURCE_DIR}/include
	${CMAKE_SOURCE_DIR}/nlohmann
	${CMAKE_SOURCE_DIR}/libuv-1.46.0/include
)

target_sources(cpl_hook PRIVATE
	dllmain.cpp
	cplhook.cpp
	cplhook.windows.cpp
)

target_link_libraries(cpl_hook PUBLIC
	xlua
	uv_a
	cpl_debugger_shared
	cpl_debugger
)

install(TARGETS cpl_hook
	RUNTIME DESTINATION .
	LIBRARY DESTINATION .
	ARCHIVE DESTINATION .)