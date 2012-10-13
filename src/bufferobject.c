#include "bufferobject.h"
#include "helper.h"

#include <lua.h>
#include <lauxlib.h>

#include <glew.h>

#include <stdlib.h>
#include <string.h>

static const char* INTERNAL_NAME = "G4L.bufferobject";

static void fill_buffer_with_table(lua_State* L, bufferobject* b, int idx, int offset)
{
	int count = lua_objlen(L, idx);
	void* data = NULL;

#define _FILL(type)                         \
	b->element_size = sizeof(type);         \
	data = malloc(count * sizeof(type));    \
	if (NULL == data)                       \
		luaL_error(L, "Out of memory");     \
	for (int i = 1; i <= count; ++i) {      \
		lua_rawgeti(L, idx, i);             \
		lua_Number v = lua_tonumber(L, -1); \
		((type*)data)[i-1] = (type)v;       \
	}                                       \
	lua_pop(L, count)

	switch (b->element_type)
	{
	case GL_BYTE:
		_FILL(GLbyte);
		break;
	case GL_UNSIGNED_BYTE:
		_FILL(GLubyte);
		break;
	case GL_SHORT:
		_FILL(GLshort);
		break;
	case GL_UNSIGNED_SHORT:
		_FILL(GLushort);
		break;
	case GL_INT:
		_FILL(GLint);
		break;
	case GL_UNSIGNED_INT:
		_FILL(GLuint);
		break;
	case GL_FLOAT:
		_FILL(GLfloat);
		break;
	case GL_DOUBLE:
		_FILL(GLdouble);
		break;
	default:
		luaL_error(L, "Invalid data type");
	};
#undef _FILL

	int size_old = b->count * b->element_size;
	int size_new = count * b->element_size;
	offset = b->element_size;

	glBindBuffer(b->target, b->id);
	if (size_old >= size_new + offset)
	{
		// reuse old buffer
		glBufferSubData(b->target, offset, size_new, data);
	}
	else if (b->count == 0)
	{
		// this is a new buffer
		glBufferData(b->target, size_new, data, b->usage);
		b->count = count;
	}
	else
	{
		// new data extends buffer. bollux.
		void* new_data = malloc(size_new + offset);
		if (NULL == new_data)
		{
			free(data);
			luaL_error(L, "Out of memory");
		}

		// merge unchanged with new part
		void* old_data = NULL;
		glGetBufferSubData(b->target, 0, offset, old_data);
		memcpy(new_data, old_data, offset);
		memcpy((void*)((char*)new_data + offset), data, size_new);

		glBufferData(b->target, size_new + offset, new_data, b->usage);
		free(new_data);
		b->count = count;
	}
	free(data);
}

bufferobject* l_checkbufferobject(lua_State* L, int idx)
{
	return (bufferobject*)luaL_checkudata(L, idx, INTERNAL_NAME);
}

static int l_bufferobject___gc(lua_State* L)
{
	bufferobject* b = (bufferobject*)lua_touserdata(L, 1);
	glDeleteBuffers(1, &(b->id));
	return 0;
}

int l_bufferobject_draw(lua_State* L)
{
	bufferobject* b = l_checkbufferobject(L, 1);
	GLenum mode = luaL_checkinteger(L, 2);

	glBindBuffer(b->target, b->id);
	glGetError();
	switch (b->target)
	{
	case GL_ARRAY_BUFFER:
		glDrawArrays(mode, 0, b->count);
		break;
	case GL_ELEMENT_ARRAY_BUFFER:
		glDrawElements(mode, b->count, b->element_type, NULL);
		break;
	default:
		return luaL_error(L, "Not implemented");
	}
	if (GL_INVALID_ENUM == glGetError())
		return luaL_error(L, "Invalid draw mode");
	return 0;
}

static int l_bufferobject_update(lua_State* L)
{
	bufferobject* b = l_checkbufferobject(L, 1);
	int top = lua_gettop(L);
	int offset = 0;

	if (top > 2)
		offset = luaL_checkinteger(L, 2) - 1;

	while (GL_NO_ERROR != glGetError())
		/*clear error flags*/;

	fill_buffer_with_table(L, b, top, offset);

	if (GL_NO_ERROR != glGetError())
		return luaL_error(L, "Unable to create data storage");

	lua_settop(L, 1);
	return 1;
}

int l_bufferobject_new(lua_State* L)
{
	if (!context_available())
		return luaL_error(L, "No OpenGL context available. Create a window first.");

	assert_extension(L, ARB_vertex_buffer_object);

	GLenum target = GL_ARRAY_BUFFER;
	GLenum usage = GL_STATIC_DRAW;
	GLenum element_type = GL_FLOAT;

	int top = lua_gettop(L);
	if (!lua_istable(L, top))
		return luaL_typerror(L, top, "table");

	if (top > 1)
		target       = luaL_checkinteger(L, 1);
	if (top > 2)
		usage        = luaL_checkinteger(L, 2);
	if (top > 3)
		element_type = luaL_checkinteger(L, 3);

	switch (target)
	{
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

	switch (usage)
	{
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
		return luaL_error(L, "Invalid buffer usage");
	}

	bufferobject* b = (bufferobject*)lua_newuserdata(L, sizeof(bufferobject));
	if (NULL == b)
		return luaL_error(L, "Out of memory");

	GLuint id;
	glGenBuffers(1, &id);

	b->id = id;
	b->target = target;
	b->usage = usage;
	b->count = 0;
	b->element_type = element_type;

	while (GL_NO_ERROR != glGetError())
		/*clear error flags*/;

	fill_buffer_with_table(L, b, top, 0);

	if (GL_NO_ERROR != glGetError())
	{
		glDeleteBuffers(1, &(id));
		return luaL_error(L, "Unable to create data storage");
	}

	if (luaL_newmetatable(L, INTERNAL_NAME))
	{
		luaL_reg meta[] =
		{
			{"__gc",      l_bufferobject___gc},
			{"update",    l_bufferobject_update},
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
