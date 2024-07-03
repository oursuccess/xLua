cmake_minimum_required(VERSION 3.14)

project(cpl_debugger)
set(CMAKE_CXX_STANDARD 20)

include_directories(
	${LUA_SRC_PATH}
	${cpl_debugger_SOURCE_DIR}/include
	${CMAKE_SOURCE_DIR}/nlohmann
	${CMAKE_SOURCE_DIR}/libuv-1.46.0/include
)

set(cpl_debugger
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
