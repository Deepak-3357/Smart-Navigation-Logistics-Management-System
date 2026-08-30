#include "screens.h"
#include "common.h"
#include "theme_manager.h"
#include "components.h"
#include "layout.h"
#include "route.h"
#include "window_manager.h"
#undef USE_LIBTYPE_SHARED
#include "raygui.h"
#include <stdio.h>
#include <string.h>

#define ROUTE_COUNT 7
#define HUB_COUNT 7
#define ROUTE_DROPDOWN_OPTION_H 28

typedef struct {
    const char *identifier;
    const char *transitNodes;
    const char *duration;
    const char *fuel;
    const char *conditions;
    int nodeIds[3];
    float totalHours;
} RouteMetadata;

/* Presentation metadata corresponding to the current Indian demo graph. */
static const RouteMetadata s_RouteMetadata[ROUTE_COUNT] = {
    { "R-CHN-01", "CHN -> BLR -> HYD", "7.0 Hours",  "92 Gallons",  "Clear",            { 1, 2, 3 }, 7.0f },
    { "R-BLR-02", "BLR -> CBE -> HYD", "4.0 Hours",  "52 Gallons",  "Moderate Traffic", { 2, 4, 3 }, 4.0f },
    { "R-CHN-03", "CHN -> BLR -> CBE", "6.0 Hours",  "76 Gallons",  "Clear",            { 1, 2, 4 }, 6.0f },
    { "R-HYD-04", "HYD -> MUM -> DEL", "11.0 Hours", "145 Gallons", "Heavy Traffic",    { 3, 5, 6 }, 11.0f },
    { "R-CBE-05", "CBE -> HYD -> MUM", "7.0 Hours",  "96 Gallons",  "Clear",            { 4, 3, 5 }, 7.0f },
    { "R-MUM-06", "MUM -> DEL -> KOL", "10.0 Hours", "145 Gallons", "Moderate Traffic", { 5, 6, 7 }, 10.0f },
    { "R-HYD-07", "HYD -> MUM -> DEL", "11.0 Hours", "150 Gallons", "Weather Alert",    { 3, 5, 6 }, 11.0f }
};

static const char *s_HubNames[HUB_COUNT] = {
    "CHN", "BLR", "HYD", "CBE", "MUM", "DEL", "KOL"
};

typedef struct {
    int sourceId;
    int destinationId;
    float weight;
} DemoEdge;

/*
 * Academic demonstration weights, not live distance, duration, or pricing.
 * Each listed connection is bidirectional, so BuildRouteGraph adds both
 * directed RouteEdge records explicitly.
 *
 * Edge insertion order is intentional: it makes the traversal output stable
 * with the existing head-inserted adjacency lists.
 */
static const DemoEdge s_DemoEdges[] = {
    { 1, 2, 4.0f }, /* CHN <-> BLR */
    { 2, 4, 2.0f }, /* BLR <-> CBE */
    { 2, 3, 3.0f }, /* BLR <-> HYD */
    { 4, 3, 2.0f }, /* CBE <-> HYD */
    { 3, 5, 5.0f }, /* HYD <-> MUM */
    { 5, 6, 6.0f }, /* MUM <-> DEL */
    { 6, 7, 4.0f }  /* DEL <-> KOL */
};

static RouteGraph s_RouteGraph;
static bool s_GraphInitialized = false;

static bool s_PathModalActive = false;
static int s_PathHubCount = 0;
static int s_PathSourceIndex = 0;
static int s_PathDestinationIndex = 2;
static int s_PathHubIds[HUB_COUNT];
static const RouteNode *s_PathHubNodes[HUB_COUNT];
static bool s_SourceDropdownOpen = false;
static bool s_DestinationDropdownOpen = false;

const RouteGraph *GetRouteGraph(void) {
    return &s_RouteGraph;
}

static const char *HubName(int nodeId) {
    if (nodeId < 1 || nodeId > HUB_COUNT) {
        return "Unknown";
    }
    return s_HubNames[nodeId - 1];
}

static bool BuildRouteGraph(void) {
    if (s_GraphInitialized) {
        return true;
    }
    if (!InitializeRouteGraph(&s_RouteGraph)) {
        return false;
    }

    for (int i = 0; i < HUB_COUNT; ++i) {
        if (!AddRouteNode(&s_RouteGraph, i + 1, s_HubNames[i])) {
            DestroyRouteGraph(&s_RouteGraph);
            return false;
        }
    }

    for (size_t i = 0; i < sizeof(s_DemoEdges) / sizeof(s_DemoEdges[0]); ++i) {
        const DemoEdge edge = s_DemoEdges[i];
        if (!AddRouteEdge(&s_RouteGraph, edge.sourceId, edge.destinationId, edge.weight) ||
            !AddRouteEdge(&s_RouteGraph, edge.destinationId, edge.sourceId, edge.weight)) {
            DestroyRouteGraph(&s_RouteGraph);
            return false;
        }
    }

    s_GraphInitialized = true;
    return true;
}

static void ResetPathFields(void) {
    s_PathSourceIndex = 0;
    s_PathDestinationIndex = 2;
    s_SourceDropdownOpen = false;
    s_DestinationDropdownOpen = false;
}

static void PreparePathHubOptions(void) {
    s_PathHubCount = 0;
    for (const RouteNode *node = s_RouteGraph.nodesHead;
         node && s_PathHubCount < HUB_COUNT; node = node->next) {
        int insertAt = s_PathHubCount++;
        while (insertAt > 0 && s_PathHubIds[insertAt - 1] > node->nodeId) {
            s_PathHubIds[insertAt] = s_PathHubIds[insertAt - 1];
            s_PathHubNodes[insertAt] = s_PathHubNodes[insertAt - 1];
            --insertAt;
        }
        s_PathHubIds[insertAt] = node->nodeId;
        s_PathHubNodes[insertAt] = node;
    }

    s_PathSourceIndex = 0;
    s_PathDestinationIndex = s_PathHubCount > 2 ? 2 : (s_PathHubCount > 1 ? 1 : 0);
}

static void FormatTraversal(const char *label, const int *order, int count, char *out, size_t outSize) {
    size_t used = (size_t)snprintf(out, outSize, "%s:", label);
    for (int i = 0; i < count && used < outSize; ++i) {
        int written = snprintf(out + used, outSize - used, "%s%s", i == 0 ? " " : " -> ", HubName(order[i]));
        if (written < 0 || (size_t)written >= outSize - used) {
            break;
        }
        used += (size_t)written;
    }
}

static void onSolveNetwork(void) {
    if (!s_GraphInitialized && !BuildRouteGraph()) {
        Toast_Show("Unable to initialize the route graph.", TOAST_ERROR);
        return;
    }
    if (GetRouteNodeCount(&s_RouteGraph) == 0) {
        Toast_Show("Route graph is empty; nothing to solve.", TOAST_WARNING);
        return;
    }

    int bfsOrder[MAX_TRAVERSAL_NODES];
    int dfsOrder[MAX_TRAVERSAL_NODES];
    int bfsCount = 0;
    int dfsCount = 0;
    const int sourceId = 1;
    const int destinationId = 3;

    if (!BFSRouteSearch(&s_RouteGraph, sourceId, destinationId, bfsOrder, &bfsCount) ||
        !DFSRouteSearch(&s_RouteGraph, sourceId, dfsOrder, &dfsCount)) {
        Toast_Show("Route graph analysis could not reach the selected hubs.", TOAST_WARNING);
        return;
    }

    char bfsText[512];
    char dfsText[512];
    char result[1100];
    FormatTraversal("BFS", bfsOrder, bfsCount, bfsText, sizeof(bfsText));
    FormatTraversal("DFS", dfsOrder, dfsCount, dfsText, sizeof(dfsText));
    snprintf(result, sizeof(result),
             "Graph nodes: %zu\nGraph edges: %zu\n\nAnalysis from %s to %s\n%s\n%s",
             GetRouteNodeCount(&s_RouteGraph), GetRouteEdgeCount(&s_RouteGraph),
             HubName(sourceId), HubName(destinationId), bfsText, dfsText);
    Modal_Show("Route Network Analysis", result, MODAL_SUCCESS, NULL);

    char toast[128];
    snprintf(toast, sizeof(toast), "Network solved: %d hubs reached from %s.", dfsCount, HubName(sourceId));
    Toast_Show(toast, TOAST_SUCCESS);
}

void RunRouteOptimizerWorkflow(void) {
    if (!s_GraphInitialized && !BuildRouteGraph()) {
        Toast_Show("Unable to initialize the route graph.", TOAST_ERROR);
        return;
    }
    if (GetRouteNodeCount(&s_RouteGraph) == 0 || ROUTE_COUNT == 0) {
        Toast_Show("Route graph is empty; no route can be optimized.", TOAST_WARNING);
        return;
    }

    /* Use the first active route record as the dashboard's default optimization request. */
    int sourceId = s_RouteMetadata[0].nodeIds[0];
    int destinationId = s_RouteMetadata[0].nodeIds[2];
    float totalDistance = 0.0f;
    int path[MAX_TRAVERSAL_NODES];
    int pathLength = 0;

    if (!FindShortestRoute(&s_RouteGraph, sourceId, destinationId,
                           &totalDistance, path, &pathLength)) {
        Toast_Show("No valid route is available for optimization.", TOAST_WARNING);
        return;
    }

    char pathText[512] = "";
    size_t used = 0;
    for (int i = 0; i < pathLength && used < sizeof(pathText); ++i) {
        int written = snprintf(pathText + used, sizeof(pathText) - used,
                                "%s%s", i == 0 ? "" : " -> ", HubName(path[i]));
        if (written < 0 || (size_t)written >= sizeof(pathText) - used) {
            break;
        }
        used += (size_t)written;
    }

    char result[800];
    snprintf(result, sizeof(result),
             "Source: %s\nDestination: %s\n\nOptimal Route:\n%s\n\n"
             "Total Distance: %.2f hours\nNodes: %d",
             HubName(sourceId), HubName(destinationId), pathText,
             totalDistance, pathLength);
    Modal_Show("Route Optimizer", result, MODAL_SUCCESS, NULL);

    char toast[160];
    snprintf(toast, sizeof(toast), "Route optimized across %d graph nodes.", pathLength);
    Toast_Show(toast, TOAST_SUCCESS);
}

static void onCalculateShortestPath(void) {
    if (!s_GraphInitialized && !BuildRouteGraph()) {
        Toast_Show("Unable to initialize the route graph.", TOAST_ERROR);
        return;
    }

    if (s_PathHubCount <= 0 || s_PathSourceIndex < 0 ||
        s_PathSourceIndex >= s_PathHubCount || s_PathDestinationIndex < 0 ||
        s_PathDestinationIndex >= s_PathHubCount) {
        Toast_Show("Select valid source and destination hubs.", TOAST_ERROR);
        return;
    }

    const int sourceId = s_PathHubIds[s_PathSourceIndex];
    const int destinationId = s_PathHubIds[s_PathDestinationIndex];
    if (sourceId == destinationId) {
        Toast_Show("Source and destination hubs must be different.", TOAST_WARNING);
        return;
    }

    float totalDistance = 0.0f;
    int path[MAX_TRAVERSAL_NODES];
    int pathLength = 0;
    if (!FindShortestRoute(&s_RouteGraph, sourceId, destinationId, &totalDistance, path, &pathLength)) {
        s_PathModalActive = false;
        s_SourceDropdownOpen = false;
        s_DestinationDropdownOpen = false;
        char warning[192];
        snprintf(warning, sizeof(warning), "No route exists from %s to %s.", HubName(sourceId), HubName(destinationId));
        Modal_Show("No Route Available", warning, MODAL_WARNING, NULL);
        Toast_Show("No reachable path was found between those hubs.", TOAST_WARNING);
        return;
    }

    char pathText[512] = "";
    size_t used = 0;
    for (int i = 0; i < pathLength && used < sizeof(pathText); ++i) {
        int written = snprintf(pathText + used, sizeof(pathText) - used, "%s%s", i == 0 ? "" : " -> ", HubName(path[i]));
        if (written < 0 || (size_t)written >= sizeof(pathText) - used) {
            break;
        }
        used += (size_t)written;
    }

    s_PathModalActive = false;
    s_SourceDropdownOpen = false;
    s_DestinationDropdownOpen = false;
    char result[800];
    snprintf(result, sizeof(result),
             "Source: %s\nDestination: %s\n\nShortest Path:\n%s\n\nTotal Distance: %.2f hours\nNodes in Path: %d",
             HubName(sourceId), HubName(destinationId), pathText, totalDistance, pathLength);
    Modal_Show("Shortest Route Result", result, MODAL_SUCCESS, NULL);

    char toast[160];
    snprintf(toast, sizeof(toast), "Shortest route %s -> %s calculated.", HubName(sourceId), HubName(destinationId));
    Toast_Show(toast, TOAST_SUCCESS);
}

static void onOpenShortestPathModal(void) {
    if (!s_GraphInitialized && !BuildRouteGraph()) {
        Toast_Show("Unable to initialize the route graph.", TOAST_ERROR);
        return;
    }
    ResetPathFields();
    PreparePathHubOptions();
    s_PathModalActive = true;
}

static void onResetConfirm(bool confirmed) {
    if (!confirmed) {
        return;
    }

    if (s_GraphInitialized) {
        DestroyRouteGraph(&s_RouteGraph);
        s_GraphInitialized = false;
    }
    ResetPathFields();

    if (BuildRouteGraph()) {
        Toast_Show("Route graph reset and rebuilt from active route data.", TOAST_SUCCESS);
    } else {
        Toast_Show("Route graph reset, but reinitialization failed.", TOAST_ERROR);
    }
}

static void onResetGraph(void) {
    if (!s_GraphInitialized || GetRouteNodeCount(&s_RouteGraph) == 0) {
        Toast_Show("Route graph is already empty.", TOAST_WARNING);
        return;
    }
    Modal_Show("Reset Graph Nodes", "Reset and rebuild the route graph from the active route data?", MODAL_CONFIRMATION, onResetConfirm);
}

void InitRoutesScreen(void) {
    s_PathModalActive = false;
    ResetPathFields();
    if (!BuildRouteGraph()) {
        Toast_Show("Route graph initialization failed.", TOAST_ERROR);
    }
}

void UpdateRoutesScreen(void) {
}

static void DrawRouteDropdownField(Rectangle field, int selectedIndex) {
    const char *selectedText = "Select hub";
    if (selectedIndex >= 0 && selectedIndex < s_PathHubCount) {
        selectedText = s_PathHubNodes[selectedIndex]->name;
    }
    DrawRectangleRounded(field, 0.12f, 4, g_Theme.bgTertiary);
    DrawRectangleRoundedLines(field, 0.12f, 4, g_Theme.border);
    DrawTextRegular(selectedText, field.x + 12, field.y + 8,
                    FONT_NORMAL, g_Theme.textPrimary);
    DrawTextRegular("v", field.x + field.width - 20, field.y + 8,
                    FONT_NORMAL, g_Theme.textSecondary);
}

static bool DrawRouteDropdown(Rectangle field, int *selectedIndex, bool *isOpen,
                              Vector2 mouse) {
    bool consumedClick = false;
    DrawRouteDropdownField(field, *selectedIndex);

    if (CheckCollisionPointRec(mouse, field) && IsMouseButtonPressed(MOUSE_BUTTON_LEFT)) {
        *isOpen = !*isOpen;
        consumedClick = true;
    }

    if (*isOpen) {
        Rectangle popup = { field.x, field.y + field.height, field.width,
                            (float)(s_PathHubCount * ROUTE_DROPDOWN_OPTION_H) };
        DrawRectangle(popup.x, popup.y, popup.width, popup.height, g_Theme.bgTertiary);
        DrawRectangleLines((int)popup.x, (int)popup.y,
                           (int)popup.width, (int)popup.height, g_Theme.border);
        for (int i = 0; i < s_PathHubCount; ++i) {
            Rectangle option = { popup.x, popup.y + i * ROUTE_DROPDOWN_OPTION_H,
                                 popup.width, ROUTE_DROPDOWN_OPTION_H };
            bool hovered = CheckCollisionPointRec(mouse, option);
            if (hovered) {
                DrawRectangle(option.x, option.y, option.width, option.height,
                              g_Theme.hoverColor);
            }
            DrawTextRegular(s_PathHubNodes[i]->name, option.x + 12, option.y + 6,
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

static void DrawShortestPathModal(void) {
    DrawRectangle(0, 0, GetScreenWidth(), GetScreenHeight(), ColorAlpha(BLACK, 0.4f));

    const float modalW = 500.0f;
    const bool dropdownOpen = s_SourceDropdownOpen || s_DestinationDropdownOpen;
    const float modalH = dropdownOpen ? 500.0f : 280.0f;
    const float modalX = (GetScreenWidth() - modalW) / 2.0f;
    const float modalY = (GetScreenHeight() - modalH) / 2.0f;
    Rectangle modalRect = { modalX, modalY, modalW, modalH };

    DrawRectangleRounded(modalRect, 0.08f, 4, g_Theme.bgSecondary);
    DrawRectangleRoundedLines(modalRect, 0.08f, 4, g_Theme.border);
    DrawRectangleRounded((Rectangle){ modalX, modalY, modalW, 48.0f }, 0.08f, 4, g_Theme.bgTertiary);
    DrawRectangle((int)modalX, (int)modalY + 43, (int)modalW, 5, g_Theme.accentBlue);
    DrawTextSemiBold("Find Optimized Route", modalX + 24, modalY + 14,
                     FONT_MODAL_TITLE, g_Theme.textPrimary);

    Vector2 mouse = Window_GetInputMousePosition();
    const float sourceY = modalY + 78.0f;
    const float fieldX = modalX + 170.0f;
    const float fieldW = modalW - 194.0f;
    const float fieldH = 34.0f;
    const float sourcePopupH = s_SourceDropdownOpen
        ? (float)(s_PathHubCount * ROUTE_DROPDOWN_OPTION_H) : 0.0f;
    const float destinationY = sourceY + fieldH +
        (s_SourceDropdownOpen ? sourcePopupH + 28.0f : 18.0f);
    Rectangle sourceField = { fieldX, sourceY, fieldW, fieldH };
    Rectangle destinationField = { fieldX, destinationY, fieldW, fieldH };

    DrawTextRegular("Source Hub:", modalX + 24, sourceY + 7,
                    FONT_NORMAL, g_Theme.textSecondary);
    DrawTextRegular("Destination Hub:", modalX + 24, destinationY + 7,
                    FONT_NORMAL, g_Theme.textSecondary);

    bool clickConsumed = false;
    if (s_SourceDropdownOpen) {
        clickConsumed = DrawRouteDropdown(sourceField, &s_PathSourceIndex,
                                          &s_SourceDropdownOpen, mouse);
        DrawRouteDropdownField(destinationField, s_PathDestinationIndex);
        if (!clickConsumed && CheckCollisionPointRec(mouse, destinationField) &&
            IsMouseButtonPressed(MOUSE_BUTTON_LEFT)) {
            s_SourceDropdownOpen = false;
            s_DestinationDropdownOpen = true;
            clickConsumed = true;
        }
        if (!clickConsumed && IsMouseButtonPressed(MOUSE_BUTTON_LEFT)) {
            Rectangle popup = { sourceField.x, sourceField.y + sourceField.height,
                                sourceField.width, sourcePopupH };
            if (!CheckCollisionPointRec(mouse, sourceField) &&
                !CheckCollisionPointRec(mouse, popup)) {
                s_SourceDropdownOpen = false;
            }
        }
    } else if (s_DestinationDropdownOpen) {
        DrawRouteDropdownField(sourceField, s_PathSourceIndex);
        if (CheckCollisionPointRec(mouse, sourceField) &&
            IsMouseButtonPressed(MOUSE_BUTTON_LEFT)) {
            s_DestinationDropdownOpen = false;
            s_SourceDropdownOpen = true;
            clickConsumed = true;
        }
        clickConsumed = DrawRouteDropdown(destinationField, &s_PathDestinationIndex,
                                          &s_DestinationDropdownOpen, mouse) ||
                        clickConsumed;
        if (!clickConsumed && IsMouseButtonPressed(MOUSE_BUTTON_LEFT)) {
            Rectangle popup = { destinationField.x, destinationField.y + destinationField.height,
                                destinationField.width,
                                (float)(s_PathHubCount * ROUTE_DROPDOWN_OPTION_H) };
            if (!CheckCollisionPointRec(mouse, destinationField) &&
                !CheckCollisionPointRec(mouse, popup)) {
                s_DestinationDropdownOpen = false;
            }
        }
    } else {
        clickConsumed = DrawRouteDropdown(sourceField, &s_PathSourceIndex,
                                          &s_SourceDropdownOpen, mouse);
        if (!s_SourceDropdownOpen) {
            clickConsumed = DrawRouteDropdown(destinationField, &s_PathDestinationIndex,
                                              &s_DestinationDropdownOpen, mouse) ||
                            clickConsumed;
        }
    }

    float instructionY = dropdownOpen ? modalY + modalH - 92.0f
                                      : destinationY + fieldH + 24.0f;
    DrawTextRegular("Select two different hubs from the current route graph.",
                    modalX + 24, instructionY, FONT_SMALL_LABEL, g_Theme.textSecondary);

    Rectangle calculateRect = { modalX + modalW - 236, modalY + modalH - 52,
                                100.0f, 34.0f };
    Rectangle cancelRect = { modalX + modalW - 124, modalY + modalH - 52,
                             100.0f, 34.0f };
    if (!clickConsumed && !dropdownOpen && DrawUIButton(calculateRect, "Calculate", true, NULL)) {
        onCalculateShortestPath();
    }
    if (!clickConsumed && !dropdownOpen && DrawUIButton(cancelRect, "Cancel", false, NULL)) {
        s_PathModalActive = false;
        s_SourceDropdownOpen = false;
        s_DestinationDropdownOpen = false;
    }
    if (IsKeyPressed(KEY_ESCAPE)) {
        s_PathModalActive = false;
        s_SourceDropdownOpen = false;
        s_DestinationDropdownOpen = false;
    }
}

void DrawRoutesScreen(void) {
    int contentX = GetContentX();
    int contentWidth = GetContentWidth();
    int cardY = GetContentY();
    int cardHeight = CARD_HEIGHT;
    bool blockParentInput = s_PathModalActive || Modal_IsActive();

    char nodeCard[32];
    char edgeCard[32];
    char stateCard[32];
    snprintf(nodeCard, sizeof(nodeCard), "%zu Hubs", GetRouteNodeCount(&s_RouteGraph));
    snprintf(edgeCard, sizeof(edgeCard), "%zu Links", GetRouteEdgeCount(&s_RouteGraph));
    snprintf(stateCard, sizeof(stateCard), "%s", s_GraphInitialized ? "Ready" : "Unavailable");

    DrawUICard(GetCardRect(0, 0, 3, cardHeight), "ROUTE GRAPH NODES", nodeCard, "Active logistics hubs", g_Theme.accentTeal);
    DrawUICard(GetCardRect(1, 0, 3, cardHeight), "WEIGHTED CONNECTIONS", edgeCard, "Direct route links", g_Theme.accentBlue);
    DrawUICard(GetCardRect(2, 0, 3, cardHeight), "GRAPH STATUS", stateCard, "BFS / DFS / Dijkstra ready", g_Theme.success);

    int toolbarY = GetToolbarY(cardY + cardHeight);
    int btnW = 145;
    DrawUIButton((Rectangle){ (float)contentX, (float)toolbarY, (float)btnW, (float)BUTTON_HEIGHT }, "Solve Network", !blockParentInput, blockParentInput ? NULL : onSolveNetwork);
    DrawUIButton((Rectangle){ (float)(contentX + 1 * (btnW + BUTTON_SPACING)), (float)toolbarY, (float)btnW, (float)BUTTON_HEIGHT }, "Find Shortest Path", !blockParentInput, blockParentInput ? NULL : onOpenShortestPathModal);
    DrawUIButton((Rectangle){ (float)(contentX + 2 * (btnW + BUTTON_SPACING)), (float)toolbarY, (float)btnW, (float)BUTTON_HEIGHT }, "Reset Graph Nodes", !blockParentInput, blockParentInput ? NULL : onResetGraph);

    int tableY = GetTableY(toolbarY + BUTTON_HEIGHT);
    int tableH = GetScreenHeight() - tableY - STATUS_BAR_HEIGHT;
    DrawSectionTitle("Active Dispatch Fleet Routes", (float)contentX, (float)(tableY - SUBTITLE_OFFSET));

    static const char *headers[] = { "Route Identifier", "Transit Nodes", "Travel Duration", "Fuel Allocation", "Transit Conditions" };
    static const float colWidths[] = { 0.18f, 0.35f, 0.15f, 0.17f, 0.15f };
    static const char *rowCells[ROUTE_COUNT][5];
    static const char **rowPointers[ROUTE_COUNT];
    static bool rowsInitialized = false;
    if (!rowsInitialized) {
        for (int i = 0; i < ROUTE_COUNT; ++i) {
            rowCells[i][0] = s_RouteMetadata[i].identifier;
            rowCells[i][1] = s_RouteMetadata[i].transitNodes;
            rowCells[i][2] = s_RouteMetadata[i].duration;
            rowCells[i][3] = s_RouteMetadata[i].fuel;
            rowCells[i][4] = s_RouteMetadata[i].conditions;
            rowPointers[i] = rowCells[i];
        }
        rowsInitialized = true;
    }

    static int selectedRow = 0;
    static int sortCol = 0;
    static bool sortAsc = true;
    int hoveredRow = -1;
    DrawUITable((Rectangle){ (float)contentX, (float)tableY, (float)contentWidth, (float)tableH },
                headers, colWidths, 5, rowPointers, ROUTE_COUNT, false, &hoveredRow, &selectedRow, &sortCol, &sortAsc);

    if (s_PathModalActive) {
        DrawShortestPathModal();
    }
}
