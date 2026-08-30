#include "screens.h"
#include "common.h"
#include "theme_manager.h"
#include "components.h"
#include "layout.h"
#include "package.h"
#include "window_manager.h"
#undef USE_LIBTYPE_SHARED
#include "raygui.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static PackageQueue s_PackageQueue;
static bool s_QueueInitialized = false;
static int s_DispatchedTodayCount = 0;

// Form state buffers
static bool s_FormModalActive = false;
static int s_ActiveEditField = -1; // -1 = none, 0 = ID, 1 = Origin, 2 = Destination, 3 = Cargo Type, 4 = Weight, 5 = Priority, 6 = Status

static char s_FormIdBuf[16] = "";
static char s_FormOriginBuf[PACKAGE_ORIGIN_LEN] = "";
static char s_FormDestBuf[PACKAGE_DESTINATION_LEN] = "";
static char s_FormCargoTypeBuf[PACKAGE_CARGO_TYPE_LEN] = "";
static char s_FormWeightBuf[16] = "";
static char s_FormPriorityBuf[16] = "";
static char s_FormStatusBuf[PACKAGE_STATUS_LEN] = "";

// Table data cell storage
#define MAX_TABLE_ROWS 100
#define TABLE_COLS       6
#define CELL_LEN         64
static char s_TableCells[MAX_TABLE_ROWS][TABLE_COLS][CELL_LEN];
static const char* s_TableRowPtrs[MAX_TABLE_ROWS][TABLE_COLS];
static const char** s_TableRowsPtrs[MAX_TABLE_ROWS];
static Package s_TablePackages[MAX_TABLE_ROWS];
static Package* s_TableRowPackagePtrs[MAX_TABLE_ROWS];
static int s_SelectedRow = 0;
static int s_SortCol = 4;
static bool s_SortAsc = true;

static int CompareTablePackages(const Package *left, const Package *right, int sortCol) {
    switch (sortCol) {
        case 0: return left->packageId - right->packageId;
        case 1: return strcmp(left->origin, right->origin);
        case 2: return strcmp(left->destination, right->destination);
        case 3: return left->weight < right->weight ? -1 : (left->weight > right->weight ? 1 : 0);
        case 4: return left->priority - right->priority;
        case 5: return strcmp(left->status, right->status);
        default: return 0;
    }
}

static void SortTableRows(int rowCount) {
    for (int i = 1; i < rowCount; ++i) {
        Package *candidate = s_TableRowPackagePtrs[i];
        const char **candidateRow = s_TableRowsPtrs[i];
        int j = i - 1;
        while (j >= 0) {
            int comparison = CompareTablePackages(s_TableRowPackagePtrs[j], candidate, s_SortCol);
            bool shouldMove = s_SortAsc ? comparison > 0 : comparison < 0;
            if (!shouldMove) {
                break;
            }
            s_TableRowPackagePtrs[j + 1] = s_TableRowPackagePtrs[j];
            s_TableRowsPtrs[j + 1] = s_TableRowsPtrs[j];
            --j;
        }
        s_TableRowPackagePtrs[j + 1] = candidate;
        s_TableRowsPtrs[j + 1] = candidateRow;
    }
}

const PackageQueueNode *GetPackageQueueHead(void) {
    return s_PackageQueue.front;
}

static bool IsDuplicatePackageID(int id) {
    PackageQueueNode *curr = s_PackageQueue.front;
    while (curr) {
        if (curr->package.packageId == id) {
            return true;
        }
        curr = curr->next;
    }
    return false;
}

static void PopulateDemoPackages(void) {
    static const Package demoPackages[] = {
        { 1, "Chennai",    "Bengaluru", "Electronics",          125.5f, 2, "Pending" },
        { 2, "Mumbai",     "Delhi",     "Pharmaceuticals",        85.0f, 2, "In Transit" },
        { 3, "Chennai",    "Hyderabad", "Automobile Parts",      210.0f, 1, "Pending" },
        { 4, "Kolkata",    "Delhi",     "Textiles",              175.5f, 0, "Pending" },
        { 5, "Bengaluru",  "Mumbai",    "Consumer Electronics",    95.0f, 1, "In Transit" },
        { 6, "Coimbatore", "Chennai",   "Machinery Components",   320.0f, 2, "Pending" },
        { 7, "Delhi",      "Mumbai",    "Industrial Equipment",   450.0f, 0, "Pending" }
    };

    for (size_t i = 0; i < sizeof(demoPackages) / sizeof(demoPackages[0]); ++i) {
        EnqueuePackage(&s_PackageQueue, &demoPackages[i]);
    }
}

static void onSaveEnqueue(void) {
    int id = atoi(s_FormIdBuf);
    float weight = atof(s_FormWeightBuf);
    int priority = atoi(s_FormPriorityBuf);

    if (s_FormIdBuf[0] == '\0' || id <= 0) {
        Toast_Show("Error: Package ID must be a positive number!", TOAST_ERROR);
        return;
    }
    if (IsDuplicatePackageID(id)) {
        Toast_Show("Error: Duplicate Package ID!", TOAST_ERROR);
        return;
    }
    if (s_FormOriginBuf[0] == '\0') {
        Toast_Show("Error: Origin HUB cannot be empty!", TOAST_ERROR);
        return;
    }
    if (s_FormDestBuf[0] == '\0') {
        Toast_Show("Error: Destination HUB cannot be empty!", TOAST_ERROR);
        return;
    }
    if (s_FormCargoTypeBuf[0] == '\0') {
        Toast_Show("Error: Cargo Type cannot be empty!", TOAST_ERROR);
        return;
    }
    if (s_FormWeightBuf[0] == '\0' || weight < 0.0f) {
        Toast_Show("Error: Weight must be 0 or more!", TOAST_ERROR);
        return;
    }
    if (s_FormPriorityBuf[0] == '\0' || priority < 0) {
        Toast_Show("Error: Priority must be 0 or more!", TOAST_ERROR);
        return;
    }
    if (s_FormStatusBuf[0] == '\0') {
        Toast_Show("Error: Status cannot be empty!", TOAST_ERROR);
        return;
    }

    Package pkg;
    pkg.packageId = id;
    strncpy(pkg.origin, s_FormOriginBuf, sizeof(pkg.origin) - 1);
    pkg.origin[sizeof(pkg.origin) - 1] = '\0';
    strncpy(pkg.destination, s_FormDestBuf, sizeof(pkg.destination) - 1);
    pkg.destination[sizeof(pkg.destination) - 1] = '\0';
    strncpy(pkg.cargoType, s_FormCargoTypeBuf, sizeof(pkg.cargoType) - 1);
    pkg.cargoType[sizeof(pkg.cargoType) - 1] = '\0';
    pkg.weight = weight;
    pkg.priority = priority;
    strncpy(pkg.status, s_FormStatusBuf, sizeof(pkg.status) - 1);
    pkg.status[sizeof(pkg.status) - 1] = '\0';

    if (EnqueuePackage(&s_PackageQueue, &pkg)) {
        Toast_Show("Package enqueued successfully!", TOAST_SUCCESS);
        s_FormModalActive = false;
        s_ActiveEditField = -1;
    } else {
        Toast_Show("Error: Enqueue failed!", TOAST_ERROR);
    }
}

static void onEnqueueCallback(void) {
    s_FormIdBuf[0] = '\0';
    s_FormOriginBuf[0] = '\0';
    s_FormDestBuf[0] = '\0';
    s_FormCargoTypeBuf[0] = '\0';
    s_FormWeightBuf[0] = '\0';
    s_FormPriorityBuf[0] = '\0';
    strcpy(s_FormStatusBuf, "Pending");

    s_FormModalActive = true;
    s_ActiveEditField = -1;
}

static void onDispatchCallback(void) {
    if (IsPackageQueueEmpty(&s_PackageQueue)) {
        Toast_Show("Queue is empty! No package to dispatch.", TOAST_WARNING);
        return;
    }

    Package dispatchedPkg;
    if (DequeuePackage(&s_PackageQueue, &dispatchedPkg)) {
        s_DispatchedTodayCount++;
        char msg[128];
        snprintf(msg, sizeof(msg), "Package #%d successfully dispatched!", dispatchedPkg.packageId);
        Toast_Show(msg, TOAST_SUCCESS);
    } else {
        Toast_Show("Error: Failed to dequeue package.", TOAST_ERROR);
    }
}

void DispatchNextFleetWorkflow(void) {
    if (!s_QueueInitialized) {
        if (!InitializePackageQueue(&s_PackageQueue)) {
            Toast_Show("Package queue initialization failed.", TOAST_ERROR);
            return;
        }
        s_QueueInitialized = true;
    }

    if (IsPackageQueueEmpty(&s_PackageQueue)) {
        Toast_Show("No packages waiting for dispatch.", TOAST_WARNING);
        return;
    }

    Package dispatchedPackage;
    if (!DequeuePackage(&s_PackageQueue, &dispatchedPackage)) {
        Toast_Show("Unable to dispatch the next package.", TOAST_ERROR);
        return;
    }

    s_DispatchedTodayCount++;
    char result[512];
    snprintf(result, sizeof(result),
             "Package #%d dispatched\nDestination: %s\nCargo Type: %s\n"
             "Weight: %.2f kg\nPriority: %d",
             dispatchedPackage.packageId, dispatchedPackage.destination,
             dispatchedPackage.cargoType, dispatchedPackage.weight,
             dispatchedPackage.priority);
    Modal_Show("Fleet Dispatch", result, MODAL_SUCCESS, NULL);
    Toast_Show("Front package dispatched using FIFO order.", TOAST_SUCCESS);
}

static void onPeekCallback(void) {
    if (IsPackageQueueEmpty(&s_PackageQueue)) {
        Toast_Show("Queue is empty! Nothing to peek.", TOAST_WARNING);
        return;
    }

    Package pkg;
    if (PeekPackage(&s_PackageQueue, &pkg)) {
        char msg[512];
        snprintf(msg, sizeof(msg),
                 "Package ID: #%d\n"
                 "Origin: %s\n"
                 "Destination: %s\n"
                 "Cargo Type: %s\n"
                 "Weight: %.2f kg\n"
                 "Priority: %d\n"
                 "Status: %s",
                 pkg.packageId, pkg.origin, pkg.destination, pkg.cargoType, pkg.weight, pkg.priority, pkg.status);
        Modal_Show("Peek Queue Head", msg, MODAL_INFO, NULL);
    } else {
        Toast_Show("Error: Failed to peek package.", TOAST_ERROR);
    }
}

static void onClearConfirm(bool confirmed) {
    if (confirmed) {
        DestroyPackageQueue(&s_PackageQueue);
        InitializePackageQueue(&s_PackageQueue);
        Toast_Show("Package registry queue cleared successfully!", TOAST_SUCCESS);
    }
}

static void onClearQueueCallback(void) {
    if (IsPackageQueueEmpty(&s_PackageQueue)) {
        Toast_Show("Registry queue is already empty.", TOAST_WARNING);
        return;
    }
    Modal_Show("Clear Package Registry", "Are you sure you want to clear the entire package queue?", MODAL_CONFIRMATION, onClearConfirm);
}

static int PopulateTableData(void) {
    PackageQueueNode *curr = s_PackageQueue.front;
    int row = 0;
    while (curr && row < MAX_TABLE_ROWS) {
        s_TablePackages[row] = curr->package;
        snprintf(s_TableCells[row][0], CELL_LEN, "#%d", curr->package.packageId);
        snprintf(s_TableCells[row][1], CELL_LEN, "%s", curr->package.origin);
        snprintf(s_TableCells[row][2], CELL_LEN, "%s", curr->package.destination);
        snprintf(s_TableCells[row][3], CELL_LEN, "%.1f kg", curr->package.weight);
        snprintf(s_TableCells[row][4], CELL_LEN, "%d", curr->package.priority);
        snprintf(s_TableCells[row][5], CELL_LEN, "%s", curr->package.status);
        
        for (int c = 0; c < TABLE_COLS; ++c) {
            s_TableRowPtrs[row][c] = s_TableCells[row][c];
        }
        s_TableRowsPtrs[row] = s_TableRowPtrs[row];
        s_TableRowPackagePtrs[row] = &s_TablePackages[row];
        row++;
        curr = curr->next;
    }
    return row;
}

void InitPackagesScreen(void) {
    if (!s_QueueInitialized) {
        InitializePackageQueue(&s_PackageQueue);
        PopulateDemoPackages();
        s_QueueInitialized = true;
    }
    s_DispatchedTodayCount = 0;
    s_SelectedRow = 0;
    s_SortCol = 4;
    s_SortAsc = true;
}

void UpdatePackagesScreen(void) {
}

static void DrawAddEditModal(void) {
    DrawRectangle(0, 0, GetScreenWidth(), GetScreenHeight(), ColorAlpha(BLACK, 0.4f));

    float modalW = 500;
    float modalH = 460;
    float modalX = (GetScreenWidth() - modalW) / 2.0f;
    float modalY = (GetScreenHeight() - modalH) / 2.0f;
    Rectangle modalRect = { modalX, modalY, modalW, modalH };

    DrawRectangleRounded(modalRect, 0.08f, 4, g_Theme.bgSecondary);
    DrawRectangleRoundedLines(modalRect, 0.08f, 4, g_Theme.border);

    float bannerH = 48.0f;
    DrawRectangleRounded((Rectangle){ modalRect.x, modalRect.y, modalRect.width, bannerH }, 0.08f, 4, g_Theme.bgTertiary);
    DrawRectangle((int)modalRect.x, (int)modalRect.y + 43, (int)modalW, 5, g_Theme.accentBlue);

    const char *title = "Enqueue Cargo Package";
    Vector2 titleSize = MeasureWithFont(g_FontSemiBold, title, FONT_MODAL_TITLE);
    DrawTextSemiBold(title, modalRect.x + 24, modalRect.y + (bannerH - titleSize.y) / 2.0f, FONT_MODAL_TITLE, g_Theme.textPrimary);

    float startY = modalY + 65;
    float labelX = modalRect.x + 24;
    float inputX = modalRect.x + 160;
    float inputW = modalW - 160 - 24;
    float inputH = 28;
    float gapY = 40;

    const char* labels[] = { "Package ID:", "Origin HUB:", "Destination HUB:", "Cargo Type:", "Weight (kg):", "Priority:", "Status:" };
    char* buffers[] = { s_FormIdBuf, s_FormOriginBuf, s_FormDestBuf, s_FormCargoTypeBuf, s_FormWeightBuf, s_FormPriorityBuf, s_FormStatusBuf };
    int maxLens[] = { 10, PACKAGE_ORIGIN_LEN - 1, PACKAGE_DESTINATION_LEN - 1, PACKAGE_CARGO_TYPE_LEN - 1, 10, 10, PACKAGE_STATUS_LEN - 1 };

    Vector2 mouse = Window_GetInputMousePosition();

    for (int i = 0; i < 7; ++i) {
        float y = startY + i * gapY;
        DrawTextRegular(labels[i], labelX, y + 4, FONT_NORMAL, g_Theme.textSecondary);

        Rectangle inputRect = { inputX, y, inputW, inputH };
        bool isEditing = (s_ActiveEditField == i);
        
        if (CheckCollisionPointRec(mouse, inputRect)) {
            SetMouseCursor(MOUSE_CURSOR_POINTING_HAND);
            if (IsMouseButtonPressed(MOUSE_BUTTON_LEFT)) {
                s_ActiveEditField = i;
            }
        }

        int clicked = GuiTextBox(inputRect, buffers[i], maxLens[i], isEditing);
        if (clicked) {
            if (isEditing) {
                s_ActiveEditField = -1;
            } else {
                s_ActiveEditField = i;
            }
        }
    }

    float btnY = modalY + modalH - 54;
    float btnW = 100;
    float btnH = 34;

    Rectangle saveRect = { modalRect.x + modalW - 236, btnY, btnW, btnH };
    Rectangle cancelRect = { modalRect.x + modalW - 124, btnY, btnW, btnH };

    if (DrawUIButton(saveRect, "Enqueue", true, NULL)) {
        onSaveEnqueue();
    }
    if (DrawUIButton(cancelRect, "Cancel", false, NULL)) {
        s_FormModalActive = false;
        s_ActiveEditField = -1;
    }
}

void DrawPackagesScreen(void) {
    int screenWidth = GetScreenWidth();
    int screenHeight = GetScreenHeight();
    int contentX = SIDEBAR_WIDTH + CONTENT_MARGIN;
    int contentWidth = screenWidth - SIDEBAR_WIDTH - (CONTENT_MARGIN * 2);
    
    // Summary Cards (3 cards)
    int cardY = GetContentY();
    int cardHeight = CARD_HEIGHT;
    
    char totalParcelsVal[32];
    snprintf(totalParcelsVal, sizeof(totalParcelsVal), "%zu Units", GetPackageQueueSize(&s_PackageQueue));
    
    char dispatchedVal[32];
    snprintf(dispatchedVal, sizeof(dispatchedVal), "%d Units", s_DispatchedTodayCount);
    
    int exceptionCount = 0;
    PackageQueueNode *node = s_PackageQueue.front;
    while (node) {
        if (strstr(node->package.status, "Delay") != NULL ||
            strstr(node->package.status, "Exception") != NULL ||
            strstr(node->package.status, "Delayed") != NULL) {
            exceptionCount++;
        }
        node = node->next;
    }
    char exceptionVal[32];
    snprintf(exceptionVal, sizeof(exceptionVal), "%d Shipments", exceptionCount);

    int cardWidth = (contentWidth - (CARD_SPACING * 2)) / 3;
    DrawUICard(
        (Rectangle){ (float)contentX, (float)cardY, (float)cardWidth, (float)cardHeight },
        "TOTAL REGISTERED PARCELS", totalParcelsVal, "All shipping lines included", g_Theme.accentBlue
    );
    
    DrawUICard(
        (Rectangle){ (float)(contentX + cardWidth + CARD_SPACING), (float)cardY,
                     (float)cardWidth, (float)cardHeight },
        "DISPATCHED TODAY", dispatchedVal, "High volume peak hours", g_Theme.accentTeal
    );
    
    DrawUICard(
        (Rectangle){ (float)(contentX + 2 * (cardWidth + CARD_SPACING)), (float)cardY,
                     (float)cardWidth, (float)cardHeight },
        "EXCEPTION / DELAYS", exceptionVal, "Requires immediate attention", g_Theme.danger
    );
    
    // Toolbar Actions
    int toolbarY = GetToolbarY(cardY + cardHeight);
    int btnW = 145;
    
    bool blockParentInput = s_FormModalActive;
    
    DrawUIButton((Rectangle){ (float)contentX, (float)toolbarY, (float)btnW, (float)BUTTON_HEIGHT }, "Enqueue Cargo", !blockParentInput, blockParentInput ? NULL : onEnqueueCallback);
    DrawUIButton((Rectangle){ (float)(contentX + 1 * (btnW + BUTTON_SPACING)), (float)toolbarY, (float)btnW, (float)BUTTON_HEIGHT }, "Dispatch Transit", !blockParentInput, blockParentInput ? NULL : onDispatchCallback);
    DrawUIButton((Rectangle){ (float)(contentX + 2 * (btnW + BUTTON_SPACING)), (float)toolbarY, (float)btnW, (float)BUTTON_HEIGHT }, "Peek Queue Head", !blockParentInput, blockParentInput ? NULL : onPeekCallback);
    DrawUIButton((Rectangle){ (float)(contentX + 3 * (btnW + BUTTON_SPACING)), (float)toolbarY, (float)btnW, (float)BUTTON_HEIGHT }, "Clear Registry", !blockParentInput, blockParentInput ? NULL : onClearQueueCallback);

    // Packages List Table
    int tableY = GetTableY(toolbarY + BUTTON_HEIGHT);
    int tableH = screenHeight - tableY - STATUS_BAR_HEIGHT;
    
    DrawSectionTitle("Package Registry & Parcel Location Status", (float)contentX, (float)(tableY - SUBTITLE_OFFSET));
    
    static const char* headers[] = { "Barcode ID", "Origin HUB", "Destination HUB", "Weight", "Priority", "Shipping Status" };
    static float colWidths[] = { 0.17f, 0.21f, 0.21f, 0.13f, 0.11f, 0.17f };
    
    int rowCount = PopulateTableData();
    SortTableRows(rowCount);
    int hoveredRow = -1;
    
    DrawUITable(
        (Rectangle){ (float)contentX, (float)tableY, (float)contentWidth, (float)tableH },
        headers, colWidths, TABLE_COLS, s_TableRowsPtrs, rowCount, false, &hoveredRow, &s_SelectedRow, &s_SortCol, &s_SortAsc
    );

    if (s_FormModalActive) {
        DrawAddEditModal();
    }
}
