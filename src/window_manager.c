#include "window_manager.h"
#include <raylib.h>

static Vector2 s_InputMousePosition = { 0.0f, 0.0f };

#ifdef _WIN32
typedef struct { long x; long y; } WindowInputPoint;
typedef struct { long left; long top; long right; long bottom; } WindowInputRect;
__declspec(dllimport) int __stdcall GetCursorPos(WindowInputPoint *point);
__declspec(dllimport) int __stdcall GetClientRect(void *window, WindowInputRect *rect);
__declspec(dllimport) int __stdcall ClientToScreen(void *window, WindowInputPoint *point);

static Vector2 Window_GetWindowsMappedMousePosition(void) {
    WindowInputPoint cursor = { 0, 0 };
    WindowInputRect client = { 0, 0, 0, 0 };
    void *window = GetWindowHandle();

    if (!GetCursorPos(&cursor) || !GetClientRect(window, &client)) {
        return GetMousePosition();
    }

    WindowInputPoint origin = { 0, 0 };
    if (!ClientToScreen(window, &origin)) {
        return GetMousePosition();
    }

    float clientWidth = (float)(client.right - client.left);
    float clientHeight = (float)(client.bottom - client.top);
    if (clientWidth <= 0.0f || clientHeight <= 0.0f) {
        return GetMousePosition();
    }

    return (Vector2){
        ((float)cursor.x - (float)origin.x) * (float)GetScreenWidth() / clientWidth,
        ((float)cursor.y - (float)origin.y) * (float)GetScreenHeight() / clientHeight
    };
}
#endif

bool Window_Init(const char* title, int width, int height) {
    // Enable 4x Multi-Sample Anti-Aliasing for the fixed-size demo window.
    SetConfigFlags(FLAG_MSAA_4X_HINT);
    InitWindow(width, height, title);
    SetTargetFPS(60);
    return IsWindowReady();
}

void Window_Shutdown(void) {
    CloseWindow();
}

bool Window_ShouldClose(void) {
    return WindowShouldClose();
}

void Window_BeginInputFrame(void) {
#ifdef _WIN32
    s_InputMousePosition = Window_GetWindowsMappedMousePosition();
#else
    s_InputMousePosition = GetMousePosition();
#endif
    SetMouseCursor(MOUSE_CURSOR_DEFAULT);
}

Vector2 Window_GetInputMousePosition(void) {
    return s_InputMousePosition;
}
