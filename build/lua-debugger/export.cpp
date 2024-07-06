#include "util/util.h"
#include "debugger/debugger_lib.h"
#include "debugger/facade.h"

#define DEBUGGER_API extern "C" __declspec(dllexport)

using namespace CPL;

#if WIN32
#include <processthreadsapi.h>

DEBUGGER_API void lua_startupHookMode()
{
	CPL::DebuggerFacade::Get().SetWorkMode(CPL::WorkMode::Attach);
	const int pid = (int)GetCurrentProcessId();
	CPL::DebuggerFacade::Get().StartupHookMode(pid);
}
#endif

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

DEBUGGER_API int luaopen_debugger(lua_State* L) {

	DebuggerFacade::Get().SetWorkMode(WorkMode::Core);
	if (!install_debugger(L)) return false;
	luaL_newlibtable(L, debugger_lib);
	luaL_setfuncs(L, debugger_lib, 0);

	lua_pushglobaltable(L);
	lua_pushstring(L, "cpl_core");
	lua_pushvalue(L, -3);
	lua_rawset(L, -3);
	lua_pop(L, 1);

	return 1;
}
