#include "ui_manager.h"
#include "theme_manager.h"
#include "screen_manager.h"
#include "components.h"
#include "layout.h"
#include "window_manager.h"
#include <stdio.h>
#include <math.h>
#include <time.h>

static const char* s_ScreenTitles[] = {
    "Dashboard Overview",
    "Warehouse Catalog",
    "Package Registry",
    "Route Optimization",
    "Performance Analytics",
    "System Settings"
};

static const char* s_SidebarLabels[] = {
    "Dashboard",
    "Warehouse",
    "Packages",
    "Routes",
    "Analytics",
    "Settings"
};

// =============================================================
// Sidebar layout constants — centralized to avoid magic numbers
// =============================================================
#define SIDEBAR_NAV_START_Y       100
#define SIDEBAR_NAV_ITEM_HEIGHT   44
#define SIDEBAR_NAV_ITEM_PADDING  8
#define SIDEBAR_ICON_LEFT_PAD     16
#define SIDEBAR_LOGO_X            25
#define SIDEBAR_LOGO_Y            25
#define SIDEBAR_LOGO_BLOCK_SIZE   12
#define SIDEBAR_LOGO_TEXT_X       60
#define SIDEBAR_DIVIDER_PAD       20
#define SIDEBAR_DIVIDER_Y         78
#define SIDEBAR_SLIDER_X          20
#define SIDEBAR_SLIDER_ROUNDNESS  0.2f
#define SIDEBAR_INDICATOR_WIDTH   4
#define SIDEBAR_INDICATOR_VPAD    6

// Smooth transition animation states
static float s_IndicatorY = (float)SIDEBAR_NAV_START_Y;
static float s_IndicatorTargetY = (float)SIDEBAR_NAV_START_Y;
static bool s_IndicatorInitialized = false;

void UI_Init(void) {
    s_IndicatorInitialized = false;
}

void UI_Update(float dt) {
    s_IndicatorTargetY = (float)(SIDEBAR_NAV_START_Y + (int)g_AppState.currentScreen * (SIDEBAR_NAV_ITEM_HEIGHT + SIDEBAR_NAV_ITEM_PADDING));
    
    if (!s_IndicatorInitialized) {
        s_IndicatorY = s_IndicatorTargetY;
        s_IndicatorInitialized = true;
    } else {
        float speed = 15.0f * g_AppState.animationSpeed;
        s_IndicatorY += (s_IndicatorTargetY - s_IndicatorY) * speed * dt;
    }
}

// Draw a geometric icon based on screen type
static void DrawSidebarIcon(ScreenType type, int x, int y, int size, Color color) {
    switch (type) {
        case SCREEN_DASHBOARD: {
            int half = size / 2;
            int gap = 2;
            DrawRectangle(x, y, half - gap, half - gap, color);
            DrawRectangle(x + half, y, half - gap, half - gap, color);
            DrawRectangle(x, y + half, half - gap, half - gap, color);
            DrawRectangle(x + half, y + half, half - gap, half - gap, color);
            break;
        }
        case SCREEN_WAREHOUSE: {
            DrawRectangleLines(x, y, size, size, color);
            DrawLine(x, y + size / 2, x + size, y + size / 2, color);
            DrawLine(x + size / 2, y, x + size / 2, y + size, color);
            break;
        }
        case SCREEN_PACKAGES: {
            DrawRectangleLines(x + 2, y + 2, size - 4, size - 4, color);
            DrawRectangle(x + 5, y + 5, size - 10, size - 10, color);
            break;
        }
        case SCREEN_ROUTES: {
            DrawCircle(x + 3, y + size - 3, 3, color);
            DrawCircle(x + size - 3, y + 3, 3, color);
            DrawLine(x + 3, y + size - 3, x + size - 3, y + 3, color);
            break;
        }
        case SCREEN_ANALYTICS: {
            int barWidth = size / 4;
            int gap = 2;
            DrawRectangle(x, y + size - 6, barWidth, 6, color);
            DrawRectangle(x + barWidth + gap, y + size - 12, barWidth, 12, color);
            DrawRectangle(x + 2 * (barWidth + gap), y + size - size, barWidth, size, color);
            break;
        }
        case SCREEN_SETTINGS: {
            DrawCircleLines(x + size / 2, y + size / 2, size / 3, color);
            DrawCircle(x + size / 2, y + size / 2, 2, color);
            for (int i = 0; i < 8; ++i) {
                float angle = i * 45.0f * DEG2RAD;
                int tx = x + size / 2 + (int)(cosf(angle) * (size / 2.0f - 1.0f));
                int ty = y + size / 2 + (int)(sinf(angle) * (size / 2.0f - 1.0f));
                DrawCircle(tx, ty, 1, color);
            }
            break;
        }
        default:
            break;
    }
}

void UI_DrawLayout(void) {
    // 1. Sidebar Background
    int screenWidth = GetScreenWidth();
    int screenHeight = GetScreenHeight();
    DrawRectangle(0, 0, SIDEBAR_WIDTH, screenHeight, g_Theme.bgSecondary);
    DrawLine(SIDEBAR_WIDTH, 0, SIDEBAR_WIDTH, screenHeight, g_Theme.border);

    // 2. Logo / Header — Application Title
    DrawRectangle(SIDEBAR_LOGO_X, SIDEBAR_LOGO_Y, SIDEBAR_LOGO_BLOCK_SIZE, SIDEBAR_LOGO_BLOCK_SIZE, g_Theme.accentBlue);
    DrawRectangle(SIDEBAR_LOGO_X + 14, SIDEBAR_LOGO_Y, SIDEBAR_LOGO_BLOCK_SIZE, SIDEBAR_LOGO_BLOCK_SIZE, g_Theme.accentTeal);
    DrawRectangle(SIDEBAR_LOGO_X, SIDEBAR_LOGO_Y + 14, 26, SIDEBAR_LOGO_BLOCK_SIZE, g_Theme.textPrimary);
    
    // Logo text — vertically aligned with logo block
    DrawTextBold("LOGISTICS", SIDEBAR_LOGO_TEXT_X, SIDEBAR_LOGO_Y - 7, FONT_APP_TITLE, g_Theme.textPrimary);
    DrawTextRegular("INTELLIGENCE", SIDEBAR_LOGO_TEXT_X, SIDEBAR_LOGO_Y + 25, FONT_SIDEBAR_LABEL, g_Theme.textSecondary);

    DrawLine(SIDEBAR_DIVIDER_PAD, SIDEBAR_DIVIDER_Y, SIDEBAR_WIDTH - SIDEBAR_DIVIDER_PAD, SIDEBAR_DIVIDER_Y, g_Theme.border);

    // Block interaction if a modal is active
    bool modalActive = Modal_IsActive();

    // 3. Sliding Navigation Indicator Background
    float sliderW = (float)(SIDEBAR_WIDTH - SIDEBAR_SLIDER_X * 2);
    Rectangle sliderRect = { (float)SIDEBAR_SLIDER_X, s_IndicatorY, sliderW, (float)SIDEBAR_NAV_ITEM_HEIGHT };
    DrawRectangleRounded(sliderRect, SIDEBAR_SLIDER_ROUNDNESS, 4, g_Theme.accentBlue);
    DrawRectangle(SIDEBAR_SLIDER_X, (int)s_IndicatorY + SIDEBAR_INDICATOR_VPAD, SIDEBAR_INDICATOR_WIDTH, SIDEBAR_NAV_ITEM_HEIGHT - SIDEBAR_INDICATOR_VPAD * 2, RAYWHITE);

    // 4. Navigation links
    Vector2 mousePos = Window_GetInputMousePosition();

    for (int i = 0; i < SCREEN_COUNT; ++i) {
        int yPos = SIDEBAR_NAV_START_Y + i * (SIDEBAR_NAV_ITEM_HEIGHT + SIDEBAR_NAV_ITEM_PADDING);
        Rectangle btnRect = { (float)SIDEBAR_SLIDER_X, (float)yPos, sliderW, (float)SIDEBAR_NAV_ITEM_HEIGHT };
        bool isHovered = (!modalActive && CheckCollisionPointRec(mousePos, btnRect));
        bool isActive = ((int)g_AppState.currentScreen == i);

        // Styling selection logic
        Color textColor = g_Theme.textSecondary;
        Color iconColor = g_Theme.textSecondary;

        if (isActive) {
            textColor = RAYWHITE;
            iconColor = RAYWHITE;
        } else if (isHovered) {
            SetMouseCursor(MOUSE_CURSOR_POINTING_HAND);
            DrawRectangleRounded(btnRect, SIDEBAR_SLIDER_ROUNDNESS, 4, g_Theme.hoverColor);
            textColor = g_Theme.textPrimary;
            iconColor = g_Theme.accentBlue;
        }

        int iconSize = ICON_SIZE;
        int iconX = btnRect.x + SIDEBAR_ICON_LEFT_PAD;
        int iconY = btnRect.y + (SIDEBAR_NAV_ITEM_HEIGHT - iconSize) / 2;
        DrawSidebarIcon(i, iconX, iconY, iconSize, iconColor);

        // Sidebar label — vertically centered in nav item
        float labelY = btnRect.y + (SIDEBAR_NAV_ITEM_HEIGHT - FONT_SIDEBAR_LABEL) / 2.0f;
        DrawTextSemiBold(s_SidebarLabels[i], iconX + iconSize + ICON_TEXT_GAP, labelY, FONT_SIDEBAR_LABEL, textColor);

        if (isHovered && IsMouseButtonPressed(MOUSE_BUTTON_LEFT)) {
            Screen_ChangeTo(i);
            char switchMsg[64];
            sprintf(switchMsg, "Switched to: %s", s_SidebarLabels[i]);
            Toast_Show(switchMsg, TOAST_INFO);
        }
    }

    // 5. Theme toggle switch
    int themeBtnY = screenHeight - 120;
    float themeBtnH = 36.0f;
    Rectangle themeBtnRect = { (float)SIDEBAR_SLIDER_X, (float)themeBtnY, sliderW, themeBtnH };
    bool hoverTheme = (!modalActive && CheckCollisionPointRec(mousePos, themeBtnRect));
    
    DrawRectangleRounded(themeBtnRect, SIDEBAR_SLIDER_ROUNDNESS, 4, hoverTheme ? g_Theme.hoverColor : g_Theme.bgTertiary);
    
    // Center theme label vertically in toggle button
    float themeLabelY = themeBtnRect.y + (themeBtnH - FONT_SIDEBAR_LABEL) / 2.0f;
    DrawTextSemiBold(g_AppState.isDarkTheme ? "Dark Mode" : "Light Mode", themeBtnRect.x + 15, themeLabelY, FONT_SIDEBAR_LABEL, g_Theme.textPrimary);
    
    // Toggle switch
    float switchW = 32;
    float switchH = 16;
    float switchX = themeBtnRect.x + themeBtnRect.width - switchW - 15;
    float switchY = themeBtnRect.y + (themeBtnH - switchH) / 2;
    DrawRectangleRounded((Rectangle){ switchX, switchY, switchW, switchH }, 0.5f, 4, g_Theme.border);
    
    float handleRadius = 6.0f;
    float handleX = g_AppState.isDarkTheme ? (switchX + switchW - handleRadius - 2) : (switchX + handleRadius + 2);
    DrawCircle(handleX, switchY + switchH / 2, handleRadius, g_Theme.accentBlue);

    if (hoverTheme) {
        SetMouseCursor(MOUSE_CURSOR_POINTING_HAND);
        if (IsMouseButtonPressed(MOUSE_BUTTON_LEFT)) {
            Theme_Toggle();
            Toast_Show(g_AppState.isDarkTheme ? "Dark theme enabled" : "Light theme enabled", TOAST_SUCCESS);
        }
    }

    // Divider
    DrawLine(SIDEBAR_DIVIDER_PAD, screenHeight - 70, SIDEBAR_WIDTH - SIDEBAR_DIVIDER_PAD, screenHeight - 70, g_Theme.border);

    // Administrator tag — vertically aligned
    int adminY = screenHeight - 55;
    DrawCircle(35, adminY + 5, 14, g_Theme.accentBlue);
    Vector2 aSize = MeasureWithFont(g_FontBold, "A", FONT_STATUS_BAR);
    DrawTextBold("A", 35.0f - aSize.x / 2.0f, (adminY + 5.0f) - aSize.y / 2.0f, FONT_STATUS_BAR, RAYWHITE);
    DrawTextSemiBold("System Administrator", 60, adminY - 3, FONT_SMALL_LABEL, g_Theme.textPrimary);
    DrawTextRegular("Active (UCRT64)", 60, adminY + 12, FONT_STATUS_BAR, g_Theme.success);

    // 6. Header bar drawing
    DrawRectangle(SIDEBAR_WIDTH, 0, screenWidth - SIDEBAR_WIDTH, HEADER_HEIGHT, g_Theme.bgSecondary);
    DrawLine(SIDEBAR_WIDTH, HEADER_HEIGHT, screenWidth, HEADER_HEIGHT, g_Theme.border);

    // Page title — vertically centered in header
    DrawTextBold(s_ScreenTitles[g_AppState.currentScreen], SIDEBAR_WIDTH + CONTENT_MARGIN, (HEADER_HEIGHT - FONT_PAGE_TITLE) / 2.0f, FONT_PAGE_TITLE, g_Theme.textPrimary);

    // Header right — vertically centered and right-aligned to match the content margin
    const char* protocolText = "LOGISTICS PROTOCOL: V1.1";
    Vector2 protocolSize = MeasureWithFont(g_FontRegular, protocolText, FONT_STATUS_BAR);
    float protocolX = screenWidth - protocolSize.x - CONTENT_MARGIN;
    float protocolY = (HEADER_HEIGHT - FONT_STATUS_BAR) / 2.0f;
    DrawTextRegular(protocolText, protocolX, protocolY, FONT_STATUS_BAR, g_Theme.textSecondary);
}

void UI_DrawStatusBar(void) {
    int screenWidth = GetScreenWidth();
    int screenHeight = GetScreenHeight();
    int barH = STATUS_BAR_HEIGHT;
    int barY = screenHeight - barH;
    
    Rectangle barRect = { (float)SIDEBAR_WIDTH, (float)barY, (float)(screenWidth - SIDEBAR_WIDTH), (float)barH };
    
    // Status bar container
    DrawRectangleRec(barRect, g_Theme.bgSecondary);
    DrawLine(SIDEBAR_WIDTH, barY, screenWidth, barY, g_Theme.border);
    
    // All status bar text is vertically centered at the same baseline
    float textY = barY + (barH - FONT_STATUS_BAR) / 2.0f;
    
    // Left: Current screen
    char viewStr[64];
    sprintf(viewStr, "Active View: %s", s_SidebarLabels[g_AppState.currentScreen]);
    DrawTextRegular(viewStr, SIDEBAR_WIDTH + CONTENT_MARGIN, textY, FONT_STATUS_BAR, g_Theme.textSecondary);
    
    // Center: FPS
    char fpsStr[32];
    sprintf(fpsStr, "System Rate: %d FPS", GetFPS());
    Vector2 fpsSize = MeasureWithFont(g_FontRegular, fpsStr, FONT_STATUS_BAR);
    float fpsX = SIDEBAR_WIDTH + ((screenWidth - SIDEBAR_WIDTH) - fpsSize.x) / 2.0f;
    DrawTextRegular(fpsStr, fpsX, textY, FONT_STATUS_BAR, g_Theme.textSecondary);
    
    // Right: Date & Clock + Version
    time_t rawtime;
    struct tm* timeinfo;
    char timeStr[64];
    time(&rawtime);
    timeinfo = localtime(&rawtime);
    strftime(timeStr, sizeof(timeStr), "%H:%M:%S", timeinfo);
    
    char systemStr[128];
    sprintf(systemStr, "Time: %s  |  Build v1.1.0", timeStr);
    
    // Measure to right-align
    Vector2 sysSize;
    if (g_FontRegular.texture.id > 0) {
        sysSize = MeasureTextEx(g_FontRegular, systemStr, FONT_STATUS_BAR, FONT_STATUS_BAR * FONT_SPACING_FACTOR);
    } else {
        sysSize = (Vector2){ (float)MeasureText(systemStr, FONT_STATUS_BAR), (float)FONT_STATUS_BAR };
    }
    DrawTextRegular(systemStr, screenWidth - sysSize.x - CONTENT_MARGIN, textY, FONT_STATUS_BAR, g_Theme.textSecondary);
}
