#ifndef __G4L_HELPER_H
#define __G4L_HELPER_H

struct lua_State;
struct luaL_Reg;

typedef struct l_constant_reg
{
	const char* name;
	unsigned int constant;
} l_constant_reg;

void l_registerFunctions(struct lua_State* L, int idx, const struct luaL_Reg* r);
void l_registerConstants(struct lua_State* L, int idx, const l_constant_reg* c);
void l_stacktrace(struct lua_State* L);

int context_available();
void context_set_available();

#define assert_extension(L, ext) \
	if (!GLEW_##ext) luaL_error(L, "Extension `%s' not supported", #ext)

#endif
