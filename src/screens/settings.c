#include "screens.h"
#include "common.h"
#include "theme_manager.h"
#include "components.h"
#include "layout.h"
#include "window_manager.h"
#include <stdio.h>
#include <math.h>

// Settings screen layout constants (local to this module)
#define SETTINGS_SECTION_GAP        24
#define SETTINGS_THEME_CARD_H       90
#define SETTINGS_THEME_CARD_GAP     24
#define SETTINGS_INFO_PANEL_H       220
#define SETTINGS_ABOUT_PANEL_H      144
#define SETTINGS_CHECKBOX_SIZE      20
#define SETTINGS_INSET              16
#define SETTINGS_ROW_GAP            10
#define SETTINGS_SLIDER_WIDTH       280
#define SETTINGS_SLIDER_HEIGHT      8
#define SETTINGS_BROWSE_BTN_W       80

static void onSaveSettings(void) {
    Toast_Show("Settings saved to config.ini", TOAST_SUCCESS);
}

static void onResetConfirm(bool confirmed) {
    if (confirmed) {
        g_AppState.autoSave = true;
        g_AppState.animationSpeed = 1.0f;
        if (!g_AppState.isDarkTheme) {
            Theme_Toggle();
        }
        Toast_Show("Settings restored to factory defaults!", TOAST_SUCCESS);
    }
}

static void onResetSettings(void) {
    Modal_Show("Reset System Defaults", "Are you sure you want to revert all system settings to defaults?", MODAL_CONFIRMATION, onResetConfirm);
}

void InitSettingsScreen(void) {
    g_AppState.autoSave = true;
    g_AppState.animationSpeed = 1.0f;
    snprintf(g_AppState.dataFolder, sizeof(g_AppState.dataFolder), "C:\\SIMATS\\capstone\\data structure\\Project\\data");
}

void UpdateSettingsScreen(void) {
    // Input handling for custom widgets
    if (Modal_IsActive()) return;

    Vector2 mouse = Window_GetInputMousePosition();

    // Compute layout positions using shared helpers
    int startX = GetContentX();
    int contentY = GetContentY();
    int themeCardY = contentY + SETTINGS_SECTION_GAP;
    int configY = themeCardY + SETTINGS_THEME_CARD_H + SETTINGS_THEME_CARD_GAP;
    int infoPanelY = configY + SETTINGS_SECTION_GAP;
    
    // 1. Checkbox hit test
    Rectangle checkRect = { (float)(startX + SETTINGS_INSET), (float)(infoPanelY + SETTINGS_INSET + SETTINGS_ROW_GAP), SETTINGS_CHECKBOX_SIZE, SETTINGS_CHECKBOX_SIZE };
    if (CheckCollisionPointRec(mouse, checkRect) && IsMouseButtonPressed(MOUSE_BUTTON_LEFT)) {
        g_AppState.autoSave = !g_AppState.autoSave;
        SetMouseCursor(MOUSE_CURSOR_POINTING_HAND);
        Toast_Show(g_AppState.autoSave ? "Auto Save enabled" : "Auto Save disabled", TOAST_INFO);
    }

    // 2. Slider dragging
    Rectangle sliderRect = { (float)(startX + SETTINGS_INSET), (float)(infoPanelY + SETTINGS_INSET + 70), SETTINGS_SLIDER_WIDTH, SETTINGS_SLIDER_HEIGHT };
    if (IsMouseButtonDown(MOUSE_BUTTON_LEFT)) {
        // Expand hover collision height slightly for comfortable dragging
        Rectangle dragArea = { sliderRect.x, sliderRect.y - 10, sliderRect.width, sliderRect.height + 20 };
        if (CheckCollisionPointRec(mouse, dragArea)) {
            float relX = mouse.x - sliderRect.x;
            if (relX < 0) relX = 0;
            if (relX > sliderRect.width) relX = sliderRect.width;
            
            // Map 0 to width -> 0.5f to 2.0f
            g_AppState.animationSpeed = 0.5f + (relX / sliderRect.width) * 1.5f;
            // Round to nearest 0.1
            g_AppState.animationSpeed = roundf(g_AppState.animationSpeed * 10.0f) / 10.0f;
        }
    }
}

void DrawSettingsScreen(void) {
    int startX = GetContentX();
    int contentWidth = GetContentWidth();
    int contentY = GetContentY();
    
    // Theme Card Settings
    DrawTextSemiBold("System Appearance & Themes", (float)startX, (float)contentY, FONT_SECTION_HEADING, g_Theme.textPrimary);
    
    int themeCardW = (contentWidth - CARD_SPACING) / 2;
    int themeCardH = SETTINGS_THEME_CARD_H;
    int themeCardY = contentY + SETTINGS_SECTION_GAP;
    
    // Dark Theme selector card
    Rectangle darkRect = { (float)startX, (float)themeCardY, (float)themeCardW, (float)themeCardH };
    DrawRectangleRounded(darkRect, 0.1f, 4, g_Theme.bgSecondary);
    DrawRectangleRoundedLines(darkRect, 0.1f, 4, g_AppState.isDarkTheme ? g_Theme.accentBlue : g_Theme.border);
    DrawTextBold("Sleek Dark Mode Theme", darkRect.x + SETTINGS_INSET, darkRect.y + 18, FONT_SMALL_LABEL, g_Theme.textPrimary);
    DrawTextRegular("Optimized for low-light warehouses and control screens.", darkRect.x + SETTINGS_INSET, darkRect.y + 42, FONT_MICRO_LABEL, g_Theme.textSecondary);
    if (g_AppState.isDarkTheme) {
        DrawCircle(darkRect.x + darkRect.width - 25, darkRect.y + 25, 5, g_Theme.accentTeal);
    }
    
    // Light Theme selector card
    Rectangle lightRect = { (float)(startX + themeCardW + CARD_SPACING), (float)themeCardY, (float)themeCardW, (float)themeCardH };
    DrawRectangleRounded(lightRect, 0.1f, 4, g_Theme.bgSecondary);
    DrawRectangleRoundedLines(lightRect, 0.1f, 4, !g_AppState.isDarkTheme ? g_Theme.accentBlue : g_Theme.border);
    DrawTextBold("Soft Light Mode Theme", lightRect.x + SETTINGS_INSET, lightRect.y + 18, FONT_SMALL_LABEL, g_Theme.textPrimary);
    DrawTextRegular("Designed for high-visibility outdoor mobile tablets.", lightRect.x + SETTINGS_INSET, lightRect.y + 42, FONT_MICRO_LABEL, g_Theme.textSecondary);
    if (!g_AppState.isDarkTheme) {
        DrawCircle(lightRect.x + lightRect.width - 25, lightRect.y + 25, 5, g_Theme.accentTeal);
    }
    
    // Mouse hover cursor hand
    Vector2 mouse = Window_GetInputMousePosition();
    bool modalActive = Modal_IsActive();
    if (!modalActive) {
        if (CheckCollisionPointRec(mouse, darkRect) || CheckCollisionPointRec(mouse, lightRect)) {
            SetMouseCursor(MOUSE_CURSOR_POINTING_HAND);
        }
        // Handle clicks
        if (IsMouseButtonPressed(MOUSE_BUTTON_LEFT)) {
            if (CheckCollisionPointRec(mouse, darkRect) && !g_AppState.isDarkTheme) {
                Theme_Toggle();
                Toast_Show("Applied Sleek Dark Mode", TOAST_SUCCESS);
            } else if (CheckCollisionPointRec(mouse, lightRect) && g_AppState.isDarkTheme) {
                Theme_Toggle();
                Toast_Show("Applied Soft Light Mode", TOAST_SUCCESS);
            }
        }
    }
    
    // Settings Division
    int configY = themeCardY + themeCardH + SETTINGS_THEME_CARD_GAP;
    DrawTextSemiBold("User Preferences & Directories", (float)startX, (float)configY, FONT_SECTION_HEADING, g_Theme.textPrimary);
    
    int infoPanelY = configY + SETTINGS_SECTION_GAP;
    int infoPanelW = (int)(contentWidth * 0.55f) - CARD_SPACING / 2;
    int infoPanelH = SETTINGS_INFO_PANEL_H;
    Rectangle infoPanel = { (float)startX, (float)infoPanelY, (float)infoPanelW, (float)infoPanelH };
    
    DrawRectangleRounded(infoPanel, 0.04f, 4, g_Theme.bgSecondary);
    DrawRectangleRoundedLines(infoPanel, 0.04f, 4, g_Theme.border);
    
    // Custom Checkbox for Auto-Save
    Rectangle checkRect = { (float)(startX + SETTINGS_INSET), (float)(infoPanelY + SETTINGS_INSET + SETTINGS_ROW_GAP), SETTINGS_CHECKBOX_SIZE, SETTINGS_CHECKBOX_SIZE };
    DrawRectangleRounded(checkRect, 0.25f, 4, g_Theme.bgTertiary);
    DrawRectangleRoundedLines(checkRect, 0.25f, 4, g_Theme.border);
    if (g_AppState.autoSave) {
        DrawRectangleRounded((Rectangle){ checkRect.x + 4, checkRect.y + 4, 12, 12 }, 0.25f, 4, g_Theme.accentBlue);
    }
    DrawTextBold("Enable Settings Auto-Save", checkRect.x + 32, checkRect.y + 3, FONT_TOAST_HEADER, g_Theme.textPrimary);
    DrawTextRegular("Saves configuration immediately on value changes.", checkRect.x + 32, checkRect.y + SETTINGS_INSET, FONT_MICRO_LABEL, g_Theme.textSecondary);

    // Custom Slider for Animation Speed
    Rectangle sliderRect = { (float)(startX + SETTINGS_INSET), (float)(infoPanelY + SETTINGS_INSET + 70), SETTINGS_SLIDER_WIDTH, SETTINGS_SLIDER_HEIGHT };
    DrawRectangleRounded(sliderRect, 0.5f, 4, g_Theme.border);
    
    // Filled portion
    float percentage = (g_AppState.animationSpeed - 0.5f) / 1.5f;
    float fillW = sliderRect.width * percentage;
    DrawRectangleRounded((Rectangle){ sliderRect.x, sliderRect.y, fillW, sliderRect.height }, 0.5f, 4, g_Theme.accentBlue);
    
    // Slider handle
    float handleX = sliderRect.x + fillW;
    DrawCircle((int)handleX, (int)(sliderRect.y + sliderRect.height / 2), 7, g_Theme.accentBlue);
    
    // Draw slider text
    char speedText[32];
    sprintf(speedText, "Sidebar Speed: %.1fx", g_AppState.animationSpeed);
    DrawTextBold(speedText, sliderRect.x + sliderRect.width + 15, sliderRect.y - 4, FONT_TOAST_HEADER, g_Theme.textPrimary);
    DrawTextRegular("Scales linear interpolation speed of UI transitions.", sliderRect.x, sliderRect.y + 14, FONT_MICRO_LABEL, g_Theme.textSecondary);

    // Data Folder panel
    int folderY = infoPanelY + SETTINGS_INFO_PANEL_H - 70;
    DrawTextBold("Demo Data Base Folder Directory Path:", (float)(startX + SETTINGS_INSET), (float)folderY, FONT_MICRO_LABEL, g_Theme.textSecondary);
    
    float folderBoxW = (float)(infoPanelW - SETTINGS_INSET * 2 - SETTINGS_BROWSE_BTN_W - BUTTON_SPACING);
    Rectangle folderBox = { (float)(startX + SETTINGS_INSET), (float)(folderY + 16), folderBoxW, (float)BUTTON_HEIGHT };
    DrawRectangleRec(folderBox, g_Theme.bgPrimary);
    DrawRectangleLinesEx(folderBox, 1, g_Theme.border);
    
    // Scroll paths if text overflows
    DrawTextRegular(g_AppState.dataFolder, folderBox.x + SETTINGS_ROW_GAP, folderBox.y + (BUTTON_HEIGHT - FONT_MICRO_LABEL) / 2.0f, FONT_MICRO_LABEL, g_Theme.textPrimary);
    
    Rectangle browseBtn = { folderBox.x + folderBox.width + BUTTON_SPACING, folderBox.y, (float)SETTINGS_BROWSE_BTN_W, (float)BUTTON_HEIGHT };
    if (DrawUIButton(browseBtn, "Browse", false, NULL)) {
        Modal_Show("Browse Directory", "Data folder selection will be implemented in Phase 2.", MODAL_INFO, NULL);
    }

    // About Panel
    int aboutX = startX + infoPanelW + CARD_SPACING;
    int aboutW = contentWidth - infoPanelW - CARD_SPACING;
    Rectangle aboutPanel = { (float)aboutX, (float)infoPanelY, (float)aboutW, (float)SETTINGS_ABOUT_PANEL_H };
    DrawRectangleRounded(aboutPanel, 0.04f, 4, g_Theme.bgSecondary);
    DrawRectangleRoundedLines(aboutPanel, 0.04f, 4, g_Theme.border);

    DrawTextSemiBold("About Platform", aboutPanel.x + SETTINGS_INSET, aboutPanel.y + SETTINGS_INSET, FONT_CARD_TITLE, g_Theme.textPrimary);
    DrawTextBold("Logistics Intelligence Suite", aboutPanel.x + SETTINGS_INSET, aboutPanel.y + 45, FONT_TABLE_CONTENT, g_Theme.accentBlue);
    DrawTextRegular("Version: 1.1.0-Polished Build", aboutPanel.x + SETTINGS_INSET, aboutPanel.y + 68, FONT_TABLE_CONTENT, g_Theme.textSecondary);
    DrawTextRegular("Toolchain: GCC 16.1.0 | Raylib 6.0", aboutPanel.x + SETTINGS_INSET, aboutPanel.y + 88, FONT_TABLE_CONTENT, g_Theme.textSecondary);
    DrawTextRegular("Workspace Target: UCRT64 C17", aboutPanel.x + SETTINGS_INSET, aboutPanel.y + 108, FONT_TABLE_CONTENT, g_Theme.textSecondary);
    
    // Save/Reset Buttons
    int btnY = infoPanelY + infoPanelH + SETTINGS_THEME_CARD_GAP;
    int btnW = 180;
    
    DrawUIButton((Rectangle){ (float)startX, (float)btnY, (float)btnW, (float)BUTTON_HEIGHT }, "Save Preferences", true, onSaveSettings);
    DrawUIButton((Rectangle){ (float)(startX + btnW + BUTTON_SPACING), (float)btnY, (float)(btnW + 40), (float)BUTTON_HEIGHT }, "Reset Demo Defaults", false, onResetSettings);
}
