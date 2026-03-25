#ifndef WINDOW_CLASS_H
#define WINDOW_CLASS_H

#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include <util.h>
#include <stb/stb_image.h>
#include <rtGL/camera.h>


// window class is mostly made to apply RAII,since it proves useful in
// situations like this where i need to destroy the window after use
class Window
{
public:

	// store delta time (made public so that everyone can access it)
	double delta{ 0.0 };
	double fps{ 0.0 };
	float fpsRefresh{ 0.1 };

	int width{};
	int height{};

	bool isPaused{ true };
	// for setting glfw swap interval
	void swap_interval(int turn) const;

	//check for key presses and others
	inline bool isKeyPressed(int key) const
	{
		return key >= 0 && key <= GLFW_KEY_LAST && m_keys_pressed[key];
	}
	inline bool isKeyHeld(int key) const
	{
		return key >= 0 && key <= GLFW_KEY_LAST && m_keys_held[key];
	}
	inline bool isKeyReleased(int key) const
	{
		return key >= 0 && key <= GLFW_KEY_LAST && m_keys_released[key];
	}
	// handle keyboard input that need to be managed per frame
	void handleInput(Camera& cam);
	// just calls glfwpollevents and cleans up the input each frame
	void Update();

	// create an empty window object
	Window();
	// initialize a window with a given width and height
	void init(int width, int height, const char* windowName);

	// returns the raw window pointer
	GLFWwindow* get() const;

	void get_size(int& x, int& y);

	// sets a custom cursor based on a given image location
	void set_custom_cursor(const char* location);

	void setMovement(GLFWwindow* window);

	void cleanup();

private:
	GLFWwindow* m_window{};
	// every window object gets initialized to the default os cursor via constructor
	GLFWcursor* m_cursor{};
	
	// mouse state for the camera
	float lastX{};
	float lastY{};
	float m_xoffset{};
	float m_yoffset{};
	bool firstMouse{ true };
	bool canMove{ false };

	// for fps calculation
	double lastFrame{ 0.0 };
	double currentFrame{ 0.0 };
	int fpsFrameCount{ 0 };
	double fpsTimer{ 0.0 };

	

	// check for pressed keys
	bool m_keys_pressed[GLFW_KEY_LAST + 1] = {};
	bool m_keys_held[GLFW_KEY_LAST + 1] = {};
	bool m_keys_released[GLFW_KEY_LAST + 1] = {};
	// static callback function to let glfw handle some keyboard input mostly by itself
	static void key_callback(GLFWwindow* window, int key, int scancode, int action, int mods);
	// static callback function to let glfw handle mouse position mostly by itself
	static void mouse_pos_callback(GLFWwindow* window, double xposIn, double yposIn);
	// static callback function to let glfw handle mouse input
	static void mouse_button_callback(GLFWwindow* window, int button, int action, int mods);
};

#endif // !WINDOW_CLASS_H