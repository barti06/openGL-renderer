#pragma once

#include <rtGL/window.h>
#include <rtGL/shader.h>
#include <rtGL/camera.h>

class Engine
{
public:
	Window window;
	Shader shader;
	Camera cam;

	glm::mat4 model = glm::mat4(1.0f);

	GLuint VBO, VAO;
	glm::vec2 winSizeF;
	vec2i winSizeI;

	void init();
	
	void draw();

	void run();

	void cleanup();
};