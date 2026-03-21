if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE Release CACHE STRING "Build type" FORCE)
endif()
cmake_minimum_required(VERSION 4.0)
project(openGL VERSION 0.01 LANGUAGES C)

#my source files
set(SOURCES 
    src/main.c
    src/shader.c
    src/window.c
    src/engine.c
    src/camera.c
    src/implementations.c
    src/model.c
    src/primitive.c
)

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# send the openGL bins to ./bin
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/bin)

add_executable(${PROJECT_NAME} ${SOURCES})

# compiler settings
target_compile_features(${PROJECT_NAME} PRIVATE 
    c_std_99)
target_compile_options(${PROJECT_NAME} PRIVATE
        $<$<CONFIG:Release>:-O3 -march=native -ffast-math -funroll-loops -fomit-frame-pointer>
)
set_property(TARGET ${PROJECT_NAME} PROPERTY
    INTERPROCEDURAL_OPTIMIZATION_RELEASE TRUE
)
# my header files
target_include_directories(${PROJECT_NAME} 
    PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/include
)

# include glfw 
add_subdirectory(lib/glfw)

# include cglm and dont build its docs or tests
set(CGLM_STATIC ON CACHE INTERNAL "") 
add_subdirectory(lib/cglm)

# include glad
add_library(glad STATIC lib/glad/src/glad.c)
target_include_directories(glad PUBLIC lib/glad/include)

# include log.c
add_library(log.c STATIC lib/log.c/src/log.c)
target_include_directories(log.c PUBLIC lib/log.c/src)

# include cgltf
add_library(cgltf INTERFACE)
target_include_directories(cgltf INTERFACE lib/cgltf)

# include stb image
add_library(stb_image INTERFACE)
target_include_directories(stb_image INTERFACE lib/stb)

# setup libraries
target_link_libraries(${PROJECT_NAME}
    PRIVATE
    glfw
    glad
    cglm
    log.c
    cgltf
    stb_image
    ${CMAKE_DL_LIBS}
)

add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_directory
        "${CMAKE_SOURCE_DIR}/shaders"
        "${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/shaders"
    )

include (CTest)
enable_testing()