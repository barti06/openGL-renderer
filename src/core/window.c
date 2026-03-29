#include "window.h"
#include <log.h>
#include <string.h>

static void key_callback(GLFWwindow* window, int key, int scancode, int action, int mods);
static void mouse_button_callback(GLFWwindow* window, int button, int action, int mods);
static void mouse_pos_callback(GLFWwindow* window, double xposIn, double yposIn);

#define DEFAULT_WINDOW_WIDTH 1600
#define DEFAULT_WINDOW_HEIGHT 900

void window_init(Window* window, int32_t w, int32_t h)
{
    if (!glfwInit())
	{
		const char* error;
		int glfwErrorCode = glfwGetError(&error);
		log_error("Error at Engine::init. GLFW error %d with following motive:\n %s", glfwErrorCode, error);
        return;
	}

    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
	glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 6);
	glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
	glfwWindowHint(GLFW_SAMPLES, 4);

	int32_t width, height;
	// if argument width or height was not set fallback to default 1600x900
	if(!w || !h)
	{
		width = DEFAULT_WINDOW_WIDTH;
		height = DEFAULT_WINDOW_HEIGHT;
	}
	else 
	{
		width = w;
		height = h;
	}

	// create a GLFW window and check if it created correctly
	window->ptr = glfwCreateWindow(width, height, "HOLA", NULL, NULL);
	if (window->ptr == NULL)
	{
		const char* description;
		int errorCode = glfwGetError(&description);

		log_error("GLFW Error Code and description: %d\n%s", errorCode, description);

		glfwTerminate();
	}
    window->width = width;
    window->height = height;

    glfwMakeContextCurrent(window->ptr);

    glfwSetWindowUserPointer(window->ptr, window);

    glfwSetKeyCallback(window->ptr, key_callback);
	glfwSetCursorPosCallback(window->ptr, mouse_pos_callback);
	glfwSetMouseButtonCallback(window->ptr, mouse_button_callback);

	window->is_paused = true;
	window->first_mouse = true;
}

void key_callback(GLFWwindow* window, int key, int scancode, int action, int mods)
{
	// get the pointer to the window struct from the GLFWwindow
	Window* self = (glfwGetWindowUserPointer(window));
	if (!self)
		return;

	if (key < 0 || key > GLFW_KEY_LAST)
		return;

	if (action == GLFW_PRESS) 
	{
		self->keys_pressed[key] = true;
		self->keys_held[key] = true;
	}
	else if (action == GLFW_RELEASE) 
	{
		self->keys_released[key] = true;
		self->keys_held[key] = false;
	}

}

void mouse_pos_callback(GLFWwindow* window, double xposIn, double yposIn)
{
	// get the pointer to the window class from the GLFWwindow
	Window* self = (glfwGetWindowUserPointer(window));
	if (!self)
		return;

	if (!self->is_paused)
	{
		float xpos = xposIn;
		float ypos = yposIn;

		if (self->first_mouse)
		{
			self->lastX = xpos;
			self->lastY = ypos;
			self->first_mouse = false;
		}

		self->xoffset = xpos - self->lastX;
		self->yoffset = self->lastY - ypos; // reversed since y-coordinates go from bottom to top

		self->lastX = xpos;
		self->lastY = ypos;
	}
}

void mouse_button_callback(GLFWwindow* window, int button, int action, int mods)
{
	// get the pointer to the window class from the GLFWwindow
	Window* self = (glfwGetWindowUserPointer(window));
	if (!self)
		return;
	
	if (button < 0 || button > GLFW_KEY_LAST)
		return;

	if(!self->is_paused)
	{
		if (action == GLFW_PRESS) 
		{
			self->mouse_buttons_pressed[button] = true;
			self->mouse_buttons_held[button] = true;
		}
		else if (action == GLFW_RELEASE) 
		{
			self->mouse_buttons_released[button] = true;
			self->mouse_buttons_held[button] = false;
		}
	}
}

void window_update(Window* window)
{
    // get window size at the end of each frame
	glfwGetFramebufferSize(window->ptr, &window->width, &window->height);

	// wipe memory on held keys and buttons
	memset(window->keys_pressed, 0, sizeof(window->keys_pressed));
	memset(window->keys_released, 0, sizeof(window->keys_released));
	memset(window->mouse_buttons_pressed, 0, sizeof(window->mouse_buttons_pressed));
	memset(window->mouse_buttons_released, 0, sizeof(window->mouse_buttons_released));

	// restart mouse offset for camera
	window->xoffset = 0.0f;
    window->yoffset = 0.0f;

	// update glfw
    glfwPollEvents();
    glfwSwapBuffers(window->ptr);
}

void window_cleanup(Window* window)
{
    glfwDestroyWindow(window->ptr);
    glfwTerminate();
}

void window_pause(Window* window)
{
	window->is_paused = true;
	glfwSetInputMode(window->ptr, GLFW_CURSOR, GLFW_CURSOR_NORMAL);
}

void window_unpause(Window* window)
{
	window->is_paused = false;
	glfwSetInputMode(window->ptr, GLFW_CURSOR, GLFW_CURSOR_DISABLED);
	window->first_mouse = true;
}
