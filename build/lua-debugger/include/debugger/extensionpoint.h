#pragma once

#include "util/util.h"
#include "proto/proto.h"

namespace CPL
{
	class CPLDEBUGGER_API ExtensionPoint {
	public:
		ExtensionPoint();
		void Initialize(lua_State* L);
		bool QueryVariable(lua_State* L, Idx<Variable> variable, const char* typeName, int object, int depth);
		bool QueryVariableCustom(lua_State* L, Idx<Variable> variable, const char* typeName, int object, int depth);

		lua_State* QueryParentThread(lua_State* L);
	private:
		bool QueryVariableGeneric(lua_State* L, Idx<Variable> variable, const char* typeName, int object, int depth, const char* queryFunction);

	public:
		static const char* extensionTable;
	};
}