#include "debugger/debugger_manager.h"

#include <ranges>

extern "C" {
#include "lstate.h"
}

namespace CPL
{
	DebuggerManager::DebuggerManager() : 
		stateBreak(std::make_shared<HookStateBreak>()), stateContinue(std::make_shared<HookStateContinue>()),
		stateStepOver(std::make_shared<HookStateStepOver>()), stateStepIn(std::make_shared<HookStateStepIn>()),
		stateStepOut(std::make_shared<HookStateStepOut>()), stateSStop(std::make_shared<HookStateStop>()),
		isRunning(false)
	{
	}

	DebuggerManager::~DebuggerManager()
	{
	}

	std::shared_ptr<Debugger> DebuggerManager::GetDebugger(lua_State* L)
	{
		std::lock_guard<std::mutex> lock(debuggerMtx);
		auto identify = GetUniqueIdentify(L);
		auto it = debuggers.find(identify);
		if (it != debuggers.end())
			return it->second;
		return nullptr;
	}

	std::shared_ptr<Debugger> DebuggerManager::AddDebugger(lua_State* L)
	{
		std::lock_guard<std::mutex> lock(debuggerMtx);
		auto identify = GetUniqueIdentify(L);
		std::shared_ptr<Debugger> debugger = nullptr;
		auto it = debuggers.find(identify);
		if (it == debuggers.end())
		{
			debugger = std::make_shared<Debugger>(reinterpret_cast<lua_State*>(identify), this);
			debuggers.insert(std::make_pair(identify, debugger));
		}
		else
		{
			debugger = it->second;
		}

		debugger->SetLuaState(L);
		return debugger;
	}

	std::shared_ptr<Debugger> DebuggerManager::RemoveDebugger(lua_State* L)
	{
		std::lock_guard<std::mutex> lock(debuggerMtx);
		auto identify = GetUniqueIdentify(L);
		auto it = debuggers.find(identify);
		if (it != debuggers.end())
		{
			auto debugger = it->second;
			debuggers.erase(it);
			return debugger;
		}
		return nullptr;
	}

	std::vector<std::shared_ptr<Debugger>> DebuggerManager::GetDebuggers()
	{
		std::lock_guard<std::mutex> lock(debuggerMtx);
		std::vector<std::shared_ptr<Debugger>> res;
		std::ranges::transform(debuggers, std::back_inserter(res), [](const auto& pair) { return pair.second; });
		return res;
	}

	void DebuggerManager::RemoveAllDebugger()
	{
		std::lock_guard<std::mutex> lock(debuggerMtx);
		debuggers.clear();
	}
	std::shared_ptr<Debugger> DebuggerManager::GetHitBreakpoint()
	{
		std::lock_guard<std::mutex> lock(breakDebuggerMtx);
		return hitDebugger;
	}
	void DebuggerManager::SetHitDebugger(std::shared_ptr<Debugger> debugger)
	{
		std::lock_guard<std::mutex> lock(breakDebuggerMtx);
		hitDebugger = debugger;
	}
	bool DebuggerManager::IsDebuggerEmpty()
	{
		std::lock_guard<std::mutex> lock(debuggerMtx);
		return debuggers.empty();
	}
	void DebuggerManager::AddBreakpoint(std::shared_ptr<BreakPoint> breakpoint)
	{
		std::lock_guard<std::mutex> lock(breakPointsMtx);
		bool isAdd = false;
		for (auto& bp : breakpoints) {
			if (bp->line == breakpoint->line && CompareIgnoreCase(breakpoint->file, bp->file) == 0) {
				bp = breakpoint;
				isAdd = true;
			}
		}

		if (!isAdd) breakpoints.push_back(breakpoint);
		RefreshLineSet();
	}
	std::vector<std::shared_ptr<BreakPoint>> DebuggerManager::GetBreakPoints()
	{
		std::lock_guard<std::mutex> lock(breakPointsMtx);
		return breakpoints;
	}
	void DebuggerManager::RemoveBreakpoint(const std::string& file, int line)
	{
		std::lock_guard<std::mutex> lock(breakPointsMtx);
		auto it = breakpoints.begin();
		while (it != breakpoints.end()) {
			const auto bp = *it;
			if (bp->line == line && CompareIgnoreCase(bp->file, file) == 0) {
				breakpoints.erase(it);
				break;
			}
			++it;
		}
		RefreshLineSet();
	}
	void DebuggerManager::RemoveAllBreakpoints()
	{
		std::lock_guard<std::mutex> lock(breakPointsMtx);
		breakpoints.clear();
		lineSet.clear();
	}
	void DebuggerManager::RefreshLineSet()
	{
		lineSet.clear();
		for (auto bp : breakpoints) lineSet.insert(bp->line);
	}
	std::unordered_set<int> DebuggerManager::GetLineSet()
	{
		std::lock_guard<std::mutex> lock(breakPointsMtx);
		return lineSet;
	}
	void DebuggerManager::HandleBreak(lua_State* L)
	{
		auto debugger = GetDebugger(L);
		if (debugger) debugger->SetLuaState(L);
		else debugger = AddDebugger(L);

		SetHitDebugger(debugger);
		debugger->HandleBreak();
	}
	void DebuggerManager::DoAction(DebugAction action)
	{
		if (auto debugger = GetHitBreakpoint()) {
			debugger->DoAction(action);
		}
	}
	void DebuggerManager::Eval(std::shared_ptr<EvalContext> ctx)
	{
		if (auto debugger = GetHitBreakpoint())
		{
			debugger->Eval(ctx, false);
		}
	}
	void DebuggerManager::OnDisconnect()
	{
		SetRunning(false);
		std::lock_guard<std::mutex> lock(debuggerMtx);
		for (auto it : debuggers) it.second->Stop();
	}
	void DebuggerManager::SetRunning(bool value)
	{
		isRunning = value;
		if (isRunning) {
			for (auto debugger : GetDebuggers()) debugger->Start();
		}
	}
	bool DebuggerManager::IsRunning()
	{
		return isRunning;
	}
	DebuggerManager::UniqueIdentifyType DebuggerManager::GetUniqueIdentify(lua_State* L)
	{
		return reinterpret_cast<UniqueIdentifyType>(G(L)->mainthread);
	}
}
