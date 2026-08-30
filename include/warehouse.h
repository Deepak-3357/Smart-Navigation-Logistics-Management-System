#ifndef WAREHOUSE_H
#define WAREHOUSE_H

#include <stdbool.h>

typedef struct Warehouse {
    int warehouseId;
    char warehouseName[50];
    char city[40];
    char manager[40];
    char phone[20];
    int capacity;
    int currentStock;
    struct Warehouse *next;
} Warehouse;

typedef enum {
    WH_SUCCESS = 0,
    WH_ERROR_NULL_POINTER,
    WH_ERROR_DUPLICATE_ID,
    WH_ERROR_INVALID_CAPACITY,
    WH_ERROR_INVALID_STOCK,
    WH_ERROR_EMPTY_NAME,
    WH_ERROR_EMPTY_CITY,
    WH_ERROR_NOT_FOUND,
    WH_ERROR_MEMORY_ALLOCATION,
    WH_ERROR_LIST_EMPTY,
    WH_ERROR_FILE_OPEN,
    WH_ERROR_FILE_READ,
    WH_ERROR_FILE_WRITE
} WarehouseStatus;

WarehouseStatus InitializeWarehouseList(Warehouse **head);

WarehouseStatus CreateWarehouse(int id, const char *name, const char *city,
                                const char *manager, const char *phone,
                                int capacity, int currentStock,
                                Warehouse **outWarehouse);

WarehouseStatus InsertWarehouse(Warehouse **head, Warehouse *newWarehouse);

WarehouseStatus DeleteWarehouse(Warehouse **head, int id);

WarehouseStatus UpdateWarehouse(Warehouse *head, int id, const char *name,
                                const char *city, const char *manager,
                                const char *phone, int capacity, int currentStock);

Warehouse *SearchWarehouseByID(Warehouse *head, int id);

Warehouse *SearchWarehouseByName(Warehouse *head, const char *name);

int GetWarehouseCount(Warehouse *head);

bool IsWarehouseEmpty(Warehouse *head);

void DisplayWarehouses(Warehouse *head);

void DestroyWarehouseList(Warehouse **head);

WarehouseStatus LoadWarehouseData(Warehouse **head);
WarehouseStatus SaveWarehouseData(Warehouse *head);
Warehouse *GetWarehouseListHead(void);

WarehouseStatus BuildWarehouseSearchArray(Warehouse *head, Warehouse ***outArray, int *outCount);
void SortWarehouseSearchArray(Warehouse **array, int count);
Warehouse *BinarySearchWarehouseByID(Warehouse **array, int count, int targetId);

#endif