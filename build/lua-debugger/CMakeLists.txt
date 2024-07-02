cmake_minimum_required(VERSION 3.14)

project(cpl_debugger)
set(CMAKE_CXX_STANDARD 20)

add_library(cpl_debugger STATIC)

add_dependencies(cpl_debugger uv_a cpl_debugger_shared)

target_include_directories(cpl_debugger PUBLIC
	${LUA_SRC_PATH}
	${cpl_debugger_SOURCE_DIR}/include
	${CMAKE_SOURCE_DIR}/nlohmann
	${CMAKE_SOURCE_DIR}/libuv-1.46.0/include
)

target_sources(cpl_debugger
	PRIVATE
    src/util/util.cpp
    src/proto/proto.cpp
    src/proto/proto_handler.cpp
    src/debugger/debugger_lib.cpp
    src/debugger/debugger_manager.cpp
    src/debugger/hookstate.cpp
    src/debugger/debugger.cpp
    src/debugger/extensionpoint.cpp
    src/debugger/facade.cpp
    src/transporter/transporter.cpp
    src/transporter/socket_server_transporter.cpp
    src/transporter/socket_client_transporter.cpp
    "export.cpp"
)

target_link_libraries(cpl_debugger PUBLIC uv_a cpl_debugger_shared)
