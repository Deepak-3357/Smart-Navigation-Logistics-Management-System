#ifndef LAYOUT_H
#define LAYOUT_H

#include <raylib.h>
#include "common.h"
#include "theme_manager.h"
#include "components.h"

// =============================================================
// Layout Helper Functions
// =============================================================
// All layout constants are defined once in common.h.
// These inline helpers compute dynamic positions from those constants.

static inline int GetContentX(void) {
    return SIDEBAR_WIDTH + CONTENT_MARGIN;
}

static inline int GetContentWidth(void) {
    return GetScreenWidth() - SIDEBAR_WIDTH - (CONTENT_MARGIN * 2);
}

static inline int GetContentY(void) {
    return HEADER_HEIGHT + CONTENT_TOP_MARGIN;
}

static inline int GetContentHeight(void) {
    return GetScreenHeight() - HEADER_HEIGHT - STATUS_BAR_HEIGHT - CONTENT_TOP_MARGIN;
}

static inline Rectangle GetContentRect(void) {
    return (Rectangle){ 
        (float)GetContentX(), 
        (float)GetContentY(), 
        (float)GetContentWidth(), 
        (float)GetContentHeight() 
    };
}

static inline int GetCardWidth(int columns) {
    int contentWidth = GetContentWidth();
    return (contentWidth - (CARD_SPACING * (columns - 1))) / columns;
}

static inline int GetCardY(int row, int cardHeight) {
    return GetContentY() + row * (cardHeight + CARD_SPACING);
}

static inline Rectangle GetCardRect(int col, int row, int columns, int cardHeight) {
    int x = GetContentX() + col * (GetCardWidth(columns) + CARD_SPACING);
    int y = GetCardY(row, cardHeight);
    return (Rectangle){ (float)x, (float)y, (float)GetCardWidth(columns), (float)cardHeight };
}

static inline int GetToolbarY(int cardBottom) {
    return cardBottom + SECTION_SPACING;
}

static inline int GetTableY(int toolbarBottom) {
    return toolbarBottom + SECTION_SPACING;
}

static inline Rectangle GetTableRect(int toolbarBottom) {
    int y = GetTableY(toolbarBottom);
    return (Rectangle){ 
        (float)GetContentX(), 
        (float)y, 
        (float)GetContentWidth(), 
        (float)(GetScreenHeight() - y - STATUS_BAR_HEIGHT) 
    };
}

static inline void DrawSectionTitle(const char* text, float x, float y) {
    DrawTextSemiBold(text, x, y, FONT_SECTION_HEADING, g_Theme.textPrimary);
}

static inline void DrawPageTitle(const char* text, float x, float y) {
    DrawTextBold(text, x, y, FONT_PAGE_TITLE, g_Theme.textPrimary);
}

static inline void DrawSubtitle(const char* text, float x, float y) {
    DrawTextSemiBold(text, x, y, FONT_SECTION_HEADING, g_Theme.textPrimary);
}

static inline Rectangle GetButtonRect(int index, int btnX, int btnY, int btnWidth) {
    return (Rectangle){ 
        (float)(btnX + index * (btnWidth + BUTTON_SPACING)), 
        (float)btnY, 
        (float)btnWidth, 
        (float)BUTTON_HEIGHT 
    };
}

static inline void DrawButtonBar(int count, int btnX, int btnY, int btnWidth, int btnHeight, 
                                  const char** labels, bool* activeStates, ButtonCallback* callbacks) {
    for (int i = 0; i < count; ++i) {
        DrawUIButton(GetButtonRect(i, btnX, btnY, btnWidth), labels[i], activeStates[i], callbacks[i]);
    }
}

static inline void DrawTableHeader(const char** headers, const float* columnWidths, int colCount, 
                                    float x, float y, float width, int* sortCol, bool* sortAsc) {
    float currentX = x;
    for (int col = 0; col < colCount; ++col) {
        float colWidth = width * columnWidths[col];
        int textY = y + (int)((TABLE_HEADER_HEIGHT - FONT_TABLE_HEADER) / 2.0f);
        DrawTextSemiBold(headers[col], currentX + TABLE_CELL_PADDING, textY, FONT_TABLE_HEADER, g_Theme.textPrimary);
        currentX += colWidth;
    }
}

#endif
