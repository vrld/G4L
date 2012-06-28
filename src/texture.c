#include "texture.h"
#include "helper.h"
#include "image.h"

#include <lua.h>
#include <lauxlib.h>
#include <assert.h>
#include <string.h>

static const char* INTERNAL_NAME = "G4L.texture";
static int unit_max = 0;

texture* l_checktexture(lua_State* L, int idx)
{
	return (texture*)luaL_checkudata(L, idx, INTERNAL_NAME);
}

int l_istexture(lua_State* L, int idx)
{
	if (NULL == lua_touserdata(L, idx))
		return 0;

	luaL_getmetatable(L, INTERNAL_NAME);
	lua_getmetatable(L, idx);
	int equal = lua_rawequal(L, -1, -2);
	lua_pop(L, 2);

	return equal;
}

static int l_texture___index(lua_State* L)
{
	lua_getmetatable(L, 1);
	lua_pushvalue(L, 2);
	lua_rawget(L, -2);
	if (!lua_isnoneornil(L, -1))
		return 1;

	texture* tex = (texture*)lua_touserdata(L, 1);
	const char* key = luaL_checkstring(L, 2);

	if (0 == strcmp(key, "unit"))
	{
		lua_pushinteger(L, tex->unit);
	}
	else
	{
		lua_pushnil(L);
	}

	return 1;
}

static int l_texture___newindex(lua_State* L)
{
	texture* tex = (texture*)lua_touserdata(L, 1);
	const char* key = luaL_checkstring(L, 2);

	if (0 == strcmp(key, "unit"))
	{
		int unit = luaL_checkinteger(L, 3);
		if (unit < 1 || unit >= unit_max)
			return luaL_error(L, "Invalid texture unit: %d", unit);

		tex->unit = unit;
		texture_bind(tex);
	}
	else
	{
		return luaL_error(L, "Cannot set property `%s'", key);
	}

	return 0;
}

static int l_texture_filter(lua_State* L)
{
	texture* tex = l_checktexture(L, 1);
	GLenum mag_filter = luaL_checkinteger(L, 2);
	GLenum min_filter = luaL_optinteger(L, 3, mag_filter);

	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_2D, tex->id);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, min_filter);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, mag_filter);

	lua_settop(L, 1);
	return 1;
}

static int l_texture_wrap(lua_State* L)
{
	texture* tex = l_checktexture(L, 1);
	GLenum wrap_s = luaL_checkinteger(L, 2);
	GLenum wrap_t = luaL_optinteger(L, 3, wrap_s);

	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_2D, tex->id);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, wrap_s);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, wrap_t);

	lua_settop(L, 1);
	return 1;
}

static int l_texture_setData(lua_State* L)
{
	texture* tex = l_checktexture(L, 1);
	image* img = l_checkimage(L, 2);
	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_2D, tex->id);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA,
	             (GLsizei)img->width, (GLsizei)img->height, 0,
	             GL_RGBA, GL_UNSIGNED_BYTE, img->data);

	lua_settop(L, 1);
	return 1;
}

static int l_texture___gc(lua_State* L)
{
	texture* tex = (texture*)lua_touserdata(L, 1);
	glDeleteTextures(1, &tex->id);
	return 0;
}

int l_texture_new(lua_State* L)
{
	glGetIntegerv(GL_MAX_COMBINED_TEXTURE_IMAGE_UNITS, &unit_max);

	GLsizei width, height;
	int idx_unit = 2;
	void* data = NULL;
	if (lua_isnumber(L, 1) && lua_isnumber(L, 2))
	{
		idx_unit++;
		width  = lua_tonumber(L, 1);
		height = lua_tonumber(L, 2);
		data   = NULL;
	}
	else
	{
		image* img = l_checkimage(L, 1);
		width  = img->width;
		height = img->height;
		data   = img->data;
	}

	int unit = luaL_optinteger(L, idx_unit, 1);
	if (unit < 1 || unit >= unit_max)
		return luaL_error(L, "Invalid texture unit: %d", unit);

	GLenum mag_filter = luaL_optinteger(L, idx_unit + 1, GL_LINEAR);
	GLenum min_filter = luaL_optinteger(L, idx_unit + 2, GL_LINEAR);
	GLenum wrap_s = luaL_optinteger(L, idx_unit + 3, GL_REPEAT);
	GLenum wrap_t = luaL_optinteger(L, idx_unit + 4, GL_REPEAT);

	GLuint id;
	glGenTextures(1, &id);

	glActiveTexture(GL_TEXTURE0 + unit);
	glBindTexture(GL_TEXTURE_2D, id);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, min_filter);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, mag_filter);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, wrap_s);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, wrap_t);

	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, width, height, 0,
	             GL_RGBA, GL_UNSIGNED_BYTE, data);

	texture* tex = (texture*)lua_newuserdata(L, sizeof(texture));
	tex->id = id;
	tex->unit = unit;

	if (luaL_newmetatable(L, INTERNAL_NAME))
	{
		luaL_reg meta[] =
		{
			{"__gc",       l_texture___gc},
			{"__index",    l_texture___index},
			{"__newinedx", l_texture___newindex},
			{"filter",     l_texture_filter},
			{"wrap",       l_texture_wrap},
			{"setData",    l_texture_setData},

			{NULL, NULL}
		};
		l_registerFunctions(L, -1, meta);
	}
	lua_setmetatable(L, -2);

	return 1;
}

void texture_bind(texture* tex)
{
	assert(NULL != tex);
	glActiveTexture(GL_TEXTURE0 + tex->unit);
	glBindTexture(GL_TEXTURE_2D, tex->id);
}
