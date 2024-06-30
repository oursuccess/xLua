#include "debugger/debugger_lib.h"
#include "debugger/facade.h"

using namespace CPL;

bool query_variable(lua_State* L, std::shared_ptr<CPL::Variable> variable, const char* typeName, int object, int depth)
{
	return false;
}

int tcpListen(lua_State* L)
{
	luaL_checkstring(L, 1);
	std::string err;
	const auto host = lua_tostring(L, 1);
	luaL_checknumber(L, 2);
	const auto port = lua_tointeger(L, 2);
	bool succ = DebuggerFacade::Get().TcpListen(L, host, static_cast<int>(port), err);
	lua_pushboolean(L, succ);
	if (succ) return 1;
	lua_pushstring(L, err.c_str());
	return 2;
}

int tcpConnect(lua_State* L)
{
	luaL_checkstring(L, 1);
	std::string err;
	const auto host = lua_tostring(L, 1);
	luaL_checknumber(L, 2);
	const auto port = lua_tointeger(L, 2);
	const auto succ = DebuggerFacade::Get().TcpConnect(L, host, static_cast<int>(port), err);
	lua_pushboolean(L, succ);
	if (succ) return 1;
	lua_pushstring(L, err.c_str());
	return 2;
}

int breakHere(lua_State* L)
{
	const bool suc = DebuggerFacade::Get().BreakHere(L);
	lua_pushboolean(L, suc);
	return 1;
}

int waitIDE(lua_State* L)
{
	int top = lua_gettop(L);
	int timeout = 0;
	if (top >= 1) timeout = static_cast<int>(luaL_checknumber(L, 1));
	DebuggerFacade::Get().WaitIDE(false, timeout);
	return 0;
}

int tcpSharedListen(lua_State* L)
{
	luaL_checkstring(L, 1);
	std::string err;
	const auto host = lua_tostring(L, 1);
	luaL_checknumber(L, 2);
	const auto port = lua_tointeger(L, 2);
	const auto suc = DebuggerFacade::Get().TcpSharedListen(L, host, static_cast<int>(port), err);
	lua_pushboolean(L, suc);
	if (suc) return 1;
	lua_pushstring(L, err.c_str());
	return 2;
}

int stop(lua_State* L)
{
	DebuggerFacade::Get().Destroy();
	return 0;
}

int registerTypeName(lua_State* L)
{
	luaL_checkstring(L, 1);
	std::string err;
	const auto typeName = lua_tostring(L, 1);
	const auto suc = DebuggerFacade::Get().RegisterTypeName(L, typeName, err);
	lua_pushboolean(L, suc);
	if (suc) return 1;
	lua_pushstring(L, err.c_str());
	return 2;
}

#pragma region Private
int gc(lua_State* L) {
	DebuggerFacade::Get().OnLuaStateGC(L);
	return 0;
}

void handleStateClose(lua_State* L) {
	lua_getfield(L, LUA_REGISTRYINDEX, "__CPL__GC__");
	int isnil = lua_isnil(L, -1);
	lua_pop(L, 1);
	if (!isnil) return;

	lua_newtable(L);
	lua_pushcfunction(L, gc);
	lua_setfield(L, -2, "__gc");

	lua_newuserdata(L, 1);
	lua_pushvalue(L, -2);
	lua_setmetatable(L, -2);

	lua_setfield(L, LUA_REGISTRYINDEX, "__CPL__GC__");
	lua_pop(L, 1);
}
#pragma endregion

bool install_debugger(lua_State* L)
{
	DebuggerFacade::Get().GetDebuggerManager().extension.Initialize(L);
	handleStateClose(L);

	return true;
}

std::string prepareEvalExpr(const std::string& eval)
{
	if (eval.empty()) return eval;

	int lastIndex = static_cast<int>(eval.size() - 1);
	for (int i = lastIndex; i >= 0; --i) {
		auto c = eval[i];
		if (c == ':') {
			auto newStr = eval;
			newStr[i] = '.';	//replace : to .
			return eval;
		}
	}
	return eval;
}
