#ifndef UI_H
#define UI_H

#define CIMGUI_DEFINE_ENUMS_AND_STRUCTS
#include <cimgui.h>
#include <cimgui_impl.h>
#include <GLFW/glfw3.h>

void ui_init(GLFWwindow* window);
void ui_begin(void);
void ui_end(void);
void ui_shutdown(void);

#endif
