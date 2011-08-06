project (bspviewer C CXX)
cmake_minimum_required (VERSION 2.8)

set (CMAKE_RUNTIME_OUTPUT_DIRECTORY ${PROJECT_SOURCE_DIR})

set(QT_USE_QTOPENGL TRUE)
find_package(Qt4 REQUIRED)
find_package(OpenGL REQUIRED)
include(${QT_USE_FILE})

set(bspviewer_src
    src/frutsum.hpp
    src/frutsum.cpp
    src/bsp.hpp
    src/bsp.cpp
    src/glwidget.hpp
    src/glwidget.cpp
    src/main.cpp
)

QT4_WRAP_CPP(bspviewer_moc
    src/glwidget.hpp
)

QT4_ADD_RESOURCES(bspviewer_rcc
    
)

add_executable(bspviewer ${bspviewer_src} ${bspviewer_moc} ${bspviewer_rcc})
target_link_libraries(bspviewer ${QT_LIBRARIES} ${OPENGL_LIBRARIES})
