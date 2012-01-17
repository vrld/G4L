#include "window.h"
#include "helper.h"

#include <lua.h>
#include <lauxlib.h>
#include <lualib.h>

#include <GL/glew.h>
#include <GL/glut.h>

#include <assert.h>
#include <string.h>

static const char* INTERNAL_NAME = "OpenGLua.Window";
static const char* CALLBACKS_NAME = "OpenGLua.callbacks";

static Window* active_window = NULL;
static int is_glew_init = 0;

// helper
#define set_active(win)\
	if (active_window != win)\
		glutSetWindow((active_window = (win))->id)

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

static inline int get_callback(Window* win, const char* name)
{
	lua_State* L = win->L;

	// get callback registry for active window
	lua_pushstring(L, CALLBACKS_NAME);
	lua_rawget(L, LUA_REGISTRYINDEX);
	lua_pushinteger(L, win->id);
	lua_rawget(L, -2);

	// get requested callback
	lua_pushstring(L, name);
	lua_rawget(L, -2);

	int is_function = lua_isfunction(L, -1);
	if (is_function) {
		// leave only function on stack
		lua_replace(L, -3);
		lua_pop(L, 1);
	} else {
		lua_pop(L, 3);
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
	win->L = L;
	set_active(win);

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

	if (!is_glew_init) {
		GLenum glew_status = glewInit();
		if (GLEW_OK != glew_status)
			return luaL_error(L, "Error initializing GLEW: %s\n",
								glewGetErrorString(glew_status));
		is_glew_init = 1;
	}

	if (luaL_newmetatable(L, INTERNAL_NAME)) {
		lua_pushcfunction(L, l_window___index);
		lua_setfield(L, -2, "__index");

		lua_pushcfunction(L, l_window___newindex);
		lua_setfield(L, -2, "__newindex");

		luaL_Reg reg[] = {
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

		lua_newtable(L);
		lua_setfield(L, LUA_REGISTRYINDEX, CALLBACKS_NAME);
	}
	lua_setmetatable(L, -2);

	// create callback table
	lua_getfield(L, LUA_REGISTRYINDEX, CALLBACKS_NAME);
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
	if (get_callback(win, key))
		return 1;

	Window* tmp = active_window;
	set_active(win);
	if (0 == strcmp("pos", key)) {
		lua_createtable(L, 2, 0);
		lua_pushinteger(L, glutGet(GLUT_WINDOW_X));
		lua_setfield(L, -2, "x");
		lua_pushinteger(L, glutGet(GLUT_WINDOW_Y));
		lua_setfield(L, -2, "y");
	} else if (0 == strcmp("x", key)) {
		lua_pushinteger(L, glutGet(GLUT_WINDOW_X));
	} else if (0 == strcmp("y", key)) {
		lua_pushinteger(L, glutGet(GLUT_WINDOW_Y));
	} else if (0 == strcmp("size", key)) {
		lua_createtable(L, 2, 0);
		lua_pushinteger(L, glutGet(GLUT_WINDOW_WIDTH));
		lua_setfield(L, -2, "width");
		lua_pushinteger(L, glutGet(GLUT_WINDOW_HEIGHT));
		lua_setfield(L, -2, "height");
	} else if (0 == strcmp("width", key)) {
		lua_pushinteger(L, glutGet(GLUT_WINDOW_WIDTH));
	} else if (0 == strcmp("height", key)) {
		lua_pushinteger(L, glutGet(GLUT_WINDOW_HEIGHT));
	} else {
		lua_pushnil(L);
	}
	set_active(tmp);
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
	    (0 != strcmp("mouseenter", key))) {
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
	win->L = NULL;
	return 0;
}

static int l_window_redraw(lua_State* L)
{
	Window* win = l_checkWindow(L, 1);
	set_active(win);
	glutPostRedisplay();
	return 0;
}

static int l_window_swap(lua_State* L)
{
	Window* win = l_checkWindow(L, 1);
	set_active(win);
	glutSwapBuffers();
	return 0;
}

static int l_window_position(lua_State* L)
{
	Window* win = l_checkWindow(L, 1);
	int x = luaL_checkinteger(L, 2);
	int y = luaL_checkinteger(L, 3);

	set_active(win);
	glutPositionWindow(x, y);
	return 0;
}

static int l_window_resize(lua_State* L)
{
	Window* win = l_checkWindow(L, 1);
	int w = luaL_checkinteger(L, 2);
	int h = luaL_checkinteger(L, 3);

	set_active(win);
	glutReshapeWindow(w,h);
	return 0;
}

static int l_window_fullscreen(lua_State* L)
{
	Window* win = l_checkWindow(L, 1);
	set_active(win);
	glutFullScreen();
	return 0;
}

static int l_window_show(lua_State* L)
{
	Window* win = l_checkWindow(L, 1);
	set_active(win);
	glutShowWindow();
	return 0;
}

static int l_window_hide(lua_State* L)
{
	Window* win = l_checkWindow(L, 1);
	set_active(win);
	glutHideWindow();
	return 0;
}

static int l_window_iconify(lua_State* L)
{
	Window* win = l_checkWindow(L, 1);
	set_active(win);
	glutIconifyWindow();
	return 0;
}

static int l_window_title(lua_State* L)
{
	Window* win = l_checkWindow(L, 1);
	const char* title = luaL_checkstring(L, 2);

	set_active(win);
	glutSetWindowTitle(title);
	return 0;
}

static int l_window_cursor(lua_State* L)
{
	Window* win = l_checkWindow(L, 1);
	int cursor = luaL_checkinteger(L, 2);
	set_active(win);
	glutSetCursor(cursor);
	return 0;
}

static void _draw()
{
	if (get_callback(active_window, "draw"))
		lua_call(active_window->L, 0, 0);
}

static void _reshape(int width, int height)
{
	if (!get_callback(active_window, "reshape"))
		return;
	lua_pushinteger(active_window->L, width);
	lua_pushinteger(active_window->L, height);
	lua_call(active_window->L, 2, 0);
}

static void _update()
{
	lua_State* L = active_window->L;

	lua_pushstring(L, CALLBACKS_NAME);
	lua_rawget(L, LUA_REGISTRYINDEX);
	lua_pushinteger(L, active_window->id);
	lua_rawget(L, -2);

	lua_pushstring(L, "__time");
	lua_rawget(L, -2);
	GLfloat lasttime = lua_tonumber(L, -1);

	GLfloat time     = (GLfloat)glutGet(GLUT_ELAPSED_TIME) / 1000.;

	lua_pushstring(L, "update");
	lua_rawget(L, -3);
	if (lua_isfunction(L, -1)) {
		lua_pushnumber(L, time - lasttime);
		lua_call(active_window->L, 1, 0);
	}

	lua_pushstring(L, "__time");
	lua_pushnumber(L, time);
	lua_rawset(L, -4);
	lua_pop(L, 2);
}

static void _visibility(int state)
{
	if (!get_callback(active_window, "visibility"))
		return;
	lua_pushboolean(active_window->L, GLUT_VISIBLE == state);
	lua_call(active_window->L, 1, 0);
}

static void _keyboard(unsigned char key, int x, int y)
{
	if (!get_callback(active_window, "keyboard"))
		return;
	lua_pushlstring(active_window->L, (const char*)(&key), 1);
	lua_pushinteger(active_window->L, x);
	lua_pushinteger(active_window->L, y);
	lua_call(active_window->L, 3, 0);
}

static void _special(int key, int x, int y)
{
	if (!get_callback(active_window, "keyboard"))
		return;

	const char* name;
	switch (key) {
		case GLUT_KEY_F1:        name = "F1"; break;
		case GLUT_KEY_F2:        name = "F2"; break;
		case GLUT_KEY_F3:        name = "F3"; break;
		case GLUT_KEY_F4:        name = "F4"; break;
		case GLUT_KEY_F5:        name = "F5"; break;
		case GLUT_KEY_F6:        name = "F6"; break;
		case GLUT_KEY_F7:        name = "F7"; break;
		case GLUT_KEY_F8:        name = "F8"; break;
		case GLUT_KEY_F9:        name = "F9"; break;
		case GLUT_KEY_F10:       name = "F10"; break;
		case GLUT_KEY_F11:       name = "F11"; break;
		case GLUT_KEY_F12:       name = "F12"; break;
		case GLUT_KEY_LEFT:      name = "left"; break;
		case GLUT_KEY_UP:        name = "up"; break;
		case GLUT_KEY_RIGHT:     name = "right"; break;
		case GLUT_KEY_DOWN:      name = "down"; break;
		case GLUT_KEY_PAGE_UP:   name = "pageup"; break;
		case GLUT_KEY_PAGE_DOWN: name = "pagedown"; break;
		case GLUT_KEY_HOME:      name = "home"; break;
		case GLUT_KEY_END:       name = "end"; break;
		case GLUT_KEY_INSERT:    name = "insert"; break;
		default:                 name = "unknown";
	}

	lua_pushstring(active_window->L, name);
	lua_pushinteger(active_window->L, x);
	lua_pushinteger(active_window->L, y);
	lua_call(active_window->L, 3, 0);
}

static void _mouse(int button, int status, int x, int y)
{
	int has_callback;
	if (GLUT_UP == status)
		has_callback = get_callback(active_window, "mousereleased");
	else
		has_callback = get_callback(active_window, "mousepressed");

	if (!has_callback)
		return;

	const char* name;
	switch (button) {
		case GLUT_LEFT_BUTTON:   name = "left"; break;
		case GLUT_MIDDLE_BUTTON: name = "middle"; break;
		case GLUT_RIGHT_BUTTON:  name = "right"; break;
		default:                 name = "unknown";
	}

	lua_pushinteger(active_window->L, x);
	lua_pushinteger(active_window->L, y);
	lua_pushstring(active_window->L, name);
	lua_call(active_window->L, 3, 0);

}

static void _motion(int x, int y)
{
	if (!get_callback(active_window, "mousedrag"))
		return;
	// TODO: get mouse buttons
	lua_pushinteger(active_window->L, x);
	lua_pushinteger(active_window->L, y);
	lua_call(active_window->L, 2, 0);
}

static void _passiveMotion(int x, int y)
{
	if (!get_callback(active_window, "mousemove"))
		return;

	lua_pushinteger(active_window->L, x);
	lua_pushinteger(active_window->L, y);
	lua_call(active_window->L, 2, 0);
}

static void _entry(int state)
{
	int has_callback;
	if (GLUT_LEFT == state)
		has_callback = get_callback(active_window, "mouseleave");
	else
		has_callback = get_callback(active_window, "mouseenter");

	if (has_callback)
		lua_call(active_window->L, 0, 0);
}
