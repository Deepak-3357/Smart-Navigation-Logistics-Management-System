#include "components.h"
#include "theme_manager.h"
#include "common.h"
#include "window_manager.h"
#include <string.h>
#include <math.h>
#include <stdio.h>

// Toast notification state array
typedef struct {
    char message[128];
    ToastType type;
    float remainingTime;
    float slideX; // Animation state
    bool active;
} ToastState;

static ToastState s_Toasts[5] = {0};

// Modal dialog state
static struct {
    char title[64];
    char message[256];
    ModalType type;
    bool active;
    ModalCallback callback;
} s_Modal = {0};

// =============================================================
// Typography Helpers — Improved spacing and vertical centering
// =============================================================

// Compute letter spacing for a given font size
static inline float FontSpacing(float size) {
    return size * FONT_SPACING_FACTOR;
}

// Measure text width using the appropriate font
Vector2 MeasureWithFont(Font font, const char* text, float size) {
    if (font.texture.id > 0) {
        return MeasureTextEx(font, text, size, FontSpacing(size));
    }
    return (Vector2){ (float)MeasureText(text, (int)size), size };
}

void DrawTextRegular(const char* text, float x, float y, float size, Color color) {
    if (g_FontRegular.texture.id > 0) {
        DrawTextEx(g_FontRegular, text, (Vector2){ x, y }, size, FontSpacing(size), color);
    } else {
        DrawText(text, (int)x, (int)y, (int)size, color);
    }
}

void DrawTextSemiBold(const char* text, float x, float y, float size, Color color) {
    if (g_FontSemiBold.texture.id > 0) {
        DrawTextEx(g_FontSemiBold, text, (Vector2){ x, y }, size, FontSpacing(size), color);
    } else if (g_FontBold.texture.id > 0) {
        DrawTextEx(g_FontBold, text, (Vector2){ x, y }, size, FontSpacing(size), color);
    } else {
        DrawText(text, (int)x, (int)y, (int)size, color);
    }
}

void DrawTextBold(const char* text, float x, float y, float size, Color color) {
    if (g_FontBold.texture.id > 0) {
        DrawTextEx(g_FontBold, text, (Vector2){ x, y }, size, FontSpacing(size), color);
    } else {
        DrawText(text, (int)x, (int)y, (int)size, color);
    }
}

void DrawTextRegularEx(const char* text, Vector2 pos, float size, float spacing, Color color) {
    if (g_FontRegular.texture.id > 0) {
        DrawTextEx(g_FontRegular, text, pos, size, spacing, color);
    } else {
        DrawText(text, (int)pos.x, (int)pos.y, (int)size, color);
    }
}

void DrawTextSemiBoldEx(const char* text, Vector2 pos, float size, float spacing, Color color) {
    if (g_FontSemiBold.texture.id > 0) {
        DrawTextEx(g_FontSemiBold, text, pos, size, spacing, color);
    } else if (g_FontBold.texture.id > 0) {
        DrawTextEx(g_FontBold, text, pos, size, spacing, color);
    } else {
        DrawText(text, (int)pos.x, (int)pos.y, (int)size, color);
    }
}

void DrawTextBoldEx(const char* text, Vector2 pos, float size, float spacing, Color color) {
    if (g_FontBold.texture.id > 0) {
        DrawTextEx(g_FontBold, text, pos, size, spacing, color);
    } else {
        DrawText(text, (int)pos.x, (int)pos.y, (int)size, color);
    }
}

// Draw text centered both horizontally and vertically in a rectangle
void DrawTextCenteredV(const char* text, Rectangle bounds, float size, float spacing, Color color, Font font) {
    Vector2 textSize = MeasureTextEx(font, text, size, spacing);
    float x = bounds.x + (bounds.width - textSize.x) / 2.0f;
    float y = bounds.y + (bounds.height - textSize.y) / 2.0f;
    DrawTextEx(font, text, (Vector2){ x, y }, size, spacing, color);
}

// =============================================================
// Toast System Implementation
// =============================================================

void Toast_Show(const char* message, ToastType type) {
    for (int i = 0; i < 5; ++i) {
        if (!s_Toasts[i].active) {
            strncpy(s_Toasts[i].message, message, sizeof(s_Toasts[i].message) - 1);
            s_Toasts[i].type = type;
            s_Toasts[i].remainingTime = 3.0f;
            s_Toasts[i].slideX = 350.0f;
            s_Toasts[i].active = true;
            return;
        }
    }
    // If array is full, overwrite the first one
    strncpy(s_Toasts[0].message, message, sizeof(s_Toasts[0].message) - 1);
    s_Toasts[0].type = type;
    s_Toasts[0].remainingTime = 3.0f;
    s_Toasts[0].slideX = 350.0f;
    s_Toasts[0].active = true;
}

void Toast_Update(float dt) {
    for (int i = 0; i < 5; ++i) {
        if (s_Toasts[i].active) {
            s_Toasts[i].remainingTime -= dt;
            if (s_Toasts[i].remainingTime <= 0.0f) {
                s_Toasts[i].active = false;
            } else {
                // Smooth slide interpolation
                s_Toasts[i].slideX += (0.0f - s_Toasts[i].slideX) * 0.15f;
            }
        }
    }
}

void Toast_Draw(void) {
    int toastW = TOAST_WIDTH;
    int toastH = TOAST_HEIGHT;
    int spacing = TOAST_SPACING;
    int currentY = 20;

    for (int i = 0; i < 5; ++i) {
        if (!s_Toasts[i].active) continue;

        float drawX = GetScreenWidth() - toastW - 20 + s_Toasts[i].slideX;
        Rectangle toastRect = { drawX, (float)currentY, (float)toastW, (float)toastH };

        // Determine Accent Color based on toast type
        Color accentColor;
        const char* typeHeader;
        switch (s_Toasts[i].type) {
            case TOAST_SUCCESS:
                accentColor = g_Theme.success;
                typeHeader = "SUCCESS";
                break;
            case TOAST_WARNING:
                accentColor = g_Theme.warning;
                typeHeader = "WARNING";
                break;
            case TOAST_ERROR:
                accentColor = g_Theme.danger;
                typeHeader = "ERROR";
                break;
            case TOAST_INFO:
            default:
                accentColor = g_Theme.accentBlue;
                typeHeader = "INFO";
                break;
        }

        // Draw Card panel
        DrawRectangleRounded(toastRect, 0.15f, 4, g_Theme.bgSecondary);
        DrawRectangleRoundedLines(toastRect, 0.15f, 4, g_Theme.border);
        DrawRectangleRounded((Rectangle){ toastRect.x, toastRect.y, 5, toastRect.height }, 0.15f, 4, accentColor);

        // Toast text — vertically distribute header and message
        float startX = toastRect.x + 16.0f;
        float textBlockHeight = FONT_TOAST_HEADER + 6.0f + FONT_TOAST_MESSAGE;
        float topPadding = (toastH - textBlockHeight) / 2.0f;
        float titleY = toastRect.y + topPadding;
        float messageY = titleY + FONT_TOAST_HEADER + 6.0f;
        DrawTextBold(typeHeader, startX, titleY, FONT_TOAST_HEADER, accentColor);
        DrawTextRegular(s_Toasts[i].message, startX, messageY, FONT_TOAST_MESSAGE, g_Theme.textPrimary);

        currentY += toastH + spacing;
    }
}

// =============================================================
// Modal System Implementation
// =============================================================

void Modal_Show(const char* title, const char* message, ModalType type, ModalCallback callback) {
    strncpy(s_Modal.title, title, sizeof(s_Modal.title) - 1);
    strncpy(s_Modal.message, message, sizeof(s_Modal.message) - 1);
    s_Modal.type = type;
    s_Modal.callback = callback;
    s_Modal.active = true;
}

void Modal_Update(void) {
    if (!s_Modal.active) return;
    
    // Lock mouse cursor to pointer on modal
    SetMouseCursor(MOUSE_CURSOR_DEFAULT);
}

void Modal_Draw(void) {
    if (!s_Modal.active) return;

    // Draw full-screen overlay mask
    DrawRectangle(0, 0, GetScreenWidth(), GetScreenHeight(), ColorAlpha(BLACK, 0.5f));

    // Card frame
    int modalW = MODAL_WIDTH;
    int modalH = MODAL_HEIGHT;
    Rectangle modalRect = {
        (GetScreenWidth() - modalW) / 2.0f,
        (GetScreenHeight() - modalH) / 2.0f,
        (float)modalW,
        (float)modalH
    };

    DrawRectangleRounded(modalRect, 0.08f, 4, g_Theme.bgSecondary);
    DrawRectangleRoundedLines(modalRect, 0.08f, 4, g_Theme.border);

    // Decorator icon bar
    Color bannerColor;
    switch (s_Modal.type) {
        case MODAL_SUCCESS:
            bannerColor = g_Theme.success;
            break;
        case MODAL_WARNING:
            bannerColor = g_Theme.warning;
            break;
        case MODAL_ERROR:
            bannerColor = g_Theme.danger;
            break;
        case MODAL_CONFIRMATION:
            bannerColor = g_Theme.accentBlue;
            break;
        case MODAL_INFO:
        default:
            bannerColor = g_Theme.accentTeal;
            break;
    }

    // Modal title banner — 48px tall header
    float bannerH = 48.0f;
    DrawRectangleRounded((Rectangle){ modalRect.x, modalRect.y, modalRect.width, bannerH }, 0.08f, 4, g_Theme.bgTertiary);
    DrawRectangle((int)modalRect.x, (int)modalRect.y + 43, modalW, 5, bannerColor);
    
    // Center title text vertically in banner
    Vector2 titleSize = MeasureWithFont(g_FontSemiBold, s_Modal.title, FONT_MODAL_TITLE);
    float titleY = modalRect.y + (bannerH - titleSize.y) / 2.0f;
    DrawTextSemiBold(s_Modal.title, modalRect.x + 24, titleY, FONT_MODAL_TITLE, g_Theme.textPrimary);
    
    // Draw Message Body
    DrawTextRegular(s_Modal.message, modalRect.x + 24, modalRect.y + 68, FONT_MODAL_BODY, g_Theme.textSecondary);

    // Draw action buttons
    Vector2 mouse = Window_GetInputMousePosition();
    if (s_Modal.type == MODAL_CONFIRMATION || s_Modal.type == MODAL_WARNING) {
        // Yes and No buttons
        Rectangle yesRect = { modalRect.x + modalW - 220, modalRect.y + modalH - 56, (float)MODAL_BUTTON_WIDTH, (float)MODAL_BUTTON_HEIGHT };
        Rectangle noRect = { modalRect.x + modalW - 115, modalRect.y + modalH - 56, (float)MODAL_BUTTON_WIDTH, (float)MODAL_BUTTON_HEIGHT };

        bool hoverYes = CheckCollisionPointRec(mouse, yesRect);
        bool hoverNo = CheckCollisionPointRec(mouse, noRect);

        if (hoverYes || hoverNo) SetMouseCursor(MOUSE_CURSOR_POINTING_HAND);

        // Draw Yes button
        DrawRectangleRounded(yesRect, 0.2f, 4, hoverYes ? g_Theme.success : g_Theme.bgTertiary);
        DrawRectangleRoundedLines(yesRect, 0.2f, 4, g_Theme.border);
        // Center "Yes" text in button
        Vector2 yesSize = MeasureWithFont(g_FontSemiBold, "Yes", FONT_BUTTON);
        DrawTextSemiBold("Yes", yesRect.x + (yesRect.width - yesSize.x) / 2.0f, yesRect.y + (yesRect.height - yesSize.y) / 2.0f, FONT_BUTTON, g_Theme.textPrimary);

        // Draw No button
        DrawRectangleRounded(noRect, 0.2f, 4, hoverNo ? g_Theme.danger : g_Theme.bgTertiary);
        DrawRectangleRoundedLines(noRect, 0.2f, 4, g_Theme.border);
        Vector2 noSize = MeasureWithFont(g_FontSemiBold, "No", FONT_BUTTON);
        DrawTextSemiBold("No", noRect.x + (noRect.width - noSize.x) / 2.0f, noRect.y + (noRect.height - noSize.y) / 2.0f, FONT_BUTTON, g_Theme.textPrimary);

        if (hoverYes && IsMouseButtonPressed(MOUSE_BUTTON_LEFT)) {
            s_Modal.active = false;
            if (s_Modal.callback) s_Modal.callback(true);
        }
        if (hoverNo && IsMouseButtonPressed(MOUSE_BUTTON_LEFT)) {
            s_Modal.active = false;
            if (s_Modal.callback) s_Modal.callback(false);
        }
    } else {
        // Simple Dismiss OK button
        Rectangle okRect = { modalRect.x + modalW - 120, modalRect.y + modalH - 56, (float)MODAL_OK_WIDTH, (float)MODAL_BUTTON_HEIGHT };
        bool hoverOk = CheckCollisionPointRec(mouse, okRect);

        if (hoverOk) SetMouseCursor(MOUSE_CURSOR_POINTING_HAND);

        DrawRectangleRounded(okRect, 0.2f, 4, hoverOk ? g_Theme.accentBlue : g_Theme.bgTertiary);
        DrawRectangleRoundedLines(okRect, 0.2f, 4, g_Theme.border);
        Vector2 closeSize = MeasureWithFont(g_FontSemiBold, "Close", FONT_BUTTON);
        DrawTextSemiBold("Close", okRect.x + (okRect.width - closeSize.x) / 2.0f, okRect.y + (okRect.height - closeSize.y) / 2.0f, FONT_BUTTON, RAYWHITE);

        if (hoverOk && IsMouseButtonPressed(MOUSE_BUTTON_LEFT)) {
            s_Modal.active = false;
            if (s_Modal.callback) s_Modal.callback(true);
        }
    }
}

bool Modal_IsActive(void) {
    return s_Modal.active;
}

// =============================================================
// Interactive Button Component — Centered text, proper sizing
// =============================================================

bool DrawUIButton(Rectangle bounds, const char* text, bool isActive, ButtonCallback callback) {
    // If a modal is showing, block button inputs
    if (s_Modal.active) {
        Color bg = isActive ? ColorAlpha(g_Theme.accentBlue, 0.4f) : ColorAlpha(g_Theme.bgTertiary, 0.4f);
        Color fg = ColorAlpha(g_Theme.textSecondary, 0.4f);
        DrawRectangleRounded(bounds, 0.20f, 4, bg);
        DrawRectangleRoundedLines(bounds, 0.20f, 4, g_Theme.border);
        // Center text in disabled button
        Vector2 textSize = MeasureWithFont(g_FontSemiBold, text, FONT_BUTTON);
        float tx = bounds.x + (bounds.width - textSize.x) / 2.0f;
        float ty = bounds.y + (bounds.height - textSize.y) / 2.0f;
        DrawTextSemiBold(text, tx, ty, FONT_BUTTON, fg);
        return false;
    }

    Vector2 mouse = Window_GetInputMousePosition();
    bool hovered = CheckCollisionPointRec(mouse, bounds);
    bool clicked = false;
    
    // Animation scale/shift variables
    float pressOffset = 0.0f;
    Color bg = isActive ? g_Theme.accentBlue : g_Theme.bgTertiary;
    Color fg = isActive ? RAYWHITE : g_Theme.textPrimary;
    
    if (hovered) {
        SetMouseCursor(MOUSE_CURSOR_POINTING_HAND);
        bg = isActive ? ColorAlpha(g_Theme.accentBlue, 0.85f) : g_Theme.hoverColor;
        
        if (IsMouseButtonDown(MOUSE_BUTTON_LEFT)) {
            pressOffset = 1.5f;
            bg = isActive ? ColorAlpha(g_Theme.accentBlue, 0.70f) : g_Theme.activeColor;
        }
        
        if (IsMouseButtonReleased(MOUSE_BUTTON_LEFT)) {
            clicked = true;
            if (callback) callback();
        }
    }
    
    // Draw button background with pressOffset
    Rectangle drawRec = { bounds.x, bounds.y + pressOffset, bounds.width, bounds.height };
    DrawRectangleRounded(drawRec, 0.20f, 4, bg);
    DrawRectangleRoundedLines(drawRec, 0.20f, 4, g_Theme.border);
    
    // Center text in button using MeasureTextEx
    Vector2 textSize = MeasureWithFont(g_FontSemiBold, text, FONT_BUTTON);
    float tx = drawRec.x + (drawRec.width - textSize.x) / 2.0f;
    float ty = drawRec.y + (drawRec.height - textSize.y) / 2.0f;
    DrawTextSemiBold(text, tx, ty, FONT_BUTTON, fg);
    
    return clicked;
}

// =============================================================
// Stat Card Component — Standardized typography
// =============================================================

void DrawUICard(Rectangle bounds, const char* title, const char* value, const char* subtext, Color accentColor) {
    // Card background
    DrawRectangleRounded(bounds, 0.12f, 4, g_Theme.bgSecondary);
    DrawRectangleRoundedLines(bounds, 0.12f, 4, g_Theme.border);
    
    // Accent line on the left side
    DrawRectangleRounded((Rectangle){ bounds.x, bounds.y, 4, bounds.height }, 0.12f, 4, accentColor);
    
    // Typography with named constants and proper spacing
    float startX = bounds.x + CARD_INTERNAL_PADDING;
    float titleY = bounds.y + CARD_INTERNAL_PADDING - 2;
    float valueY = titleY + FONT_CARD_TITLE + CARD_TITLE_VALUE_GAP;
    float subtextY = valueY + FONT_CARD_VALUE + CARD_VALUE_SUBTEXT_GAP;
    
    DrawTextSemiBold(title, startX, titleY, FONT_CARD_TITLE, g_Theme.textSecondary);
    DrawTextBold(value, startX, valueY, FONT_CARD_VALUE, g_Theme.textPrimary);
    if (subtext) {
        DrawTextRegular(subtext, startX, subtextY, FONT_CARD_SUBTITLE, g_Theme.textSecondary);
    }
}

// =============================================================
// Interactive Data Table Component — Standardized typography
// =============================================================

void DrawUITable(Rectangle bounds, const char** headers, const float* columnWidths, int colCount, 
                 const char*** rows, int rowCount, bool isLoading, int* hoveredRow, 
                 int* selectedRow, int* sortCol, bool* sortAsc) {
    Vector2 mouse = Window_GetInputMousePosition();
    if (hoveredRow) *hoveredRow = -1;
    
    float headerHeight = (float)TABLE_HEADER_HEIGHT;
    float rowHeight = (float)TABLE_ROW_HEIGHT;
    
    // Frame background
    DrawRectangleRec(bounds, g_Theme.bgSecondary);
    DrawRectangleLines(bounds.x, bounds.y, bounds.width, bounds.height, g_Theme.border);
    
    // 1. Draw Header Background
    Rectangle headerRect = { bounds.x, bounds.y, bounds.width, headerHeight };
    DrawRectangleRec(headerRect, g_Theme.bgTertiary);
    DrawLine(bounds.x, bounds.y + headerHeight, bounds.x + bounds.width, bounds.y + headerHeight, g_Theme.border);
    
    // 2. Draw Column Headers and check click inputs
    float currentX = bounds.x;
    bool modalActive = s_Modal.active;
    
    for (int col = 0; col < colCount; ++col) {
        float colWidth = bounds.width * columnWidths[col];
        Rectangle headerCell = { currentX, bounds.y, colWidth, headerHeight };
        
        // Table Header: vertically centered
        Vector2 hdrSize = MeasureWithFont(g_FontSemiBold, headers[col], FONT_TABLE_HEADER);
        float textY = bounds.y + (headerHeight - hdrSize.y) / 2.0f;
        
        DrawTextSemiBold(headers[col], currentX + TABLE_CELL_PADDING, textY, FONT_TABLE_HEADER, g_Theme.textPrimary);
        
        // Sorting indicator arrow
        if (sortCol && *sortCol == col) {
            int arrowX = currentX + colWidth - 24;
            int arrowY = bounds.y + (headerHeight / 2) - 3;
            if (*sortAsc) {
                DrawTriangle((Vector2){ arrowX + 5, arrowY }, (Vector2){ arrowX, arrowY + 6 }, (Vector2){ arrowX + 10, arrowY + 6 }, g_Theme.accentBlue);
            } else {
                DrawTriangle((Vector2){ arrowX, arrowY }, (Vector2){ arrowX + 5, arrowY + 6 }, (Vector2){ arrowX + 10, arrowY }, g_Theme.accentBlue);
            }
        }
        
        // Check for click sorting
        if (!modalActive && CheckCollisionPointRec(mouse, headerCell)) {
            SetMouseCursor(MOUSE_CURSOR_POINTING_HAND);
            if (IsMouseButtonPressed(MOUSE_BUTTON_LEFT)) {
                if (sortCol && sortAsc) {
                    if (*sortCol == col) {
                        *sortAsc = !(*sortAsc);
                    } else {
                        *sortCol = col;
                        *sortAsc = true;
                    }
                    Toast_Show("Sorting table by column...", TOAST_INFO);
                }
            }
        }
        
        // Vertical divider lines
        if (col < colCount - 1) {
            DrawLine(currentX + colWidth, bounds.y, currentX + colWidth, bounds.y + bounds.height, g_Theme.border);
        }
        currentX += colWidth;
    }
    
    // 3. Draw Table Data or states
    if (isLoading) {
        // Loading State Layout
        int spinnerSize = 24;
        float sy = bounds.y + headerHeight + (bounds.height - headerHeight - spinnerSize) / 2.0f;
        
        // Pulsing loading spinner simulation
        float t = GetTime() * 5.0f;
        int loadingDots = ((int)t) % 4;
        char loadStr[32] = "Loading cargo registry";
        for (int d = 0; d < loadingDots; ++d) strcat(loadStr, ".");
        
        Vector2 loadSize = MeasureWithFont(g_FontRegular, loadStr, FONT_NORMAL);
        DrawTextRegular(loadStr, bounds.x + (bounds.width - loadSize.x) / 2, sy - 15, FONT_NORMAL, g_Theme.textSecondary);
        
        // Progress bar simulation
        Rectangle loadingBar = { bounds.x + 100, sy + 15, bounds.width - 200, 4 };
        DrawRectangleRec(loadingBar, g_Theme.border);
        
        float progressWidth = (loadingBar.width) * (0.5f + 0.5f * sinf(GetTime() * 2.0f));
        DrawRectangleRec((Rectangle){ loadingBar.x, loadingBar.y, progressWidth, loadingBar.height }, g_Theme.accentBlue);
        
    } else if (rowCount == 0) {
        // Empty State Layout
        const char* emptyMsg = "Cargo Registry Empty. No Dispatches Logged.";
        Vector2 emptySize = MeasureWithFont(g_FontRegular, emptyMsg, FONT_NORMAL);
        float tx = bounds.x + (bounds.width - emptySize.x) / 2.0f;
        float ty = bounds.y + headerHeight + (bounds.height - headerHeight - emptySize.y) / 2.0f;
        DrawTextRegular(emptyMsg, tx, ty, FONT_NORMAL, g_Theme.textSecondary);
        
    } else {
        // Draw Data Grid Rows
        for (int row = 0; row < rowCount; ++row) {
            float rowY = bounds.y + headerHeight + row * rowHeight;
            if (rowY + rowHeight > bounds.y + bounds.height) break; // Overflow clip
            
            Rectangle rowRect = { bounds.x, rowY, bounds.width, rowHeight };
            bool isRowHovered = (!modalActive && CheckCollisionPointRec(mouse, rowRect));
            bool isRowSelected = (selectedRow && *selectedRow == row);
            
            // Background row highlight selection
            if (isRowSelected) {
                DrawRectangleRec(rowRect, ColorAlpha(g_Theme.accentBlue, 0.25f));
            } else if (isRowHovered) {
                DrawRectangleRec(rowRect, g_Theme.hoverColor);
                if (hoveredRow) *hoveredRow = row;
            } else if (row % 2 == 1) {
                // Zebra Striping
                DrawRectangleRec(rowRect, ColorAlpha(g_Theme.bgTertiary, 0.15f));
            }
            
            // Handle row clicks
            if (isRowHovered && IsMouseButtonPressed(MOUSE_BUTTON_LEFT) && selectedRow) {
                *selectedRow = row;
                char msg[64];
                sprintf(msg, "Selected record: %s", rows[row][0]);
                Toast_Show(msg, TOAST_INFO);
            }
            
            // Draw cells — Table Content: vertically centered
            float colX = bounds.x;
            for (int col = 0; col < colCount; ++col) {
                float colWidth = bounds.width * columnWidths[col];
                
                // Vertically center text in row
                Vector2 cellSize = MeasureWithFont(g_FontRegular, rows[row][col], FONT_TABLE_CONTENT);
                float textY = rowY + (rowHeight - cellSize.y) / 2.0f;
                
                Color textCol = isRowSelected ? g_Theme.accentBlue : (isRowHovered ? g_Theme.textPrimary : g_Theme.textSecondary);
                DrawTextRegular(rows[row][col], colX + TABLE_CELL_PADDING, textY, FONT_TABLE_CONTENT, textCol);
                
                colX += colWidth;
            }
            
            // Draw horizontal row borders
            DrawLine(bounds.x, rowY + rowHeight, bounds.x + bounds.width, rowY + rowHeight, g_Theme.border);
        }
    }
}
