#include "shader.h"
#include "helper.h"

#include <lua.h>
#include <lauxlib.h>
#include <stdlib.h>

static const char* INTERNAL_NAME = "G4L.Shader";

shader* l_checkshader(lua_State* L, int idx)
{
	return (shader*)luaL_checkudata(L, idx, INTERNAL_NAME);
}

static int l_shader___gc(lua_State* L)
{
	shader* s = (shader*)lua_touserdata(L, 1);
	glDeleteProgram(s->id);
	return 0;
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
	if (!status) {
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
	if (!status) {

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
	if (!status) {
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

	if (luaL_newmetatable(L, INTERNAL_NAME)) {
		luaL_reg meta[] = {
			{"__gc",       l_shader___gc},
			{NULL, NULL}
		};
		l_registerFunctions(L, -1, meta);

		lua_pushvalue(L, -1);
		lua_setfield(L, -1, "__index");
	}
	lua_setmetatable(L, -2);

	return 1;
}
