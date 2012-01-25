#include "vbo.h"
#include "helper.h"

#include <lua.h>
#include <lauxlib.h>

#include <GL/glew.h>

#include <stdlib.h>

static const char* INTERNAL_NAME = "G4L.VBO";

vbo* l_checkvbo(lua_State* L, int idx)
{
	return (vbo*)luaL_checkudata(L, idx, INTERNAL_NAME);
}

static int l_vbo___gc(lua_State* L)
{
	vbo* v = lua_touserdata(L, 1);
	free(v->data);
	glDeleteBuffers(1, &(v->id));
	return 0;
}

static int l_vbo_push(lua_State* L)
{
	int top = lua_gettop(L);
	if (top == 1) return 1;

	vbo* v = l_checkvbo(L, 1);

	// check argument sanity
	for (int i = 2; i <= top; ++i) {
		if (!lua_istable(L, i))
			return luaL_typerror(L, i, "table");
		if (v->record_size != (GLsizei)lua_objlen(L, i))
			return luaL_error(L, "Invalid record size in argument %d: expected %d, got %d",
					i, v->record_size, lua_objlen(L, i));
	}

	// resize buffer if needed
	while (v->pos + (top-1) > v->max_size) {
		v->max_size *= 2;
		v->data = realloc(v->data, sizeof(GLfloat) * v->record_size * v->max_size);
	}

	// push arguments
	GLfloat* data = (GLfloat*)(v->data);
	size_t offset = v->pos * v->record_size;
	for (int i = 2; i <= top; ++i, ++v->pos) {
		for (int k = 1; k <= v->record_size; ++k, ++offset) {
			lua_rawgeti(L, i, k);
			data[offset] = lua_tonumber(L, -1);
		}
		lua_pop(L, v->record_size);
	}

	lua_settop(L, 1);
	return 1;
}

static int l_vbo_clear(lua_State* L)
{
	vbo* v = l_checkvbo(L, 1);
	v->pos = 0;
	lua_settop(L, 1);
	return 1;
}

static int l_vbo_finish(lua_State* L)
{
	vbo* v = l_checkvbo(L, 1);

	glGetError();
	glBindBuffer(GL_ARRAY_BUFFER, v->id);
	glBufferData(GL_ARRAY_BUFFER,
			sizeof(GLfloat) * v->pos * v->record_size,
			v->data, v->usage);
	GLenum err = glGetError();
	//glBindBuffer(GL_ARRAY_BUFFER, 0);

	if (GL_NO_ERROR != err)
		return luaL_error(L, "Unable to create data storage");

	lua_settop(L, 1);
	return 1;
}

int l_vbo_draw(lua_State* L)
{
	vbo* v = l_checkvbo(L, 1);
	GLenum mode = luaL_checkinteger(L, 2);

	glBindBuffer(GL_ARRAY_BUFFER, v->id);
	glGetError();
	glDrawArrays(mode, 0, v->pos);
	if (GL_INVALID_ENUM == glGetError())
		return luaL_error(L, "Invalid draw mode");
	return 0;
}

int l_vbo_new(lua_State* L)
{
	if (!context_available())
		return luaL_error(L, "No OpenGL context available. Create a window first.");

	assert_extension(L, ARB_vertex_buffer_object);

	GLsizei record_size = luaL_optinteger(L, 1, 4);
	GLenum usage = luaL_optinteger(L, 2, GL_STATIC_DRAW);
	GLsizei initial_size = luaL_optinteger(L, 3, 32);

	if (initial_size <= 0)
		return luaL_error(L, "Invalid initial size: %d", initial_size);

	vbo* v = (vbo*)lua_newuserdata(L, sizeof(vbo));
	if (NULL == v)
		return luaL_error(L, "out of memory");

	GLuint id;
	glGenBuffers(1, &id);

	v->id = id;
	v->usage = usage;
	v->max_size = initial_size;
	v->pos = 0;
	v->record_size = record_size;
	v->data = malloc(sizeof(GLfloat) * record_size * initial_size);

	if (luaL_newmetatable(L, INTERNAL_NAME)) {
		luaL_reg meta[] = {
			{"__gc",      l_vbo___gc},
			{"push",      l_vbo_push},
			{"clear",     l_vbo_clear},
			{"finish",    l_vbo_finish},
			{"draw",      l_vbo_draw},
			{NULL, NULL}
		};
		l_registerFunctions(L, -1, meta);
		lua_pushvalue(L, -1);
		lua_setfield(L, -1, "__index");
	}
	lua_setmetatable(L, -2);

	return 1;
}
