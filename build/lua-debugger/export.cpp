#include "util/util.h"
#include "debugger/debugger_lib.h"

static const luaL_Reg debugger_lib[] = {
	{ "tcpListen", tcpListen },
	{ "tcpConnect", tcpConnect },
	{ "breakHere", breakHere },
	{ "waitIDE", waitIDE },
	{ "tcpSharedListen", tcpSharedListen },
	{ "stop", stop },
	{ "registerTypeName", registerTypeName },
	{ NULL, NULL }
};

#define DEBUGGER_API extern "C" __declspec(dllexport)

DEBUGGER_API int luaopen_cpl_debugger(lua_State* L) {
	luaL_newlib(L, debugger_lib);
	return 1;
}
