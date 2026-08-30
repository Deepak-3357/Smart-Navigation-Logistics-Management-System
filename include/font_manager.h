#ifndef FONT_MANAGER_H
#define FONT_MANAGER_H

#include <raylib.h>

typedef struct {
    Font regular;
    Font semiBold;
    Font bold;
    bool loaded;
} FontManager;

extern FontManager g_FontManager;

void FontManager_Init(void);
void FontManager_Shutdown(void);
Font FontManager_GetRegular(void);
Font FontManager_GetSemiBold(void);
Font FontManager_GetBold(void);
bool FontManager_IsLoaded(void);

#endif