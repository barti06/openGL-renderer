#include <engine.h>

void engine_init(Engine* engine)
{
    init_glfw(&engine->window);

    if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress))
	{
		log_error("\nError at Engine::init. glad couldn't load!\n");
	}

    glViewport(0, 0, 1600, 900);
	glEnable(GL_DEPTH_TEST);

    glEnable(GL_CULL_FACE);
    glfwSwapInterval(1);
    engine->swap_interv = true;

    // init shader
	shader_init(&engine->shader, "shaders/vertex.vert", "shaders/fragment.frag");

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
    if(is_key_pressed(&engine->window, GLFW_KEY_F5))
        shader_reload_frag(&engine->shader);
    if(is_key_pressed(&engine->window, GLFW_KEY_J))
    {
        engine->swap_interv = !engine->swap_interv;
        glfwSwapInterval(engine->swap_interv);
    }
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
        mat4 modelMat[16];
        for(int index = 0; index < engine->loaded; index++)
        {
            glm_mat4_identity(modelMat[index]);
            glm_translate(modelMat[index], (vec3){(float)index * 50.0f, 0.0f, 0.0f});
            glm_scale(modelMat[index], (vec3){0.1f, 0.1f, 0.1f});
            shader_set_mat4(&engine->shader, "u_model", modelMat[index]);
            model_draw(&engine->model, &engine->shader);
        }

        // send updated camera matrices to main shader
        shader_set_mat4(&engine->shader, "u_view", engine->camera.view);
        shader_set_mat4(&engine->shader, "u_projection", engine->camera.projection);
        shader_set_vec3(&engine->shader, "u_camera_position", engine->camera.position);

}

void engine_loop(Engine* engine)
{
    engine->loaded = 0;
    model_load(&engine->model, "resources/sponza/Sponza.gltf");
    
    while(!glfwWindowShouldClose(engine->window.ptr))
    {
        engine_updates(engine);
        
        engine_draw(engine);
        
        log_info("FPS: %lf", 1.0 / engine->delta_time);

        if(is_key_pressed(&engine->window, GLFW_KEY_F))
        {
            engine->loaded++;
        }

        window_update(&engine->window);

    }
    model_free(&engine->model);
}

void engine_cleanup(Engine* engine)
{
    glDeleteVertexArrays(1, &engine->VAO);
    glDeleteBuffers(1, &engine->VBO);
    window_cleanup(&engine->window);
}