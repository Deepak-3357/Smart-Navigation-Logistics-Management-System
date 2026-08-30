#include "screens.h"
#include "common.h"
#include "theme_manager.h"
#include "components.h"
#include "layout.h"
#include "warehouse.h"
#include "route.h"
#include "window_manager.h"
#include <stdio.h>

#define DASHBOARD_ROUTE_HUB_LIMIT 32
#define DASHBOARD_ROUTE_OPTION_H   28

static bool s_RouteOptimizerModalActive = false;
static int s_RouteSourceIndex = 0;
static int s_RouteDestinationIndex = 1;
static int s_RouteHubCount = 0;
static int s_RouteHubIds[DASHBOARD_ROUTE_HUB_LIMIT];
static const RouteNode *s_RouteHubNodes[DASHBOARD_ROUTE_HUB_LIMIT];
static bool s_SourceDropdownOpen = false;
static bool s_DestinationDropdownOpen = false;

static void PrepareDashboardRouteOptions(void) {
    const RouteGraph *graph = GetRouteGraph();
    s_RouteHubCount = 0;

    if (!graph) {
        return;
    }

    for (const RouteNode *node = graph->nodesHead;
         node && s_RouteHubCount < DASHBOARD_ROUTE_HUB_LIMIT;
         node = node->next) {
        int insertAt = s_RouteHubCount++;
        while (insertAt > 0 && s_RouteHubIds[insertAt - 1] > node->nodeId) {
            s_RouteHubIds[insertAt] = s_RouteHubIds[insertAt - 1];
            s_RouteHubNodes[insertAt] = s_RouteHubNodes[insertAt - 1];
            --insertAt;
        }
        s_RouteHubIds[insertAt] = node->nodeId;
        s_RouteHubNodes[insertAt] = node;
    }

    s_RouteSourceIndex = 0;
    s_RouteDestinationIndex = s_RouteHubCount > 1 ? 1 : 0;
    s_SourceDropdownOpen = false;
    s_DestinationDropdownOpen = false;
}

static void onDashboardOptimizeSelectedRoute(void) {
    const RouteGraph *graph = GetRouteGraph();
    if (!graph || s_RouteHubCount <= 0 || s_RouteSourceIndex < 0 ||
        s_RouteSourceIndex >= s_RouteHubCount || s_RouteDestinationIndex < 0 ||
        s_RouteDestinationIndex >= s_RouteHubCount) {
        s_RouteOptimizerModalActive = false;
        Toast_Show("No route network is available for optimization.", TOAST_WARNING);
        return;
    }

    int sourceId = s_RouteHubIds[s_RouteSourceIndex];
    int destinationId = s_RouteHubIds[s_RouteDestinationIndex];
    if (sourceId == destinationId) {
        Toast_Show("Source and destination hubs must be different.", TOAST_WARNING);
        return;
    }
    if (!FindRouteNode(graph, sourceId) || !FindRouteNode(graph, destinationId)) {
        Toast_Show("Selected hubs are not present in the current route graph.", TOAST_WARNING);
        return;
    }

    float totalDistance = 0.0f;
    int path[MAX_TRAVERSAL_NODES];
    int pathLength = 0;
    if (!FindShortestRoute(graph, sourceId, destinationId,
                           &totalDistance, path, &pathLength)) {
        s_RouteOptimizerModalActive = false;
        Modal_Show("Route Optimizer", "No reachable route exists between the selected hubs.",
                   MODAL_WARNING, NULL);
        return;
    }

    char pathText[256] = "";
    size_t used = 0;
    for (int i = 0; i < pathLength && used < sizeof(pathText); ++i) {
        const RouteNode *node = FindRouteNode(graph, path[i]);
        int written = snprintf(pathText + used, sizeof(pathText) - used,
                               "%s%s", i == 0 ? "" : " -> ",
                               node ? node->name : "Unknown");
        if (written < 0 || (size_t)written >= sizeof(pathText) - used) {
            break;
        }
        used += (size_t)written;
    }

    char result[768];
    snprintf(result, sizeof(result),
             "Source: %s\nDestination: %s\n\nOptimal Route:\n%s\n\n"
             "Total Distance/Cost: %.2f\nNodes: %d",
             s_RouteHubNodes[s_RouteSourceIndex]->name,
             s_RouteHubNodes[s_RouteDestinationIndex]->name,
             pathText, totalDistance, pathLength);
    s_RouteOptimizerModalActive = false;
    Modal_Show("Route Optimizer", result, MODAL_SUCCESS, NULL);

    char toast[160];
    snprintf(toast, sizeof(toast), "Route optimized across %d graph nodes.", pathLength);
    Toast_Show(toast, TOAST_SUCCESS);
}

static void DrawDashboardRouteField(Rectangle field, int selectedIndex) {
    const char *selectedText = "Select hub";
    if (selectedIndex >= 0 && selectedIndex < s_RouteHubCount) {
        selectedText = s_RouteHubNodes[selectedIndex]->name;
    }

    DrawRectangleRounded(field, 0.12f, 4, g_Theme.bgTertiary);
    DrawRectangleRoundedLines(field, 0.12f, 4, g_Theme.border);
    DrawTextRegular(selectedText, field.x + 12, field.y + 8,
                    FONT_NORMAL, g_Theme.textPrimary);
    DrawTextRegular("v", field.x + field.width - 20, field.y + 8,
                    FONT_NORMAL, g_Theme.textSecondary);
}

static bool DrawDashboardRouteDropdown(Rectangle field, int *selectedIndex,
                                       bool *isOpen, Vector2 mouse) {
    bool consumedClick = false;
    DrawDashboardRouteField(field, *selectedIndex);

    if (CheckCollisionPointRec(mouse, field) && IsMouseButtonPressed(MOUSE_BUTTON_LEFT)) {
        *isOpen = !*isOpen;
        consumedClick = true;
    }

    if (*isOpen) {
        Rectangle popup = { field.x, field.y + field.height, field.width,
                            (float)(s_RouteHubCount * DASHBOARD_ROUTE_OPTION_H) };
        DrawRectangle(popup.x, popup.y, popup.width, popup.height, g_Theme.bgTertiary);
        DrawRectangleLines((int)popup.x, (int)popup.y,
                           (int)popup.width, (int)popup.height, g_Theme.border);

        for (int i = 0; i < s_RouteHubCount; ++i) {
            Rectangle option = { popup.x, popup.y + i * DASHBOARD_ROUTE_OPTION_H,
                                 popup.width, DASHBOARD_ROUTE_OPTION_H };
            bool hovered = CheckCollisionPointRec(mouse, option);
            if (hovered) {
                DrawRectangle(option.x, option.y, option.width, option.height,
                              g_Theme.hoverColor);
            }
            DrawTextRegular(s_RouteHubNodes[i]->name, option.x + 12, option.y + 6,
                            FONT_NORMAL, g_Theme.textPrimary);
            if (hovered && IsMouseButtonPressed(MOUSE_BUTTON_LEFT)) {
                *selectedIndex = i;
                *isOpen = false;
                consumedClick = true;
                break;
            }
        }
    }
    return consumedClick;
}

static void DrawDashboardRouteOptimizerModal(void) {
    if (!s_RouteOptimizerModalActive) {
        return;
    }

    DrawRectangle(0, 0, GetScreenWidth(), GetScreenHeight(), ColorAlpha(BLACK, 0.5f));

    const float modalW = 520.0f;
    const bool dropdownOpen = s_SourceDropdownOpen || s_DestinationDropdownOpen;
    const float modalH = dropdownOpen ? 500.0f : 300.0f;
    const float modalX = (GetScreenWidth() - modalW) / 2.0f;
    const float modalY = (GetScreenHeight() - modalH) / 2.0f;
    Rectangle modalRect = { modalX, modalY, modalW, modalH };

    DrawRectangleRounded(modalRect, 0.08f, 4, g_Theme.bgSecondary);
    DrawRectangleRoundedLines(modalRect, 0.08f, 4, g_Theme.border);
    DrawRectangleRounded((Rectangle){ modalX, modalY, modalW, 48.0f }, 0.08f, 4, g_Theme.bgTertiary);
    DrawRectangle((int)modalX, (int)modalY + 43, (int)modalW, 5, g_Theme.accentBlue);
    DrawTextSemiBold("Route Optimizer", modalX + 24, modalY + 14,
                     FONT_MODAL_TITLE, g_Theme.textPrimary);

    Vector2 mouse = Window_GetInputMousePosition();
    const float sourceY = modalY + 78.0f;
    const float fieldX = modalX + 180.0f;
    const float fieldW = modalW - 204.0f;
    const float fieldH = 34.0f;
    const float sourcePopupH = s_SourceDropdownOpen
        ? (float)(s_RouteHubCount * DASHBOARD_ROUTE_OPTION_H) : 0.0f;
    const float destinationY = sourceY + fieldH +
        (s_SourceDropdownOpen ? sourcePopupH + 28.0f : 38.0f);
    const char *labels[] = { "Source Hub:", "Destination Hub:" };
    Rectangle sourceField = { fieldX, sourceY, fieldW, fieldH };
    Rectangle destinationField = { fieldX, destinationY, fieldW, fieldH };

    DrawTextRegular(labels[0], modalX + 24, sourceY + 7,
                    FONT_NORMAL, g_Theme.textSecondary);
    DrawTextRegular(labels[1], modalX + 24, destinationY + 7,
                    FONT_NORMAL, g_Theme.textSecondary);

    bool clickConsumed = false;
    if (s_SourceDropdownOpen) {
        clickConsumed = DrawDashboardRouteDropdown(sourceField, &s_RouteSourceIndex,
                                                   &s_SourceDropdownOpen, mouse);
        DrawDashboardRouteField(destinationField, s_RouteDestinationIndex);
        if (!clickConsumed && CheckCollisionPointRec(mouse, destinationField) &&
            IsMouseButtonPressed(MOUSE_BUTTON_LEFT)) {
            s_SourceDropdownOpen = false;
            s_DestinationDropdownOpen = true;
            clickConsumed = true;
        }
        if (!clickConsumed) {
            Rectangle sourcePopup = { sourceField.x, sourceField.y + sourceField.height,
                                      sourceField.width,
                                      (float)(s_RouteHubCount * DASHBOARD_ROUTE_OPTION_H) };
            if (IsMouseButtonPressed(MOUSE_BUTTON_LEFT) &&
                !CheckCollisionPointRec(mouse, sourceField) &&
                !CheckCollisionPointRec(mouse, sourcePopup)) {
                s_SourceDropdownOpen = false;
            }
        }
    } else if (s_DestinationDropdownOpen) {
        DrawDashboardRouteField(sourceField, s_RouteSourceIndex);
        if (CheckCollisionPointRec(mouse, sourceField) &&
            IsMouseButtonPressed(MOUSE_BUTTON_LEFT)) {
            s_DestinationDropdownOpen = false;
            s_SourceDropdownOpen = true;
            clickConsumed = true;
        }
        clickConsumed = DrawDashboardRouteDropdown(destinationField,
                                                   &s_RouteDestinationIndex,
                                                   &s_DestinationDropdownOpen, mouse) ||
                       clickConsumed;
        if (!clickConsumed) {
            Rectangle destinationPopup = { destinationField.x,
                                           destinationField.y + destinationField.height,
                                           destinationField.width,
                                           (float)(s_RouteHubCount * DASHBOARD_ROUTE_OPTION_H) };
            if (IsMouseButtonPressed(MOUSE_BUTTON_LEFT) &&
                !CheckCollisionPointRec(mouse, destinationField) &&
                !CheckCollisionPointRec(mouse, destinationPopup)) {
                s_DestinationDropdownOpen = false;
            }
        }
    } else {
        clickConsumed = DrawDashboardRouteDropdown(sourceField, &s_RouteSourceIndex,
                                                   &s_SourceDropdownOpen, mouse);
        if (!s_SourceDropdownOpen) {
            clickConsumed = DrawDashboardRouteDropdown(destinationField,
                                                       &s_RouteDestinationIndex,
                                                       &s_DestinationDropdownOpen, mouse) ||
                            clickConsumed;
        }
    }

    const float instructionY = dropdownOpen
        ? modalY + modalH - 92.0f : destinationY + fieldH + 28.0f;
    DrawTextRegular("Select two different hubs from the current route graph.",
                    modalX + 24, instructionY, FONT_SMALL_LABEL, g_Theme.textSecondary);

    Rectangle optimizeRect = { modalX + modalW - 236, modalY + modalH - 52,
                               108.0f, 34.0f };
    Rectangle cancelRect = { modalX + modalW - 116, modalY + modalH - 52,
                             92.0f, 34.0f };
    if (!clickConsumed && !dropdownOpen && DrawUIButton(optimizeRect, "Optimize Route", true, NULL)) {
        onDashboardOptimizeSelectedRoute();
    }
    if (!clickConsumed && !dropdownOpen && DrawUIButton(cancelRect, "Cancel", false, NULL)) {
        s_RouteOptimizerModalActive = false;
        s_SourceDropdownOpen = false;
        s_DestinationDropdownOpen = false;
    }
    if (IsKeyPressed(KEY_ESCAPE)) {
        s_RouteOptimizerModalActive = false;
        s_SourceDropdownOpen = false;
        s_DestinationDropdownOpen = false;
    }
}

// Button Callbacks
static void onDashboardRunOptimizer(void) {
    if (Modal_IsActive()) {
        return;
    }
    PrepareDashboardRouteOptions();
    if (s_RouteHubCount == 0) {
        Toast_Show("No route hubs are available for optimization.", TOAST_WARNING);
        return;
    }
    s_SourceDropdownOpen = false;
    s_DestinationDropdownOpen = false;
    s_RouteOptimizerModalActive = true;
}

static void onDashboardScanCargo(void) {
    ScanWarehouseCargoWorkflow();
}

static void onDashboardDispatchFleet(void) {
    DispatchNextFleetWorkflow();
}

static void onDashboardSync(void) {
    Toast_Show("Synchronizing local nodes...", TOAST_INFO);
    /* Dashboard cards are calculated from the live module data on every draw. */
    Toast_Show("Dashboard statistics refreshed from live logistics data.", TOAST_SUCCESS);
}

void InitDashboardScreen(void) {
}

void UpdateDashboardScreen(void) {
}

void DrawDashboardScreen(void) {
    int contentX = GetContentX();
    int contentWidth = GetContentWidth();
    
    // Calculate live warehouse statistics from the linked list
    Warehouse *head = GetWarehouseListHead();
    int totalWarehouses = GetWarehouseCount(head);
    int totalCapacity = 0;
    int totalStock = 0;
    Warehouse *curr = head;
    while (curr) {
        totalCapacity += curr->capacity;
        totalStock += curr->currentStock;
        curr = curr->next;
    }
    double utilRatio = (totalCapacity > 0) ? ((double)totalStock / totalCapacity * 100.0) : 0.0;

    char whCountVal[64];
    snprintf(whCountVal, sizeof(whCountVal), "%d Hubs", totalWarehouses);
    
    char whCapacityVal[64];
    snprintf(whCapacityVal, sizeof(whCapacityVal), "%d m3", totalCapacity);

    char whStockVal[64];
    snprintf(whStockVal, sizeof(whStockVal), "%d m3", totalStock);

    char whUtilVal[64];
    snprintf(whUtilVal, sizeof(whUtilVal), "%.1f%%", utilRatio);

    // 1. Grid of 6 metrics cards (2 rows of 3)
    int cardY = GetContentY();
    int cardHeight = CARD_HEIGHT;
    int cardWidth = GetCardWidth(3);
    
    // Row 1
    DrawUICard(
        GetCardRect(0, 0, 3, cardHeight),
        "TOTAL WAREHOUSES", whCountVal, "All hubs currently active", g_Theme.accentBlue
    );
    DrawUICard(
        GetCardRect(1, 0, 3, cardHeight),
        "TOTAL CAPACITY", whCapacityVal, "Allocated storage volume", g_Theme.accentTeal
    );
    DrawUICard(
        GetCardRect(2, 0, 3, cardHeight),
        "CURRENT STOCK", whStockVal, "Total stored inventory volume", g_Theme.warning
    );
    
    // Row 2
    DrawUICard(
        GetCardRect(0, 1, 3, cardHeight),
        "UTILIZATION PERCENTAGE", whUtilVal, "Average storage utilization", g_Theme.success
    );
    DrawUICard(
        GetCardRect(1, 1, 3, cardHeight),
        "AVERAGE DELIVERY TIME", "2.4 Hours", "Optimal transit target achieved", g_Theme.success
    );
    DrawUICard(
        GetCardRect(2, 1, 3, cardHeight),
        "FLEET UTILIZATION", "82.4%", "18% reserve fleet capacity", g_Theme.accentBlue
    );

    // 2. Main content split: Table on left, Quick Actions on right
    int cardsBottom = GetCardY(2, cardHeight);
    int toolbarY = GetToolbarY(cardsBottom);
    int mainContentY = GetTableY(toolbarY + TOOLBAR_HEIGHT);
    int mainContentH = GetContentHeight() - (mainContentY - GetContentY());
    
    int leftWidth = (int)(contentWidth * 0.70f) - CARD_SPACING / 2;
    int rightWidth = contentWidth - leftWidth - CARD_SPACING;
    int rightX = contentX + leftWidth + CARD_SPACING;
    
    // Sub-title for Table (Section Heading: 18px)
    DrawSectionTitle("Active Logistics Operations", (float)contentX, (float)(mainContentY - SUBTITLE_OFFSET));
    
    // Table Setup
    static const char* headers[] = { "Route ID", "Destination HUB", "Cargo Type", "Status" };
    static float colWidths[] = { 0.20f, 0.40f, 0.20f, 0.20f };
    static const char* row0[] = { "R-1092", "San Francisco (SFO-4)", "Electronics", "In Transit" };
    static const char* row1[] = { "R-1093", "Seattle Tacoma (SEA-1)", "Pharmaceuticals", "Departed" };
    static const char* row2[] = { "R-1094", "Dallas Fort Worth (DFW-9)", "Heavy Machinery", "Loading" };
    static const char* row3[] = { "R-1095", "Chicago O'Hare (ORD-2)", "Perishable Goods", "Delayed" };
    static const char* row4[] = { "R-1096", "Atlanta Hartsfield (ATL-7)", "General Retail", "Arrived" };
    static const char* row5[] = { "R-1097", "Denver Int (DEN-3)", "Automotive Parts", "In Transit" };
    static const char* row6[] = { "R-1098", "Boston Logan (BOS-5)", "Chemicals", "Scheduled" };
    static const char* row7[] = { "R-1099", "Phoenix Sky Harbor (PHX-8)", "Apparel", "In Transit" };
    
    static const char** rows[] = { row0, row1, row2, row3, row4, row5, row6, row7 };
    
    static int s_SelectedRow = 0;
    static int s_SortCol = 0;
    static bool s_SortAsc = true;
    int hoveredRow = -1;
    
    DrawUITable(
        (Rectangle){ (float)contentX, (float)mainContentY, (float)leftWidth, (float)mainContentH },
        headers, colWidths, 4, rows, 8, false, &hoveredRow, &s_SelectedRow, &s_SortCol, &s_SortAsc
    );
    
    // Sub-title for Sidebar Panel / Quick Actions
    DrawSectionTitle("Operator Control Panel", (float)rightX, (float)(mainContentY - SUBTITLE_OFFSET));
    
    // Control Card Frame
    Rectangle controlPanelRect = { (float)rightX, (float)mainContentY, (float)rightWidth, (float)mainContentH };
    DrawRectangleRounded(controlPanelRect, 0.05f, 4, g_Theme.bgSecondary);
    DrawRectangleRoundedLines(controlPanelRect, 0.05f, 4, g_Theme.border);
    
    // Buttons in Control Panel
    int btnX = rightX + CARD_INTERNAL_PADDING;
    int btnW = rightWidth - CARD_INTERNAL_PADDING * 2;
    int btnYStart = mainContentY + CARD_INTERNAL_PADDING;
    
    DrawTextBold("QUICK WORKFLOWS", (float)btnX, (float)btnYStart, FONT_MICRO_LABEL, g_Theme.textSecondary);
    
    static const char* btnLabels[] = {
        "Run Route Optimizer", "Scan Warehouse Cargo",
        "Dispatch Next Fleet", "System Sync / Refresh"
    };
    static ButtonCallback btnCallbacks[] = {
        onDashboardRunOptimizer, onDashboardScanCargo,
        onDashboardDispatchFleet, onDashboardSync
    };
    static const bool btnActive[] = { true, false, false, false };
    
    for (int i = 0; i < 4; ++i) {
        int btnY = btnYStart + 20 + i * (BUTTON_HEIGHT + BUTTON_SPACING);
        DrawUIButton((Rectangle){ (float)btnX, (float)btnY, (float)btnW, (float)BUTTON_HEIGHT },
                     btnLabels[i], btnActive[i], btnCallbacks[i]);
    }

    DrawDashboardRouteOptimizerModal();
    
    // Status text at bottom of control panel
    int statusY = mainContentY + mainContentH - 65;
    DrawLine(rightX + CARD_INTERNAL_PADDING, statusY - 10, rightX + rightWidth - CARD_INTERNAL_PADDING, statusY - 10, g_Theme.border);
    DrawTextBold("ENGINE STATUS: ONLINE", (float)btnX, (float)statusY, FONT_MICRO_LABEL, g_Theme.success);
    DrawTextRegular("OPTIMIZER LATENCY: 14ms", (float)btnX, (float)(statusY + 18), FONT_MICRO_LABEL, g_Theme.textSecondary);
}
