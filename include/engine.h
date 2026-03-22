#ifndef ENGINE_H
#define ENGINE_H

#include <shader.h>
#include <model.h>
#include <window.h>
#include <camera.h>
#include <utils.h>
#include "world.h"

typedef struct Engine
{
    Window window;
    Shader shader;
    Camera camera;

    Svec2f viewportSize;

    float last_time;
    float delta_time;
    float current_time;

    bool swap_interv;

    float nearZ;
    float farZ; 
    bool canMove;

    World* world;
    
} Engine;

// init the engine
void engine_init(Engine* engine);

// check for window key inputs
void engine_handleInput(Engine* engine);

// handles engine updates 
// (mouse movement, keyboard input, delta time, etc)
void engine_updates(Engine* engine);

// engine draw function (may be moved later on)
void engine_draw(Engine* engine);

// main engine window loop
void engine_loop(Engine* engine);

// clean everything
void engine_cleanup(Engine* engine);
#endif
