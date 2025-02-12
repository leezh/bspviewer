cmake_minimum_required(VERSION 3.28)
project(bspviewer LANGUAGES C CXX)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
include(FetchContent)

FetchContent_Declare(
    glad
    GIT_REPOSITORY https://github.com/Dav1dde/glad.git
    GIT_TAG v2.0.8
    GIT_SHALLOW ON
    EXCLUDE_FROM_ALL
    SYSTEM
)
FetchContent_Declare(
    glm
    GIT_REPOSITORY https://github.com/g-truc/glm.git
    GIT_TAG 1.0.1
    GIT_SHALLOW ON
    EXCLUDE_FROM_ALL
    SYSTEM
)
FetchContent_Declare(
    PhysicsFS
    GIT_REPOSITORY https://github.com/icculus/physfs.git
    GIT_TAG release-3.2.0
    GIT_SHALLOW ON
    EXCLUDE_FROM_ALL
    SYSTEM
)
FetchContent_Declare(
    SFML
    GIT_REPOSITORY https://github.com/SFML/SFML.git
    GIT_TAG 3.0.0
    GIT_SHALLOW ON
    EXCLUDE_FROM_ALL
    SYSTEM
)
FetchContent_MakeAvailable(
    glad
    glm
    PhysicsFS
    SFML
)
add_subdirectory("${glad_SOURCE_DIR}/cmake" glad_cmake)
glad_add_library(glad_gl REPRODUCIBLE API gl:core=3.3)

add_executable(bspviewer)
target_compile_features(bspviewer PRIVATE cxx_std_17)
target_sources(
    bspviewer PRIVATE
	src/main.cpp
	src/frutsum.hpp
	src/frutsum.cpp
	src/filestream.hpp
	src/filestream.cpp
	src/bsp.hpp
	src/bsp.cpp
	src/shaders.inc
)
target_compile_definitions(bspviewer PRIVATE
	GLM_FORCE_CXX17
	GLM_FORCE_SWIZZLE
)
target_link_libraries(bspviewer PRIVATE
	glm::glm
	SFML::System
	SFML::Window
	SFML::Graphics
	PhysFS::PhysFS-static
	glad_gl
)
