#ifndef SCREEN_MANAGER_H
#define SCREEN_MANAGER_H

#include "common.h"

void Screen_Init(void);
void Screen_Update(void);
void Screen_Draw(void);
void Screen_ChangeTo(ScreenType screenType);

#endif // SCREEN_MANAGER_H
