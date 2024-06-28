#include "debugger_extensionpoint.h"

namespace CPL
{
	ExtensionPoint::ExtensionPoint()
	{
	}

	void ExtensionPoint::Initialize(lua_State* L)
	{
	}

	bool ExtensionPoint::QueryVariable(lua_State* L, Idx<Variable> variable, const char* typeName, int object, int depth)
	{
		return false;
	}

	bool ExtensionPoint::QueryVariableCustom(lua_State* L, Idx<Variable> variable, const char* typeName, int object, int depth)
	{
		return false;
	}

	lua_State* ExtensionPoint::QueryParentThread(lua_State* L)
	{
		return nullptr;
	}

	bool ExtensionPoint::QueryVariableGeneric(lua_State* L, Idx<Variable> variable, const char* typeName, int object, int depth, const char* queryFunction)
	{
		return false;
	}
}
