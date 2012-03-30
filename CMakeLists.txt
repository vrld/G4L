cmake_minimum_required(VERSION 2.6)
project(G4L)

if(CMAKE_COMPILER_IS_GNUCC OR CMAKE_C_COMPILER MATCHES "(clang)")
	set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -Wall -Wextra -pedantic -Werror -std=c99")
elseif(CMAKE_BUILD_TOOL MATCHES "(msdev|devenv|nmake)")
	message(FATAL_ERROR "MSVC does not support C99.")
endif()

set(HAS_ERROR false)

find_package(Lua51)
find_package(OpenGL)
find_package(GLUT)
find_package(PNG)
find_package(JPEG)

if(NOT LUA51_FOUND)
	message(SEND_ERROR "Cannot find Lua")
	set(HAS_ERROR true)
endif(NOT LUA51_FOUND)

if(NOT OPENGL_FOUND)
	message(SEND_ERROR "Cannot find OpenGL")
	set(HAS_ERROR true)
endif(NOT OPENGL_FOUND)

#find GLEW
find_path(GLEW_INCLUDE_DIR glew.h
	HINTS ${OPENGL_INCLUDE_DIR}
	PATH_SUFFIXES include/gl include/GL include/OpenGL include
	PATHS
		~/Library/Frameworks
		/Library/Frameworks
		/usr/local
		/usr
		/sw
		/opt/local
		/opt/csw
		/opt)
mark_as_advanced(GLEW_INCLUDE_DIR)

if(NOT GLEW_INCLUDE_DIR)
	message(SEND_ERROR "Cannot find GLEW")
	set(HAS_ERROR true)
endif(NOT GLEW_INCLUDE_DIR)

if(NOT GLUT_FOUND)
	message(SEND_ERROR "Cannot find GLUT")
	set(HAS_ERROR true)
endif(NOT GLUT_FOUND)

if(NOT PNG_FOUND)
	message(SEND_ERROR "Cannot find libpng")
	set(HAS_ERROR true)
endif(NOT PNG_FOUND)

if(NOT JPEG_FOUND)
	message(SEND_ERROR "Cannot find libjpeg")
	set(HAS_ERROR true)
endif(NOT JPEG_FOUND)


if(NOT HAS_ERROR)
	include_directories(
		${OPENGL_INCLUDE_DIR}
		${GLEW_INCLUDE_DIR}
		${GLUT_INCLUDE_DIR}
		${PNG_INCLUDE_DIR}
		${JPEG_INCLUDE_DIR})

	file(GLOB SOURCES src/*.c)
	add_library(G4L ${SOURCES})

	target_link_libraries(G4L
		${OPENGL_LIBRARIES}
		${GLUT_LIBRARIES}
		${PNG_LIBRARIES}
		${JPEG_LIBRARIES})
endif(NOT HAS_ERROR)
