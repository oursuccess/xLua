#include "debugger_extensionpoint.h"
#include <string>

namespace CPL
{
	std::string ExtensionPoint::extensionTable = "cplHelper";

	int metaQuery(lua_State* L) {
		const int argN = lua_gettop(L);
		auto pVar = (Idx<Variable>*)lua_touserdata(L, 1);
		const int index = 2;
		const auto depth = lua_tonumber(L, 3);
		bool queryHelper = false;
		if (argN >= 4) queryHelper = lua_tonumber(L, 4);
		//TODO: add implemetation
		return -1;
	}

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
