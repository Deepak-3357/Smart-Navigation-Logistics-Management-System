#ifndef COMMON_H
#define COMMON_H

#include <raylib.h>
#include <stdbool.h>

// =============================================================
// Window Dimensions
// =============================================================
#define WINDOW_WIDTH  1400
#define WINDOW_HEIGHT 900

// =============================================================
// Layout Structure Constants
// =============================================================
#define SIDEBAR_WIDTH             280
#define HEADER_HEIGHT             70
#define STATUS_BAR_HEIGHT         34
#define CONTENT_MARGIN            24
#define CONTENT_GAP               24
#define CONTENT_TOP_MARGIN        24

// =============================================================
// Card & Section Layout
// =============================================================
#define CARD_SPACING              16
#define CARD_HEIGHT               108
#define CARD_INTERNAL_PADDING     16
#define CARD_TITLE_VALUE_GAP      8
#define CARD_VALUE_SUBTEXT_GAP    8
#define SECTION_SPACING           24
#define SUBTITLE_OFFSET           26

// =============================================================
// Button Layout
// =============================================================
#define BUTTON_HEIGHT             38
#define BUTTON_SPACING            12

// =============================================================
// Table Layout
// =============================================================
#define TABLE_MARGIN              0
#define TABLE_HEADER_HEIGHT       48
#define TABLE_ROW_HEIGHT          44
#define TABLE_CELL_PADDING        20

// =============================================================
// Toolbar Layout
// =============================================================
#define TOOLBAR_HEIGHT            38
#define TOOLBAR_GAP               12

// =============================================================
// Icon Layout
// =============================================================
#define ICON_SIZE                 16
#define ICON_TEXT_GAP             12

// =============================================================
// Modal & Toast Layout
// =============================================================
#define MODAL_WIDTH               460
#define MODAL_HEIGHT              220
#define MODAL_BUTTON_WIDTH        95
#define MODAL_BUTTON_HEIGHT       38
#define MODAL_OK_WIDTH            100
#define TOAST_WIDTH               320
#define TOAST_HEIGHT              72
#define TOAST_SPACING             12

// =============================================================
// Typography System — Standardized Font Sizes (px)
// =============================================================
#define FONT_APP_TITLE            34
#define FONT_PAGE_TITLE           28
#define FONT_SECTION_HEADING      22
#define FONT_CARD_VALUE           30
#define FONT_CARD_TITLE           16
#define FONT_CARD_SUBTITLE        14
#define FONT_NORMAL               15
#define FONT_BUTTON               16
#define FONT_TABLE_HEADER         16
#define FONT_TABLE_CONTENT        15
#define FONT_STATUS_BAR           14
#define FONT_SMALL_LABEL          14
#define FONT_SIDEBAR_LABEL        16
#define FONT_TOAST_HEADER         15
#define FONT_TOAST_MESSAGE        14
#define FONT_MODAL_TITLE          22
#define FONT_MODAL_BODY           15
#define FONT_MICRO_LABEL          14

// Improved letter spacing factor (multiplied by font size)
#define FONT_SPACING_FACTOR       0.04f

// Legacy aliases for backward compatibility
#define SECTION_HEADING_SIZE      FONT_SECTION_HEADING
#define PAGE_TITLE_SIZE           FONT_PAGE_TITLE
#define APP_TITLE_SIZE            FONT_APP_TITLE
#define SIDEBAR_LABEL_SIZE        FONT_SIDEBAR_LABEL

// =============================================================
// Screen Types
// =============================================================
typedef enum {
    SCREEN_SPLASH = -1,        // Splash screen (before dashboard)
    SCREEN_DASHBOARD = 0,
    SCREEN_WAREHOUSE,
    SCREEN_PACKAGES,
    SCREEN_ROUTES,
    SCREEN_ANALYTICS,
    SCREEN_SETTINGS,
    SCREEN_COUNT
} ScreenType;

// =============================================================
// Application State
// =============================================================
typedef struct {
    ScreenType currentScreen;
    bool isDarkTheme;
    bool shouldClose;

    // Splash screen state
    bool inSplash;
    float splashProgress;
    float splashFade;
    int   splashPhase;       // Current loading phase index
    float splashTimer;       // Timer for phase progression

    // Polished settings state
    bool autoSave;
    float animationSpeed;
    char dataFolder[128];
} AppState;

extern AppState g_AppState;
extern Font g_FontRegular;
extern Font g_FontSemiBold;
extern Font g_FontBold;

#endif // COMMON_H
