#include "debugger/hookstate.h"
#include "debugger/debugger.h"
#include "debugger/debugger_manager.h"

namespace CPL
{
	HookState::HookState() : currL(nullptr)
	{
	}

	HookState::~HookState()
	{
	}

	bool HookState::Start(std::shared_ptr<Debugger> debugger, lua_State* L)
	{
		currL = L;
		return true;
	}

	void HookState::ProcessHook(std::shared_ptr<Debugger> debugger, lua_State* L, lua_Debug* ar)
	{
	}

	bool HookStateContinue::Start(std::shared_ptr<Debugger> debugger, lua_State* L)
	{
		debugger->ExitDebugMode();
		return true;
	}

	bool StackLevelBasedState::Start(std::shared_ptr<Debugger> debugger, lua_State* L)
	{
		if (L == nullptr) return false;
		currL = L;
		oriStackLevel = newStackLevel = debugger->GetStackLevel(false);
		return true;
	}

	void StackLevelBasedState::UpdateStackLevel(std::shared_ptr<Debugger> debugger, lua_State* L, lua_Debug* ar)
	{
		if (L != currL) return;
		//pcall or xpcall or error may change the stack level(c stack, not lua stack)
		newStackLevel = debugger->GetStackLevel(false);
	}

	bool HookStateStepIn::Start(std::shared_ptr<Debugger> debugger, lua_State* L)
	{
		if (!StackLevelBasedState::Start(debugger, L)) return false;
		lua_Debug ar{};
		lua_getstack(L, 0, &ar);
		lua_getinfo(L, "nSl", &ar);
		file = ar.source;
		line = ar.currentline;
		debugger->ExitDebugMode();
		return true;
	}

	void HookStateStepIn::ProcessHook(std::shared_ptr<Debugger> debugger, lua_State* L, lua_Debug* ar)
	{
		UpdateStackLevel(debugger, L, ar);
		if (ar->event == LUA_HOOKLINE) {
			auto currLine = ar->currentline;
			auto source = ar->source;
			if (currLine != line || file != source) debugger->HandleBreak();
			return;
		}

		StackLevelBasedState::ProcessHook(debugger, L, ar);
	}

	bool HookStateStepOut::Start(std::shared_ptr<Debugger> debugger, lua_State* L)
	{
		if (!StackLevelBasedState::Start(debugger, L)) return false;
		debugger->ExitDebugMode();
		return true;
	}

	void HookStateStepOut::ProcessHook(std::shared_ptr<Debugger> debugger, lua_State* L, lua_Debug* ar)
	{
		UpdateStackLevel(debugger, L, ar);
		if (newStackLevel < oriStackLevel) {
			debugger->HandleBreak();
			return;
		}
		StackLevelBasedState::ProcessHook(debugger, L, ar);
	}

	bool HookStateStepOver::Start(std::shared_ptr<Debugger> debugger, lua_State* L)
	{
		if (!StackLevelBasedState::Start(debugger, L)) return false;
		lua_Debug ar{};
		lua_getstack(L, 0, &ar);
		lua_getinfo(L, "nSl", &ar);
		file = ar.source;
		line = ar.currentline;
		debugger->ExitDebugMode();
		return true;
	}

	void HookStateStepOver::ProcessHook(std::shared_ptr<Debugger> debugger, lua_State* L, lua_Debug* ar)
	{
		UpdateStackLevel(debugger, L, ar);
		if (newStackLevel < oriStackLevel) {
			debugger->HandleBreak();
			return;
		}

		if (ar->event == LUA_HOOKLINE && ar->currentline != line && newStackLevel == oriStackLevel) {
			lua_getinfo(L, "Sl", ar);
			if (ar->source == file || line == -1) {
				debugger->HandleBreak();
				return;
			}
		}

		StackLevelBasedState::ProcessHook(debugger, L, ar);
	}

	void HookStateBreak::ProcessHook(std::shared_ptr<Debugger> debugger, lua_State* L, lua_Debug* ar)
	{
		if (ar->event == LUA_HOOKLINE) {
			debugger->HandleBreak();
		}
		else {
			HookState::ProcessHook(debugger, L, ar);
		}
	}

	bool HookStateStop::Start(std::shared_ptr<Debugger> debugger, lua_State* L)
	{
		if (L == nullptr) return false;

		//see emmylua attach debugger->hook_state.cpp HookStateStop::Start
		debugger->SetHookState(debugger->GetManager()->stateContinue);
		return true;
	}
}
