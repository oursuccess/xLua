#include "debugger_lib.h"

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
	//ToDo: add proxy(called facade in emmy)
	bool succ = false;
	lua_pushboolean(L, succ);
	if (succ) return 1;
	lua_pushstring(L, err.c_str());
	return 2;
}

int tcpConnect(lua_State* L)
{
	return 0;
}

int breakHere(lua_State* L)
{
	return 0;
}

int waitIDE(lua_State* L)
{
	return 0;
}

int tcpSharedListen(lua_State* L)
{
	return 0;
}

int stop(lua_State* L)
{
	return 0;
}

int registerTypeName(lua_State* L)
{
	return 0;
}

int install_debugger(lua_State* L)
{
	return 0;
}

std::string prepareEvalExpr(const std::string& eval)
{
	return std::string();
}
