#include "theme_manager.h"
#include "common.h"

AppTheme g_Theme;

static void ApplyDarkTheme(void) {
    g_Theme.bgPrimary     = (Color){ 0x0F, 0x17, 0x2A, 0xFF }; // slate-900
    g_Theme.bgSecondary   = (Color){ 0x1E, 0x29, 0x3B, 0xFF }; // slate-800
    g_Theme.bgTertiary    = (Color){ 0x33, 0x41, 0x55, 0xFF }; // slate-700
    g_Theme.textPrimary   = (Color){ 0xF8, 0xFA, 0xFC, 0xFF }; // slate-50
    g_Theme.textSecondary = (Color){ 0x94, 0xA3, 0xB8, 0xFF }; // slate-400
    g_Theme.accentBlue    = (Color){ 0x3B, 0x82, 0xF6, 0xFF }; // blue-500
    g_Theme.accentTeal    = (Color){ 0x0D, 0x94, 0x88, 0xFF }; // teal-600
    g_Theme.border        = (Color){ 0x33, 0x41, 0x55, 0xFF }; // slate-700
    g_Theme.hoverColor    = (Color){ 0x47, 0x55, 0x69, 0xFF }; // slate-600
    g_Theme.activeColor   = (Color){ 0x64, 0x74, 0x8B, 0xFF }; // slate-500
    g_Theme.cardBg        = (Color){ 0x1E, 0x29, 0x3B, 0xFF };
    g_Theme.success       = (Color){ 0x10, 0xB9, 0x81, 0xFF }; // emerald-500
    g_Theme.warning       = (Color){ 0xF5, 0x9E, 0x0B, 0xFF }; // amber-500
    g_Theme.danger        = (Color){ 0xEF, 0x44, 0x44, 0xFF }; // red-500
}

static void ApplyLightTheme(void) {
    g_Theme.bgPrimary     = (Color){ 0xF8, 0xFA, 0xFC, 0xFF }; // slate-50
    g_Theme.bgSecondary   = (Color){ 0xFF, 0xFF, 0xFF, 0xFF }; // white
    g_Theme.bgTertiary    = (Color){ 0xE2, 0xE8, 0xF0, 0xFF }; // slate-200
    g_Theme.textPrimary   = (Color){ 0x0F, 0x17, 0x2A, 0xFF }; // slate-900
    g_Theme.textSecondary = (Color){ 0x64, 0x74, 0x8B, 0xFF }; // slate-500
    g_Theme.accentBlue    = (Color){ 0x25, 0x63, 0xEB, 0xFF }; // blue-600
    g_Theme.accentTeal    = (Color){ 0x0F, 0x76, 0x6E, 0xFF }; // teal-700
    g_Theme.border        = (Color){ 0xE2, 0xE8, 0xF0, 0xFF }; // slate-200
    g_Theme.hoverColor    = (Color){ 0xF1, 0xF5, 0xF9, 0xFF }; // slate-100
    g_Theme.activeColor   = (Color){ 0xE2, 0xE8, 0xF0, 0xFF }; // slate-200
    g_Theme.cardBg        = (Color){ 0xFF, 0xFF, 0xFF, 0xFF };
    g_Theme.success       = (Color){ 0x05, 0x96, 0x69, 0xFF }; // emerald-600
    g_Theme.warning       = (Color){ 0xD9, 0x77, 0x06, 0xFF }; // amber-600
    g_Theme.danger        = (Color){ 0xDC, 0x26, 0x26, 0xFF }; // red-600
}

void Theme_Init(void) {
    if (g_AppState.isDarkTheme) {
        ApplyDarkTheme();
    } else {
        ApplyLightTheme();
    }
}

void Theme_Toggle(void) {
    g_AppState.isDarkTheme = !g_AppState.isDarkTheme;
    if (g_AppState.isDarkTheme) {
        ApplyDarkTheme();
    } else {
        ApplyLightTheme();
    }
}
