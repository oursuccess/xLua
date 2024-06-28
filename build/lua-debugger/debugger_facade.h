#pragma once

#include <mutex>
#include <unordered_map>
#include <condition_variable>

#include "nlohmann/json_fwd.hpp"

#include "debugger_util.h"
#include "debugger_manager.h"

namespace CPL
{
	enum class LogType {
		Info,
		Warning,
		Error
	};

	enum class WorkMode {
		Attach,
	};

	class DebuggerFacade {
	public:
		static DebuggerFacade& Get();
		static void HookLua(lua_State* L, lua_Debug* ar);
		static void ReadyLuaHook(lua_State* L, lua_Debug* ar);

		DebuggerFacade();
		~DebuggerFacade();

		bool TcpListen(lua_State* L, const std::string& host, int port, std::string& err);
		bool TcpSharedListen(lua_State* L, const std::string& host, int port, std::string& err);
		bool TcpConnect(lua_State* L, const std::string& host, int port, std::string& err);

		bool BreakHere(lua_State* L);
		bool RegisterTypeName(lua_State* L, const std::string& typeName, std::string& err);

		int OnConnect(bool succ);
		int OnDisconnect();
		void WaitIDE(bool force = false, int timeout = 0);
		bool OnBreak(std::shared_ptr<Debugger> debugger);
		void OnDestroy();
		void OnEvalResult(std::shared_ptr<EvalContext> context);
		void SendLog(LogType type, const char* fmt, ...);
		void OnLuaStateGC(lua_State* L);
		void Hook(lua_State* L, lua_Debug* ar);

		DebuggerManager& GetDebuggerManager();
		std::shared_ptr<Debugger> GetDebugger();

		void SetReadyHook(lua_State* L);
		void StartDebug();

		void StartupHookMode(int port);
		void Attach(lua_State* L);

		void SetWorkMode(WorkMode mode);
		WorkMode GetWorkMode();

		void InitReq(InitParams& params);
		void ReadyReq();
		void OnReceiveMessage(nlohmann::json document);

	public:
		std::function<void()> StartHook;

	private:
		std::mutex waitIDEMtx;
		std::condition_variable waitIDECV;
		//TODO: add Transporter, which is about network communication
		//std::shared_ptr<Transporter> transporter;

		bool isIDEReady;
		bool isAPIReady;
		bool isWaitForIDE;
		WorkMode workMode;

		//states that use tcpListen
		std::unordered_set<lua_State*> mainStates;

		std::atomic<bool> readyHook;

		DebuggerManager debuggerManager;

		//TODO: add proto handler
	};
}