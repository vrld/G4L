cmake_minimum_required(VERSION 2.6)
project(G4L)

if(CMAKE_COMPILER_IS_GNUCC OR CMAKE_C_COMPILER MATCHES "(clang)")
	set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -Wall -Wextra -pedantic -Werror -std=c99")
elseif(CMAKE_BUILD_TOOL MATCHES "(msdev|devenv|nmake)")
	message(FATAL_ERROR "MSVC does not support C99.")
endif()

if(APPLE)
	set(CMAKE_FIND_FRAMEWORK LAST)
endif()

if(NOT WIN32) 
	set(CMAKE_SHARED_MODULE_PREFIX "")
	set(CMAKE_SHARED_MODULE_SUFFIX ".so")
endif()

find_package(Lua51  REQUIRED)
find_package(OpenGL REQUIRED)
find_package(GLUT   REQUIRED)
find_package(PNG    REQUIRED)

#find libjpeg-turbo
find_path(TJPEG_INCLUDE_DIR turbojpeg.h HINTS ${PNG_INCLUDE_DIR})
find_library(TJPEG_LIBRARY NAMES turbojpeg)

include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(TJPEG DEFAULT_MSG TJPEG_LIBRARY TJPEG_INCLUDE_DIR)

if(TJPEG_FOUND)
	set(TJPEG_LIBRARIES ${TJPEG_LIBRARY})
endif()

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

FIND_LIBRARY(GLEW_LIBRARIES
	NAMES GLEW
	PATH_SUFFIXES lib64 lib
	PATHS
		~/Library/Frameworks
		/Library/Frameworks
		/usr/local
		/usr
		/sw
		/opt/local
		/opt/csw
		/opt)
mark_as_advanced(GLEW_LIBRARIES)

if(NOT GLEW_INCLUDE_DIR OR NOT GLEW_LIBRARIES)
	message(FATAL_ERROR "Cannot find GLEW")
endif()

include_directories(
	${OPENGL_INCLUDE_DIR}
	${GLEW_INCLUDE_DIR}
	${GLUT_INCLUDE_DIR}
	${PNG_INCLUDE_DIR}
	${TJPEG_INCLUDE_DIR}
	${LUA_INCLUDE_DIR})

file(GLOB SOURCES src/*.c)
add_library(G4L SHARED ${SOURCES})
set_target_properties(G4L PROPERTIES PREFIX "")

target_link_libraries(G4L
	${LUA_LIBRARIES}
	${OPENGL_LIBRARIES}
	${GLUT_LIBRARIES}
	${GLEW_LIBRARIES}
	${PNG_LIBRARIES}
	${TJPEG_LIBRARIES})
