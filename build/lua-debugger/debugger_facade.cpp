#include "debugger_facade.h"

namespace CPL
{
	DebuggerFacade& CPL::DebuggerFacade::Get()
	{
		static DebuggerFacade instance;
		return instance;
	}
	void DebuggerFacade::HookLua(lua_State* L, lua_Debug* ar)
	{
	}
	void DebuggerFacade::ReadyLuaHook(lua_State* L, lua_Debug* ar)
	{
	}
	DebuggerFacade::DebuggerFacade()
	{
	}
	DebuggerFacade::~DebuggerFacade()
	{
	}
	bool DebuggerFacade::TcpListen(lua_State* L, const std::string& host, int port, std::string& err)
	{
		return false;
	}
	bool DebuggerFacade::TcpSharedListen(lua_State* L, const std::string& host, int port, std::string& err)
	{
		return false;
	}
	bool DebuggerFacade::TcpConnect(lua_State* L, const std::string& host, int port, std::string& err)
	{
		return false;
	}
	bool DebuggerFacade::BreakHere(lua_State* L)
	{
		return false;
	}
	bool DebuggerFacade::RegisterTypeName(lua_State* L, const std::string& typeName, std::string& err)
	{
		return false;
	}
	int DebuggerFacade::OnConnect(bool succ)
	{
		return 0;
	}
	int DebuggerFacade::OnDisconnect()
	{
		return 0;
	}
	void DebuggerFacade::WaitIDE(bool force, int timeout)
	{
	}
	bool DebuggerFacade::OnBreak(std::shared_ptr<Debugger> debugger)
	{
		return false;
	}
	void DebuggerFacade::OnDestroy()
	{
	}
	void DebuggerFacade::OnEvalResult(std::shared_ptr<EvalContext> context)
	{
	}
	void DebuggerFacade::SendLog(LogType type, const char* fmt, ...)
	{
	}
	void DebuggerFacade::OnLuaStateGC(lua_State* L)
	{
	}
	void DebuggerFacade::Hook(lua_State* L, lua_Debug* ar)
	{
	}
	DebuggerManager& DebuggerFacade::GetDebuggerManager()
	{
		return debuggerManager;
	}
	std::shared_ptr<Debugger> DebuggerFacade::GetDebugger()
	{
		return std::shared_ptr<Debugger>();
	}
	void DebuggerFacade::SetReadyHook(lua_State* L)
	{
	}
	void DebuggerFacade::StartDebug()
	{
	}
	void DebuggerFacade::StartupHookMode(int port)
	{
	}
	void DebuggerFacade::Attach(lua_State* L)
	{
	}
	void DebuggerFacade::SetWorkMode(WorkMode mode)
	{
	}
	WorkMode DebuggerFacade::GetWorkMode()
	{
		return WorkMode();
	}
	void DebuggerFacade::InitReq(InitParams& params)
	{
	}
	void DebuggerFacade::ReadyReq()
	{
	}
	void DebuggerFacade::OnReceiveMessage(nlohmann::json document)
	{
	}
}
