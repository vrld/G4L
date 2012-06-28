#include "window.h"
#include "helper.h"

#include <lua.h>
#include <lauxlib.h>
#include <lualib.h>

#include <glew.h>
#include <glut.h>

#include <assert.h>
#include <string.h>

static const char* INTERNAL_NAME = "G4L.Window";
static const char* CALLBACKS_NAME = "G4L.callbacks";

static lua_State* LUA = NULL;

// helper
Window* l_toWindow(lua_State* L, int idx)
{
	return (Window*)luaL_checkudata(L, idx, INTERNAL_NAME);
}

Window* l_checkWindow(lua_State* L, int idx)
{
	Window* win = (Window*)luaL_checkudata(L, idx, INTERNAL_NAME);
	if (NULL == win)
		luaL_typerror(L, idx, "Window");
	return win;
}

static inline int get_callback(int win, const char* name)
{
	// get callback registry for active window
	lua_pushstring(LUA, CALLBACKS_NAME);
	lua_rawget(LUA, LUA_REGISTRYINDEX);
	lua_pushinteger(LUA, win);
	lua_rawget(LUA, -2);

	// get requested callback
	lua_pushstring(LUA, name);
	lua_rawget(LUA, -2);

	int is_function = lua_isfunction(LUA, -1);
	if (is_function)
	{
		// leave only function on stack
		lua_replace(LUA, -3);
		lua_pop(LUA, 1);
	}
	else
	{
		lua_pop(LUA, 3);
	}
	return is_function;
}

// window functions
static int l_window___index(lua_State* L);
static int l_window___newindex(lua_State* L);
static int l_window___gc(lua_State* L);
static int l_window_destroy(lua_State* L);
static int l_window_redraw(lua_State* L);
static int l_window_swap(lua_State* L);
static int l_window_position(lua_State* L);
static int l_window_resize(lua_State* L);
static int l_window_fullscreen(lua_State* L);
static int l_window_show(lua_State* L);
static int l_window_hide(lua_State* L);
static int l_window_iconify(lua_State* L);
static int l_window_title(lua_State* L);
static int l_window_cursor(lua_State* L);

// GLUT callbacks
static void _draw();
static void _reshape(int width, int height);
static void _update();
static void _visibility(int state);
static void _keyboard(unsigned char key, int x, int y);
static void _special(int key, int x, int y);
static void _mouse(int button, int status, int x, int y);
static void _motion(int x, int y);
static void _passiveMotion(int x, int y);
static void _entry(int state);

int l_window_new(lua_State* L)
{
	const char* name = luaL_checkstring(L, 1);
	int w = luaL_checkinteger(L, 2);
	int h = luaL_checkinteger(L, 3);

	int x = luaL_optinteger(L, 4, -1);
	int y = luaL_optinteger(L, 5, -1);

	Window* win = (Window*)lua_newuserdata(L, sizeof(Window));
	if (NULL == win)
		return luaL_error(L, "Cannot create window: Out of memory.");

	glutInitWindowSize(w,h);
	glutInitWindowPosition(x,y);
	win->id = glutCreateWindow(name);
	LUA = L;
	glutSetWindow(win->id);

	glutDisplayFunc(_draw);
	glutReshapeFunc(_reshape);
	glutIdleFunc(_update);
	glutVisibilityFunc(_visibility);
	glutKeyboardFunc(_keyboard);
	glutSpecialFunc(_special);
	glutMouseFunc(_mouse);
	glutMotionFunc(_motion);
	glutPassiveMotionFunc(_passiveMotion);
	glutEntryFunc(_entry);

	if (!context_available())
	{
		GLenum glew_status = glewInit();
		if (GLEW_OK != glew_status)
			return luaL_error(L, "Error initializing GLEW: %s",
			                  glewGetErrorString(glew_status));

		// check for opengl 3.3
		if (!GLEW_VERSION_3_3)
			return luaL_error(L, "OpenGL version too old");

		context_set_available();
	}

	if (luaL_newmetatable(L, INTERNAL_NAME))
	{
		lua_pushcfunction(L, l_window___index);
		lua_setfield(L, -2, "__index");

		lua_pushcfunction(L, l_window___newindex);
		lua_setfield(L, -2, "__newindex");

		luaL_Reg reg[] =
		{
			{"__gc",       l_window___gc},
			{"destroy",    l_window_destroy},
			{"redraw",     l_window_redraw},
			{"swap",       l_window_swap},
			{"position",   l_window_position},
			{"resize",     l_window_resize},
			{"fullscreen", l_window_fullscreen},
			{"show",       l_window_show},
			{"hide",       l_window_hide},
			{"iconify",    l_window_iconify},
			{"title",      l_window_title},
			{"cursor",     l_window_cursor},
			{NULL, NULL}
		};
		l_registerFunctions(L, -1, reg);
	}
	lua_setmetatable(L, -2);

	// create callback table
	luaL_newmetatable(L, CALLBACKS_NAME);
	lua_newtable(L);
	lua_rawseti(L, -2, win->id);
	lua_pop(L, 1);

	return 1;
}

static int l_window___index(lua_State* L)
{
	luaL_getmetatable(L, INTERNAL_NAME);
	lua_pushvalue(L, 2);
	lua_gettable(L, -2);
	if (!lua_isnoneornil(L, -1))
		return 1;

	if (!lua_isstring(L, 2))
		return 0;
	const char* key = lua_tostring(L, 2);

	Window* win = l_toWindow(L, 1);
	if (get_callback(win->id, key))
		return 1;

	int current = glutGetWindow();
	glutSetWindow(win->id);
	if (0 == strcmp("pos", key))
	{
		lua_createtable(L, 2, 0);
		lua_pushinteger(L, glutGet(GLUT_WINDOW_X));
		lua_setfield(L, -2, "x");
		lua_pushinteger(L, glutGet(GLUT_WINDOW_Y));
		lua_setfield(L, -2, "y");
	}
	else if (0 == strcmp("x", key))
	{
		lua_pushinteger(L, glutGet(GLUT_WINDOW_X));
	}
	else if (0 == strcmp("y", key))
	{
		lua_pushinteger(L, glutGet(GLUT_WINDOW_Y));
	}
	else if (0 == strcmp("size", key))
	{
		lua_createtable(L, 2, 0);
		lua_pushinteger(L, glutGet(GLUT_WINDOW_WIDTH));
		lua_setfield(L, -2, "width");
		lua_pushinteger(L, glutGet(GLUT_WINDOW_HEIGHT));
		lua_setfield(L, -2, "height");
	}
	else if (0 == strcmp("width", key))
	{
		lua_pushinteger(L, glutGet(GLUT_WINDOW_WIDTH));
	}
	else if (0 == strcmp("height", key))
	{
		lua_pushinteger(L, glutGet(GLUT_WINDOW_HEIGHT));
	}
	else
	{
		lua_pushnil(L);
	}
	glutSetWindow(current);
	return 1;
}

static int l_window___newindex(lua_State* L)
{
	Window* win = l_toWindow(L, 1);
	const char* key = luaL_checkstring(L, 2);
	if (!lua_isfunction(L, 3))
		return luaL_typerror(L, 3, "function");

	if ((0 != strcmp("draw", key)) &&
	        (0 != strcmp("reshape", key)) &&
	        (0 != strcmp("update", key)) &&
	        (0 != strcmp("visibility", key)) &&
	        (0 != strcmp("keyboard", key)) &&
	        (0 != strcmp("mousereleased", key)) &&
	        (0 != strcmp("mousepressed", key)) &&
	        (0 != strcmp("mousedrag", key)) &&
	        (0 != strcmp("mousemove", key)) &&
	        (0 != strcmp("mouseleave", key)) &&
	        (0 != strcmp("mouseenter", key)))
	{
		return luaL_error(L, "Invalid key: %s", key);
	}

	// get callback registry for active window
	lua_pushstring(L, CALLBACKS_NAME);
	lua_rawget(L, LUA_REGISTRYINDEX);
	lua_pushinteger(L, win->id);
	lua_rawget(L, -2);

	// callbacks[win->id][key] = func
	lua_pushvalue(L, 2);
	lua_pushvalue(L, 3);
	lua_rawset(L, -3);
	return 0;
}

static int l_window___gc(lua_State* L)
{
	Window* win = l_checkWindow(L, 1);
	if (win->id > -1)
		l_window_destroy(L);
	return 0;
}

static int l_window_destroy(lua_State* L)
{
	Window* win = l_checkWindow(L, 1);
	glutDestroyWindow(win->id);
	win->id = -1;
	return 0;
}

static int l_window_redraw(lua_State* L)
{
	Window* win = l_checkWindow(L, 1);
	glutSetWindow(win->id);
	glutPostRedisplay();
	return 0;
}

static int l_window_swap(lua_State* L)
{
	Window* win = l_checkWindow(L, 1);
	glutSetWindow(win->id);
	glutSwapBuffers();
	return 0;
}

static int l_window_position(lua_State* L)
{
	Window* win = l_checkWindow(L, 1);
	int x = luaL_checkinteger(L, 2);
	int y = luaL_checkinteger(L, 3);

	glutSetWindow(win->id);
	glutPositionWindow(x, y);
	return 0;
}

static int l_window_resize(lua_State* L)
{
	Window* win = l_checkWindow(L, 1);
	int w = luaL_checkinteger(L, 2);
	int h = luaL_checkinteger(L, 3);

	glutSetWindow(win->id);
	glutReshapeWindow(w,h);
	return 0;
}

static int l_window_fullscreen(lua_State* L)
{
	Window* win = l_checkWindow(L, 1);
	glutSetWindow(win->id);
	glutFullScreen();
	return 0;
}

static int l_window_show(lua_State* L)
{
	Window* win = l_checkWindow(L, 1);
	glutSetWindow(win->id);
	glutShowWindow();
	return 0;
}

static int l_window_hide(lua_State* L)
{
	Window* win = l_checkWindow(L, 1);
	glutSetWindow(win->id);
	glutHideWindow();
	return 0;
}

static int l_window_iconify(lua_State* L)
{
	Window* win = l_checkWindow(L, 1);
	glutSetWindow(win->id);
	glutIconifyWindow();
	return 0;
}

static int l_window_title(lua_State* L)
{
	Window* win = l_checkWindow(L, 1);
	const char* title = luaL_checkstring(L, 2);

	glutSetWindow(win->id);
	glutSetWindowTitle(title);
	return 0;
}

static int l_window_cursor(lua_State* L)
{
	Window* win = l_checkWindow(L, 1);
	int cursor = luaL_checkinteger(L, 2);
	glutSetWindow(win->id);
	glutSetCursor(cursor);
	return 0;
}

static void _draw()
{
	if (get_callback(glutGetWindow(), "draw"))
		lua_call(LUA, 0, 0);
}

static void _reshape(int width, int height)
{
	if (!get_callback(glutGetWindow(), "reshape"))
		return;
	lua_pushinteger(LUA, width);
	lua_pushinteger(LUA, height);
	lua_call(LUA, 2, 0);
}

static void _update()
{
	int top = lua_gettop(LUA);

	lua_pushstring(LUA, CALLBACKS_NAME);
	lua_rawget(LUA, LUA_REGISTRYINDEX);
	lua_pushinteger(LUA,glutGetWindow());
	lua_rawget(LUA, -2);

	lua_pushstring(LUA, "__time");
	lua_rawget(LUA, -2);
	GLfloat lasttime = lua_tonumber(LUA, -1);

	GLfloat time = (GLfloat)glutGet(GLUT_ELAPSED_TIME) / 1000.;

	lua_pushstring(LUA, "update");
	lua_rawget(LUA, -3);
	if (lua_isfunction(LUA, -1))
	{
		lua_pushnumber(LUA, time - lasttime);
		lua_call(LUA, 1, 0);
	}
	else
	{
		lua_pop(LUA, 1);
	}

	lua_pushstring(LUA, "__time");
	lua_pushnumber(LUA, time);
	lua_rawset(LUA, -4);

	lua_settop(LUA, top);
}

static void _visibility(int state)
{
	if (!get_callback(glutGetWindow(), "visibility"))
		return;
	lua_pushboolean(LUA, GLUT_VISIBLE == state);
	lua_call(LUA, 1, 0);
}

static void _keyboard(unsigned char key, int x, int y)
{
	if (!get_callback(glutGetWindow(), "keyboard"))
		return;
	lua_pushlstring(LUA, (const char*)(&key), 1);
	lua_pushinteger(LUA, x);
	lua_pushinteger(LUA, y);
	lua_call(LUA, 3, 0);
}

static void _special(int key, int x, int y)
{
	if (!get_callback(glutGetWindow(), "keyboard"))
		return;

	const char* name;
	switch (key)
	{
	case GLUT_KEY_F1:
		name = "F1";
		break;
	case GLUT_KEY_F2:
		name = "F2";
		break;
	case GLUT_KEY_F3:
		name = "F3";
		break;
	case GLUT_KEY_F4:
		name = "F4";
		break;
	case GLUT_KEY_F5:
		name = "F5";
		break;
	case GLUT_KEY_F6:
		name = "F6";
		break;
	case GLUT_KEY_F7:
		name = "F7";
		break;
	case GLUT_KEY_F8:
		name = "F8";
		break;
	case GLUT_KEY_F9:
		name = "F9";
		break;
	case GLUT_KEY_F10:
		name = "F10";
		break;
	case GLUT_KEY_F11:
		name = "F11";
		break;
	case GLUT_KEY_F12:
		name = "F12";
		break;
	case GLUT_KEY_LEFT:
		name = "left";
		break;
	case GLUT_KEY_UP:
		name = "up";
		break;
	case GLUT_KEY_RIGHT:
		name = "right";
		break;
	case GLUT_KEY_DOWN:
		name = "down";
		break;
	case GLUT_KEY_PAGE_UP:
		name = "pageup";
		break;
	case GLUT_KEY_PAGE_DOWN:
		name = "pagedown";
		break;
	case GLUT_KEY_HOME:
		name = "home";
		break;
	case GLUT_KEY_END:
		name = "end";
		break;
	case GLUT_KEY_INSERT:
		name = "insert";
		break;
	default:
		name = "unknown";
	}

	lua_pushstring(LUA, name);
	lua_pushinteger(LUA, x);
	lua_pushinteger(LUA, y);
	lua_call(LUA, 3, 0);
}

static void _mouse(int button, int status, int x, int y)
{
	int has_callback;
	if (GLUT_UP == status)
		has_callback = get_callback(glutGetWindow(), "mousereleased");
	else
		has_callback = get_callback(glutGetWindow(), "mousepressed");

	if (!has_callback)
		return;

	const char* name;
	switch (button)
	{
	case GLUT_LEFT_BUTTON:
		name = "left";
		break;
	case GLUT_MIDDLE_BUTTON:
		name = "middle";
		break;
	case GLUT_RIGHT_BUTTON:
		name = "right";
		break;
	default:
		name = "unknown";
	}

	lua_pushinteger(LUA, x);
	lua_pushinteger(LUA, y);
	lua_pushstring(LUA, name);
	lua_call(LUA, 3, 0);

}

static void _motion(int x, int y)
{
	if (!get_callback(glutGetWindow(), "mousedrag"))
		return;
	// TODO: get mouse buttons
	lua_pushinteger(LUA, x);
	lua_pushinteger(LUA, y);
	lua_call(LUA, 2, 0);
}

static void _passiveMotion(int x, int y)
{
	if (!get_callback(glutGetWindow(), "mousemove"))
		return;

	lua_pushinteger(LUA, x);
	lua_pushinteger(LUA, y);
	lua_call(LUA, 2, 0);
}

static void _entry(int state)
{
	int has_callback;
	if (GLUT_LEFT == state)
		has_callback = get_callback(glutGetWindow(), "mouseleave");
	else
		has_callback = get_callback(glutGetWindow(), "mouseenter");

	if (has_callback)
		lua_call(LUA, 0, 0);
}
