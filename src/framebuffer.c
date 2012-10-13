#include "framebuffer.h"
#include "helper.h"
#include "texture.h"

#include <lua.h>
#include <lauxlib.h>
#include <string.h>

static const char* INTERNAL_NAME = "G4L.framebuffer";
static const char* ATTACHMENTS_NAME = "G4L.framebuffer.attachments";

// binds framebuffer `id' and returns previously bound framebuffer
inline static GLuint switch_framebuffer(GLuint id)
{
	GLint previd;
	glGetIntegerv(GL_DRAW_FRAMEBUFFER_BINDING, &previd);
	glBindFramebuffer(GL_FRAMEBUFFER, id);
	return (GLuint)previd;
}

#define WITH_FRAMEBUFFER_EXPAND2(id, LINE) for (GLuint __fbo##LINE = switch_framebuffer(id); __fbo##LINE; glBindFramebuffer(GL_FRAMEBUFFER, __fbo##LINE), __fbo##LINE = 0)
#define WITH_FRAMEBUFFER_EXPAND(id, LINE) WITH_FRAMEBUFFER_EXPAND2(id, LINE)
#define with_framebuffer(id) WITH_FRAMEBUFFER_EXPAND(id, __LINE__)

framebuffer* l_checkframebuffer(lua_State* L, int idx)
{
	return (framebuffer*)luaL_checkudata(L, idx, INTERNAL_NAME);
}

int l_isframebuffer(lua_State* L, int idx)
{
	if (NULL == lua_touserdata(L, idx))
		return 0;

	luaL_getmetatable(L, INTERNAL_NAME);
	lua_getmetatable(L, idx);
	int equal = lua_rawequal(L, -1, -2);
	lua_pop(L, 2);

	return equal;
}

static int l_framebuffer_add_attachment(lua_State* L)
{
	framebuffer* fbo = l_checkframebuffer(L, 1);
	int unit = luaL_optinteger(L, 2, 1);

	// generate new texture
	lua_pushcfunction(L, l_texture_new);
	lua_pushinteger(L, fbo->width);
	lua_pushinteger(L, fbo->height);
	lua_pushinteger(L, unit);
	lua_call(L, 3, 1);
	texture* tex = l_checktexture(L, -1);

	// register texture
	luaL_getmetatable(L, ATTACHMENTS_NAME);
	lua_rawgeti(L, -1, fbo->id);
	int attachment = lua_objlen(L, -1);
	lua_pushvalue(L, -3);
	lua_rawseti(L, -2, attachment+1);

	// attach texture
	with_framebuffer(fbo->id)
	{
		glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0 + attachment,
		                       GL_TEXTURE_2D, tex->id, 0);
	}

	// return texture object
	lua_pop(L, 2);
	return 1;
}

static int l_framebuffer_is_complete(lua_State* L)
{
	framebuffer* fbo = l_checkframebuffer(L, 1);
	GLenum status = GL_FRAMEBUFFER_COMPLETE;

	with_framebuffer(fbo->id)
	{
		status = glCheckFramebufferStatus(GL_FRAMEBUFFER);
	}

	int ok = GL_FRAMEBUFFER_COMPLETE == status;
	lua_pushboolean(L, ok);
	if (!ok)
	{
		switch (status)
		{
		case GL_FRAMEBUFFER_INCOMPLETE_ATTACHMENT:
			lua_pushliteral(L, "Framebuffer attachments are incomplete");
			break;
		case GL_FRAMEBUFFER_INCOMPLETE_MISSING_ATTACHMENT:
			lua_pushliteral(L, "Framebuffer does not have an image attachment");
			break;
		case GL_FRAMEBUFFER_INCOMPLETE_DRAW_BUFFER:
			lua_pushliteral(L, "Incomplete draw buffer");
			break;
		case GL_FRAMEBUFFER_INCOMPLETE_READ_BUFFER:
			lua_pushliteral(L, "Incomplete read buffer");
			break;
		case GL_FRAMEBUFFER_UNSUPPORTED:
			lua_pushliteral(L, "Combination of internal formats of attached images not supported by implementation");
			break;
		case GL_FRAMEBUFFER_INCOMPLETE_MULTISAMPLE:
			lua_pushliteral(L, "Incomplete multisample");
			// returned if the value of GL_RENDERBUFFER_SAMPLES is not the
			// same for all attached renderbuffers; if the value of
			// GL_TEXTURE_SAMPLES is the not same for all attached
			// textures; or, if the attached images are a mix of
			// renderbuffers and textures, the value of
			// GL_RENDERBUFFER_SAMPLES does not match the value of
			// GL_TEXTURE_SAMPLES.
			//
			// also returned if the value of
			// GL_TEXTURE_FIXED_SAMPLE_LOCATIONS is not the same for all
			// attached textures; or, if the attached images are a mix of
			// renderbuffers and textures, the value of
			// GL_TEXTURE_FIXED_SAMPLE_LOCATIONS is not GL_TRUE for all
			// attached textures.
			break;
		case GL_FRAMEBUFFER_INCOMPLETE_LAYER_TARGETS:
			lua_pushliteral(L, "At least one attachment is layered and one is not or all color attachments are not textures from the same target");
			break;
		}
	}
	return 1;
}

static int l_framebuffer___index(lua_State* L)
{
	lua_getmetatable(L, 1);
	lua_pushvalue(L, 2);
	lua_rawget(L, -2);
	if (!lua_isnoneornil(L, -1))
		return 1;

	framebuffer* fbo = (framebuffer*)lua_touserdata(L, 1);
	const char* key = luaL_checkstring(L, 2);

	if (0 == strcmp(key, "width"))
	{
		lua_pushinteger(L, fbo->width);
	}
	else if (0 == strcmp(key, "height"))
	{
		lua_pushinteger(L, fbo->height);
	}
	else if (0 == strcmp(key, "textures"))
	{
		luaL_getmetatable(L, ATTACHMENTS_NAME);
		lua_rawgeti(L, -1, fbo->id);
	}
	else
	{
		lua_pushnil(L);
	}

	return 1;
}

static int l_framebuffer___gc(lua_State* L)
{
	framebuffer* fbo = (framebuffer*)lua_touserdata(L, 1);

	luaL_getmetatable(L, ATTACHMENTS_NAME);
	lua_pushnil(L);
	lua_rawseti(L, -2, fbo->id);

	if (0 != fbo->renderbuffer)
		glDeleteRenderbuffers(1, &fbo->renderbuffer);
	glDeleteFramebuffers(1, &fbo->id);
	return 0;
}

int l_framebuffer_new(lua_State* L)
{
	int width = luaL_checkinteger(L, 1);
	int height = luaL_checkinteger(L, 2);

	GLuint id, renderbuffer = 0;
	glGenFramebuffers(1, &id);

	// make renderbuffer unless otherwise requested
	if (lua_isnone(L, 3) || lua_toboolean(L, 3))
	{
		glGenRenderbuffers(1, &renderbuffer);
		glBindRenderbuffer(GL_RENDERBUFFER, renderbuffer);
		glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH_STENCIL, width, height);
		glBindRenderbuffer(GL_RENDERBUFFER, 0);

		with_framebuffer(id)
		{
			glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_STENCIL_ATTACHMENT, GL_RENDERBUFFER, renderbuffer);
			glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT,   GL_RENDERBUFFER, renderbuffer);
		}
	}

	// the user data
	framebuffer* fbo = (framebuffer*)lua_newuserdata(L, sizeof(framebuffer));
	fbo->id = id;
	fbo->renderbuffer = renderbuffer;
	fbo->width = width;
	fbo->height = height;

	if (luaL_newmetatable(L, INTERNAL_NAME))
	{
		luaL_reg meta[] =
		{
			{"__gc",           l_framebuffer___gc},
			{"__index",        l_framebuffer___index},
			{"isComplete",     l_framebuffer_is_complete},
			{"addAttachment",  l_framebuffer_add_attachment},

			{NULL, NULL}
		};
		l_registerFunctions(L, -1, meta);
	}
	lua_setmetatable(L, -2);

	luaL_newmetatable(L, ATTACHMENTS_NAME);
	lua_newtable(L);
	lua_rawseti(L, -2, fbo->id);
	lua_pop(L, 1);

	return 1;
}

int l_framebuffer_bind(struct lua_State* L)
{
	if (lua_isnoneornil(L, 1))
	{
		glBindFramebuffer(GL_FRAMEBUFFER, 0);
		return 0;
	}

	framebuffer* fbo = l_checkframebuffer(L, 1);
	glBindFramebuffer(GL_FRAMEBUFFER, fbo->id);
	return 0;
}
