cmake_minimum_required(VERSION 3.20)
# CMake 3.24+ permite renovar timestamps ao extrair dependencias.
# Versoes anteriores nao reconhecem DOWNLOAD_EXTRACT_TIMESTAMP.
if(POLICY CMP0135)
    cmake_policy(SET CMP0135 NEW)
endif()
project(Museu3D VERSION 1.0 LANGUAGES C)
set(CMAKE_C_STANDARD 99)
set(CMAKE_C_STANDARD_REQUIRED ON)
set(CMAKE_C_EXTENSIONS OFF)

find_package(OpenGL REQUIRED)
find_package(glfw3 3.4 QUIET)
if(NOT TARGET glfw)
    include(FetchContent)
    set(GLFW_BUILD_DOCS OFF CACHE BOOL "" FORCE)
    set(GLFW_BUILD_TESTS OFF CACHE BOOL "" FORCE)
    set(GLFW_BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)
    FetchContent_Declare(glfw
        URL https://github.com/glfw/glfw/archive/refs/tags/3.4.tar.gz)
    FetchContent_MakeAvailable(glfw)
endif()

add_executable(museu3d
    src/main.c
    src/museum.c
    src/camera.c
    src/building.c
    src/interaction.c
    src/geometry.c
    src/textures.c
    src/artwork_render.c
    src/scene.c
    src/self_test.c
    src/lighting.c
    src/balance.c
    src/exhibits.c)
target_include_directories(museu3d PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}/include")
add_custom_command(TARGET museu3d POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E make_directory "$<TARGET_FILE_DIR:museu3d>/assets"
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
        "${CMAKE_CURRENT_SOURCE_DIR}/assets/earth.ppm"
        "${CMAKE_CURRENT_SOURCE_DIR}/assets/CREDITS.md"
        "${CMAKE_CURRENT_SOURCE_DIR}/assets/THREEJS-LICENSE.txt"
        "$<TARGET_FILE_DIR:museu3d>/assets")
target_link_libraries(museu3d PRIVATE glfw OpenGL::GL OpenGL::GLU)
if(MSVC)
    target_compile_options(museu3d PRIVATE /W4)
else()
    target_compile_options(museu3d PRIVATE -Wall -Wextra -Wpedantic)
    target_link_libraries(museu3d PRIVATE m)
endif()
enable_testing()
add_test(NAME movement COMMAND museu3d --self-test)
add_test(NAME rendering COMMAND museu3d --smoke-test)
add_test(NAME rendering_unlit COMMAND museu3d --smoke-test --unlit)
