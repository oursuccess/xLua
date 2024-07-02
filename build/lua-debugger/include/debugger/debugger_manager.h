#pragma once
#include <memory>
#include <unordered_set>

#include "util/util.h"
#include "debugger/debugger.h"
#include "debugger/hookstate.h"
#include "debugger/extensionpoint.h"

namespace CPL
{
	class CPLDEBUGGER_API DebuggerManager
	{
	public:
		using UniqueIdentifyType = unsigned long long;
		DebuggerManager();
		~DebuggerManager();

		//获取L的MainThread所在的debugger
		std::shared_ptr<Debugger> GetDebugger(lua_State* L);
		//若L为main thread，则添加Debugger，否则返回L所在的MainThread的debugger
		std::shared_ptr<Debugger> AddDebugger(lua_State* L);
		//移除L所在的mainThread的debugger并返回
		std::shared_ptr<Debugger> RemoveDebugger(lua_State* L);
		//获得所有debugger
		std::vector<std::shared_ptr<Debugger>> GetDebuggers();
		void RemoveAllDebugger();
		//获得当前命中的debugger
		std::shared_ptr<Debugger> GetHitBreakpoint();

		void SetHitDebugger(std::shared_ptr<Debugger> debugger);
		bool IsDebuggerEmpty();

		void AddBreakpoint(std::shared_ptr<BreakPoint> breakpoint);
		//返回拷贝后的断点列表
		std::vector<std::shared_ptr<BreakPoint>> GetBreakPoints();

		void RemoveBreakpoint(const std::string& file, int line);
		void RemoveAllBreakpoints();

		void RefreshLineSet();
		//返回拷贝后的断点行
		std::unordered_set<int> GetLineSet();

		void HandleBreak(lua_State* L);

		void DoAction(DebugAction action);
		void Eval(std::shared_ptr<EvalContext> ctx);

		void OnDisconnect();
		void SetRunning(bool value);
		bool IsRunning();

	public:
		std::shared_ptr<HookStateBreak> stateBreak;
		std::shared_ptr<HookStateStepOver> stateStepOver;
		std::shared_ptr<HookStateStepIn> stateStepIn;
		std::shared_ptr<HookStateStepOut> stateStepOut;
		std::shared_ptr<HookStateContinue> stateContinue;
		std::shared_ptr<HookStateStop> stateStop;

		std::string helperCode;
		std::vector<std::string> extNames;

		ExtensionPoint extension;

	private:
		UniqueIdentifyType GetUniqueIdentify(lua_State* L);

		std::mutex debuggerMtx;
		std::map<UniqueIdentifyType, std::shared_ptr<Debugger>> debuggers;

		std::mutex breakDebuggerMtx;
		std::shared_ptr<Debugger> hitDebugger;

		std::mutex breakPointsMtx;
		std::vector<std::shared_ptr<BreakPoint>> breakpoints;

		std::unordered_set<int> lineSet;
		std::atomic<bool> isRunning;
	};
}
