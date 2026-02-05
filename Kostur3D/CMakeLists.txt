cmake_minimum_required(VERSION 3.16)
project(Kostur3D LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

add_executable(Kostur3D
    main.cpp
    Util.cpp
    Source/tiny_obj_loader.cpp
    Source/Globals.cpp
    Source/Camera.cpp
    Source/MeshBuilders.cpp
    Source/Renderer.cpp
)

target_include_directories(Kostur3D PRIVATE .)

if(APPLE)
    if(EXISTS /opt/homebrew)
        list(APPEND CMAKE_PREFIX_PATH "/opt/homebrew")
    endif()
    if(EXISTS /usr/local)
        list(APPEND CMAKE_PREFIX_PATH "/usr/local")
    endif()
endif()

find_package(OpenGL REQUIRED)
find_package(glfw3 REQUIRED)
find_package(GLEW REQUIRED)
find_package(glm REQUIRED)

target_include_directories(Kostur3D PRIVATE ${OPENGL_INCLUDE_DIRS})
target_link_libraries(Kostur3D PRIVATE OpenGL::GL glfw GLEW::GLEW glm::glm)

add_custom_command(TARGET Kostur3D POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
        ${CMAKE_SOURCE_DIR}/basic.vert
        $<TARGET_FILE_DIR:Kostur3D>/basic.vert
)

add_custom_command(TARGET Kostur3D POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
        ${CMAKE_SOURCE_DIR}/basic.frag
        $<TARGET_FILE_DIR:Kostur3D>/basic.frag
)

add_custom_command(TARGET Kostur3D POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_directory
        ${CMAKE_SOURCE_DIR}/res
        $<TARGET_FILE_DIR:Kostur3D>/res
)
