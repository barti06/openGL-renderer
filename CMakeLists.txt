cmake_minimum_required(VERSION 4.0)
project(openGL-renderer VERSION 0.01 LANGUAGES C)

if(MSVC)
    add_definitions(-D_CRT_SECURE_NO_WARNINGS)
endif()

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# send the openGL bins to ./bin/xxx
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY_DEBUG   ${CMAKE_CURRENT_SOURCE_DIR}/bin/debug)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY_RELEASE ${CMAKE_CURRENT_SOURCE_DIR}/bin/release)

if(NOT CMAKE_RUNTIME_OUTPUT_DIRECTORY)
    string(TOLOWER "${CMAKE_BUILD_TYPE}" _cfg_lower)
    set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/bin/${_cfg_lower})
endif()

add_executable(${PROJECT_NAME} ${SOURCES})
#my source files
target_sources(${PROJECT_NAME} PRIVATE
    src/main.c
    src/shader.c
    src/window.c
    src/engine.c
    src/camera.c
    src/implementations.c
    src/model.c
    src/primitive.c
    src/world.c
    src/entity.c
    src/texture.c
    src/renderer.c
)

# use std C11
target_compile_features(${PROJECT_NAME} PRIVATE c_std_11)

# compiler flags
target_compile_options(${PROJECT_NAME} PRIVATE
    
    $<$<AND:$<CONFIG:Release>,$<COMPILE_LANG_AND_ID:C,GNU,Clang,AppleClang>>:
        -O3 -march=native -ffast-math -funroll-loops -fomit-frame-pointer>
    $<$<AND:$<CONFIG:Release>,$<COMPILE_LANG_AND_ID:C,MSVC>>:
        /O2 /fp:fast /GL>

    $<$<AND:$<CONFIG:Debug>,$<COMPILE_LANG_AND_ID:C,GNU,Clang,AppleClang>>:
        -g -O0 -fno-omit-frame-pointer>
    $<$<AND:$<CONFIG:Debug>,$<COMPILE_LANG_AND_ID:C,MSVC>>:
        /ZI /Od /RTC1>
)

target_link_options(${PROJECT_NAME} PRIVATE
    $<$<AND:$<CONFIG:Debug>,$<COMPILE_LANG_AND_ID:C,MSVC>>:/DEBUG>
)

set_property(TARGET ${PROJECT_NAME} PROPERTY
    INTERPROCEDURAL_OPTIMIZATION_RELEASE TRUE
)

# my header files
target_include_directories(${PROJECT_NAME} 
    PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/include
)

# include glfw
set(GLFW_BUILD_EXAMPLES OFF CACHE INTERNAL "")
set(GLFW_BUILD_TESTS OFF CACHE INTERNAL "")
set(GLFW_BUILD_DOCS OFF CACHE INTERNAL "")
add_subdirectory(lib/glfw)

# include cglm
set(CGLM_STATIC ON CACHE INTERNAL "") 
set(CMAKE_BUILD_TYPE Debug CACHE STRING "" FORCE)
add_subdirectory(lib/cglm)

# include glad
add_library(glad STATIC lib/glad/src/glad.c)
target_include_directories(glad PUBLIC lib/glad/include)

# include log.c
add_library(log_lib STATIC lib/log.c/src/log.c)
target_include_directories(log_lib PUBLIC lib/log.c/src)

# include cgltf
add_library(cgltf INTERFACE)
target_include_directories(cgltf INTERFACE lib/cgltf)

# include stb image
add_library(stb_image INTERFACE)
target_include_directories(stb_image INTERFACE lib/stb)

# setup libraries
target_link_libraries(${PROJECT_NAME} PRIVATE
    glfw
    glad
    cglm
    log_lib
    cgltf
    stb_image
    ${CMAKE_DL_LIBS}
)

if(UNIX AND NOT APPLE)
    target_link_libraries(${PROJECT_NAME} PRIVATE m)
endif()

add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_directory
        "${CMAKE_SOURCE_DIR}/shaders"
        "${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/shaders"
    )