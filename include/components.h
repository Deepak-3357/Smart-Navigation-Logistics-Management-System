#ifndef COMPONENTS_H
#define COMPONENTS_H

#include <raylib.h>
#include <stdbool.h>

// Typography helpers using loaded Inter font family
void DrawTextRegular(const char* text, float x, float y, float size, Color color);
void DrawTextSemiBold(const char* text, float x, float y, float size, Color color);
void DrawTextBold(const char* text, float x, float y, float size, Color color);
void DrawTextRegularEx(const char* text, Vector2 pos, float size, float spacing, Color color);
void DrawTextSemiBoldEx(const char* text, Vector2 pos, float size, float spacing, Color color);
void DrawTextBoldEx(const char* text, Vector2 pos, float size, float spacing, Color color);

// Measure text dimensions using the given font and size
Vector2 MeasureWithFont(Font font, const char* text, float size);

// Draw text centered vertically in a rectangle
void DrawTextCenteredV(const char* text, Rectangle bounds, float size, float spacing, Color color, Font font);

// Toast Notification System
typedef enum {
    TOAST_SUCCESS = 0,
    TOAST_INFO,
    TOAST_WARNING,
    TOAST_ERROR
} ToastType;

void Toast_Show(const char* message, ToastType type);
void Toast_Update(float dt);
void Toast_Draw(void);

// Modal Dialog System
typedef enum {
    MODAL_INFO = 0,
    MODAL_SUCCESS,
    MODAL_WARNING,
    MODAL_CONFIRMATION,
    MODAL_ERROR
} ModalType;

typedef void (*ModalCallback)(bool confirmed);

void Modal_Show(const char* title, const char* message, ModalType type, ModalCallback callback);
void Modal_Update(void);
void Modal_Draw(void);
bool Modal_IsActive(void);

// Reusable Button Component
typedef void (*ButtonCallback)(void);
bool DrawUIButton(Rectangle bounds, const char* text, bool isActive, ButtonCallback callback);

// Reusable Card Component
void DrawUICard(Rectangle bounds, const char* title, const char* value, const char* subtext, Color accentColor);

// Reusable Table Component
void DrawUITable(Rectangle bounds, const char** headers, const float* columnWidths, int colCount, 
                 const char*** rows, int rowCount, bool isLoading, int* hoveredRow, 
                 int* selectedRow, int* sortCol, bool* sortAsc);

#endif // COMPONENTS_H
