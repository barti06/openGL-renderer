#ifndef ENGINE_H
#define ENGINE_H

#include "world.h"
#include "window.h"
#include "renderer.h"

typedef struct Engine
{
    Window window;
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

// main engine window loop
void engine_loop(Engine* engine);

// clean everything
void engine_cleanup(Engine* engine);
#endif
