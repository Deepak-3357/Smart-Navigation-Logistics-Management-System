#include "screen_manager.h"
#include "screens.h"

void Screen_Init(void) {
    InitDashboardScreen();
    InitWarehouseScreen();
    InitPackagesScreen();
    InitRoutesScreen();
    InitAnalyticsScreen();
    InitSettingsScreen();
}

void Screen_Update(void) {
    switch (g_AppState.currentScreen) {
        case SCREEN_SPLASH:    break; // Handled in main.c
        case SCREEN_DASHBOARD: UpdateDashboardScreen(); break;
        case SCREEN_WAREHOUSE: UpdateWarehouseScreen(); break;
        case SCREEN_PACKAGES:  UpdatePackagesScreen();  break;
        case SCREEN_ROUTES:    UpdateRoutesScreen();    break;
        case SCREEN_ANALYTICS: UpdateAnalyticsScreen(); break;
        case SCREEN_SETTINGS:  UpdateSettingsScreen();  break;
        default: break;
    }
}

void Screen_Draw(void) {
    switch (g_AppState.currentScreen) {
        case SCREEN_SPLASH:    break; // Handled in main.c
        case SCREEN_DASHBOARD: DrawDashboardScreen(); break;
        case SCREEN_WAREHOUSE: DrawWarehouseScreen(); break;
        case SCREEN_PACKAGES:  DrawPackagesScreen();  break;
        case SCREEN_ROUTES:    DrawRoutesScreen();    break;
        case SCREEN_ANALYTICS: DrawAnalyticsScreen(); break;
        case SCREEN_SETTINGS:  DrawSettingsScreen();  break;
        default: break;
    }
}

void Screen_ChangeTo(ScreenType screenType) {
    if (screenType >= 0 && screenType < SCREEN_COUNT) {
        g_AppState.currentScreen = screenType;
    }
}
