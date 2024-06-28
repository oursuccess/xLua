#pragma once

#include "debugger_util.h"
#include <string>
#include <memory>

namespace CPL
{
	class Variable;
}

bool query_variable(lua_State* L, std::shared_ptr<CPL::Variable> variable, const char* typeName, int object, int depth);

int tcpListen(lua_State* L);
int tcpConnect(lua_State* L);
int breakHere(lua_State* L);
int waitIDE(lua_State* L);
int tcpSharedListen(lua_State* L);
int stop(lua_State* L);
int registerTypeName(lua_State* L);
int install_debugger(lua_State* L);

std::string prepareEvalExpr(const std::string& eval);