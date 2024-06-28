#pragma once

#include "debugger_util.h"
#include "debugger_proto.h"

namespace CPL
{
	class ExtensionPoint {
	public:
		ExtensionPoint();
		void Initialize(lua_State* L);
		bool QueryVariable(lua_State* L, Idx<Variable> variable, const char* typeName, int object, int depth);
		bool QueryVariableCustom(lua_State* L, Idx<Variable> variable, const char* typeName, int object, int depth);

		lua_State* QueryParentThread(lua_State* L);
	private:
		bool QueryVariableGeneric(lua_State* L, Idx<Variable> variable, const char* typeName, int object, int depth, const char* queryFunction);

	public:
		static std::string extensionTable;
	};
}