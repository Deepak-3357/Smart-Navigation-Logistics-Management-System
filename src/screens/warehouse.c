#include "screens.h"
#include "common.h"
#include "theme_manager.h"
#include "components.h"
#include "layout.h"
#include "warehouse.h"
#include "window_manager.h"
#undef USE_LIBTYPE_SHARED   // raygui is header-only, not a DLL
#include "raygui.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

// Singly Linked List head
static Warehouse *s_WarehouseListHead = NULL;

// Form state buffers
static bool s_FormModalActive = false;
static bool s_FormIsUpdate = false;
static int s_ActiveEditField = -1; // -1 = none, 0 = ID, 1 = Name, 2 = City, 3 = Manager, 4 = Phone, 5 = Capacity, 6 = Stock, 98 = Binary Search, 99 = Search

static char s_FormIdBuf[16] = "";
static char s_FormNameBuf[50] = "";
static char s_FormCityBuf[40] = "";
static char s_FormManagerBuf[40] = "";
static char s_FormPhoneBuf[20] = "";
static char s_FormCapacityBuf[16] = "";
static char s_FormStockBuf[16] = "";

// Search state buffers
static bool s_SearchModalActive = false;
static char s_SearchQueryBuf[64] = "";
static char s_ActiveSearchQuery[64] = "";

// Binary Search state buffers
static bool s_BinarySearchModalActive = false;
static char s_BinarySearchQueryBuf[16] = "";

// Table data — flat cell storage + pointer indirection for DrawUITable
// DrawUITable expects const char*** : pointer to array of row pointers,
// where each row pointer is a pointer to array of cell string pointers.
#define MAX_TABLE_ROWS 100
#define TABLE_COLS       5
#define CELL_LEN         64
static char s_TableCells[MAX_TABLE_ROWS][TABLE_COLS][CELL_LEN];
static const char* s_TableRowPtrs[MAX_TABLE_ROWS][TABLE_COLS]; // cell string pointers
static const char** s_TableRowsPtrs[MAX_TABLE_ROWS];           // row pointers
static Warehouse* s_TableRowNodePtrs[MAX_TABLE_ROWS];

// Selection, sorting, loading states
static int s_SelectedRow = 0;
static int s_SortCol = 0;
static bool s_SortAsc = true;
static bool s_IsLoading = false;
static float s_LoadingTimer = 0.0f;

// Dynamic Card statistics
static char s_CapacityCardVal[64] = "0 Cubic Meters";
static char s_CapacityCardSub[64] = "Allocated across 0 active hubs";
static char s_AvgCostCardVal[64] = "INR 0 / Month";

#define INR_STORAGE_COST_PER_M3_MONTH 350.00f

// Forward declarations
static int PopulateTableData(void);
static void SortTableRows(int rowCount, int sortCol, bool sortAsc);

// Populates default warehouses at start
static void PopulateDefaultWarehouses(void) {
    Warehouse *wh = NULL;
    CreateWarehouse(5, "Sector E - General Goods", "Atlanta", "Evan Wright", "555-0177", 4500, 4095, &wh);
    InsertWarehouse(&s_WarehouseListHead, wh);
    
    CreateWarehouse(4, "Sector D - Bulk Heavy", "Chicago", "Diana Prince", "555-0155", 6000, 1500, &wh);
    InsertWarehouse(&s_WarehouseListHead, wh);
    
    CreateWarehouse(3, "Sector C - Hazmat Cargo", "Dallas", "Charlie Brown", "555-0188", 3000, 2880, &wh);
    InsertWarehouse(&s_WarehouseListHead, wh);
    
    CreateWarehouse(2, "Sector B - Cold Storage", "Seattle", "Bob Smith", "555-0143", 8000, 6160, &wh);
    InsertWarehouse(&s_WarehouseListHead, wh);
    
    CreateWarehouse(1, "Sector A - Electronics", "San Francisco", "Alice Johnson", "555-0192", 5000, 4500, &wh);
    InsertWarehouse(&s_WarehouseListHead, wh);
}

// Maps backend status to toast notifications
static void ShowErrorMessage(WarehouseStatus status) {
    switch (status) {
        case WH_ERROR_DUPLICATE_ID:
            Toast_Show("Error: Warehouse ID already exists!", TOAST_ERROR);
            break;
        case WH_ERROR_INVALID_CAPACITY:
            Toast_Show("Error: Capacity must be strictly positive!", TOAST_ERROR);
            break;
        case WH_ERROR_INVALID_STOCK:
            Toast_Show("Error: Stock must be 0+ and <= Capacity!", TOAST_ERROR);
            break;
        case WH_ERROR_EMPTY_NAME:
            Toast_Show("Error: Warehouse Name cannot be empty!", TOAST_ERROR);
            break;
        case WH_ERROR_EMPTY_CITY:
            Toast_Show("Error: City cannot be empty!", TOAST_ERROR);
            break;
        case WH_ERROR_MEMORY_ALLOCATION:
            Toast_Show("Error: Memory allocation failed!", TOAST_ERROR);
            break;
        default:
            Toast_Show("Error: Invalid input data!", TOAST_ERROR);
            break;
    }
}

// Save form handler
static void onSaveForm(void) {
    int id = atoi(s_FormIdBuf);
    const char *name = s_FormNameBuf;
    const char *city = s_FormCityBuf;
    const char *manager = s_FormManagerBuf;
    const char *phone = s_FormPhoneBuf;
    int capacity = atoi(s_FormCapacityBuf);
    int currentStock = atoi(s_FormStockBuf);

    if (s_FormIsUpdate) {
        WarehouseStatus status = UpdateWarehouse(s_WarehouseListHead, id, name, city, manager, phone, capacity, currentStock);
        if (status == WH_SUCCESS) {
            Toast_Show("Warehouse updated successfully!", TOAST_SUCCESS);
            s_FormModalActive = false;
            s_ActiveEditField = -1;
            SaveWarehouseData(s_WarehouseListHead);
        } else {
            ShowErrorMessage(status);
        }
    } else {
        Warehouse *newWh = NULL;
        WarehouseStatus status = CreateWarehouse(id, name, city, manager, phone, capacity, currentStock, &newWh);
        if (status == WH_SUCCESS) {
            status = InsertWarehouse(&s_WarehouseListHead, newWh);
            if (status == WH_SUCCESS) {
                Toast_Show("Warehouse added successfully!", TOAST_SUCCESS);
                s_FormModalActive = false;
                s_ActiveEditField = -1;
                s_SelectedRow = 0;
                SaveWarehouseData(s_WarehouseListHead);
            } else {
                free(newWh);
                ShowErrorMessage(status);
            }
        } else {
            ShowErrorMessage(status);
        }
    }
}

// Add button action
static void onAddWarehouse(void) {
    s_FormIdBuf[0] = '\0';
    s_FormNameBuf[0] = '\0';
    s_FormCityBuf[0] = '\0';
    s_FormManagerBuf[0] = '\0';
    s_FormPhoneBuf[0] = '\0';
    s_FormCapacityBuf[0] = '\0';
    s_FormStockBuf[0] = '\0';
    
    s_FormIsUpdate = false;
    s_FormModalActive = true;
    s_ActiveEditField = -1;
}

// Update button action
static void onUpdateWarehouse(void) {
    int rowCount = GetWarehouseCount(s_WarehouseListHead);
    if (rowCount == 0) {
        Toast_Show("No warehouses available to update.", TOAST_WARNING);
        return;
    }
    
    if (s_SelectedRow < 0 || s_SelectedRow >= MAX_TABLE_ROWS) {
        Toast_Show("Please select a warehouse from the table first.", TOAST_WARNING);
        return;
    }
    Warehouse *selectedWh = s_TableRowNodePtrs[s_SelectedRow];
    if (!selectedWh) {
        Toast_Show("Please select a warehouse from the table first.", TOAST_WARNING);
        return;
    }

    snprintf(s_FormIdBuf, sizeof(s_FormIdBuf), "%d", selectedWh->warehouseId);
    snprintf(s_FormNameBuf, sizeof(s_FormNameBuf), "%s", selectedWh->warehouseName);
    snprintf(s_FormCityBuf, sizeof(s_FormCityBuf), "%s", selectedWh->city);
    snprintf(s_FormManagerBuf, sizeof(s_FormManagerBuf), "%s", selectedWh->manager);
    snprintf(s_FormPhoneBuf, sizeof(s_FormPhoneBuf), "%s", selectedWh->phone);
    snprintf(s_FormCapacityBuf, sizeof(s_FormCapacityBuf), "%d", selectedWh->capacity);
    snprintf(s_FormStockBuf, sizeof(s_FormStockBuf), "%d", selectedWh->currentStock);

    s_FormIsUpdate = true;
    s_FormModalActive = true;
    s_ActiveEditField = -1;
}

// Delete confirm callback
static void onDeleteConfirm(bool confirmed) {
    if (confirmed) {
        if (s_SelectedRow < 0 || s_SelectedRow >= MAX_TABLE_ROWS) return;
        Warehouse *selectedWh = s_TableRowNodePtrs[s_SelectedRow];
        if (selectedWh) {
            WarehouseStatus status = DeleteWarehouse(&s_WarehouseListHead, selectedWh->warehouseId);
            if (status == WH_SUCCESS) {
                Toast_Show("Warehouse deleted successfully!", TOAST_SUCCESS);
                s_SelectedRow = 0;
                SaveWarehouseData(s_WarehouseListHead);
            } else {
                Toast_Show("Error: Warehouse not found!", TOAST_ERROR);
            }
        }
    }
}

// Delete button action
static void onDeleteWarehouse(void) {
    int rowCount = GetWarehouseCount(s_WarehouseListHead);
    if (rowCount == 0) {
        Toast_Show("No warehouses available to delete.", TOAST_WARNING);
        return;
    }
    
    if (s_SelectedRow < 0 || s_SelectedRow >= MAX_TABLE_ROWS) {
        Toast_Show("Please select a warehouse from the table first.", TOAST_WARNING);
        return;
    }
    Warehouse *selectedWh = s_TableRowNodePtrs[s_SelectedRow];
    if (!selectedWh) {
        Toast_Show("Please select a warehouse from the table first.", TOAST_WARNING);
        return;
    }

    Modal_Show("Delete Warehouse", "Are you sure you want to delete this warehouse hub?", MODAL_CONFIRMATION, onDeleteConfirm);
}

// Search button action
static void onSearchWarehouse(void) {
    s_SearchModalActive = true;
    s_ActiveEditField = -1;
}

// Binary Search button action
static void onBinarySearchWarehouse(void) {
    s_BinarySearchModalActive = true;
    s_ActiveEditField = -1;
    s_BinarySearchQueryBuf[0] = '\0';
}

// Binary Search confirm action
static void onBinarySearchConfirm(void) {
    if (s_WarehouseListHead == NULL) {
        Toast_Show("Error: Warehouse list is empty!", TOAST_WARNING);
        s_BinarySearchModalActive = false;
        s_ActiveEditField = -1;
        return;
    }

    if (s_BinarySearchQueryBuf[0] == '\0') {
        Toast_Show("Error: Please enter a Warehouse ID!", TOAST_ERROR);
        return;
    }

    char *endptr;
    int targetId = (int)strtol(s_BinarySearchQueryBuf, &endptr, 10);
    if (endptr == s_BinarySearchQueryBuf || *endptr != '\0') {
        Toast_Show("Error: Please enter a valid numeric ID!", TOAST_ERROR);
        return;
    }

    Warehouse **searchArray = NULL;
    int count = 0;
    WarehouseStatus status = BuildWarehouseSearchArray(s_WarehouseListHead, &searchArray, &count);
    if (status != WH_SUCCESS) {
        Toast_Show("Error: Failed to build search data!", TOAST_ERROR);
        if (searchArray) free(searchArray);
        return;
    }

    if (count <= 0) {
        Toast_Show("Error: Warehouse list is empty!", TOAST_WARNING);
        if (searchArray) free(searchArray);
        s_BinarySearchModalActive = false;
        s_ActiveEditField = -1;
        return;
    }

    SortWarehouseSearchArray(searchArray, count);
    Warehouse *result = BinarySearchWarehouseByID(searchArray, count, targetId);

    if (result) {
        // Clear active general search filter so all rows are visible
        s_ActiveSearchQuery[0] = '\0';
        s_SearchQueryBuf[0] = '\0';

        // Repopulate table to include the found item
        int rowCount = PopulateTableData();
        SortTableRows(rowCount, s_SortCol, s_SortAsc);

        // Locate row index of found item
        int foundRow = -1;
        for (int i = 0; i < rowCount; ++i) {
            if (s_TableRowNodePtrs[i] && s_TableRowNodePtrs[i]->warehouseId == targetId) {
                foundRow = i;
                break;
            }
        }
        if (foundRow != -1) {
            s_SelectedRow = foundRow;
        }

        // Show details in success toast
        char msg[128];
        snprintf(msg, sizeof(msg), "Found ID %d: %s (City: %s, Capacity: %d m3)",
                 result->warehouseId, result->warehouseName, result->city, result->capacity);
        Toast_Show(msg, TOAST_SUCCESS);
        s_BinarySearchModalActive = false;
        s_ActiveEditField = -1;
    } else {
        char msg[64];
        snprintf(msg, sizeof(msg), "Warehouse ID %d not found.", targetId);
        Toast_Show(msg, TOAST_WARNING);
    }

    if (searchArray) {
        free(searchArray);
    }
}

// Refresh button action
static void onRefreshWarehouse(void) {
    s_IsLoading = true;
    s_LoadingTimer = 1.0f;
    s_SearchQueryBuf[0] = '\0';
    s_ActiveSearchQuery[0] = '\0';
    s_SelectedRow = 0;
    Toast_Show("Reloading warehouse database...", TOAST_INFO);
}

// Table populater with search filtering
static int PopulateTableData(void) {
    // Clear stale node pointers so callbacks never access invalid memory
    memset(s_TableRowNodePtrs, 0, sizeof(s_TableRowNodePtrs));

    Warehouse *current = s_WarehouseListHead;
    int row = 0;
    while (current && row < MAX_TABLE_ROWS) {
        bool matches = true;
        if (s_ActiveSearchQuery[0] != '\0') {
            char *endptr;
            int qId = (int)strtol(s_ActiveSearchQuery, &endptr, 10);
            bool isNumeric = (endptr != s_ActiveSearchQuery && *endptr == '\0');
            
            if (isNumeric) {
                if (current->warehouseId != qId && strstr(current->warehouseName, s_ActiveSearchQuery) == NULL) {
                    matches = false;
                }
            } else {
                char nameLower[64];
                char queryLower[64];
                strncpy(nameLower, current->warehouseName, sizeof(nameLower) - 1);
                nameLower[sizeof(nameLower) - 1] = '\0';
                strncpy(queryLower, s_ActiveSearchQuery, sizeof(queryLower) - 1);
                queryLower[sizeof(queryLower) - 1] = '\0';
                for (int i = 0; nameLower[i]; i++) nameLower[i] = tolower((unsigned char)nameLower[i]);
                for (int i = 0; queryLower[i]; i++) queryLower[i] = tolower((unsigned char)queryLower[i]);
                
                if (strstr(nameLower, queryLower) == NULL) {
                    matches = false;
                }
            }
        }
        
        if (matches) {
            snprintf(s_TableCells[row][0], CELL_LEN, "%s", current->warehouseName);
            snprintf(s_TableCells[row][1], CELL_LEN, "%d m3", current->capacity);
            snprintf(s_TableCells[row][2], CELL_LEN, "%d m3", current->currentStock);
            
            float ratio = (current->capacity > 0) ? ((float)current->currentStock / current->capacity * 100.0f) : 0.0f;
            snprintf(s_TableCells[row][3], CELL_LEN, "%.0f%% Occupied", ratio);
            
            const char *status = "Optimal";
            if (ratio >= 95.0f) status = "Full";
            else if (ratio >= 90.0f) status = "Near Capacity";
            else if (ratio <= 30.0f) status = "Underutilized";
            snprintf(s_TableCells[row][4], CELL_LEN, "%s", status);
            
            // Wire pointer indirection: cell ptrs → cell data, row ptr → row of cell ptrs
            for (int c = 0; c < TABLE_COLS; ++c) {
                s_TableRowPtrs[row][c] = s_TableCells[row][c];
            }
            s_TableRowsPtrs[row] = s_TableRowPtrs[row];
            s_TableRowNodePtrs[row] = current;
            row++;
        }
        current = current->next;
    }
    return row;
}

// Table sorter — swaps cell data in place and re-wires pointer indirection
static void SortTableRows(int rowCount, int sortCol, bool sortAsc) {
    if (rowCount < 2 || sortCol < 0 || sortCol >= TABLE_COLS) return;

    for (int i = 0; i < rowCount - 1; ++i) {
        for (int j = 0; j < rowCount - i - 1; ++j) {
            bool swap = false;
            const char* val1 = s_TableCells[j][sortCol];
            const char* val2 = s_TableCells[j + 1][sortCol];
            
            if (sortCol == 1 || sortCol == 2 || sortCol == 3) {
                float n1 = atof(val1);
                float n2 = atof(val2);
                if (sortAsc) {
                    if (n1 > n2) swap = true;
                } else {
                    if (n1 < n2) swap = true;
                }
            } else {
                int cmp = strcmp(val1, val2);
                if (sortAsc) {
                    if (cmp > 0) swap = true;
                } else {
                    if (cmp < 0) swap = true;
                }
            }
            
            if (swap) {
                // Swap cell data in place
                char tempCell[TABLE_COLS][CELL_LEN];
                for (int c = 0; c < TABLE_COLS; ++c) {
                    strncpy(tempCell[c], s_TableCells[j][c], CELL_LEN);
                    strncpy(s_TableCells[j][c], s_TableCells[j + 1][c], CELL_LEN);
                    strncpy(s_TableCells[j + 1][c], tempCell[c], CELL_LEN);
                }
                // Re-wire pointer indirection for both swapped rows
                for (int c = 0; c < TABLE_COLS; ++c) {
                    s_TableRowPtrs[j][c]     = s_TableCells[j][c];
                    s_TableRowPtrs[j + 1][c] = s_TableCells[j + 1][c];
                }
                // Swap node pointers
                Warehouse *tempNode = s_TableRowNodePtrs[j];
                s_TableRowNodePtrs[j] = s_TableRowNodePtrs[j + 1];
                s_TableRowNodePtrs[j + 1] = tempNode;
            }
        }
    }
}

Warehouse *GetWarehouseListHead(void) {
    return s_WarehouseListHead;
}

void ScanWarehouseCargoWorkflow(void) {
    Warehouse *head = GetWarehouseListHead();
    if (!head) {
        Toast_Show("No active warehouses are available to scan.", TOAST_WARNING);
        return;
    }

    int totalCapacity = 0;
    int totalStock = 0;
    Warehouse *curr = head;
    while (curr) {
        totalCapacity += curr->capacity;
        totalStock += curr->currentStock;
        curr = curr->next;
    }

    int warehouseCount = GetWarehouseCount(head);
    double utilization = totalCapacity > 0
        ? ((double)totalStock / (double)totalCapacity) * 100.0
        : 0.0;

    char result[512];
    snprintf(result, sizeof(result),
             "Active Warehouses: %d\nTotal Capacity: %d m³\n"
             "Current Stock: %d m³\nOverall Utilization: %.1f%%",
             warehouseCount, totalCapacity, totalStock, utilization);
    Modal_Show("Warehouse Scan Complete", result, MODAL_SUCCESS, NULL);
    Toast_Show("Warehouse cargo scan completed from the live linked list.", TOAST_SUCCESS);
}

void InitWarehouseScreen(void) {
    s_IsLoading = false;
    s_LoadingTimer = 0.0f;
    
    if (s_WarehouseListHead == NULL) {
        InitializeWarehouseList(&s_WarehouseListHead);
        WarehouseStatus status = LoadWarehouseData(&s_WarehouseListHead);
        if (status == WH_SUCCESS) {
            Toast_Show("Warehouse database loaded successfully!", TOAST_SUCCESS);
        } else if (status == WH_ERROR_NOT_FOUND) {
            // Keep empty list as per requirement.
        } else {
            s_WarehouseListHead = NULL;
        }
    }
}

void UpdateWarehouseScreen(void) {
    if (s_IsLoading) {
        s_LoadingTimer -= GetFrameTime();
        if (s_LoadingTimer <= 0.0f) {
            s_IsLoading = false;
            LoadWarehouseData(&s_WarehouseListHead);
            Toast_Show("Warehouse database reloaded successfully!", TOAST_SUCCESS);
        }
    }
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

    const char *title = s_FormIsUpdate ? "Update Warehouse Hub" : "Add Warehouse Hub";
    Vector2 titleSize = MeasureWithFont(g_FontSemiBold, title, FONT_MODAL_TITLE);
    DrawTextSemiBold(title, modalRect.x + 24, modalRect.y + (bannerH - titleSize.y) / 2.0f, FONT_MODAL_TITLE, g_Theme.textPrimary);

    float startY = modalY + 65;
    float labelX = modalRect.x + 24;
    float inputX = modalRect.x + 160;
    float inputW = modalW - 160 - 24;
    float inputH = 28;
    float gapY = 40;

    const char* labels[] = { "Warehouse ID:", "Name:", "City:", "Manager:", "Phone:", "Capacity:", "Current Stock:" };
    char* buffers[] = { s_FormIdBuf, s_FormNameBuf, s_FormCityBuf, s_FormManagerBuf, s_FormPhoneBuf, s_FormCapacityBuf, s_FormStockBuf };
    int maxLens[] = { 10, 49, 39, 39, 19, 10, 10 };

    Vector2 mouse = Window_GetInputMousePosition();

    for (int i = 0; i < 7; ++i) {
        float y = startY + i * gapY;
        DrawTextRegular(labels[i], labelX, y + 4, FONT_NORMAL, g_Theme.textSecondary);

        Rectangle inputRect = { inputX, y, inputW, inputH };
        bool isReadOnly = (i == 0 && s_FormIsUpdate);
        
        if (isReadOnly) {
            DrawRectangleRec(inputRect, g_Theme.bgTertiary);
            DrawRectangleLinesEx(inputRect, 1, g_Theme.border);
            DrawTextRegular(buffers[i], inputRect.x + 8, inputRect.y + 5, FONT_NORMAL, g_Theme.textSecondary);
        } else {
            bool isEditing = (s_ActiveEditField == i);
            
            if (CheckCollisionPointRec(mouse, inputRect) && !s_SearchModalActive) {
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
    }

    float btnY = modalY + modalH - 54;
    float btnW = 100;
    float btnH = 34;

    Rectangle saveRect = { modalRect.x + modalW - 236, btnY, btnW, btnH };
    Rectangle cancelRect = { modalRect.x + modalW - 124, btnY, btnW, btnH };

    if (DrawUIButton(saveRect, "Save", true, NULL)) {
        onSaveForm();
    }
    if (DrawUIButton(cancelRect, "Cancel", false, NULL)) {
        s_FormModalActive = false;
        s_ActiveEditField = -1;
    }
}

static void DrawSearchModal(void) {
    DrawRectangle(0, 0, GetScreenWidth(), GetScreenHeight(), ColorAlpha(BLACK, 0.4f));

    float modalW = 400;
    float modalH = 180;
    float modalX = (GetScreenWidth() - modalW) / 2.0f;
    float modalY = (GetScreenHeight() - modalH) / 2.0f;
    Rectangle modalRect = { modalX, modalY, modalW, modalH };

    DrawRectangleRounded(modalRect, 0.08f, 4, g_Theme.bgSecondary);
    DrawRectangleRoundedLines(modalRect, 0.08f, 4, g_Theme.border);

    float bannerH = 48.0f;
    DrawRectangleRounded((Rectangle){ modalRect.x, modalRect.y, modalRect.width, bannerH }, 0.08f, 4, g_Theme.bgTertiary);
    DrawRectangle((int)modalRect.x, (int)modalRect.y + 43, (int)modalW, 5, g_Theme.accentBlue);

    const char *title = "Search Warehouse Hubs";
    Vector2 titleSize = MeasureWithFont(g_FontSemiBold, title, FONT_MODAL_TITLE);
    DrawTextSemiBold(title, modalRect.x + 24, modalRect.y + (bannerH - titleSize.y) / 2.0f, FONT_MODAL_TITLE, g_Theme.textPrimary);

    float inputY = modalY + 65;
    DrawTextRegular("Query:", modalRect.x + 24, inputY + 4, FONT_NORMAL, g_Theme.textSecondary);
    
    Rectangle inputRect = { modalRect.x + 100, inputY, modalW - 100 - 24, 28 };
    Vector2 mouse = Window_GetInputMousePosition();
    bool isEditing = (s_ActiveEditField == 99);
    
    if (CheckCollisionPointRec(mouse, inputRect)) {
        SetMouseCursor(MOUSE_CURSOR_POINTING_HAND);
        if (IsMouseButtonPressed(MOUSE_BUTTON_LEFT)) {
            s_ActiveEditField = 99;
        }
    }
    
    int clicked = GuiTextBox(inputRect, s_SearchQueryBuf, sizeof(s_SearchQueryBuf) - 1, isEditing);
    if (clicked) {
        if (isEditing) {
            s_ActiveEditField = -1;
        } else {
            s_ActiveEditField = 99;
        }
    }

    float btnY = modalY + modalH - 46;
    float btnW = 85;
    float btnH = 30;

    Rectangle searchRect = { modalRect.x + modalW - 290, btnY, btnW, btnH };
    Rectangle clearRect = { modalRect.x + modalW - 195, btnY, btnW, btnH };
    Rectangle cancelRect = { modalRect.x + modalW - 100, btnY, btnW, btnH };

    if (DrawUIButton(searchRect, "Search", true, NULL)) {
        strncpy(s_ActiveSearchQuery, s_SearchQueryBuf, sizeof(s_ActiveSearchQuery) - 1);
        s_ActiveSearchQuery[sizeof(s_ActiveSearchQuery) - 1] = '\0';
        s_SearchModalActive = false;
        s_ActiveEditField = -1;
        s_SelectedRow = 0;
        Toast_Show("Search query applied!", TOAST_INFO);
    }
    if (DrawUIButton(clearRect, "Clear", false, NULL)) {
        s_SearchQueryBuf[0] = '\0';
        s_ActiveSearchQuery[0] = '\0';
        s_SearchModalActive = false;
        s_ActiveEditField = -1;
        s_SelectedRow = 0;
        Toast_Show("Search filter cleared!", TOAST_INFO);
    }
    if (DrawUIButton(cancelRect, "Cancel", false, NULL)) {
        s_SearchModalActive = false;
        s_ActiveEditField = -1;
    }
}

static void DrawBinarySearchModal(void) {
    DrawRectangle(0, 0, GetScreenWidth(), GetScreenHeight(), ColorAlpha(BLACK, 0.4f));

    float modalW = 400;
    float modalH = 180;
    float modalX = (GetScreenWidth() - modalW) / 2.0f;
    float modalY = (GetScreenHeight() - modalH) / 2.0f;
    Rectangle modalRect = { modalX, modalY, modalW, modalH };

    DrawRectangleRounded(modalRect, 0.08f, 4, g_Theme.bgSecondary);
    DrawRectangleRoundedLines(modalRect, 0.08f, 4, g_Theme.border);

    float bannerH = 48.0f;
    DrawRectangleRounded((Rectangle){ modalRect.x, modalRect.y, modalRect.width, bannerH }, 0.08f, 4, g_Theme.bgTertiary);
    DrawRectangle((int)modalRect.x, (int)modalRect.y + 43, (int)modalW, 5, g_Theme.accentBlue);

    const char *title = "Binary Search by ID";
    Vector2 titleSize = MeasureWithFont(g_FontSemiBold, title, FONT_MODAL_TITLE);
    DrawTextSemiBold(title, modalRect.x + 24, modalRect.y + (bannerH - titleSize.y) / 2.0f, FONT_MODAL_TITLE, g_Theme.textPrimary);

    float inputY = modalY + 65;
    DrawTextRegular("Hub ID:", modalRect.x + 24, inputY + 4, FONT_NORMAL, g_Theme.textSecondary);
    
    Rectangle inputRect = { modalRect.x + 100, inputY, modalW - 100 - 24, 28 };
    Vector2 mouse = Window_GetInputMousePosition();
    bool isEditing = (s_ActiveEditField == 98);
    
    if (CheckCollisionPointRec(mouse, inputRect)) {
        SetMouseCursor(MOUSE_CURSOR_POINTING_HAND);
        if (IsMouseButtonPressed(MOUSE_BUTTON_LEFT)) {
            s_ActiveEditField = 98;
        }
    }
    
    int clicked = GuiTextBox(inputRect, s_BinarySearchQueryBuf, sizeof(s_BinarySearchQueryBuf) - 1, isEditing);
    if (clicked) {
        if (isEditing) {
            s_ActiveEditField = -1;
        } else {
            s_ActiveEditField = 98;
        }
    }

    float btnY = modalY + modalH - 46;
    float btnW = 85;
    float btnH = 30;

    Rectangle searchRect = { modalRect.x + modalW - 200, btnY, btnW, btnH };
    Rectangle cancelRect = { modalRect.x + modalW - 100, btnY, btnW, btnH };

    if (DrawUIButton(searchRect, "Search", true, NULL)) {
        onBinarySearchConfirm();
    }
    if (DrawUIButton(cancelRect, "Cancel", false, NULL)) {
        s_BinarySearchModalActive = false;
        s_ActiveEditField = -1;
    }
}

void DrawWarehouseScreen(void) {
    int contentX = GetContentX();
    int contentWidth = GetContentWidth();
    
    // Top summary cards (2 cards) - Live values computed
    int cardY = GetContentY();
    int cardHeight = CARD_HEIGHT;
    
    int totalCapacity = 0;
    Warehouse *curr = s_WarehouseListHead;
    while (curr) {
        totalCapacity += curr->capacity;
        curr = curr->next;
    }
    
    float avgCost = (totalCapacity > 0) ? INR_STORAGE_COST_PER_M3_MONTH : 0.0f;
    snprintf(s_CapacityCardVal, sizeof(s_CapacityCardVal), "%d Cubic Meters", totalCapacity);
    snprintf(s_CapacityCardSub, sizeof(s_CapacityCardSub), "Allocated across %d active hubs", GetWarehouseCount(s_WarehouseListHead));
    snprintf(s_AvgCostCardVal, sizeof(s_AvgCostCardVal), "INR %.2f / Month", avgCost);
    
    DrawUICard(
        GetCardRect(0, 0, 2, cardHeight),
        "TOTAL INVENTORY STORAGE CAPACITY", s_CapacityCardVal, s_CapacityCardSub, g_Theme.accentBlue
    );
    
    DrawUICard(
        GetCardRect(1, 0, 2, cardHeight),
        "AVERAGE STORAGE COST PER M³", s_AvgCostCardVal, "-2.1% this quarter", g_Theme.accentTeal
    );
    
    // Toolbar Actions
    int toolbarY = GetToolbarY(cardY + cardHeight);
    int btnW = 145;
    
    // Check if any modal is active to block parent inputs
    bool blockParentInput = s_FormModalActive || s_SearchModalActive || s_BinarySearchModalActive;
    
    // Draw Toolbar Buttons
    DrawUIButton((Rectangle){ (float)contentX, (float)toolbarY, (float)btnW, (float)BUTTON_HEIGHT }, "Add Warehouse", !blockParentInput, blockParentInput ? NULL : onAddWarehouse);
    DrawUIButton((Rectangle){ (float)(contentX + 1 * (btnW + BUTTON_SPACING)), (float)toolbarY, (float)btnW, (float)BUTTON_HEIGHT }, "Update Warehouse", false, blockParentInput ? NULL : onUpdateWarehouse);
    DrawUIButton((Rectangle){ (float)(contentX + 2 * (btnW + BUTTON_SPACING)), (float)toolbarY, (float)btnW, (float)BUTTON_HEIGHT }, "Delete Warehouse", false, blockParentInput ? NULL : onDeleteWarehouse);
    DrawUIButton((Rectangle){ (float)(contentX + 3 * (btnW + BUTTON_SPACING)), (float)toolbarY, (float)btnW, (float)BUTTON_HEIGHT }, "Search Warehouse", false, blockParentInput ? NULL : onSearchWarehouse);
    DrawUIButton((Rectangle){ (float)(contentX + 4 * (btnW + BUTTON_SPACING)), (float)toolbarY, (float)btnW, (float)BUTTON_HEIGHT }, "Binary Search", false, blockParentInput ? NULL : onBinarySearchWarehouse);
    DrawUIButton((Rectangle){ (float)(contentX + 5 * (btnW + BUTTON_SPACING)), (float)toolbarY, (float)btnW, (float)BUTTON_HEIGHT }, "Refresh Sectors", false, blockParentInput ? NULL : onRefreshWarehouse);

    // Main sector list table
    int tableY = GetTableY(toolbarY + BUTTON_HEIGHT);
    int tableH = GetScreenHeight() - tableY - STATUS_BAR_HEIGHT;
    
    DrawSectionTitle("Warehouse Sector Storage Metrics", (float)contentX, (float)(tableY - SUBTITLE_OFFSET));
    
    static const char* headers[] = { "Sector", "Total Capacity", "Current Load", "Utilization Ratio", "Alert Status" };
    static float colWidths[] = { 0.25f, 0.18f, 0.18f, 0.22f, 0.17f };
    
    int rowCount = PopulateTableData();
    SortTableRows(rowCount, s_SortCol, s_SortAsc);
    
    // Clamp selection
    if (s_SelectedRow >= rowCount) {
        s_SelectedRow = rowCount > 0 ? rowCount - 1 : 0;
    }
    
    DrawUITable(
        (Rectangle){ (float)contentX, (float)tableY, (float)contentWidth, (float)tableH },
        headers, colWidths, 5, (const char***)s_TableRowsPtrs, rowCount, s_IsLoading, NULL, &s_SelectedRow, &s_SortCol, &s_SortAsc
    );

    // Draw active modal forms as overlays
    if (s_FormModalActive) {
        DrawAddEditModal();
    } else if (s_SearchModalActive) {
        DrawSearchModal();
    } else if (s_BinarySearchModalActive) {
        DrawBinarySearchModal();
    }
}
