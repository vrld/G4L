#include "bufferobject.h"
#include "helper.h"

#include <lua.h>
#include <lauxlib.h>

#include <GL/glew.h>

#include <stdlib.h>

static const char* INTERNAL_NAME = "G4L.bufferobject";

bufferobject* l_checkbufferobject(lua_State* L, int idx)
{
	return (bufferobject*)luaL_checkudata(L, idx, INTERNAL_NAME);
}

static int l_bufferobject___gc(lua_State* L)
{
	bufferobject* b = lua_touserdata(L, 1);
	free(b->data);
	glDeleteBuffers(1, &(b->id));
	return 0;
}

static int l_bufferobject_push(lua_State* L)
{
	int top = lua_gettop(L);
	if (top == 1) return 1;

	bufferobject* b = l_checkbufferobject(L, 1);

	// check argument sanity
	for (int i = 2; i <= top; ++i) {
		if (b->record_size == 1) {
			luaL_checknumber(L, i);
		} else {
			if (!lua_istable(L, i))
				return luaL_typerror(L, i, "table");
			else if (b->record_size != (GLsizei)lua_objlen(L, i))
				return luaL_error(L, "Invalid record size in argument %d: expected %d, got %d",
						i, b->record_size, lua_objlen(L, i));
		}
	}

	// resize buffer if needed
	while (b->pos + (top-1) > b->max_size) {
		b->max_size *= 2;
		b->data = realloc(b->data, sizeof(GLfloat) * b->record_size * b->max_size);
	}

	// push arguments
	GLfloat* data = (GLfloat*)(b->data);
	size_t offset = b->pos * b->record_size;
	if (b->record_size == 1) {
		for (int i = 2; i <= top; ++i, ++b->pos, ++offset)
			data[offset] = lua_tonumber(L, -1);
		goto out;
	}

	for (int i = 2; i <= top; ++i, ++b->pos) {
		for (int k = 1; k <= b->record_size; ++k, ++offset) {
			lua_rawgeti(L, i, k);
			data[offset] = lua_tonumber(L, -1);
		}
		lua_pop(L, b->record_size);
	}

out:
	lua_settop(L, 1);
	return 1;
}

static int l_bufferobject_clear(lua_State* L)
{
	bufferobject* b = l_checkbufferobject(L, 1);
	b->pos = 0;
	lua_settop(L, 1);
	return 1;
}

static int l_bufferobject_finish(lua_State* L)
{
	bufferobject* b = l_checkbufferobject(L, 1);

	glGetError();
	glBindBuffer(GL_ARRAY_BUFFER, b->id);
	glBufferData(GL_ARRAY_BUFFER,
			sizeof(GLfloat) * b->pos * b->record_size,
			b->data, b->usage);
	GLenum err = glGetError();
	//glBindBuffer(GL_ARRAY_BUFFER, 0);

	if (GL_NO_ERROR != err)
		return luaL_error(L, "Unable to create data storage");

	lua_settop(L, 1);
	return 1;
}

int l_bufferobject_draw(lua_State* L)
{
	bufferobject* b = l_checkbufferobject(L, 1);
	GLenum mode = luaL_checkinteger(L, 2);

	glBindBuffer(GL_ARRAY_BUFFER, b->id);
	glGetError();
	glDrawArrays(mode, 0, b->pos);
	if (GL_INVALID_ENUM == glGetError())
		return luaL_error(L, "Invalid draw mode");
	return 0;
}

int l_bufferobject_new(lua_State* L)
{
	if (!context_available())
		return luaL_error(L, "No OpenGL context available. Create a window first.");

	assert_extension(L, ARB_vertex_buffer_object);

	GLsizei record_size  = luaL_optinteger(L, 1, 4);
	GLenum target        = luaL_optinteger(L, 2, GL_ARRAY_BUFFER);
	GLenum usage         = luaL_optinteger(L, 3, GL_STATIC_DRAW);
	GLsizei initial_size = luaL_optinteger(L, 4, 32);

	if (record_size <= 0)
		return luaL_error(L, "Invalid record size: %d", record_size);

	switch (target) {
		case GL_ARRAY_BUFFER:
		case GL_COPY_READ_BUFFER:
		case GL_COPY_WRITE_BUFFER:
		case GL_ELEMENT_ARRAY_BUFFER:
		case GL_PIXEL_PACK_BUFFER:
		case GL_PIXEL_UNPACK_BUFFER:
		case GL_TEXTURE_BUFFER:
		case GL_TRANSFORM_FEEDBACK_BUFFER:
		case GL_UNIFORM_BUFFER:
			break;
		default:
			return luaL_error(L, "Invalid buffer target");
	}

	switch (usage) {
		case GL_STREAM_DRAW:
		case GL_STREAM_READ:
		case GL_STREAM_COPY:
		case GL_STATIC_DRAW:
		case GL_STATIC_READ:
		case GL_STATIC_COPY:
		case GL_DYNAMIC_DRAW:
		case GL_DYNAMIC_READ:
		case GL_DYNAMIC_COPY:
			break;
		default:
			return luaL_error(L, "Invalid usage");
	}

	if (initial_size <= 0)
		return luaL_error(L, "Invalid initial size: %d", initial_size);

	bufferobject* b = (bufferobject*)lua_newuserdata(L, sizeof(bufferobject));
	if (NULL == b)
		return luaL_error(L, "out of memory");

	GLuint id;
	glGenBuffers(1, &id);

	b->id = id;
	b->target = target;
	b->usage = usage;
	b->max_size = initial_size;
	b->pos = 0;
	b->record_size = record_size;
	b->data = malloc(sizeof(GLfloat) * record_size * initial_size);

	if (luaL_newmetatable(L, INTERNAL_NAME)) {
		luaL_reg meta[] = {
			{"__gc",      l_bufferobject___gc},
			{"push",      l_bufferobject_push},
			{"clear",     l_bufferobject_clear},
			{"finish",    l_bufferobject_finish},
			{"draw",      l_bufferobject_draw},
			{NULL, NULL}
		};
		l_registerFunctions(L, -1, meta);
		lua_pushvalue(L, -1);
		lua_setfield(L, -1, "__index");
	}
	lua_setmetatable(L, -2);

	return 1;
}
