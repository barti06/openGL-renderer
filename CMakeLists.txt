cmake_minimum_required(VERSION 4.0)
project(openGL-renderer VERSION 0.01 LANGUAGES C CXX)

if(NOT CMAKE_C_COMPILER_ID STREQUAL CMAKE_CXX_COMPILER_ID)
    message(WARNING "ERROR... C COMPILER IS ${CMAKE_C_COMPILER_ID} AND CXX COMPILER IS ${CMAKE_CXX_COMPILER_ID}.")
endif()

if(NOT CMAKE_BUILD_TYPE AND NOT CMAKE_CONFIGURATION_TYPES)
    set(CMAKE_BUILD_TYPE Release CACHE STRING "Build type" FORCE)
    set_property(CACHE CMAKE_BUILD_TYPE PROPERTY STRINGS "Debug" "Release")
endif()

add_compile_definitions(
    $<$<CXX_COMPILER_ID:MSVC>:_CRT_SECURE_NO_WARNINGS>
    $<$<AND:$<C_COMPILER_ID:Clang>,$<PLATFORM_ID:Windows>>:_CRT_SECURE_NO_WARNINGS>
)

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

add_executable(${PROJECT_NAME} ${SOURCES})

set_target_properties(${PROJECT_NAME} PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY_RELEASE "${CMAKE_CURRENT_SOURCE_DIR}/bin/release"
    RUNTIME_OUTPUT_DIRECTORY_DEBUG   "${CMAKE_CURRENT_SOURCE_DIR}/bin/debug"
)
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
    src/texture_loader.c
    src/renderer.c
    src/ui.c
)

# use std C11
target_compile_features(${PROJECT_NAME} PRIVATE c_std_11)

# compiler options + optimizations 
target_compile_options(${PROJECT_NAME} PRIVATE
    # base config
    $<$<COMPILE_LANG_AND_ID:C,GNU,Clang,AppleClang>: -W>
    $<$<COMPILE_LANG_AND_ID:C,MSVC>:/W1>
    
    # release optimizations
    $<$<AND:$<CONFIG:Release>,$<COMPILE_LANG_AND_ID:C,GNU,Clang,AppleClang>>:
        -O3 -march=native -ffast-math -funroll-loops -fomit-frame-pointer>
    $<$<AND:$<CONFIG:Release>,$<COMPILE_LANG_AND_ID:C,MSVC>>:
        /O2 /fp:fast /GL>

    # debug anti optimizations
    $<$<AND:$<CONFIG:Debug>,$<COMPILE_LANG_AND_ID:C,GNU,Clang,AppleClang>>:
        -g -O0 -fno-omit-frame-pointer>
    $<$<AND:$<CONFIG:Debug>,$<COMPILE_LANG_AND_ID:C,MSVC>>:
        /ZI /Od /RTC1>
)

target_link_options(${PROJECT_NAME} PRIVATE
    $<$<AND:$<CONFIG:Debug>,$<COMPILE_LANG_AND_ID:C,MSVC>>:/DEBUG>
)

include(CheckIPOSupported)
check_ipo_supported(RESULT ipo_supported OUTPUT ipo_error)
if(ipo_supported)
    set_property(TARGET ${PROJECT_NAME} PROPERTY INTERPROCEDURAL_OPTIMIZATION_RELEASE TRUE)
else()
    message(STATUS "ERROR... INTERPROCEDULAR OPTIMIZATION NOT SUPPORTED: ${ipo_error}")
endif()

# my header files
target_include_directories(${PROJECT_NAME} 
    PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/include
)

# include glfw
set(GLFW_BUILD_EXAMPLES OFF CACHE INTERNAL "")
set(GLFW_BUILD_TESTS OFF CACHE INTERNAL "")
set(GLFW_BUILD_DOCS OFF CACHE INTERNAL "")
add_subdirectory(lib/glfw)

# include cglm
set(CGLM_STATIC ON CACHE INTERNAL "") 
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

# include cimgui
add_library(cimgui STATIC
    lib/cimgui/cimgui.cpp
    lib/cimgui/imgui/imgui.cpp
    lib/cimgui/imgui/imgui_draw.cpp
    lib/cimgui/imgui/imgui_tables.cpp
    lib/cimgui/imgui/imgui_widgets.cpp
    lib/cimgui/imgui/imgui_demo.cpp
    lib/cimgui/imgui/backends/imgui_impl_glfw.cpp
    lib/cimgui/imgui/backends/imgui_impl_opengl3.cpp
)
target_include_directories(cimgui PUBLIC
    lib/cimgui
    lib/cimgui/imgui
    lib/cimgui/imgui/backends
)
target_compile_definitions(cimgui PUBLIC
    CIMGUI_USE_GLFW
    CIMGUI_USE_OPENGL3
    IMGUI_IMPL_API=extern\ \"C\"
    IMGUI_DISABLE_DEMO_WINDOWS
)
set_target_properties(cimgui PROPERTIES
    CXX_STANDARD 17
    CXX_STANDARD_REQUIRED ON
)
target_link_libraries(cimgui PUBLIC glfw glad)

# setup libraries
target_link_libraries(${PROJECT_NAME} PRIVATE
    glfw
    glad
    cglm
    log_lib
    cgltf
    stb_image
    cimgui
    ${CMAKE_DL_LIBS}
)

if(UNIX AND NOT APPLE)
    target_link_libraries(${PROJECT_NAME} PRIVATE m)
endif()

if(WIN32)
    target_link_libraries(${PROJECT_NAME} PRIVATE comdlg32)
endif()

add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_directory
        "${CMAKE_CURRENT_SOURCE_DIR}/shaders"
        "$<TARGET_FILE_DIR:${PROJECT_NAME}>/shaders"
    )
