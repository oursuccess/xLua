#include "util/util.h"
#include "util/tolua.h"
#include "debugger/facade.h"
#include "debugger/debugger_manager.h"

void try_attach_debugger(lua_State* L)
{
	CPL::DebuggerManager& debugger = CPL::DebuggerFacade::Get().GetDebuggerManager();
	if (debugger.IsRunning()) CPL::DebuggerFacade::Get().Attach(L);
}
