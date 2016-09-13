cmake_minimum_required(VERSION 3.1)

if(NOT CMAKE_BUILD_TYPE)
	set(CMAKE_BUILD_TYPE Debug CACHE STRING "Choose the type of build (Debug or Release)" FORCE)
endif()

project(bspviewer CXX)

list(APPEND CMAKE_MODULE_PATH "${CMAKE_SOURCE_DIR}/cmake")
file(GLOB CMAKE_PREFIX_PATH "${PROJECT_SOURCE_DIR}/libs/*")

find_package(PhysFS REQUIRED)
find_package(OpenGL REQUIRED)
find_package(GLEW REQUIRED)
find_package(SFML 2 REQUIRED system window graphics)
find_path(GLM_INCLUDE_DIR glm/glm.hpp HINTS CMAKE_PREFIX_PATH)

set(bspviewer_src
	src/main.cpp
	src/frutsum.hpp
	src/frutsum.cpp
	src/filestream.hpp
	src/filestream.cpp
	src/bsp.hpp
	src/bsp.cpp
	src/shaders.inc
)

add_executable(bspviewer ${bspviewer_src})
target_compile_features(bspviewer PUBLIC
	cxx_raw_string_literals
	cxx_defaulted_move_initializers
)
target_include_directories(bspviewer PUBLIC
	${PHYSFS_INCLUDE_DIR}
	${GLEW_INCLUDE_DIRS}
	${SFML_INCLUDE_DIR}
	${GLM_INCLUDE_DIR}
	"${CMAKE_SOURCE_DIR}/libs"
)
target_link_libraries(bspviewer
	${PHYSFS_LIBRARY}
	${SFML_LIBRARIES}
	${GLEW_LIBRARIES}
	${OPENGL_LIBRARIES}
)
