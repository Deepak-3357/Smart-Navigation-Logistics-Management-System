#ifndef WINDOW_MANAGER_H
#define WINDOW_MANAGER_H

#include <raylib.h>
#include <stdbool.h>

bool Window_Init(const char* title, int width, int height);
void Window_Shutdown(void);
bool Window_ShouldClose(void);
void Window_BeginInputFrame(void);
Vector2 Window_GetInputMousePosition(void);

#endif // WINDOW_MANAGER_H
