#include "util/util.h"
#include "debugger/debugger_lib.h"
#include "debugger/facade.h"

#define DEBUGGER_API extern "C" __declspec(dllexport)

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

DEBUGGER_API int luaopen_cpl_debugger(lua_State* L) {
	luaL_newlib(L, debugger_lib);
	return 1;
}
