#include "debugger_lib.h"

bool query_variable(lua_State* L, std::shared_ptr<Variable> variable, const char* typeName, int object, int depth)
{
	return false;
}

int tcpListen(lua_State* L)
{
	return 0;
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
