#include "camera.h"
#include <engine.h>
#include <model.h>

void engine_init(Engine* engine)
{
    init_glfw(&engine->window);

    if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress))
	{
		log_error("\nError at Engine::init. glad couldn't load!\n");
	}

    glViewport(0, 0, 1600, 900);
	glEnable(GL_DEPTH_TEST);

    float vertices[] = 
    {
	    -0.5f, -0.5f, 0.0f,
	     0.5f, -0.5f, 0.0f,
	     0.0f,  0.5f, 0.0f
	};

    // init shader
	engine->shader = init_shader("shaders/vertex.vert", "shaders/fragment.frag");

	// generate vertex objects
	glGenVertexArrays(1, &engine->VAO);
	glGenBuffers(1, &engine->VBO);
	// bind the vertex array then the vertex buffer
	glBindVertexArray(engine->VAO);
	glBindBuffer(GL_ARRAY_BUFFER, engine->VBO);
	// send the vertex data
	glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);
	glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), (void*)0);
	glEnableVertexAttribArray(0);

    engine->camera = camera_init();
    engine->last_time = 0.0f; // init delta timing

    engine->nearZ = 0.1f;
    engine->farZ = 1000.0f;

    engine->canMove = false;
}

void engine_handleInput(Engine* engine)
{
    if(is_key_held(&engine->window, GLFW_KEY_W))
        camera_process_movement(&engine->camera, engine->delta_time, FORWARD);

    if(is_key_held(&engine->window, GLFW_KEY_A))
        camera_process_movement(&engine->camera, engine->delta_time, LEFT);

    if(is_key_held(&engine->window, GLFW_KEY_S))
        camera_process_movement(&engine->camera, engine->delta_time, BACKWARD);

    if(is_key_held(&engine->window, GLFW_KEY_D))
        camera_process_movement(&engine->camera, engine->delta_time, RIGHT);

    if(is_key_held(&engine->window, GLFW_KEY_SPACE))
        camera_process_movement(&engine->camera, engine->delta_time, UP);

    if(is_key_held(&engine->window, GLFW_KEY_LEFT_CONTROL))
        camera_process_movement(&engine->camera, engine->delta_time, DOWN);

    if(is_key_held(&engine->window, GLFW_KEY_LEFT_SHIFT))
        camera_process_movement(&engine->camera, engine->delta_time, SPRINT);

    camera_process_rotation(&engine->camera, engine->window.xoffset, engine->window.yoffset);

    if(is_key_pressed(&engine->window, GLFW_KEY_P))
        glfwSetWindowShouldClose(engine->window.ptr, true);
    if(is_key_pressed(&engine->window, GLFW_KEY_ESCAPE))
        engine->canMove = !engine->canMove;
} 

void engine_updates(Engine* engine)
{
    // delta time
    engine->current_time = glfwGetTime(); 
    engine->delta_time = engine->current_time - engine->last_time;
    engine->last_time = engine->current_time;

    engine_handleInput(engine);
    
    // set viewport size to full window size (for now at least)
    engine->viewportSize.x = engine->window.width;
    engine->viewportSize.y = engine->window.height;

    // update the camera
    camera_update_matrices(&engine->camera, 
        engine->viewportSize, 
        engine->nearZ, 
        engine->farZ);

    // update cursor
    if(engine->canMove)
    {
        glfwSetInputMode(engine->window.ptr, GLFW_CURSOR, GLFW_CURSOR_DISABLED);
        engine->window.is_paused = false;
    }
    else
    { 
        glfwSetInputMode(engine->window.ptr, GLFW_CURSOR, GLFW_CURSOR_NORMAL);
        engine->window.is_paused = true;
    }
}

void engine_draw(Engine* engine)
{
        glClearColor(0.2f, 0.3f, 0.3f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
        glViewport(0, 0, engine->viewportSize.x, engine->viewportSize.y);
        
        shader_use(&engine->shader);

        // set model matrix
        mat4 model;
        glm_mat4_identity(model);
        shader_set_mat4(&engine->shader, "model", model);

        // send updated camera matrices to main shader
        shader_set_mat4(&engine->shader, "view", engine->camera.view);
        shader_set_mat4(&engine->shader, "projection", engine->camera.projection);

        glBindVertexArray(engine->VAO);
        glDrawArrays(GL_TRIANGLES, 0, 3);
}

void engine_loop(Engine* engine)
{
    Model model;
    model_load(&model, "resources/DamagedHelmet.glb");
    while(!glfwWindowShouldClose(engine->window.ptr))
    {
        engine_updates(engine);
        
        engine_draw(engine);
        
        model_draw(&model, &engine->shader);

        window_update(&engine->window);

    }
    model_free(&model);
}

void engine_cleanup(Engine* engine)
{
    glDeleteVertexArrays(1, &engine->VAO);
    glDeleteBuffers(1, &engine->VBO);
    window_cleanup(&engine->window);
}