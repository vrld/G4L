#include <lua.h>
#include <lauxlib.h>
#include <lualib.h>

#include <glew.h>
#include <glut.h>
#include <string.h>

#include "window.h"
#include "helper.h"
#include "math.h"
#include "bufferobject.h"
#include "framebuffer.h"
#include "shader.h"
#include "image.h"
#include "texture.h"

static const char* TIMER_NAME = "G4L.timer";
static lua_State* LUA = NULL;

inline static int _getOptColor(lua_State* L, int idx, GLfloat *c)
{
	if (l_isvec4(L, idx))
	{
		vec4* c = (vec4*)lua_touserdata(L, idx);
		memcpy(c, c->v, 4 * sizeof(GLfloat));
		return 1;
	}
	c[0] = luaL_checknumber(L, idx);
	c[1] = luaL_checknumber(L, idx+1);
	c[2] = luaL_checknumber(L, idx+2);
	c[3] = luaL_optnumber(L, idx+3, 1.f);
	return 4;
}

static int is_init = 0;
static int l_initMode(lua_State* L)
{
	if (is_init)
		return luaL_error(L, "Already initialized.");

	unsigned int mode = 0;
	for (int i = 1; i <= lua_gettop(L); ++i)
		mode |= (unsigned int)luaL_checkinteger(L, i);

	glutInitDisplayMode(mode);

	is_init = 1;
	return 0;
}

static int is_running = 0;
static int l_run(lua_State* L)
{
	if (is_running)
		return luaL_error(L, "G4L already running.");

	is_running = 1;
	// TODO: This, but in a thread
	glutMainLoop();
	return 0;
}

static int l_viewport(lua_State* L)
{
	GLint   x = luaL_checkinteger(L, 1);
	GLint   y = luaL_checkinteger(L, 2);
	GLsizei w = luaL_checkinteger(L, 3);
	GLsizei h = luaL_checkinteger(L, 4);

	glGetError();
	glViewport(x,y,w,h);
	if (GL_NO_ERROR != glGetError())
		return luaL_error(L, "Invalid dimensions: %ux%u", w,h);
	return 0;
}

static int l_enable(lua_State* L)
{
	int top = lua_gettop(L);
	GLenum cap;
	for (int i = 1; i <= top; ++i)
	{
		cap = luaL_checkinteger(L, i);
		glGetError();
		glEnable(cap);
		if (GL_NO_ERROR != glGetError())
			return luaL_error(L, "Invalid capability");
	}
	return 0;
}

static int l_disable(lua_State* L)
{
	int top = lua_gettop(L);
	GLenum cap;
	for (int i = 1; i <= top; ++i)
	{
		cap = luaL_checkinteger(L, i);
		glGetError();
		glDisable(cap);
		if (GL_NO_ERROR != glGetError())
			return luaL_error(L, "Invalid capability");
	}
	return 0;
}

static int l_isEnabled(lua_State* L)
{
	int top = lua_gettop(L);
	GLenum cap;
	for (int i = 1; i <= top; ++i)
	{
		cap = luaL_checkinteger(L, i);
		glGetError();
		GLboolean enabled = glIsEnabled(cap);
		if (GL_NO_ERROR != glGetError())
			return luaL_error(L, "Invalid capability");
		lua_pushboolean(L, enabled);
	}
	return top;
}

static int l_clear(lua_State* L)
{
	GLenum c = 0;
	for (int i = 1; i <= lua_gettop(L); ++i)
		c |= luaL_checkinteger(L, i);
	if (lua_gettop(L) == 0)
		c = GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT | GL_STENCIL_BUFFER_BIT;

	glGetError();
	glClear(c);
	if (GL_NO_ERROR != glGetError())
		return luaL_error(L, "Invalid buffer mask");
	return 0;
}

static int l_clearColor(lua_State* L)
{
	GLfloat c[4] = {.0f};
	_getOptColor(L, 1, c);
	glClearColor(c[0], c[1], c[2], c[3]);
	return 0;
}

static int l_clearDepth(lua_State* L)
{
	GLfloat d = luaL_optnumber(L, 1, 1.);
	glClearDepth(d);
	return 0;
}

static int l_clearStencil(lua_State* L)
{
	GLint s = luaL_optinteger(L, 1, 0);
	glClearStencil(s);
	return 0;
}

static int l_blendFunc(lua_State* L)
{
	GLenum sRGB, dRGB, sAlpha, dAlpha;
	sRGB = luaL_checkinteger(L, 1);
	dRGB = luaL_checkinteger(L, 2);
	sAlpha = luaL_optinteger(L, 3, sRGB);
	dAlpha = luaL_optinteger(L, 4, dRGB);

	glGetError();
	glBlendFuncSeparate(sRGB, dRGB, sAlpha, dAlpha);
	if (GL_NO_ERROR != glGetError())
		return luaL_error(L, "Invalid blend mode");
	return 0;
}

static int l_blendEquation(lua_State* L)
{
	GLenum m = luaL_checkinteger(L, 1);
	glGetError();
	glBlendEquation(m);
	if (GL_NO_ERROR != glGetError())
		return luaL_error(L, "Invalid blend equation");
	return 0;
}

static int l_blendColor(lua_State* L)
{
	assert_extension(L, ARB_imaging);
	GLfloat c[4] = {.0f};
	_getOptColor(L, 1, c);
	glBlendColor(c[0], c[1], c[2], c[3]);
	return 0;
}

static int l_stencilFunc(lua_State* L)
{
	GLenum func = luaL_checkinteger(L, 1);
	GLint  ref  = luaL_optinteger(L, 2, 0);
	GLuint mask = luaL_optinteger(L, 3, (GLuint)(-1));

	glGetError();
	glStencilFunc(func, ref, mask);
	if (GL_NO_ERROR != glGetError())
		return luaL_error(L, "Invalid stencil function");
	return 0;
}

static int l_stencilOp(lua_State* L)
{
	GLenum sfail  = luaL_checkinteger(L, 1);
	GLenum dpfail = luaL_optinteger(L, 2, GL_KEEP);
	GLenum dppass = luaL_optinteger(L, 3, GL_KEEP);

	glGetError();
	glStencilOp(sfail, dpfail, dppass);
	if (GL_NO_ERROR != glGetError())
		return luaL_error(L, "Invalid stencil operation");
	return 0;
}

static void _glut_timer_cb(int ref)
{
	if (NULL == LUA) return;
	lua_State* L = LUA;
	int top = lua_gettop(L);

	lua_pushstring(L, TIMER_NAME);
	lua_rawget(L, LUA_REGISTRYINDEX);
	lua_rawgeti(L, -1, ref);
	lua_call(L, 0, 1);

	// if returns number, continue timer in <number> seconds
	// else -> stop timer
	if (lua_isnumber(L, -1))
	{
		unsigned int msecs = (unsigned int)(lua_tonumber(L, -1) * 1000.);
		glutTimerFunc(msecs, _glut_timer_cb, ref);
	}
	else
	{
		lua_rawseti(L, -2, ref);
	}

	lua_settop(L, top);
}

static int l_timer(lua_State* L)
{
	unsigned int msecs = (unsigned int)(luaL_checknumber(L, 1) * 1000.);
	if (!lua_isfunction(L, 2))
		return luaL_typerror(L, 2, "function");

	lua_pushstring(L, TIMER_NAME);
	lua_rawget(L, LUA_REGISTRYINDEX);
	lua_pushvalue(L, 2);
	int ref = luaL_ref(L, -2);
	lua_pushvalue(L, 2);
	lua_rawseti(L, -2, ref);

	glutTimerFunc(msecs, _glut_timer_cb, ref);

	return 0;
}

static int l_elapsed(lua_State* L)
{
	lua_Number elapsed = (lua_Number)glutGet(GLUT_ELAPSED_TIME) / 1000.;
	lua_pushnumber(L, elapsed);
	return 1;
}

static int l_readFile(lua_State* L)
{
	// try to read from file
	const char *path = luaL_checkstring(L, 1);
	FILE *fp;
	if (NULL == (fp = fopen(path, "rb")))
		return luaL_error(L, "Cannot open `%s' for reading", path);

	fseek(fp, 0, SEEK_END);
	size_t bytes = ftell(fp);
	fseek(fp, 0, SEEK_SET);

	char *buf = malloc(bytes);
	size_t read = fread(buf, 1, bytes, fp);
	fclose(fp);

	lua_pushlstring(L, buf, read);
	free(buf);

	if (read != bytes)
		return luaL_error(L, "Error reading `%s': Expected to read %lu byte,"\
				"got only %lu", path, bytes, read);

	return 1;
}

int luaopen_G4L(lua_State* L)
{
	if (LUA != NULL)
		return luaL_error(L, "G4L already opened");
	LUA = L;

	luaL_Reg reg[] =
	{
		// GLUT stuff
		{"initMode",       l_initMode},
		{"newWindow",      l_window_new},
		{"run",            l_run},
		{"timer",          l_timer},
		{"elapsed",        l_elapsed},

		// OpenGL stuff
		{"viewport",       l_viewport},
		{"enable",         l_enable},
		{"disable",        l_disable},
		{"isEnabled",      l_isEnabled},
		{"clear",          l_clear},
		{"clearColor",     l_clearColor},
		{"clearDepth",     l_clearDepth},
		{"clearStencil",   l_clearStencil},
		{"blendFunc",      l_blendFunc},
		{"blendEquation",  l_blendEquation},
		{"blendColor",     l_blendColor},
		{"stencilFunc",    l_stencilFunc},
		{"stencilOp",      l_stencilOp},

		// G4L stuff
		{"bufferobject",   l_bufferobject_new},
		{"framebuffer",    l_framebuffer_new},
		{"setFramebuffer", l_framebuffer_bind},
		{"shader",         l_shader_new},
		{"setShader",      l_shader_set},
		{"texture",        l_texture_new},
		{"image",          l_image_new},

		// util
		{"readFile",       l_readFile},

		{NULL, NULL}
	};

	l_constant_reg screen[] =
	{
		{"rgb",          GLUT_RGB},
		{"rgba",         GLUT_RGBA},
		{"index",        GLUT_INDEX},
		{"single",       GLUT_SINGLE},
		{"double",       GLUT_DOUBLE},
		{"alpha",        GLUT_ALPHA},
		{"depth",        GLUT_DEPTH},
		{"stencil",      GLUT_STENCIL},
		{"multisample",  GLUT_MULTISAMPLE},
		{"stereo",       GLUT_STEREO},
		{"luminance",    GLUT_LUMINANCE},
		{NULL, 0}
	};

	l_constant_reg flags[] =
	{
		{"blend",                      GL_BLEND},
		//{"clip_distance?",             GL_CLIP_DISTANCEi}, --> figure this one out
		{"color_logic_op",             GL_COLOR_LOGIC_OP}, // glLogicOp
		{"cull_face",                  GL_CULL_FACE}, // glCullFace
		{"depth_clamp",                GL_DEPTH_CLAMP}, // glDepthRange
		{"depth_test",                 GL_DEPTH_TEST},
		{"dither",                     GL_DITHER},
		{"line_smooth",                GL_LINE_SMOOTH}, // glLineWidth
		{"multisample",                GL_MULTISAMPLE}, // glSampleCoverage
		{"polygon_offset_fill",        GL_POLYGON_OFFSET_FILL}, // glPolygonOffset
		{"polygon_offset_line",        GL_POLYGON_OFFSET_LINE},
		{"polygon_offset_point",       GL_POLYGON_OFFSET_POINT},
		{"polygon_smooth",             GL_POLYGON_SMOOTH},
		{"primitive_restart",          GL_PRIMITIVE_RESTART}, // glPrimitiveRestartIndex
		{"sample_alpha_to_coverage",   GL_SAMPLE_ALPHA_TO_COVERAGE},
		{"sample_alpha_to_one",        GL_SAMPLE_ALPHA_TO_ONE},
		{"sample_coverage",            GL_SAMPLE_COVERAGE}, // glSampleCoverage
		{"scissor_test",               GL_SCISSOR_TEST}, // glScissor
		{"stencil_test",               GL_STENCIL_TEST},
		{"texture_cube_map_seamless",  GL_TEXTURE_CUBE_MAP_SEAMLESS},
		{"program_point_size",         GL_PROGRAM_POINT_SIZE},

		{NULL, 0}
	};

	l_constant_reg cursor[] =
	{
		{"right_arrow",          GLUT_CURSOR_RIGHT_ARROW},
		{"left_arrow",           GLUT_CURSOR_LEFT_ARROW},
		{"left_info",            GLUT_CURSOR_INFO},
		{"left_destroy",         GLUT_CURSOR_DESTROY},
		{"help",                 GLUT_CURSOR_HELP},
		{"cycle",                GLUT_CURSOR_CYCLE},
		{"spray",                GLUT_CURSOR_SPRAY},
		{"wait",                 GLUT_CURSOR_WAIT},
		{"text",                 GLUT_CURSOR_TEXT},
		{"crosshair",            GLUT_CURSOR_CROSSHAIR},
		{"up_down",              GLUT_CURSOR_UP_DOWN},
		{"left_right",           GLUT_CURSOR_LEFT_RIGHT},
		{"top_side",             GLUT_CURSOR_TOP_SIDE},
		{"bottom_side",          GLUT_CURSOR_BOTTOM_SIDE},
		{"left_side",            GLUT_CURSOR_LEFT_SIDE},
		{"right_side",           GLUT_CURSOR_RIGHT_SIDE},
		{"top_left_corner",      GLUT_CURSOR_TOP_LEFT_CORNER},
		{"top_right_corner",     GLUT_CURSOR_TOP_RIGHT_CORNER},
		{"bottom_right_corner",  GLUT_CURSOR_BOTTOM_RIGHT_CORNER},
		{"bottom_left_corner",   GLUT_CURSOR_BOTTOM_LEFT_CORNER},
		{"full_crosshair",       GLUT_CURSOR_FULL_CROSSHAIR},
		{"none",                 GLUT_CURSOR_NONE},
		{"inherit",              GLUT_CURSOR_INHERIT},
		{NULL, 0}
	};


	l_constant_reg buffer[] =
	{
		// target
		{"array_buffer",              GL_ARRAY_BUFFER},
		{"copy_read_buffer",          GL_COPY_READ_BUFFER},
		{"copy_write_buffer",         GL_COPY_WRITE_BUFFER},
		{"element_array_buffer",      GL_ELEMENT_ARRAY_BUFFER},
		{"pixel_pack_buffer",         GL_PIXEL_PACK_BUFFER},
		{"pixel_unpack_buffer",       GL_PIXEL_UNPACK_BUFFER},
		{"texture_buffer",            GL_TEXTURE_BUFFER},
		{"transform_feedback_buffer", GL_TRANSFORM_FEEDBACK_BUFFER},
		{"uniform_buffer",            GL_UNIFORM_BUFFER},

		// usage
		{"stream_draw",    GL_STREAM_DRAW},
		{"stream_read",    GL_STREAM_READ},
		{"stream_copy",    GL_STREAM_COPY},
		{"static_draw",    GL_STATIC_DRAW},
		{"static_read",    GL_STATIC_READ},
		{"static_copy",    GL_STATIC_COPY},
		{"dynamic_draw",   GL_DYNAMIC_DRAW},
		{"dynamic_read",   GL_DYNAMIC_READ},
		{"dynamic_copy",   GL_DYNAMIC_COPY},

		// type
		{"byte",   GL_BYTE},
		{"ubyte",  GL_UNSIGNED_BYTE},
		{"short",  GL_SHORT},
		{"ushort", GL_UNSIGNED_SHORT},
		{"int",    GL_INT},
		{"uint",   GL_UNSIGNED_INT},
		{"float",  GL_FLOAT},
		{"double", GL_DOUBLE},

		{NULL, 0}
	};

	l_constant_reg blend[] =
	{
		// glBlendFunc[Separate]
		{"zero",                      GL_ZERO},
		{"one",                       GL_ONE},
		{"src_color",                 GL_SRC_COLOR},
		{"one_minus_src_color",       GL_ONE_MINUS_SRC_COLOR},
		{"dst_color",                 GL_DST_COLOR},
		{"one_minus_dst_color",       GL_ONE_MINUS_DST_COLOR},
		{"src_alpha",                 GL_SRC_ALPHA},
		{"one_minus_src_alpha",       GL_ONE_MINUS_SRC_ALPHA},
		{"dst_alpha",                 GL_DST_ALPHA},
		{"one_minus_dst_alpha",       GL_ONE_MINUS_DST_ALPHA},
		{"constant_color",            GL_CONSTANT_COLOR},
		{"one_minus_constant_color",  GL_ONE_MINUS_CONSTANT_COLOR},
		{"constant_alpha",            GL_CONSTANT_ALPHA},
		{"one_minus_constant_alpha",  GL_ONE_MINUS_CONSTANT_ALPHA},
		{"src_alpha_saturate",        GL_SRC_ALPHA_SATURATE},

		// glBlendEquation
		{"add",                       GL_FUNC_ADD},
		{"subtract",                  GL_FUNC_SUBTRACT},
		{"reverse_subtract",          GL_FUNC_REVERSE_SUBTRACT},
		{"min",                       GL_MIN},
		{"max",                       GL_MAX},

		{NULL, 0}
	};

	l_constant_reg clear_bit[] =
	{
		{"color",     GL_COLOR_BUFFER_BIT},
		{"depth",     GL_DEPTH_BUFFER_BIT},
		{"stencil",   GL_STENCIL_BUFFER_BIT},
		{NULL, 0}
	};

	l_constant_reg stencil[] =
	{
		// stencil functions
		{"never",      GL_NEVER},
		{"less",       GL_LESS},
		{"lequal",     GL_LEQUAL},
		{"greater",    GL_GREATER},
		{"gequal",     GL_GEQUAL},
		{"equal",      GL_EQUAL},
		{"notequal",   GL_NOTEQUAL},
		{"always",     GL_ALWAYS},

		// stencil ops
		{"keep",       GL_KEEP},
		{"zero",       GL_ZERO},
		{"replace",    GL_REPLACE},
		{"incr",       GL_INCR},
		{"incr_wrap",  GL_INCR_WRAP},
		{"decr",       GL_DECR},
		{"decr_wrap",  GL_DECR_WRAP},
		{"invert",     GL_INVERT},

		{NULL, 0}
	};

	l_constant_reg draw_mode[] =
	{
		{"points",                   GL_POINTS},
		{"lines",                    GL_LINES},
		{"line_strip",               GL_LINE_STRIP},
		{"line_loop",                GL_LINE_LOOP},
		{"triangles",                GL_TRIANGLES},
		{"triangle_strip",           GL_TRIANGLE_STRIP},
		{"triangle_fan",             GL_TRIANGLE_FAN},

		{"lines_adjacency",          GL_LINES_ADJACENCY},
		{"line_strip_adjacency",     GL_LINE_STRIP_ADJACENCY},
		{"triangles_adjacency",      GL_TRIANGLES_ADJACENCY},
		{"triangle_strip_adjacency", GL_TRIANGLE_STRIP_ADJACENCY},

		{NULL, 0}
	};

	l_constant_reg texture_flags[] =
	{
		// wrap
		{"clamp_to_edge",   GL_CLAMP_TO_EDGE},
		{"clamp_to_border", GL_CLAMP_TO_BORDER},
		{"mirrored_repeat", GL_MIRRORED_REPEAT},
		{"repeat",          GL_REPEAT},

		// filter
		{"nearest",         GL_NEAREST},
		{"linear",          GL_LINEAR},

		{NULL, 0}
	};

	lua_newtable(L);
	l_registerFunctions(L, -1, reg);

	// sub modules
	luaopen_G4L_math(L);
	lua_setfield(L, -2, "math");

	// constants
	lua_newtable(L);
	l_registerConstants(L, -1, screen);
	lua_setfield(L, -2, "screen");

	lua_newtable(L);
	l_registerConstants(L, -1, flags);
	lua_setfield(L, -2, "flags");

	lua_newtable(L);
	l_registerConstants(L, -1, cursor);
	lua_setfield(L, -2, "cursor");

	lua_newtable(L);
	l_registerConstants(L, -1, buffer);
	lua_setfield(L, -2, "buffer");

	lua_newtable(L);
	l_registerConstants(L, -1, blend);
	lua_setfield(L, -2, "blend");

	lua_newtable(L);
	l_registerConstants(L, -1, clear_bit);
	lua_setfield(L, -2, "clear_bit");

	lua_newtable(L);
	l_registerConstants(L, -1, stencil);
	lua_setfield(L, -2, "stencil");

	lua_newtable(L);
	l_registerConstants(L, -1, texture_flags);
	lua_setfield(L, -2, "texture_flags");

	lua_newtable(L);
	l_registerConstants(L, -1, draw_mode);
	lua_setfield(L, -2, "draw_mode");


	// timer registry
	lua_newtable(L);
	lua_setfield(L, LUA_REGISTRYINDEX, TIMER_NAME);

	// init glut
	int argc = 0;
	char* argv[] = {NULL};
	glutInit(&argc, argv);

	return 1;
}
