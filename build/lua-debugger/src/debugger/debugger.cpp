#include "debugger/debugger.h"

namespace CPL
{
	Debugger::Debugger(lua_State* L, DebuggerManager* manager) :
		currL(L), mainL(L), manager(manager), hookState(nullptr), running(false),
		skipHook(false), blocking(false), arenaRef(nullptr), displayCustomTypeInfo(false)
	{
	}

	Debugger::~Debugger()
	{
	}

	void Debugger::Start()
	{
		skipHook = false;
		blocking = false;
		running = true;
		doStringList.clear();
	}

	void Debugger::Attach()
	{
		if (!running) return;
	}

	void Debugger::Detach()
	{
	}

	void Debugger::SetLuaState(lua_State* L)
	{
	}

	void Debugger::Hook(lua_State* L, lua_Debug* ar)
	{
	}

	void Debugger::Stop()
	{
	}

	void Debugger::IsRunning() const
	{
	}

	bool Debugger::IsMainThread(lua_State* L) const
	{
		return false;
	}

	void Debugger::AsyncDoString(const std::string& code)
	{
	}

	bool Debugger::Eval(std::shared_ptr<EvalContext> evalContext, bool force)
	{
		return false;
	}

	bool Debugger::GetStacks(std::vector<Stack>& stacks)
	{
		return false;
	}

	void Debugger::GetVariable(lua_State* L, Idx<Variable> variable, int index, int depth, bool queryHelper)
	{
	}

	void Debugger::DoAction(DebugAction action)
	{
	}

	void Debugger::EnterDebugMode()
	{
	}

	void Debugger::ExitDebugMode()
	{
	}

	void Debugger::ExecuteWithSkipHook(const Executor& executor)
	{
	}

	void Debugger::ExecuteOnLuaThread(const Executor& executor)
	{
	}

	void Debugger::HandleBreak()
	{
	}

	int Debugger::GetStackLevel(bool skipC) const
	{
		return 0;
	}

	void Debugger::UpdateHook(lua_State* L, int mask)
	{
	}

	void Debugger::SetHookState(std::shared_ptr<HookState> hookState)
	{
	}

	DebuggerManager* Debugger::GetManager() const
	{
		return nullptr;
	}

	void Debugger::SetVariableArena(Arena<Variable>* arena)
	{
	}

	Arena<Variable>* Debugger::GetVariableArena() const
	{
		return nullptr;
	}

	void Debugger::ClearVariableArenaRef()
	{
	}

	bool Debugger::RegisterTypeName(const std::string& typeName, std::string& err)
	{
		return false;
	}

	std::shared_ptr<BreakPoint> Debugger::FindBreakPoint(lua_Debug* ar)
	{
		return std::shared_ptr<BreakPoint>();
	}

	std::shared_ptr<BreakPoint> Debugger::FindBreakPoint(const std::string& file, int line)
	{
		return std::shared_ptr<BreakPoint>();
	}

	std::string Debugger::GetFile(lua_Debug* ar) const
	{
		return std::string();
	}

	void Debugger::CheckDoString()
	{
	}

	bool Debugger::CreateEnv(lua_State* L, int stackLevel)
	{
		return false;
	}

	bool Debugger::ProcessBreakPoint(std::shared_ptr<BreakPoint> bp)
	{
		return false;
	}

	bool Debugger::DoEval(std::shared_ptr<EvalContext> evalContext)
	{
		return false;
	}

	void Debugger::DoLogMessage(std::shared_ptr<BreakPoint> bp)
	{
	}

	bool Debugger::DoHitCondition(std::shared_ptr<BreakPoint> bp)
	{
		return false;
	}

	int Debugger::FuzzyMatchFileName(const std::string& fileName, const std::string& chunkName) const
	{
		return 0;
	}

	void Debugger::CacheValue(int valueIndex, Idx<Variable> variable) const
	{
	}

	void Debugger::ClearCache()
	{
	}

	int Debugger::GetTypeFromName(const char* typeName)
	{
		return 0;
	}
}
