#include "shader.h"
#include "helper.h"
#include "bufferobject.h"
#include "math.h"
#include "texture.h"

#include <lua.h>
#include <lauxlib.h>
#include <stdlib.h>
#include <string.h>

#ifdef _MSC_VER
#  include "float.h"
#  define INFINITY (DBL_MAX+DBL_MAX)
#  define NAN (INFINITY-INFINITY)
#else
#  define NAN (0./.0)
#endif

#ifndef APIENTRY
#  if defined(__MINGW32__) || defined(__CYGWIN__)
#    define APIENTRY __stdcall
#  elif (_MSC_VER >= 800) || defined(_STDCALL_SUPPORTED) || defined(__BORLANDC__)
#    define APIENTRY __stdcall
#  else
#    define APIENTRY
#  endif
#endif


static shader* active = NULL;

static const char* INTERNAL_NAME   = "G4L.Shader";
static const char* UNIFORMS_NAME   = "G4L.Shader.uniforms";
static const char* ATTRIBUTES_NAME = "G4L.Shader.attributes";

static GLint get_location(lua_State*L, shader* s, const char* name,
                          const char* registry,
                          GLint (APIENTRY *getter)(GLuint, const GLchar*))
{
	lua_pushstring(L, registry);
	lua_rawget(L, LUA_REGISTRYINDEX);
	lua_pushinteger(L, s->id);
	lua_rawget(L, -2);

	lua_pushstring(L, name);
	lua_rawget(L, -2);

	// known
	if (!lua_isnil(L, -1))
	{
		int location = lua_tointeger(L, -1);
		lua_pop(L, 3);
		return location;
	}

	// unknown: get and record it
	int location = getter(s->id, name);
	if (location == -1)
	{
		lua_pop(L, 3);
		return luaL_error(L, "`%s' not found. Maybe it's optimized out?", name);
	}

	lua_pushstring(L, name);
	lua_pushinteger(L, location);
	lua_rawset(L, -4);
	lua_pop(L, 3);
	return location;
}

#define get_uniform_location(L, s, name) \
	get_location(L, s, name, UNIFORMS_NAME, glGetUniformLocation)
#define get_attribute_location(L, s, name) \
	get_location(L, s, name, ATTRIBUTES_NAME, glGetAttribLocation)

shader* l_checkshader(lua_State* L, int idx)
{
	return (shader*)luaL_checkudata(L, idx, INTERNAL_NAME);
}

int l_shader_set(lua_State* L)
{
	shader* s = NULL;
	if (lua_isnoneornil(L,1))
	{
		glUseProgram(0);
	}
	else
	{
		s = l_checkshader(L, 1);
		glUseProgram(s->id);
	}
	active = s;
	return 0;
}

static int l_shader_enableAttribute(lua_State* L)
{
	shader* s = l_checkshader(L, 1);
	const char* name;
	GLint location;

	for (int i = 2; i <= lua_gettop(L); ++i)
	{
		name = luaL_checkstring(L, i);
		location = get_attribute_location(L, s, name);
		glEnableVertexAttribArray(location);
	}

	lua_settop(L, 1);
	return 1;
}

static int l_shader_disableAttribute(lua_State* L)
{
	shader* s = l_checkshader(L, 1);
	const char* name;
	GLint location;

	for (int i = 2; i <= lua_gettop(L); ++i)
	{
		name = luaL_checkstring(L, i);
		location = get_attribute_location(L, s, name);
		glDisableVertexAttribArray(location);
	}

	lua_settop(L, 1);
	return 1;
}

static int l_shader_bindAttribute(lua_State* L)
{
	shader* s           = l_checkshader(L, 1);
	const char* name    = luaL_checkstring(L, 2);
	bufferobject* b     = l_checkbufferobject(L, 3);
	int stride          = luaL_optint(L, 4, 1);
	int low             = luaL_optint(L, 5, 1);
	int high            = luaL_optint(L, 6, stride);
	GLboolean normalize = lua_toboolean(L, 7) ? GL_TRUE : GL_FALSE;

	GLint location      = get_attribute_location(L, s, name);

	while (low <= 0)
		low += stride + 1;

	while (high <= 0)
		high += stride + 1;

	int span = high - low + 1;
	if (span <= 0 || span > 4)
		return luaL_error(L, "Invalid range: [%d:%d]. Need 1-4 elements.", low, high);

	glBindBuffer(b->target, b->id);
	glVertexAttribPointer(location, span, b->element_type, normalize,
	                      b->element_size * stride,
	                      (GLvoid*)((char*)NULL + b->element_size * (low-1)));

	lua_settop(L, 1);
	return 1;
}

static int l_shader___gc(lua_State* L)
{
	shader* s = (shader*)lua_touserdata(L, 1);
	glDeleteProgram(s->id);
	return 0;
}

static int l_shader___index(lua_State* L)
{
	luaL_getmetatable(L, INTERNAL_NAME);
	lua_pushvalue(L, 2);
	lua_rawget(L, -2);
	if (!lua_isnil(L, 1))
		return 1;

	// get uniform value
	shader* s = (shader*)lua_touserdata(L, 1);
	const char* name = luaL_checkstring(L, 2);
	GLint location = get_uniform_location(L, s, name);
	// NaNNaNNaNNaNNaNNaNNaNNaNNaNNaNNaNNaNNaNNaNNaNNaN Batman!
	GLfloat params[16] = {NAN};
	glGetUniformfv(s->id, location, params);

	lua_createtable(L, 16, 0);
	for (int i = 0; i < 16; ++i)
	{
		lua_pushnumber(L, params[i]);
		lua_rawseti(L, -2, i+1);
	}
	return 1;
}

// set uniform value
static int l_shader___newindex(lua_State* L)
{
	shader* s = (shader*)lua_touserdata(L, 1);
	const char* name = luaL_checkstring(L, 2);
	GLint location = get_uniform_location(L, s, name);

	glUseProgram(s->id);

	if (lua_isnumber(L, 3))
	{
		glUniform1f(location, lua_tonumber(L, 3));
	}
	else if (l_isanyvec(L, 3))
	{
		vec4* v = (vec4*)lua_touserdata(L, 3);
		switch (v->dim)
		{
		case 2:
			glUniform2fv(location, 1, v->v);
			break;
		case 3:
			glUniform3fv(location, 1, v->v);
			break;
		case 4:
			glUniform4fv(location, 1, v->v);
			break;
		}
	}
	else if (l_isanymat(L, 3))
	{
		mat44* m = (mat44*)lua_touserdata(L, 3);
		switch (m->rows)
		{
		case 2:
			glUniformMatrix2fv(location, 1, GL_TRUE, m->m);
			break;
		case 3:
			glUniformMatrix3fv(location, 1, GL_TRUE, m->m);
			break;
		case 4:
			glUniformMatrix4fv(location, 1, GL_TRUE, m->m);
			break;
		}
	}
	else if (l_istexture(L, 3))
	{
		texture* tex = (texture*)lua_touserdata(L, 3);
		texture_bind(tex);
		glUniform1i(location, tex->unit);
	}
	else
	{
		if (NULL != active)
			glUseProgram(active->id);
		return luaL_error(L, "Cannot set value %s: Unknown type `%s'.",
		                  name, lua_typename(L, lua_type(L, 3)));
	}

	if (NULL != active)
		glUseProgram(active->id);
	return 0;
}

static int l_shader_warnings(lua_State* L)
{
	shader* s = (shader*)lua_touserdata(L, 1);

	GLint len = 0;
	glGetProgramiv(s->id, GL_INFO_LOG_LENGTH, &len);
	char* log = (char*)malloc(len+1);
	glGetProgramInfoLog(s->id, len, NULL, log);
	lua_pushlstring(L, log, len);
	free(log);

	return 1;
}

int l_shader_new(lua_State* L)
{
	if (!context_available())
		return luaL_error(L, "No OpenGL context available. Create a window first.");
	if (!lua_isstring(L, 1))
		return luaL_typerror(L, 1, "Vertex shader code");
	if (!lua_isstring(L, 2))
		return luaL_typerror(L, 2, "Fragment shader code");

	const char* vs_source = lua_tostring(L, 1);
	const char* fs_source = lua_tostring(L, 2);

	GLint status = GL_FALSE;

	GLuint vs = glCreateShader(GL_VERTEX_SHADER);
	glShaderSource(vs, 1, &vs_source, NULL);
	glCompileShader(vs);
	glGetShaderiv(vs, GL_COMPILE_STATUS, &status);
	if (!status)
	{
		GLint len = 0;
		glGetShaderiv(vs, GL_INFO_LOG_LENGTH, &len);
		char* log = (char*)malloc(len+1);
		glGetShaderInfoLog(vs, len, NULL, log);
		lua_pushlstring(L, log, len);
		free(log);

		glDeleteShader(vs);
		return luaL_error(L, "Cannot compile vertex shader:\n%s", lua_tostring(L, -1));
	}

	GLuint fs = glCreateShader(GL_FRAGMENT_SHADER);
	glShaderSource(fs, 1, &fs_source, NULL);
	glCompileShader(fs);
	glGetShaderiv(fs, GL_COMPILE_STATUS, &status);
	if (!status)
	{

		GLint len = 0;
		glGetShaderiv(fs, GL_INFO_LOG_LENGTH, &len);
		char* log = (char*)malloc(len+1);
		glGetShaderInfoLog(fs, len, NULL, log);
		lua_pushlstring(L, log, len);
		free(log);

		glDeleteShader(fs);
		glDeleteShader(vs);

		return luaL_error(L, "Cannot compile fragment shader:\n%s", lua_tostring(L, -1));
	}

	GLuint program = glCreateProgram();
	glAttachShader(program, vs);
	glAttachShader(program, fs);
	glLinkProgram(program);
	glGetProgramiv(program, GL_LINK_STATUS, &status);
	if (!status)
	{
		GLint len = 0;
		glGetProgramiv(program, GL_INFO_LOG_LENGTH, &len);
		char* log = (char*)malloc(len+1);
		glGetProgramInfoLog(program, len, NULL, log);
		lua_pushlstring(L, log, len);
		free(log);

		glDeleteProgram(program);
		glDeleteShader(fs);
		glDeleteShader(vs);

		return luaL_error(L, "Cannot link shader:\n%s", lua_tostring(L, -1));
	}

	glDeleteShader(fs);
	glDeleteShader(vs);

	shader* s = (shader*)lua_newuserdata(L, sizeof(shader));
	s->id = program;

	if (luaL_newmetatable(L, INTERNAL_NAME))
	{
		luaL_reg meta[] =
		{
			{"__gc",       l_shader___gc},
			{"__index",    l_shader___index},
			{"__newindex", l_shader___newindex},
			{"warnings",   l_shader_warnings},

			// attribute handling
			{"enableAttribute",   l_shader_enableAttribute},
			{"disableAttribute",  l_shader_disableAttribute},
			{"bindAttribute",     l_shader_bindAttribute},

			{NULL, NULL}
		};
		l_registerFunctions(L, -1, meta);
	}
	lua_setmetatable(L, -2);

	// create uniform registry
	luaL_newmetatable(L, UNIFORMS_NAME);
	lua_newtable(L);
	lua_rawseti(L, -2, s->id);
	lua_pop(L, 1);

	// create attribute registry
	luaL_newmetatable(L, ATTRIBUTES_NAME);
	lua_newtable(L);
	lua_rawseti(L, -2, s->id);
	lua_pop(L, 1);

	return 1;
}
