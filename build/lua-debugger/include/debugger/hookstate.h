#pragma once

#include "util/util.h"

#include <memory>

namespace CPL 
{
	class Debugger;

	class HookState {
	public:
		HookState();
		virtual ~HookState();

		virtual bool Start(std::shared_ptr<Debugger> debugger, lua_State* L);
		virtual void ProcessHook(std::shared_ptr<Debugger> debugger, lua_State* L, lua_Debug* ar);
	protected:
		lua_State* currL;
	};

	class HookStateContinue : public HookState {
	public:
		virtual bool Start(std::shared_ptr<Debugger> debugger, lua_State* L) override;
	};

	class StackLevelBasedState : public HookState {
	public:
		virtual bool Start(std::shared_ptr<Debugger> debugger, lua_State* L) override;

	protected:
		int oriStackLevel;
		int newStackLevel;
		void UpdateStackLevel(std::shared_ptr<Debugger> debugger, lua_State* L, lua_Debug* ar);
	};;

	class HookStateStepIn : public StackLevelBasedState {
	public:
		virtual bool Start(std::shared_ptr<Debugger> debugger, lua_State* L) override;
		virtual void ProcessHook(std::shared_ptr<Debugger> debugger, lua_State* L, lua_Debug* ar) override;

	private:
		std::string file;
		int line = 0;
	};

	class HookStateStepOut : public StackLevelBasedState {
	public:
		virtual bool Start(std::shared_ptr<Debugger> debugger, lua_State* L) override;
		virtual void ProcessHook(std::shared_ptr<Debugger> debugger, lua_State* L, lua_Debug* ar) override;
	};

	class HookStateStepOver : public StackLevelBasedState {
	public:
		virtual bool Start(std::shared_ptr<Debugger> debugger, lua_State* L) override;
		virtual void ProcessHook(std::shared_ptr<Debugger> debugger, lua_State* L, lua_Debug* ar) override;

	private:
		std::string file;
		int line = 0;
	};

	class HookStateBreak : public HookState {
	public:
		virtual void ProcessHook(std::shared_ptr<Debugger> debugger, lua_State* L, lua_Debug* ar) override;
	};

	class HookStateStop : public HookState {
	public:
		virtual bool Start(std::shared_ptr<Debugger> debugger, lua_State* L) override;
	};
}
