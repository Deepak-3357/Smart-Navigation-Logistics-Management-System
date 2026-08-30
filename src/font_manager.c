#include "font_manager.h"
#include <stdio.h>

FontManager g_FontManager = {0};

static Font LoadFontSafe(const char* path, int fontSize, const char* fallbackName) {
    Font font = {0};
    
    if (FileExists(path)) {
        font = LoadFontEx(path, fontSize, NULL, 0);
        if (font.texture.id > 0) {
            SetTextureFilter(font.texture, TEXTURE_FILTER_BILINEAR);
            return font;
        }
        UnloadFont(font);
    }
    
    font = GetFontDefault();
    return font;
}

void FontManager_Init(void) {
    int baseSize = 128;
    
    g_FontManager.regular = LoadFontSafe("assets/fonts/Inter-Regular.ttf", baseSize, "Regular");
    g_FontManager.semiBold = LoadFontSafe("assets/fonts/Inter-SemiBold.ttf", baseSize, "SemiBold");
    g_FontManager.bold = LoadFontSafe("assets/fonts/Inter-Bold.ttf", baseSize, "Bold");
    
    g_FontManager.loaded = (g_FontManager.regular.texture.id > 0);
    
    if (g_FontManager.loaded) {
        printf("[FontManager] Inter font family loaded successfully\n");
    } else {
        printf("[FontManager] Using Raylib default font (Inter not found in assets/fonts/)\n");
    }
}

void FontManager_Shutdown(void) {
    if (g_FontManager.regular.texture.id > 0 && g_FontManager.regular.texture.id != GetFontDefault().texture.id) {
        UnloadFont(g_FontManager.regular);
    }
    if (g_FontManager.semiBold.texture.id > 0 && g_FontManager.semiBold.texture.id != GetFontDefault().texture.id) {
        UnloadFont(g_FontManager.semiBold);
    }
    if (g_FontManager.bold.texture.id > 0 && g_FontManager.bold.texture.id != GetFontDefault().texture.id) {
        UnloadFont(g_FontManager.bold);
    }
    g_FontManager.loaded = false;
}

Font FontManager_GetRegular(void) {
    return g_FontManager.regular;
}

Font FontManager_GetSemiBold(void) {
    return g_FontManager.semiBold;
}

Font FontManager_GetBold(void) {
    return g_FontManager.bold;
}

bool FontManager_IsLoaded(void) {
    return g_FontManager.loaded;
}