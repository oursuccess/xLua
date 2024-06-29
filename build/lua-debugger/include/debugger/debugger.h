#pragma once

#include <vector>
#include <mutex>
#include <condition_variable>
#include <functional>
#include <memory>
#include <queue>
#include <bitset>

#include "util/util.h"
#include "proto/proto.h"
#include "debugger/hookstate.h"

namespace CPL
{
	using Executor = std::function<void(lua_State* L)>;

	class DebuggerManager;

	class Debugger : public std::enable_shared_from_this<Debugger>
	{
	public:
		Debugger(lua_State* L, DebuggerManager* manager);
		~Debugger();

		void Start();
		void Attach();
		void Detach(); //lua main thread destroy
		void SetLuaState(lua_State* L);
		void Hook(lua_State* L, lua_Debug* ar);
		void Stop();
		bool IsRunning() const;
		bool IsMainThread(lua_State* L) const;	//or main coroutine
		void AsyncDoString(const std::string& code);
		bool Eval(std::shared_ptr<EvalContext> evalContext, bool force = false);
		bool GetStacks(std::vector<Stack>& stacks);
		void GetVariable(lua_State* L, Idx<Variable> variable, int index, int depth, bool queryHelper = true);
		void DoAction(DebugAction action);
		void EnterDebugMode();
		void ExitDebugMode();
		void ExecuteWithSkipHook(const Executor& executor);
		void ExecuteOnLuaThread(const Executor& executor);
		void HandleBreak();
		int GetStackLevel(bool skipC) const;
		void UpdateHook(lua_State* L, int mask);
		// ToDo: add Hook State 
		void SetHookState(std::shared_ptr<HookState> hookState);
		DebuggerManager* GetManager() const;
		void SetVariableArena(Arena<Variable>* arena);
		Arena<Variable>* GetVariableArena() const;
		void ClearVariableArenaRef();
		bool RegisterTypeName(const std::string& typeName, std::string& err);

	private:
		std::shared_ptr<BreakPoint> FindBreakPoint(lua_Debug* ar);
		std::shared_ptr<BreakPoint> FindBreakPoint(const std::string& file, int line);
		std::string GetFile(lua_Debug* ar) const;

		void CheckDoString();
		bool CreateEnv(lua_State* L, int stackLevel);
		bool ProcessBreakPoint(std::shared_ptr<BreakPoint> bp);
		bool DoEval(std::shared_ptr<EvalContext> evalContext);
		void DoLogMessage(std::shared_ptr<BreakPoint> bp);
		bool DoHitCondition(std::shared_ptr<BreakPoint> bp);
		//模糊匹配, 计算出匹配度最高的路径
		int FuzzyMatchFileName(const std::string& fileName, const std::string& chunkName) const;
		void CacheValue(int valueIndex, Idx<Variable> variable) const;
		void ClearCache();
		int GetTypeFromName(const char* typeName);

		lua_State* currL;
		lua_State* mainL;
		DebuggerManager* manager;

		std::mutex hookStateMtx;
		std::shared_ptr<HookState> hookState;
		bool running;
		bool skipHook;
		bool blocking;

		std::vector<std::string> doStringList;

		std::mutex runMtx;
		std::condition_variable cvRun;

		std::mutex luaThreadMtx;
		std::vector<Executor> luaThreadExecutors;

		std::mutex evalMtx;
		std::queue<std::shared_ptr<EvalContext>> evalQueue;

		Arena<Variable>* arenaRef;
		bool displayCustomTypeInfo;
		std::bitset<LUA_NUMTAGS> registedTypes;
	};
}