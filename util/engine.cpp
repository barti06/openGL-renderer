#include <rtGL/engine.h>
#include <fmt/core.h>

void Engine::init()
{
	if (!glfwInit())
	{
		const char* error;
		int glfwErrorCode = glfwGetError(&error);
		fmt::println("Error at Engine::init. GLFW error {} with following motive:\n {}", glfwErrorCode, error);
	}

	window.init(1600, 900, "rtGL");
	window.swap_interval(1);
	if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress))
	{
		fmt::println("\nError at Engine::init. glad couldn't load!\n");
	}

	glViewport(0, 0, 1600, 900);
	glEnable(GL_DEPTH_TEST);

	float vertices[] = {
	-0.5f, -0.5f, 0.0f,
	 0.5f, -0.5f, 0.0f,
	 0.0f,  0.5f, 0.0f
	};

	// init shader
	shader.init(".vertex.vert", ".fragment.frag");

	// generate vertex objects
	glGenVertexArrays(1, &VAO);
	glGenBuffers(1, &VBO);
	// bind the vertex array then the vertex buffer
	glBindVertexArray(VAO);
	glBindBuffer(GL_ARRAY_BUFFER, VBO);
	// send the vertex data
	glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);
	glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), (void*)0);
	glEnableVertexAttribArray(0);
}

void Engine::draw()
{
	shader.use();
	float time = (float)glfwGetTime();
	shader.setFloat1("ola", time);
	model = glm::rotate(model, glm::radians(90.0f * (float)window.delta), glm::vec3(0.0f, 1.0f, 0.0f));
	shader.setMat4("model", model);
	glBindVertexArray(VAO);
	glDrawArrays(GL_TRIANGLES, 0, 3);
}

void Engine::run()
{
	while (!glfwWindowShouldClose(window.get()))
	{
		

		window.get_size(winSizeI.x, winSizeI.y);
		winSizeF.x = winSizeI.x;
		winSizeF.y = winSizeI.y;

		window.handleInput(cam);
		cam.update(winSizeF);
		shader.setMat4("view", cam.view);
		shader.setMat4("projection", cam.projection);
		
		glViewport(0, 0, winSizeI.x, winSizeI.y);
		glClearColor(0.2f, 0.3f, 0.3f, 1.0f);
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

		if (window.isKeyPressed(GLFW_KEY_F))
			fmt::print("Hello, world!\n");

		draw();		

		window.Update();
		glfwSwapBuffers(window.get());

		fmt::print("\n\n\nPosition:\n X = {}, Y = {}, Z = {}\n\n\n\n\n\n\n", cam.Position.x, cam.Position.y, cam.Position.z);
	}
}

void Engine::cleanup()
{
	this->window.cleanup();
}