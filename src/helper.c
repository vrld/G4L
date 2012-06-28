#include "helper.h"

#include <lua.h>
#include <lauxlib.h>
#include <lualib.h>
#include <assert.h>

void l_registerFunctions(lua_State* L, int idx, const luaL_Reg* r)
{
	if (idx < 0)
		idx += lua_gettop(L) + 1;
	for (; r && r->name && r->func; ++r)
	{
		lua_pushstring(L, r->name);
		lua_pushcfunction(L, r->func);
		lua_rawset(L, idx);
	}
}

void l_registerConstants(lua_State* L, int idx, const l_constant_reg* c)
{
	if (idx < 0)
		idx += lua_gettop(L) + 1;
	for (; c && c->name; ++c)
	{
		lua_pushstring(L, c->name);
		lua_pushinteger(L, c->constant);
		lua_rawset(L, idx);
	}
}

void l_stacktrace(lua_State* L)
{
	lua_Debug entry;
	for (int i = 0; lua_getstack(L, i, &entry); ++i)
	{
		lua_getinfo(L, "nSl", &entry);
		printf("%s(%s): %s\n", entry.source, entry.what, entry.name ? entry.name : entry.namewhat);
	}
}

static int has_context = 0;

int context_available()
{
	return has_context;
}

void context_set_available()
{
	has_context = 1;
}
