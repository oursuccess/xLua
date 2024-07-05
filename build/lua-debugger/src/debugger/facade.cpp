#include "debugger/facade.h"

#include <string>

#include "transporter/socket_server_transporter.h"
#include "transporter/socket_client_transporter.h"
#include "debugger/debugger_lib.h"

extern "C" {
#include "lstate.h"
}

namespace CPL
{
	#pragma region private methods
	std::vector<lua_State*> FindAllCoroutine(lua_State* L)
	{
		std::vector<lua_State*> res;
		auto head = G(L)->allgc;

		while (head) {
			if (head->tt == LUA_TTHREAD) res.push_back(reinterpret_cast<lua_State*>(head));
			head = head->next;
		}
		return res;
	}

	int LuaError(lua_State* L) {
		std::string msg = lua_tostring(L, 1);
		msg = "[CPL]" + msg;
		lua_getglobal(L, "error");
		lua_pushstring(L, msg.c_str());
		lua_call(L, 1, 0);
		return 0;
	}
	#pragma endregion

	DebuggerFacade& CPL::DebuggerFacade::Get()
	{
		static DebuggerFacade instance;
		return instance;
	}
	void DebuggerFacade::HookLua(lua_State* L, lua_Debug* ar)
	{
		Get().Hook(L, ar);
	}
	void DebuggerFacade::ReadyLuaHook(lua_State* L, lua_Debug* ar)
	{
		if (!Get().readyHook) return;
		Get().readyHook = false;
		auto states = FindAllCoroutine(L);
		for (auto state : states) lua_sethook(state, HookLua, LUA_MASKCALL | LUA_MASKLINE | LUA_MASKRET, 0);
		lua_sethook(L, HookLua, LUA_MASKCALL | LUA_MASKLINE | LUA_MASKRET, 0);

		if (auto debugger = Get().GetDebugger(L)) debugger->Attach();
		Get().Hook(L, ar);
	}
	DebuggerFacade::DebuggerFacade() : transporter(nullptr), protoHandler(this),
		isIDEReady(false), isAPIReady(false), StartHook(nullptr), isWaitForIDE(false),
		workMode(WorkMode::Core), readyHook(false)
	{
	}
	DebuggerFacade::~DebuggerFacade()
	{
	}
	bool DebuggerFacade::TcpListen(lua_State* L, const std::string& host, int port, std::string& err)
	{
		Destroy();

		debuggerManager.AddDebugger(L);
		SetReadyHook(L);

		const auto s = std::make_shared<SocketServerTransporter>();
		transporter = s;

		const auto suc = s->Listen(host, port, err);
		if (!suc) {
			lua_pushcfunction(L, LuaError);
			lua_pushstring(L, err.c_str());
			lua_call(L, 1, 0);
		}
		return suc;
	}
	bool DebuggerFacade::TcpSharedListen(lua_State* L, const std::string& host, int port, std::string& err)
	{
		if (transporter == nullptr) return TcpListen(L, host, port, err);
		return true;
	}
	bool DebuggerFacade::TcpConnect(lua_State* L, const std::string& host, int port, std::string& err)
	{
		Destroy();
		debuggerManager.AddDebugger(L);
		SetReadyHook(L);

		const auto c = std::make_shared<SocketClientTransporter>();
		transporter = c;
		const auto suc = c->Connect(host, port, err);
		if (suc) WaitIDE(true);
		else {
			lua_pushcfunction(L, LuaError);
			lua_pushstring(L, err.c_str());
			lua_call(L, 1, 0);
		}

		return suc;
	}
	bool DebuggerFacade::BreakHere(lua_State* L)
	{
		if (!isIDEReady) return 0;
		debuggerManager.HandleBreak(L);
		return 1;
	}
	bool DebuggerFacade::RegisterTypeName(lua_State* L, const std::string& typeName, std::string& err)
	{
		auto debugger = GetDebugger(L);
		if (!debugger) {
			err = "Debugger doesn't exist";
			return false;
		}
		const auto suc = debugger->RegisterTypeName(typeName, err);
		return suc;
	}
	int DebuggerFacade::OnConnect(bool succ)
	{
		return 0;
	}
	int DebuggerFacade::OnDisconnect()
	{
		isIDEReady = false;
		isWaitForIDE = false;
		debuggerManager.OnDisconnect();
		debuggerManager.RemoveAllBreakpoints();
		if (workMode == WorkMode::Attach) debuggerManager.RemoveAllDebugger();

		return 0;
	}
	void DebuggerFacade::WaitIDE(bool force, int timeout)
	{
		if (transporter != nullptr && (transporter->IsServerMode() || force) && !isWaitForIDE && !isIDEReady) {
			isWaitForIDE = true;
			std::unique_lock<std::mutex> lock(waitIDEMtx);
			if (timeout > 0) waitIDECV.wait_for(lock, std::chrono::milliseconds(timeout));
			else waitIDECV.wait(lock);
			isWaitForIDE = false;
		}
	}
	bool DebuggerFacade::OnBreak(std::shared_ptr<Debugger> debugger)
	{
		if (!debugger) return false;
		std::vector<Stack> stacks;

		debuggerManager.SetHitDebugger(debugger);
		debugger->GetStacks(stacks);
		auto obj = nlohmann::json::object();
		obj["cmd"] = static_cast<int>(MessageCMD::BreakNotify);
		obj["stacks"] = JsonProtocol::Serialize(stacks);

		transporter->Send(int(MessageCMD::BreakNotify), obj);
		return true;
	}
	void DebuggerFacade::Destroy()
	{
		OnDisconnect();

		if (transporter) {
			transporter->Stop();
			transporter = nullptr;
		}
	}
	void DebuggerFacade::OnEvalResult(std::shared_ptr<EvalContext> context)
	{
		if (transporter) transporter->Send(int(MessageCMD::EvalRsp), context->Serialize());
	}
	void DebuggerFacade::SendLog(LogType type, const char* fmt, ...)
	{
		va_list args;
		va_start(args, fmt);
		char buff[1024] = { 0 };
		vsnprintf(buff, 1024, fmt, args);
		va_end(args);

		const std::string msg = buff;

		auto obj = nlohmann::json::object();
		obj["type"] = type;
		obj["message"] = msg;

		if (transporter) transporter->Send(int(MessageCMD::LogNotify), obj);
	}
	void DebuggerFacade::OnLuaStateGC(lua_State* L)
	{
		auto debugger = debuggerManager.RemoveDebugger(L);

		if (debugger) debugger->Detach();

		if (workMode == WorkMode::Core && debuggerManager.IsDebuggerEmpty()) Destroy();
	}
	void DebuggerFacade::Hook(lua_State* L, lua_Debug* ar)
	{
		auto debugger = GetDebugger(L);
		if (debugger) {
			if (!debugger->IsRunning()) {
				if (GetWorkMode() == WorkMode::Core && debugger->IsMainThread(L)) {
					SetReadyHook(L);
				}
			}

			debugger->Hook(L, ar);
		}
		else {
			if (GetWorkMode() == WorkMode::Attach) {
				debugger = debuggerManager.AddDebugger(L);
				install_debugger(L);
				if (debuggerManager.IsRunning()) {
					debugger->Start();
					debugger->Attach();
				}
				auto obj = nlohmann::json::object();
				obj["state"] = reinterpret_cast<int64_t>(L);

				this->transporter->Send(int(MessageCMD::AttachedNotify), obj);
				debugger->Hook(L, ar);
			}
		}
	}
	DebuggerManager& DebuggerFacade::GetDebuggerManager()
	{
		return debuggerManager;
	}
	std::shared_ptr<Debugger> DebuggerFacade::GetDebugger(lua_State* L)
	{
		return debuggerManager.GetDebugger(L);
	}
	void DebuggerFacade::SetReadyHook(lua_State* L)
	{
		lua_sethook(L, ReadyLuaHook, LUA_MASKCALL | LUA_MASKLINE | LUA_MASKRET, 0);
	}
	void DebuggerFacade::StartDebug()
	{
		debuggerManager.SetRunning(true);
		readyHook = true;
	}
	void DebuggerFacade::StartupHookMode(int port)
	{
		Destroy();
		while (port > 0xffff) port -= 0xffff;	//65535
		while (port < 0x400) port += 0x400;	//1024

		const auto s = std::make_shared<SocketServerTransporter>();
		std::string err;
		const auto suc = s->Listen("localhost", port, err);
		if (suc) transporter = s;
	}
	void DebuggerFacade::Attach(lua_State* L)
	{
		if (!transporter->IsConnected()) return;

		if (!isAPIReady) isAPIReady = install_debugger(L);

		lua_sethook(L, DebuggerFacade::HookLua, LUA_MASKCALL | LUA_MASKLINE | LUA_MASKRET, 0);
	}
	void DebuggerFacade::SetWorkMode(WorkMode mode)
	{
		workMode = mode;
	}
	WorkMode DebuggerFacade::GetWorkMode()
	{
		return workMode;
	}
	void DebuggerFacade::InitReq(InitParams& params)
	{
		if (StartHook) StartHook();

		debuggerManager.helperCode = params.helper;
		debuggerManager.extNames.clear();
		debuggerManager.extNames = params.ext;

		StartDebug();
	}
	void DebuggerFacade::ReadyReq()
	{
		isIDEReady = true;
		waitIDECV.notify_all();
	}
	void DebuggerFacade::OnReceiveMessage(nlohmann::json document)
	{
		protoHandler.OnDispatch(document);
	}
}
