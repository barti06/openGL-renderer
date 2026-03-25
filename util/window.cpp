#include <rtGL/window.h>

Window::Window()
	: m_window{ NULL }
{}

void Window::init(int width, int height, const char* windowName)
{
	// GLFW hint setting
	glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
	glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 6);
	glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);


	// create a GLFW window and check if it created correctly
	this->m_window = glfwCreateWindow(width, height, windowName, NULL, NULL);
	if (this->m_window == NULL)
	{
		const char* description;
		int errorCode = glfwGetError(&description);

		std::cerr << "GLFW Error Code: " << errorCode << std::endl;
		std::cerr << "GLFW Error Description: " << description << std::endl;

		glfwTerminate();
		throw std::runtime_error("Failed to create GLFW window");
	}
	this->width = width;
	this->height = height;
	// initialize lastX and lastY at the center of the window
	this->lastX = (float)width / 2;
	this->lastY = (float)height / 2;

	// give the GLFWwindow object a pointer to the window (this) 
	glfwSetWindowUserPointer(m_window, this);

	// set the cursor of the new window to the default os cursor
	m_cursor = glfwCreateStandardCursor(GLFW_POINTING_HAND_CURSOR);
	// make the new (if created) window the current glfw context
	glfwMakeContextCurrent(m_window);

	currentFrame = glfwGetTime();

	// set callbacks
	glfwSetKeyCallback(this->m_window, Window::key_callback);
	glfwSetCursorPosCallback(this->m_window, Window::mouse_pos_callback);
	glfwSetMouseButtonCallback(this->m_window, Window::mouse_button_callback);
}

GLFWwindow* Window::get() const 
{ 
	return this->m_window;
}

void Window::get_size(int& x, int& y)
{
	glfwGetWindowSize(this->m_window, &x, &y);
}


void Window::handleInput(Camera& cam)
{
	this->Update();

	if (this->isKeyPressed(GLFW_KEY_ESCAPE))
	{
		this->isPaused = !this->isPaused;
		this->setMovement(this->m_window);
	}
	
	if (!this->isPaused)
	{
		bool sprinting{ this->isKeyHeld(GLFW_KEY_LEFT_SHIFT)};
		if (this->m_keys_held[GLFW_KEY_W])
			cam.ProcessKeyboard(FORWARD, static_cast<float>(this->delta), sprinting);
		if (this->isKeyHeld(GLFW_KEY_S))
			cam.ProcessKeyboard(BACKWARD, static_cast<float>(this->delta), sprinting);
		if (this->isKeyHeld(GLFW_KEY_A))
			cam.ProcessKeyboard(LEFT, static_cast<float>(this->delta), sprinting);
		if (this->isKeyHeld(GLFW_KEY_D))
			cam.ProcessKeyboard(RIGHT, static_cast<float>(this->delta), sprinting);
		if (this->isKeyHeld(GLFW_KEY_SPACE))
			cam.ProcessKeyboard(UP, static_cast<float>(this->delta), sprinting);
		if (this->isKeyHeld(GLFW_KEY_LEFT_CONTROL))
			cam.ProcessKeyboard(DOWN, static_cast<float>(this->delta), sprinting);
	}

	cam.ProcessMouseMovement(this->m_xoffset, this->m_yoffset);
}

void Window::swap_interval(int turn) const
{
	glfwSwapInterval(turn);
}

void Window::set_custom_cursor(const char* location)
{
	// create a custom cursor
	int32_t cursorWidth{}, cursorHeight{}, cursorCh{};
	GLFWimage image{};
	// get the data for said cursor
	unsigned char* cursorData{ stbi_load(location, &cursorWidth, &cursorHeight, &cursorCh, 0) };
	if (cursorData)
	{
		// initialize the image with the loaded data
		image.width = cursorWidth;
		image.height = cursorHeight;
		image.pixels = cursorData;
		// reinitialize the object's cursor to the new image
		m_cursor = glfwCreateCursor(&image, 0, 0);
		// set the cursor
		glfwSetCursor(this->m_window, this->m_cursor);
	}
	else
	{
		// there's no need to handle extraneous cursor input since the default cursor
	    // gets initialized with the window anyway
		std::cerr << "COULDN'T LOAD CUSTOM CURSOR!\n";
	}
	stbi_image_free(cursorData);	
}

// so that there's no need to kill the window manually
void Window::cleanup()
{
	
	glfwDestroyCursor(this->m_cursor);
	glfwDestroyWindow(this->m_window);
	glfwTerminate();
}

void Window::key_callback(GLFWwindow* window, int key, int scancode, int action, int mods)
{
	// get the pointer to the window class from the GLFWwindow
	Window* self = static_cast<Window*>(glfwGetWindowUserPointer(window));
	if (!self)
		return;

	if (key < 0 || key > GLFW_KEY_LAST)
		return;

	if (action == GLFW_PRESS) {
		self->m_keys_pressed[key] = true;
		self->m_keys_held[key] = true;
	}
	else if (action == GLFW_RELEASE) {
		self->m_keys_released[key] = true;
		self->m_keys_held[key] = false;
	}

}

void Window::Update() 
{
	// get current frame
	this->currentFrame = glfwGetTime();
	// calculate delta time
	this->delta = this->currentFrame - this->lastFrame;
	this->lastFrame = this->currentFrame;

	// get fps per .5 seconds
	this->fpsFrameCount++;
	this->fpsTimer += this->delta;
	if (this->fpsTimer >= this->fpsRefresh) // update every 0.5 seconds
	{
		this->fps = this->fpsFrameCount / this->fpsTimer;

		// reset counters
		this->fpsFrameCount = 0;
		this->fpsTimer = 0.0;
	}

	// get window size each frame
	glfwGetWindowSize(this->m_window, &width, &height);

	this->m_xoffset = 0.0f;
	this->m_yoffset = 0.0f;

	memset(m_keys_pressed, 0, sizeof(m_keys_pressed));
	memset(m_keys_released, 0, sizeof(m_keys_released));

	// triggers callbacks
	glfwPollEvents();
}

void Window::mouse_pos_callback(GLFWwindow* window, double xposIn, double yposIn)
{
	// get the pointer to the window class from the GLFWwindow
	Window* self = static_cast<Window*>(glfwGetWindowUserPointer(window));
	if (!self)
		return;

	if (self->canMove)
	{
		float xpos = static_cast<float>(xposIn);
		float ypos = static_cast<float>(yposIn);

		if (self->firstMouse)
		{
			self->lastX = xpos;
			self->lastY = ypos;
			self->firstMouse = false;
		}

		self->m_xoffset = xpos - self->lastX;
		self->m_yoffset = self->lastY - ypos; // reversed since y-coordinates go from bottom to top

		self->lastX = xpos;
		self->lastY = ypos;
	}
}

void Window::mouse_button_callback(GLFWwindow* window, int button, int action, int mods)
{
	// get the pointer to the window class from the GLFWwindow
	Window* self = static_cast<Window*>(glfwGetWindowUserPointer(window));
	if (!self)
		return;
	
	if (glfwGetMouseButton(self->m_window, GLFW_MOUSE_BUTTON_RIGHT) == GLFW_PRESS)
	{
		glfwSetInputMode(self->m_window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);
		self->canMove = true;
	}
	else if (glfwGetMouseButton(window, GLFW_MOUSE_BUTTON_RIGHT) == GLFW_RELEASE)
	{
		glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_NORMAL);
		self->firstMouse = true;
		self->canMove = false;
	}
}

void Window::setMovement(GLFWwindow* window)
{
	Window* self = static_cast<Window*>(glfwGetWindowUserPointer(window));
	if (!self)
		return;


	if (!self->isPaused)
	{
		// when not lock the cursor and make it invisible
		glfwSetInputMode(self->m_window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);
		self->canMove = true;
	}
	else
	{
		// if not paused make it normal
		glfwSetInputMode(self->m_window, GLFW_CURSOR, GLFW_CURSOR_NORMAL);
		self->firstMouse = true;
		self->canMove = false;
	}
}