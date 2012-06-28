#ifndef __G4L_WINDOW_H
#define __G4L_WINDOW_H

struct lua_State;

typedef struct Window
{
	int id;
	struct lua_State* L;
} Window;

Window* l_toWindow(struct lua_State* L, int idx);
Window* l_checkWindow(struct lua_State* L, int idx);
int l_window_new(struct lua_State* L);

#endif
