#pragma once

#include <string>

extern "C" {
#include "lua.h"
#include "lualib.h"
#include "lauxlib.h"
}

namespace CPL
{
	bool CompareIgnoreCase(const std::string& l, const std::string& r);
}