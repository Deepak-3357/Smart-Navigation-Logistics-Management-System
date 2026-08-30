#ifndef THEME_MANAGER_H
#define THEME_MANAGER_H

#include <raylib.h>

typedef struct {
    Color bgPrimary;       // Main content background
    Color bgSecondary;     // Sidebar background
    Color bgTertiary;      // Card hover state / table header background
    Color textPrimary;     // Main text color
    Color textSecondary;   // Dimmer text / labels / descriptors
    Color accentBlue;      // Primary accent (e.g. active tab, main actions)
    Color accentTeal;      // Secondary accent (e.g. data points, alternate tags)
    Color border;          // Borders and dividers
    Color hoverColor;      // Button or list item hover
    Color activeColor;     // Button or list item active
    Color cardBg;          // Background of cards
    Color success;         // Success status color
    Color warning;         // Warning status color
    Color danger;          // Alert/Error status color
} AppTheme;

extern AppTheme g_Theme;

void Theme_Init(void);
void Theme_Toggle(void);

#endif // THEME_MANAGER_H
