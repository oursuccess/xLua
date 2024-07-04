#define NOMINMAX

#include "debugger/debugger.h"

#include "debugger/debugger_manager.h"
#include "debugger/facade.h"

#include <algorithm>

#define CACHE_TABLE_NAME "_cpl_cache_table_"

namespace CPL
{
	int cacheId = 1;
#pragma region Private Static
	void WaitConnectedHook(lua_State* L, lua_Debug* ar) {
	}

	int LastLevel(lua_State* L) {
		int res = 0;
		lua_Debug ar;
		while (lua_getstack(L, res, &ar)) ++res;
		return res;
	}

	bool CallMetaMethod(lua_State* L, int valueIndex, const char* method, int numResults, int& result) {
		if (lua_getmetatable(L, valueIndex)) {
			const int metaIndex = lua_gettop(L);
			if (!lua_isnil(L, metaIndex)) {
				lua_pushstring(L, method);
				lua_rawget(L, metaIndex);
				if (lua_isnil(L, -1)) {	//metamethod is nil
					lua_pop(L, 1);
					lua_remove(L, metaIndex);
					return false;
				}
				lua_pushvalue(L, valueIndex);
				result = lua_pcall(L, 1, numResults, 0);
			}
			lua_remove(L, metaIndex);
			return true;
		}
		return false;
	}

	std::string ToPointer(lua_State* L, int index) {
		const void* pointer = lua_topointer(L, index);
		std::stringstream ss;
		ss << lua_typename(L, lua_type(L, index)) << "(0x" << std::hex << pointer << ")";
		return ss.str();
	}

	//function(path) return newPath end
	int FixPath(lua_State* L) {
		const auto path = lua_tostring(L, 1);
		lua_getglobal(L, "cpl");	//TODO: add cpl.lua, which is Emmy.lua
		if (lua_istable(L, -1)) {
			lua_getfield(L, -1, "fixPath");	//TODO: this method is implied by user?
			if (lua_isfunction(L, -1)) {
				lua_pushstring(L, path);
				lua_call(L, 1, 1);
				return 1;
			}
		}
		return 0;
	}

	int EnvIndexFunction(lua_State* L) {
		const int locals = lua_upvalueindex(1);
		const int upvalues = lua_upvalueindex(2);
		const char* name = lua_tostring(L, 2);
		//up value
		lua_getfield(L, upvalues, name);
		if (lua_isnil(L, -1) == 0) return 1;
		lua_pop(L, 1);
		//local value
		lua_getfield(L, locals, name);
		if (lua_isnil(L, -1) == 0) return 1;
		lua_pop(L, 1);
		//_ENV
		lua_getfield(L, upvalues, "_ENV");
		if (lua_istable(L, -1)) {
			lua_getfield(L, -1, name); //_ENV[name]
			if (lua_isnil(L, -1) == 0) return 1;
			lua_pop(L, 1);
		}
		lua_pop(L, 1);
		//global
		lua_getglobal(L, name);
		if (lua_isnil(L, -1) == 0) return 1;
		lua_pop(L, 1);
		return 0;
	}

	struct LogMessageReplaceExpress {
	public:
		LogMessageReplaceExpress(std::string&& expr, std::size_t startIndex, std::size_t endIndex, bool needEval) :
			expr(expr), startIndex(startIndex), endIndex(endIndex), needEval(needEval)
		{
		}

		std::string expr;
		std::size_t startIndex;
		std::size_t endIndex;
		bool needEval;
	};

	std::string GetBaseName(const std::string& filePath) {	//fileName from a file path
		std::size_t sepIndex = filePath.find_last_of('/');
		if (sepIndex != std::string::npos) return filePath.substr(sepIndex + 1);
		sepIndex = filePath.find_last_of('\\');
		if (sepIndex != std::string::npos) return filePath.substr(sepIndex + 1);
		return filePath;
	}
#pragma endregion

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

		if (!manager->helperCode.empty()) {
			ExecuteOnLuaThread([this](lua_State* L) {
				const int t = lua_gettop(L);
				if (int ret = lua_pushthread(L)) {
					const int r = luaL_loadstring(L, manager->helperCode.c_str());
					if (r == LUA_OK && lua_pcall(L, 0, 0, 0) != LUA_OK) {
						printf("msg: %s", lua_tostring(L, -1));
					}
				}
				lua_settop(L, t);
				});
		}
	}

	void Debugger::Detach()
	{
	}

	void Debugger::SetCurrentState(lua_State* L)
	{
		currL = L;
	}

	void Debugger::Hook(lua_State* L, lua_Debug* ar)
	{
		if (skipHook) return;
		SetCurrentState(L);
		if (ar->event != LUA_HOOKLINE) return;
		{
			std::unique_lock<std::mutex> lock(luaThreadMtx);
			if (!luaThreadExecutors.empty()) {
				for (auto& executor : luaThreadExecutors) ExecuteWithSkipHook(executor);
				luaThreadExecutors.clear();
			}
		}
		auto bp = FindBreakPoint(ar);
		if (bp && ProcessBreakPoint(bp)) { HandleBreak(); return; }

		std::shared_ptr<HookState> state = nullptr;
		{
			std::lock_guard<std::mutex> lock(hookStateMtx);
			state = hookState;
		}
		if (state) {
			state->ProcessHook(shared_from_this(), currL, ar);
		}
	}

	void Debugger::Stop()
	{
		running = false;
		skipHook = true;
		blocking = false;

		{
			std::lock_guard<std::mutex> lock(hookStateMtx);
			hookState = nullptr;
		}
		{
			std::unique_lock<std::mutex> luaThreadLock(luaThreadMtx);
			luaThreadExecutors.clear();
		}
		ExitDebugMode();
	}

	bool Debugger::IsRunning() const
	{
		return running;
	}

	bool Debugger::IsMainThread(lua_State* L) const
	{
		return L == mainL;
	}

	void Debugger::AsyncDoString(const std::string& code)
	{
		doStringList.emplace_back(code);
	}

	bool Debugger::Eval(std::shared_ptr<EvalContext> evalContext, bool force)
	{
		if (force) return DoEval(evalContext);
		if (!blocking) return false;
		{
			std::unique_lock<std::mutex> lock(evalMtx);
			evalQueue.push(evalContext);
		}

		cvRun.notify_all();
		return true;
	}

	bool Debugger::GetStacks(std::vector<Stack>& stacks)
	{
		if (!currL) return false;
		auto prevL = currL;
		auto L = currL;
		int totalLevel = 0;
		while (true) {
			int level = 0;
			while (true) {

				lua_Debug ar{};
				if (!lua_getstack(L, level, &ar)) break;
				if (!lua_getinfo(L, "nSlu", &ar)) continue;

				stacks.emplace_back();
				auto& stack = stacks.back();
				stack.file = GetFile(&ar);
				stack.functionName = ar.name == nullptr ? "" : ar.name;
				stack.level = totalLevel++;
				stack.line = ar.currentline;

				{
					for (int i = 0; ; ++i) {
						const char* name = lua_getlocal(L, &ar, i);
						if (name == nullptr) break;
						if (name[0] == '(') {
							lua_pop(L, 1);
							continue;
						}

						auto var = stack.variableArena->Alloc();
						var->name = name;
						SetVariableArena(stack.variableArena.get());
						GetVariable(L, var, -1, 1);
						ClearVariableArenaRef();
						lua_pop(L, 1);
						stack.localVariables.push_back(var);
					}

					if (lua_getinfo(L, "f", &ar)) {
						const int fIdx = lua_gettop(L);
						for (int i = 1; ; ++i) {
							const char* name = lua_getupvalue(L, fIdx, i);
							if (!name) break;
							auto var = stack.variableArena->Alloc();
							var->name = name;
							SetVariableArena(stack.variableArena.get());
							GetVariable(L, var, -1, 1);
							ClearVariableArenaRef();
							lua_pop(L, 1);
							stack.upvalueVariables.push_back(var);
						}
						lua_pop(L, 1);
					}
				}

				++level;
			}

			lua_State* PL = manager->extension.QueryParentThread(L);
			if (PL == nullptr) break;

			L = PL;
		}

		SetCurrentState(prevL);
		return false;
	}

	void Debugger::GetVariable(lua_State* L, Idx<Variable> variable, int index, int depth, bool queryHelper)
	{
		if (!L) L = currL;
		if (!L) return;	//early return
		if (depth <= 0) return;

		const int topIndex = lua_gettop(L);
		index = lua_absindex(L, index);
		CacheValue(index, variable);
		const int type = lua_type(L, index);
		const char* typeName = lua_typename(L, type);
		variable->valueTypeName = typeName;
		variable->valueType = type;
		if (queryHelper) {
			if (displayCustomTypeInfo && type >= 0 && type < registedTypes.size() && registedTypes.test(type)
				&& manager->extension.QueryVariableCustom(L, variable, typeName, index, depth)) return;
			else if ((type == LUA_TTABLE || type == LUA_TUSERDATA || type == LUA_TFUNCTION)
				&& manager->extension.QueryVariable(L, variable, typeName, index, depth)) return;
		}
		switch (type) {
		case LUA_TNIL: {
			variable->value = "nil";
			break;
		}
		case LUA_TNUMBER: {
			variable->value = lua_tostring(L, index);
			break;
		}
		case LUA_TBOOLEAN: {
			const bool v = lua_toboolean(L, index);
			variable->value = v ? "true" : "false";
			break;
		}
		case LUA_TSTRING: {
			variable->value = lua_tostring(L, index);
			break;
		}
		case LUA_TUSERDATA: {
			auto* str = lua_tostring(L, index);
			if (str == nullptr) {
				int res;
				if (CallMetaMethod(L, topIndex, "__tostring", 1, res) && res == LUA_OK) {
					str = lua_tostring(L, -1);
					lua_pop(L, 1);
				}
			}
			variable->value = str ? str : ToPointer(L, index);
			if (depth > 1 && lua_getmetatable(L, index)) {
				GetVariable(L, variable, -1, depth);
				lua_pop(L, 1);	//pop meta
			}
			break;
		}
		case LUA_TFUNCTION: // emmy diffs in lua54 and other vm, maybe this is because lua54 can show name of function
		case LUA_TLIGHTUSERDATA:
		case LUA_TTHREAD: {
			variable->value = ToPointer(L, index);
			break;
		}
		case LUA_TTABLE: {
			std::size_t tableSize = 0;
			const void* tableAddr = lua_topointer(L, index);
			lua_pushnil(L);
			while (lua_next(L, index)) {	//k: -2, v: -1
				if (depth > 1) {
					auto v = variable.GetArena()->Alloc();
					const auto t = lua_type(L, -2);
					v->nameType = t;
					if (t == LUA_TSTRING || t == LUA_TNUMBER || t == LUA_TBOOLEAN) {
						lua_pushvalue(L, -2);	//avoid error: "invalid key to 'next'" ???
						v->name = lua_tostring(L, -1);
						lua_pop(L, 1);
					}
					else {
						v->name = ToPointer(L, -2);
					}
					GetVariable(L, v, -1, depth - 1);
					variable->children.push_back(v);
				}
				lua_pop(L, 1);
				tableSize++;
			}

			if (lua_getmetatable(L, index)) {
				auto metatable = variable.GetArena()->Alloc();
				metatable->name = "(metatable)";
				metatable->nameType = lua_type(L, -1);
				GetVariable(L, metatable, -1, depth - 1);
				variable->children.push_back(metatable);

				if (lua_istable(L, -1)) {	//get __index
					lua_pushstring(L, "__index");
					lua_rawget(L, -2);
					if (!lua_isnil(L, -1)) {
						auto v = variable.GetArena()->Alloc();
						v->name = "(metatable.__index)";
						v->nameType = lua_type(L, -1);
						GetVariable(L, v, -1, depth - 1);
						variable->children.push_back(v);
					}
					lua_pop(L, 1);
				}

				lua_pop(L, 1);	//pop metatable
			}

			std::stringstream ss;
			ss << "table(0x" << std::hex << tableAddr << std::dec << " , size = " << tableSize << ")";
			variable->value = ss.str();
			break;
		}
		}
		assert(lua_gettop(L) == topIndex);	//assert
	}

	void Debugger::DoAction(DebugAction action)
	{
		std::lock_guard<std::mutex> lock(hookStateMtx);
		switch (action) {
		case DebugAction::Break:
			SetHookState(manager->stateBreak);
			break;
		case DebugAction::Continue:
			SetHookState(manager->stateContinue);
			break;
		case DebugAction::StepOver:
			SetHookState(manager->stateStepOver);
			break;
		case DebugAction::StepIn:
			SetHookState(manager->stateStepIn);
			break;
		case DebugAction::StepOut:
			SetHookState(manager->stateStepOut);
			break;
		case DebugAction::Stop:
			SetHookState(manager->stateStop);
			break;
		}
	}

	void Debugger::EnterDebugMode()
	{
		std::unique_lock<std::mutex> lock(runMtx);
		blocking = true;
		while (true) {
			std::unique_lock<std::mutex> lockEval(evalMtx);
			if (evalQueue.empty() && blocking) {
				lockEval.unlock();
				cvRun.wait(lock);
				lockEval.lock();
			}
			if (!evalQueue.empty()) {
				const auto evalContext = evalQueue.front();
				evalQueue.pop();
				lockEval.unlock();
				const bool skip = skipHook;
				skipHook = true;
				evalContext->success = DoEval(evalContext);
				skipHook = skip;
				DebuggerFacade::Get().OnEvalResult(evalContext);
				continue;
			}
			break;
		}
		ClearCache();
	}

	void Debugger::ExitDebugMode()
	{
		blocking = false;
		cvRun.notify_all();
	}

	void Debugger::ExecuteWithSkipHook(const Executor& executor)
	{
		const bool skip = skipHook;
		skipHook = true;
		executor(currL);
		skipHook = skip;
	}

	void Debugger::ExecuteOnLuaThread(const Executor& executor)
	{
		std::unique_lock<std::mutex> lock(luaThreadMtx);
		luaThreadExecutors.push_back(executor);
	}

	void Debugger::HandleBreak()
	{
		UpdateHook(currL, LUA_MASKCALL | LUA_MASKLINE | LUA_MASKRET);

		if (DebuggerFacade::Get().OnBreak(shared_from_this())) EnterDebugMode();
	}

	int Debugger::GetStackLevel(bool skipC) const
	{
		if (!currL) return 0;
		auto L = currL;
		int level = 0, i = 0;
		lua_Debug ar{};
		while (lua_getstack(L, i, &ar)) {
			lua_getinfo(L, "l", &ar);
			if (ar.currentline >= 0 || !skipC) ++level;
			++i;
		}
		return level;
	}

	void Debugger::UpdateHook(lua_State* L, int mask)
	{
		if (mask == 0)
			lua_sethook(L, nullptr, mask, 0);
		else
			lua_sethook(L, DebuggerFacade::HookLua, mask, 0);
	}

	void Debugger::SetHookState(std::shared_ptr<HookState> newState)
	{
		if (!currL) return;
		auto L = currL;
		hookState = nullptr;
		if (newState->Start(shared_from_this(), L)) hookState = newState;
	}

	DebuggerManager* Debugger::GetManager() const
	{
		return manager;
	}

	void Debugger::SetVariableArena(Arena<Variable>* arena)
	{
		arenaRef = arena;
	}

	Arena<Variable>* Debugger::GetVariableArena() const
	{
		return arenaRef;
	}

	void Debugger::ClearVariableArenaRef()
	{
		arenaRef = nullptr;
	}

	bool Debugger::RegisterTypeName(const std::string& typeName, std::string& err)
	{
		int type = GetTypeFromName(typeName.c_str());
		if (type == -1) {
			err = "Unknown type name: " + typeName;
			return false;
		}
		displayCustomTypeInfo = true;
		registedTypes.set(type);
		return true;
	}

	std::shared_ptr<BreakPoint> Debugger::FindBreakPoint(lua_Debug* ar)
	{
		if (!currL) return nullptr;
		auto L = currL;
		const int currLine = ar->currentline;
		auto lineSet = manager->GetLineSet();

		if (currLine >= 0 && lineSet.find(currLine) != lineSet.end()) {
			lua_getinfo(L, "S", ar);
			const auto chunkname = GetFile(ar);
			return FindBreakPoint(chunkname, currLine);
		}
		return nullptr;
	}

	std::shared_ptr<BreakPoint> Debugger::FindBreakPoint(const std::string& chunkName, int line)
	{
		std::shared_ptr<BreakPoint> breakpoint = nullptr;
		int maxMatchProcess = 0;
		auto breakpoints = manager->GetBreakPoints();
		for (const auto& bp : breakpoints) {
			if (bp->line == line) {
				int matchProcess = FuzzyMatchFileName(chunkName, bp->file);
				if (matchProcess > 0 && matchProcess > maxMatchProcess) {
					maxMatchProcess = matchProcess;
					breakpoint = bp;
				}
			}
		}

		return breakpoint;
	}

	std::string Debugger::GetFile(lua_Debug* ar) const
	{
		if (!currL) return "";
		auto L = currL;
		const char* file = ar->source;
		if (ar->currentline < 0) return file;
		if (strlen(file) > 0 && file[0] == '@') ++file;
		lua_pushcclosure(L, FixPath, 0);
		lua_pushstring(L, file);
		const int res = lua_pcall(L, 1, 1, 0);
		if (res == LUA_OK) {
			const auto p = lua_tostring(L, -1);
			lua_pop(L, 1);
			if (p) return p;
		}
		return file;
	}

	void Debugger::CheckDoString()
	{
		if (!currL || doStringList.empty()) return;
		auto L = currL;

		const auto skip = skipHook;
		skipHook = true;
		const int t = lua_gettop(L);
		for (const auto& code : doStringList) {
			const int r = luaL_loadstring(L, code.c_str());
			if (r == LUA_OK) lua_pcall(L, 0, 0, 0);
			lua_settop(L, t);
		}
		skipHook = skip;
		assert(lua_gettop(L) == t);
		doStringList.clear();
	}

	bool Debugger::CreateEnv(lua_State* L, int stackLevel)
	{
		if (!L) return false;
		lua_Debug ar{};
		if (!lua_getstack(L, stackLevel, &ar)) return false;
		if (!lua_getinfo(L, "nSlu", &ar)) return false;

		lua_newtable(L);
		const int env = lua_gettop(L);
		lua_newtable(L);
		const int envMetatable = lua_gettop(L);
		lua_newtable(L);
		const int locals = lua_gettop(L);
		lua_newtable(L);
		const int upvalues = lua_gettop(L);

		int idx = 1;
		while (true) {	//local values
			const char* name = lua_getlocal(L, &ar, idx++);
			if (name == nullptr) break;
			if (name[0] == '(') {
				lua_pop(L, 1);
				continue;
			}
			lua_setfield(L, locals, name);
		}
		if (lua_getinfo(L, "f", &ar)) {	//up values
			const int fIdx = lua_gettop(L);
			idx = 1;
			while (true) {
				const char* name = lua_getupvalue(L, fIdx, idx++);
				if (name == nullptr) break;
				lua_setfield(L, upvalues, name);
			}
			lua_pop(L, 1);
		}
		assert(lua_gettop(L) == upvalues);

		lua_pushcclosure(L, EnvIndexFunction, 2);	//index function, upvalue: locals, upvalues
		lua_setfield(L, envMetatable, "__index");	//envMetatable.__index = EnvIndexFunction
		lua_setmetatable(L, env);	//setmetatable(env, envMetatable)

		assert(lua_gettop(L) == env);
		return true;
	}

	bool Debugger::ProcessBreakPoint(std::shared_ptr<BreakPoint> bp)
	{
		if (!bp->condition.empty()) {
			auto ctx = std::make_shared<EvalContext>();
			ctx->expr = bp->condition;
			ctx->depth = 1;
			bool suc = DoEval(ctx);
			return suc && ctx->result->valueType == LUA_TBOOLEAN && ctx->result->value == "true";
		}
		if (!bp->logMessage.empty()) {
			DoLogMessage(bp);
			return false;
		}
		if (!bp->hitCondition.empty()) {
			bp->hitCount++;
			return DoHitCondition(bp);
		}

		return true;
	}

	bool Debugger::DoEval(std::shared_ptr<EvalContext> evalContext)
	{
		if (!currL || !evalContext) return false;
		auto L = currL;
		int innerLevel = evalContext->stackLevel;
		while (L != nullptr) {
			int level = LastLevel(L);
			if (innerLevel > level) {
				innerLevel -= level;
				L = manager->extension.QueryParentThread(L);
			}
			else break;
		}

		if (L == nullptr) return false;

		if (evalContext->cacheId > 0) {	//From cacheId
			lua_getfield(L, LUA_REGISTRYINDEX, CACHE_TABLE_NAME);	//cache table | nil
			if (lua_type(L, -1) == LUA_TTABLE) {
				lua_getfield(L, -1, std::to_string(evalContext->cacheId).c_str());	//1. cacheTable, 2. value
				SetVariableArena(evalContext->result.GetArena());
				GetVariable(L, evalContext->result, -1, evalContext->depth);
				ClearVariableArenaRef();
				lua_pop(L, 2);
				return true;
			}
			lua_pop(L, 1);
		}

		std::string statement = "return ";	//return expr
		if (evalContext->setValue) statement = evalContext->expr + " = " + evalContext->value + " return " + evalContext->expr;
		else statement.append(evalContext->expr);

		int r = luaL_loadstring(L, statement.c_str());	//report error on syntax
		if (r == LUA_ERRSYNTAX) {
			evalContext->error = " syntax err: ";
			evalContext->error.append(evalContext->expr);
			return false;
		}

		const int fIdx = lua_gettop(L);	//call
		if (!CreateEnv(L, innerLevel)) return false;
		lua_setupvalue(L, fIdx, 1);
		assert(lua_gettop(L) == fIdx);
		//call function() return expr end
		r = lua_pcall(L, 0, 1, 0);
		if (r == LUA_OK) {
			evalContext->result->name = evalContext->expr;
			SetVariableArena(evalContext->result.GetArena());
			GetVariable(L, evalContext->result, -1, evalContext->depth);
			ClearVariableArenaRef();
			lua_pop(L, 1);
			return true;
		}

		if (r == LUA_ERRRUN) {
			evalContext->error = lua_tostring(L, -1);
		}
		lua_pop(L, 1);
		return false;
	}

	void Debugger::DoLogMessage(std::shared_ptr<BreakPoint> bp)
	{
		std::string& logMessage = bp->logMessage;
		//see emmy_debugger, why not use regex is because regex in gcc 4.8 is not implemented
		enum class ParseState {
			Normal,
			LeftBrace,
			RightBrace,
		} state = ParseState::Normal;

		std::vector<LogMessageReplaceExpress> replaceExpresses;
		std::size_t leftBraceBegin = 0, rightBraceBegin = 0, exprLeftCount = 0;

		for (std::size_t index = 0; index != logMessage.size(); ++index) {
			char ch = logMessage[index];
			switch (state) {
			case ParseState::Normal: {
				if (ch == '{') {
					state = ParseState::LeftBrace;
					leftBraceBegin = index;
					exprLeftCount = 0;
				}
				else if (ch == '}') {
					state = ParseState::RightBrace;
					rightBraceBegin = index;
				}
				break;
			}
			case ParseState::LeftBrace: {
				if (ch == '{') {
					if (index == leftBraceBegin + 1) {	//{{, 转义为可见的{
						replaceExpresses.emplace_back("{", leftBraceBegin, index, false);
						state = ParseState::Normal;
					}
					else ++exprLeftCount;
				}
				else if (ch == '}') {
					if (exprLeftCount > 0) {
						--exprLeftCount;
						continue;
					}
					replaceExpresses.emplace_back(logMessage.substr(leftBraceBegin + 1, index - leftBraceBegin - 1),
						leftBraceBegin, index, true);
					state = ParseState::Normal;
				}
				break;
			}
			case ParseState::RightBrace: {
				if (ch == '}' && (index == rightBraceBegin + 1)) {
					replaceExpresses.emplace_back("}", rightBraceBegin, index, false);
				}
				else --index;	//认为括号失配, 回退一格重新判断
				state = ParseState::Normal;
				break;
			}
			}
		}

		std::stringstream message;
		if (replaceExpresses.empty()) message << logMessage;
		else {
			std::size_t start = 0;
			for (std::size_t index = 0; index != replaceExpresses.size(); ++index) {
				auto& replaceExpress = replaceExpresses[index];
				if (start < replaceExpress.startIndex) {
					auto fragment = logMessage.substr(start, replaceExpress.startIndex - start);
					message << fragment;
					start = replaceExpress.startIndex;
				}

				if (replaceExpress.needEval) {
					auto ctx = std::make_shared<EvalContext>();
					ctx->expr = std::move(replaceExpress.expr);
					ctx->depth = 1;
					bool succeed = DoEval(ctx);
					if (succeed) message << ctx->result->value;
					else message << ctx->error;
				}
				else {
					message << replaceExpress.expr;
				}

				start = replaceExpress.endIndex + 1;
			}

			if (start < logMessage.size()) message << logMessage.substr(start, logMessage.size() - start);
		}

		std::string baseName = GetBaseName(bp->file);
		DebuggerFacade::Get().SendLog(LogType::Info, "[%s:%d] %s", baseName.c_str(), bp->line, message.str().c_str());
	}

	bool Debugger::DoHitCondition(std::shared_ptr<BreakPoint> bp)
	{
		auto& hitCondition = bp->hitCondition;
		enum class ParseState {
			ExpectedOperator,
			Gt,
			Lt,
			Assign,	//=
			ExpectedHitTimes,
			ParseDigit,
			ParseFinish
		} state = ParseState::ExpectedOperator;

		enum class Operator {
			Gt,
			Lt,
			Le,
			Ge,
			Eq,	//==
		} evalOperator = Operator::Eq;

		unsigned long long hitTimes = 0;

		for (std::size_t i = 0; i != hitCondition.size(); ++i) {
			char c = hitCondition[i];
			switch (state) {
			case ParseState::ExpectedOperator: {
				if (c == ' ') continue;
				switch (c) {
				case '=':
					state = ParseState::Assign;
					break;
				case '<':
					state = ParseState::Lt;
					break;
				case '>':
					state = ParseState::Gt;
					break;
				default:
					return false;
				}
				break;
			}
			case ParseState::Assign: {
				if (c == '=') {
					evalOperator = Operator::Eq;
					state = ParseState::ExpectedHitTimes;
				}
				else return false;
				break;
			}
			case ParseState::Gt: {
				if (c == '=') {
					evalOperator = Operator::Ge;
					state = ParseState::ExpectedHitTimes;
					break;
				}
				else if (isdigit(c)) {
					evalOperator = Operator::Gt;
					hitTimes = c - '0';
					state = ParseState::ParseDigit;
				}
				else if (c == ' ') {
					evalOperator = Operator::Gt;
					state = ParseState::ExpectedHitTimes;
				}
				else return false;
				break;
			}
			case ParseState::Lt: {
				if (c == '=') {
					evalOperator = Operator::Le;
					state = ParseState::ExpectedHitTimes;
				}
				else if (isdigit(c)) {
					evalOperator = Operator::Lt;
					hitTimes = c - '0';
					state = ParseState::ParseDigit;
				}
				else if (c == ' ') {
					evalOperator = Operator::Gt;	//FIXME: Gt???? why not Lt?
					state = ParseState::ExpectedHitTimes;
				}
				else return false;
				break;
			}
			case ParseState::ExpectedHitTimes: {
				if (c == ' ') continue;
				if (isdigit(c)) {
					hitTimes = c - '0';
					state = ParseState::ParseDigit;
				}
				else return false;
				break;
			}
			case ParseState::ParseDigit: {
				if (isdigit(c)) hitTimes = hitTimes * 10 + (c - '0');
				else if (c == ' ') state = ParseState::ParseFinish;
				else return false;
				break;
			}
			case ParseState::ParseFinish: {
				if (c == ' ') break;
				else return false;
				break;
			}
			}
		}

		switch (evalOperator) {
		case Operator::Eq: return bp->hitCount == hitTimes;
		case Operator::Gt: return bp->hitCount > hitTimes;
		case Operator::Ge: return bp->hitCount >= hitTimes;
		case Operator::Lt: return bp->hitCount <= hitTimes;
		case Operator::Le: return bp->hitCount < hitTimes;
		}

		return false;	//no entry
	}

	int Debugger::FuzzyMatchFileName(const std::string& chunkName, const std::string& fileName) const
	{
		auto chunkSize = chunkName.size();
		auto fileSize = fileName.size();

		std::size_t chunkExtSize = 0, fileExtSize = 0;
		for (const auto& ext : manager->extNames) {
			if (chunkName.ends_with(ext) && ext.size() > chunkExtSize) chunkExtSize = ext.size();
			if (fileName.ends_with(ext) && ext.size() > fileExtSize) fileExtSize = ext.size();
		}

		chunkSize -= chunkExtSize;
		fileSize -= fileExtSize;

		int maxMatchSize = static_cast<int>(std::min(chunkSize, fileSize));
		int matchProcess = 1;
		for (int i = 1; i != maxMatchSize; ++i) {
			char c = chunkName[chunkSize - i], f = fileName[fileSize - i];
			if (c != f) {
				if (::tolower(c) == ::tolower(f)) continue;
				//we assume that path from vscode can not be a relative path, but the chunkName can be(./aaa)
				//or (aaa/./bbb)
				//we don't solve the situation of ../
				if (c == '.') {
					std::size_t cLastIndex = chunkSize - i + 1;
					if (cLastIndex >= chunkSize) {
						matchProcess = 0;
						break;
					}
					char cLastChar = chunkName[cLastIndex];
					if (cLastChar != '/' && cLastChar != '\\') {
						matchProcess = 0;
						break;
					}

					int cNextIndex = static_cast<int>(chunkSize) - i - 1;	//may be negative
					if (cNextIndex < 0) break;
					char cNextChar = chunkName[cNextIndex];
					if (cNextChar != '/' && cNextChar != '\\') {	//not aaa/./bbb, only aaa./bbb
						matchProcess = 0;
						break;
					}
					++i;	//consume match of next

					fileSize += 2;	//match 2
					continue;
				}

				if (c == '/' || c == '\\') {
					if (f == '/' || f == '\\') {
						++matchProcess;
						continue;
					}
					++fileSize;	//match 1
					continue;
				}

				matchProcess = 0;
				break;
			}
		}

		return matchProcess;
	}

	void Debugger::CacheValue(int valueIndex, Idx<Variable> variable) const
	{
		if (!currL) return;
		auto L = currL;
		const int type = lua_type(L, valueIndex);
		if (type == LUA_TUSERDATA || type == LUA_TTABLE) {
			const int id = ++cacheId;
			variable->cacheId = id;
			const int top = lua_gettop(L);
			lua_getfield(L, LUA_REGISTRYINDEX, CACHE_TABLE_NAME);	//1. cachetable | nil
			if (lua_isnil(L, -1)) {
				lua_pop(L, 1);
				lua_newtable(L);
				lua_setfield(L, LUA_REGISTRYINDEX, CACHE_TABLE_NAME);
				lua_getfield(L, LUA_REGISTRYINDEX, CACHE_TABLE_NAME);
			}

			lua_pushvalue(L, valueIndex);	//1. cacheTable 2. value
			lua_setfield(L, -2, std::to_string(id).c_str());
			lua_settop(L, top);
		}
	}

	void Debugger::ClearCache()
	{
		if (!currL) return;
		auto L = currL;
		lua_getfield(L, LUA_REGISTRYINDEX, CACHE_TABLE_NAME);
		if (!lua_isnil(L, -1)) {
			lua_pushnil(L);
			lua_setfield(L, LUA_REGISTRYINDEX, CACHE_TABLE_NAME);
		}
		lua_pop(L, 1);
	}

	int Debugger::GetTypeFromName(const char* typeName)
	{
		if (strcmp(typeName, "nil") == 0) return LUA_TNIL;
		if (strcmp(typeName, "boolean") == 0) return LUA_TBOOLEAN;
		if (strcmp(typeName, "lightuserdata") == 0) return LUA_TLIGHTUSERDATA;
		if (strcmp(typeName, "number") == 0) return LUA_TNUMBER;
		if (strcmp(typeName, "string") == 0) return LUA_TSTRING;
		if (strcmp(typeName, "table") == 0) return LUA_TTABLE;
		if (strcmp(typeName, "function") == 0) return LUA_TFUNCTION;
		if (strcmp(typeName, "userdata") == 0) return LUA_TUSERDATA;
		if (strcmp(typeName, "thread") == 0) return LUA_TTHREAD;
		return -1;	//unknown type
	}
}
