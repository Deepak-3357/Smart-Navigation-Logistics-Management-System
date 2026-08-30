#include "common.h"
#include "theme_manager.h"
#include "window_manager.h"
#include "screen_manager.h"
#include "ui_manager.h"
#include "font_manager.h"
#include "components.h"

// Define RAYGUI_IMPLEMENTATION in exactly one C source file to build raygui
// Undefine USE_LIBTYPE_SHARED so raygui functions are compiled as plain
// symbols (not __declspec(dllimport)) since raygui is header-only and
// compiled into this translation unit, not a separate DLL.
#define RAYGUI_IMPLEMENTATION
#undef USE_LIBTYPE_SHARED
#define GUI_POINTER_POSITION Window_GetInputMousePosition()
#include "raygui.h"

#include <stdio.h>
#include <math.h>

AppState g_AppState;
Font g_FontRegular = {0};
Font g_FontSemiBold = {0};
Font g_FontBold = {0};

// =============================================================
// Splash Screen — Professional Loading Sequence
// =============================================================

static const char* s_SplashPhases[] = {
    "Loading Fonts...",
    "Loading Theme...",
    "Loading UI...",
    "Loading Components...",
    "Loading Modules...",
    "Ready..."
};
#define SPLASH_PHASE_COUNT 6
#define SPLASH_TOTAL_DURATION 3.0f
#define SPLASH_PHASE_DURATION (SPLASH_TOTAL_DURATION / SPLASH_PHASE_COUNT)
#define SPLASH_FADE_IN_DURATION 0.4f
#define SPLASH_FADE_OUT_DURATION 0.5f

// Draw the splash screen frame
static void DrawSplashScreen(float progress, float fade, int phase) {
    int screenWidth = GetScreenWidth();
    int screenHeight = GetScreenHeight();
    // Dark gradient background
    Color gradTop = (Color){ 0x08, 0x0C, 0x1A, 0xFF };
    Color gradBot = (Color){ 0x12, 0x1E, 0x38, 0xFF };
    ClearBackground(gradTop);

    // Draw gradient by horizontal bands
    for (int y = 0; y < screenHeight; ++y) {
        float t = (float)y / (float)screenHeight;
        Color c = {
            (unsigned char)(gradTop.r + (int)((gradBot.r - gradTop.r) * t)),
            (unsigned char)(gradTop.g + (int)((gradBot.g - gradTop.g) * t)),
            (unsigned char)(gradTop.b + (int)((gradBot.b - gradTop.b) * t)),
            0xFF
        };
        DrawLine(0, y, screenWidth, y, c);
    }

    // Professional blue accent color
    Color accentBlue = (Color){ 0x3B, 0x82, 0xF6, 0xFF };
    Color accentTeal = (Color){ 0x0D, 0x94, 0x88, 0xFF };
    Color textWhite = (Color){ 0xF8, 0xFA, 0xFC, (unsigned char)(255 * fade) };
    Color textDim = (Color){ 0x94, 0xA3, 0xB8, (unsigned char)(255 * fade) };
    Color accentFaded = (Color){ accentBlue.r, accentBlue.g, accentBlue.b, (unsigned char)(255 * fade) };

    // ---- Logo blocks ----
    float centerX = screenWidth / 2.0f;
    float logoY = screenHeight * 0.28f;

    // Geometric logo — 3 blocks
    int blockSize = 18;
    float logoBlockX = centerX - 22;
    DrawRectangle((int)logoBlockX, (int)logoY, blockSize, blockSize, 
                  (Color){ accentBlue.r, accentBlue.g, accentBlue.b, (unsigned char)(255 * fade) });
    DrawRectangle((int)logoBlockX + 20, (int)logoY, blockSize, blockSize, 
                  (Color){ accentTeal.r, accentTeal.g, accentTeal.b, (unsigned char)(255 * fade) });
    DrawRectangle((int)logoBlockX, (int)logoY + 20, 38, blockSize, 
                  (Color){ textWhite.r, textWhite.g, textWhite.b, (unsigned char)(200 * fade) });

    // ---- Title ----
    float titleY = logoY + 60;
    const char* titleLine1 = "Smart Navigation &";
    const char* titleLine2 = "Logistics Management System";

    if (g_FontBold.texture.id > 0) {
        Vector2 t1Size = MeasureTextEx(g_FontBold, titleLine1, 30, 30 * FONT_SPACING_FACTOR);
        Vector2 t2Size = MeasureTextEx(g_FontBold, titleLine2, 30, 30 * FONT_SPACING_FACTOR);
        DrawTextEx(g_FontBold, titleLine1, (Vector2){ centerX - t1Size.x / 2, titleY }, 30, 30 * FONT_SPACING_FACTOR, textWhite);
        DrawTextEx(g_FontBold, titleLine2, (Vector2){ centerX - t2Size.x / 2, titleY + 36 }, 30, 30 * FONT_SPACING_FACTOR, textWhite);
    } else {
        int t1W = MeasureText(titleLine1, 30);
        int t2W = MeasureText(titleLine2, 30);
        DrawText(titleLine1, (int)(centerX - t1W / 2), (int)titleY, 30, textWhite);
        DrawText(titleLine2, (int)(centerX - t2W / 2), (int)(titleY + 36), 30, textWhite);
    }

    // ---- Subtitle ----
    float subtitleY = titleY + 90;
    const char* subtitle = "Advanced Data Structures Capstone";
    if (g_FontSemiBold.texture.id > 0) {
        Vector2 sSize = MeasureTextEx(g_FontSemiBold, subtitle, 18, 18 * FONT_SPACING_FACTOR);
        DrawTextEx(g_FontSemiBold, subtitle, (Vector2){ centerX - sSize.x / 2, subtitleY }, 18, 18 * FONT_SPACING_FACTOR, accentFaded);
    } else {
        int sW = MeasureText(subtitle, 18);
        DrawText(subtitle, (int)(centerX - sW / 2), (int)subtitleY, 18, accentFaded);
    }

    // ---- Progress bar ----
    float barW = 400.0f;
    float barH = 6.0f;
    float barX = centerX - barW / 2;
    float barY = screenHeight * 0.62f;

    // Track
    DrawRectangleRounded((Rectangle){ barX, barY, barW, barH }, 0.5f, 4, 
                         (Color){ 0x1E, 0x29, 0x3B, (unsigned char)(200 * fade) });
    // Fill
    float fillW = barW * progress;
    if (fillW > 0) {
        DrawRectangleRounded((Rectangle){ barX, barY, fillW, barH }, 0.5f, 4, accentFaded);
        // Glow effect
        DrawRectangleRounded((Rectangle){ barX, barY - 1, fillW, barH + 2 }, 0.5f, 4, 
                             (Color){ accentBlue.r, accentBlue.g, accentBlue.b, (unsigned char)(60 * fade) });
    }

    // ---- Loading phase text ----
    float phaseY = barY + 24;
    if (phase >= 0 && phase < SPLASH_PHASE_COUNT) {
        const char* phaseText = s_SplashPhases[phase];
        if (g_FontRegular.texture.id > 0) {
            Vector2 pSize = MeasureTextEx(g_FontRegular, phaseText, 15, 15 * FONT_SPACING_FACTOR);
            DrawTextEx(g_FontRegular, phaseText, (Vector2){ centerX - pSize.x / 2, phaseY }, 15, 15 * FONT_SPACING_FACTOR, textDim);
        } else {
            int pW = MeasureText(phaseText, 15);
            DrawText(phaseText, (int)(centerX - pW / 2), (int)phaseY, 15, textDim);
        }
    }

    // ---- Version ----
    float versionY = screenHeight * 0.75f;
    const char* versionText = "v1.0";
    if (g_FontRegular.texture.id > 0) {
        Vector2 vSize = MeasureTextEx(g_FontRegular, versionText, 14, 14 * FONT_SPACING_FACTOR);
        DrawTextEx(g_FontRegular, versionText, (Vector2){ centerX - vSize.x / 2, versionY }, 14, 14 * FONT_SPACING_FACTOR, textDim);
    } else {
        int vW = MeasureText(versionText, 14);
        DrawText(versionText, (int)(centerX - vW / 2), (int)versionY, 14, textDim);
    }

    // ---- Bottom institution text ----
    float bottomY = screenHeight * 0.88f;
    const char* org = "SIMATS Engineering";
    const char* dept = "Department of Computer Science and Engineering";

    if (g_FontSemiBold.texture.id > 0) {
        Vector2 oSize = MeasureTextEx(g_FontSemiBold, org, 16, 16 * FONT_SPACING_FACTOR);
        DrawTextEx(g_FontSemiBold, org, (Vector2){ centerX - oSize.x / 2, bottomY }, 16, 16 * FONT_SPACING_FACTOR, textWhite);
    } else {
        int oW = MeasureText(org, 16);
        DrawText(org, (int)(centerX - oW / 2), (int)bottomY, 16, textWhite);
    }

    if (g_FontRegular.texture.id > 0) {
        Vector2 dSize = MeasureTextEx(g_FontRegular, dept, 13, 13 * FONT_SPACING_FACTOR);
        DrawTextEx(g_FontRegular, dept, (Vector2){ centerX - dSize.x / 2, bottomY + 24 }, 13, 13 * FONT_SPACING_FACTOR, textDim);
    } else {
        int dW = MeasureText(dept, 13);
        DrawText(dept, (int)(centerX - dW / 2), (int)(bottomY + 24), 13, textDim);
    }
}

// =============================================================
// Main Entry Point
// =============================================================

int main(void) {
    // 1. Initialize global states
    g_AppState.currentScreen = SCREEN_SPLASH;
    g_AppState.isDarkTheme = true;
    g_AppState.shouldClose = false;
    g_AppState.inSplash = true;
    g_AppState.splashProgress = 0.0f;
    g_AppState.splashFade = 0.0f;
    g_AppState.splashPhase = 0;
    g_AppState.splashTimer = 0.0f;

    // 2. Apply theme colors & initialize window
    Theme_Init();
    
    if (!Window_Init("Smart Navigation & Logistics Management System", WINDOW_WIDTH, WINDOW_HEIGHT)) {
        return 1;
    }

    // 3. Initialize font system
    FontManager_Init();
    g_FontRegular = FontManager_GetRegular();
    g_FontSemiBold = FontManager_GetSemiBold();
    g_FontBold = FontManager_GetBold();

    // 4. Initialize screens & visual managers
    Screen_Init();
    UI_Init();

    // ==========================================================
    // Splash Screen Loop (~3 seconds)
    // ==========================================================
    float splashElapsed = 0.0f;
    float totalSplash = SPLASH_TOTAL_DURATION + SPLASH_FADE_IN_DURATION + SPLASH_FADE_OUT_DURATION;

    while (!WindowShouldClose() && g_AppState.inSplash) {
        float dt = GetFrameTime();
        splashElapsed += dt;

        // Phase 1: Fade in
        if (splashElapsed < SPLASH_FADE_IN_DURATION) {
            g_AppState.splashFade = splashElapsed / SPLASH_FADE_IN_DURATION;
            g_AppState.splashProgress = 0.0f;
            g_AppState.splashPhase = 0;
        }
        // Phase 2: Loading (progress bar fills)
        else if (splashElapsed < SPLASH_FADE_IN_DURATION + SPLASH_TOTAL_DURATION) {
            g_AppState.splashFade = 1.0f;
            float loadingTime = splashElapsed - SPLASH_FADE_IN_DURATION;
            g_AppState.splashProgress = loadingTime / SPLASH_TOTAL_DURATION;
            if (g_AppState.splashProgress > 1.0f) g_AppState.splashProgress = 1.0f;
            g_AppState.splashPhase = (int)(loadingTime / SPLASH_PHASE_DURATION);
            if (g_AppState.splashPhase >= SPLASH_PHASE_COUNT) g_AppState.splashPhase = SPLASH_PHASE_COUNT - 1;
        }
        // Phase 3: Fade out
        else if (splashElapsed < totalSplash) {
            g_AppState.splashFade = 1.0f - (splashElapsed - SPLASH_FADE_IN_DURATION - SPLASH_TOTAL_DURATION) / SPLASH_FADE_OUT_DURATION;
            g_AppState.splashProgress = 1.0f;
            g_AppState.splashPhase = SPLASH_PHASE_COUNT - 1;
        }
        // Done
        else {
            g_AppState.inSplash = false;
            g_AppState.currentScreen = SCREEN_DASHBOARD;
            break;
        }

        BeginDrawing();
            DrawSplashScreen(g_AppState.splashProgress, g_AppState.splashFade, g_AppState.splashPhase);
        EndDrawing();
    }

    // ==========================================================
    // 5. Main Frame loop
    // ==========================================================
    while (!Window_ShouldClose() && !g_AppState.shouldClose) {
        // Dynamic input updates
        float dt = GetFrameTime();
        Window_BeginInputFrame();
        UI_Update(dt);
        Toast_Update(dt);
        Modal_Update();
        Screen_Update();

        // Render pass
        BeginDrawing();
            ClearBackground(g_Theme.bgPrimary);
            
            // Draw global layouts (Sidebar, Header, Toggles)
            UI_DrawLayout();
            
            // Draw page content
            Screen_Draw();
            
            // Draw overlays (modal on top of content, toast on top of everything)
            Modal_Draw();
            Toast_Draw();
            
            // Draw status bar
            UI_DrawStatusBar();
            
        EndDrawing();
    }

    // 6. Clean up
    FontManager_Shutdown();
    Window_Shutdown();

    return 0;
}
