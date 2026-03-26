#ifndef WINDOW_H
#define WINDOW_H

#include <GLFW/glfw3.h>
#include <stdbool.h>

typedef struct Window
{
    GLFWwindow* ptr;
    int32_t width;
    int32_t height;

    // check for pressed keys
	bool keys_pressed[GLFW_KEY_LAST + 1];
	bool keys_held[GLFW_KEY_LAST + 1];
	bool keys_released[GLFW_KEY_LAST + 1];
    
    bool mouse_buttons_pressed[GLFW_MOUSE_BUTTON_LAST + 1];
    bool mouse_buttons_held[GLFW_MOUSE_BUTTON_LAST + 1];
    bool mouse_buttons_released[GLFW_MOUSE_BUTTON_LAST + 1];

    bool is_paused;
    // mouse state for the camera
	float lastX;
	float lastY;
	float xoffset;
	float yoffset;
	bool first_mouse;

} Window;

void window_init(Window* window, int32_t w, int32_t h);

void window_update(Window* window);

void window_cleanup(Window* window);

void window_pause(Window* window);

void window_unpause(Window* window);

//check for key presses and others
static inline bool is_key_pressed(const Window* w, int key)
{
    return key >= 0 && key <= GLFW_KEY_LAST && w->keys_pressed[key];
}
static inline bool is_key_held(const Window* w, int key)
{
    return key >= 0 && key <= GLFW_KEY_LAST && w->keys_held[key];
}
static inline bool is_key_released(const Window* w, int key)
{
    return key >= 0 && key <= GLFW_KEY_LAST && w->keys_released[key];
}

// same but for buttons
static inline bool is_button_pressed(const Window* w, int button)
{
    return button >= 0 && button <= GLFW_MOUSE_BUTTON_LAST && w->mouse_buttons_pressed[button];
}
static inline bool is_button_held(const Window* w, int button)
{
    return button >= 0 && button <= GLFW_MOUSE_BUTTON_LAST && w->mouse_buttons_held[button];
}
static inline bool is_button_released(const Window* w, int button)
{
    return button >= 0 && button <= GLFW_MOUSE_BUTTON_LAST && w->mouse_buttons_released[button];
}


#endif
