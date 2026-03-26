#ifndef ENGINE_H
#define ENGINE_H

#include "shader.h"
#include "window.h"
#include "world.h"
#include "renderer.h"

typedef struct Engine
{
    Window window;
    Shader shader;
    World* world;
    Renderer renderer;

    float last_time;
    float delta_time;
    float current_time;

    bool swap_interv;

    bool canMove;
} Engine;

// init the engine
void engine_init(Engine* engine, int argc, char* argv[]);

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
