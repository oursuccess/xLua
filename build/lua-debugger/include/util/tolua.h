//only lua.c files need this
#pragma once

#ifdef __cplusplus
#define TOLUA_API extern "C"
#else
#define TOLUA_API
#endif

TOLUA_API void try_attach_debugger(lua_State* L);

#undef TOLUA_API