#include "warehouse.h"
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <errno.h>
#include "components.h"

#ifdef _WIN32
#include <direct.h>
#else
#include <sys/stat.h>
#endif

WarehouseStatus InitializeWarehouseList(Warehouse **head) {
    if (!head) {
        return WH_ERROR_NULL_POINTER;
    }
    *head = NULL;
    return WH_SUCCESS;
}

static WarehouseStatus ValidateWarehouseData(const char *name, const char *city,
                                             int capacity, int currentStock) {
    if (!name || name[0] == '\0') {
        return WH_ERROR_EMPTY_NAME;
    }
    if (!city || city[0] == '\0') {
        return WH_ERROR_EMPTY_CITY;
    }
    if (capacity <= 0) {
        return WH_ERROR_INVALID_CAPACITY;
    }
    if (currentStock < 0 || currentStock > capacity) {
        return WH_ERROR_INVALID_STOCK;
    }
    return WH_SUCCESS;
}

static bool IsDuplicateID(Warehouse *head, int id) {
    Warehouse *current = head;
    while (current) {
        if (current->warehouseId == id) {
            return true;
        }
        current = current->next;
    }
    return false;
}

WarehouseStatus CreateWarehouse(int id, const char *name, const char *city,
                                const char *manager, const char *phone,
                                int capacity, int currentStock,
                                Warehouse **outWarehouse) {
    if (!outWarehouse) {
        return WH_ERROR_NULL_POINTER;
    }

    WarehouseStatus valStatus = ValidateWarehouseData(name, city, capacity, currentStock);
    if (valStatus != WH_SUCCESS) {
        return valStatus;
    }

    Warehouse *newWarehouse = (Warehouse *)malloc(sizeof(Warehouse));
    if (!newWarehouse) {
        return WH_ERROR_MEMORY_ALLOCATION;
    }

    newWarehouse->warehouseId = id;
    strncpy(newWarehouse->warehouseName, name, sizeof(newWarehouse->warehouseName) - 1);
    newWarehouse->warehouseName[sizeof(newWarehouse->warehouseName) - 1] = '\0';

    strncpy(newWarehouse->city, city, sizeof(newWarehouse->city) - 1);
    newWarehouse->city[sizeof(newWarehouse->city) - 1] = '\0';

    if (manager) {
        strncpy(newWarehouse->manager, manager, sizeof(newWarehouse->manager) - 1);
        newWarehouse->manager[sizeof(newWarehouse->manager) - 1] = '\0';
    } else {
        newWarehouse->manager[0] = '\0';
    }

    if (phone) {
        strncpy(newWarehouse->phone, phone, sizeof(newWarehouse->phone) - 1);
        newWarehouse->phone[sizeof(newWarehouse->phone) - 1] = '\0';
    } else {
        newWarehouse->phone[0] = '\0';
    }

    newWarehouse->capacity = capacity;
    newWarehouse->currentStock = currentStock;
    newWarehouse->next = NULL;

    *outWarehouse = newWarehouse;
    return WH_SUCCESS;
}

WarehouseStatus InsertWarehouse(Warehouse **head, Warehouse *newWarehouse) {
    if (!head || !newWarehouse) {
        return WH_ERROR_NULL_POINTER;
    }

    if (IsDuplicateID(*head, newWarehouse->warehouseId)) {
        return WH_ERROR_DUPLICATE_ID;
    }

    newWarehouse->next = *head;
    *head = newWarehouse;
    return WH_SUCCESS;
}

WarehouseStatus DeleteWarehouse(Warehouse **head, int id) {
    if (!head) {
        return WH_ERROR_NULL_POINTER;
    }
    if (!*head) {
        return WH_ERROR_LIST_EMPTY;
    }

    Warehouse *current = *head;
    Warehouse *previous = NULL;

    while (current) {
        if (current->warehouseId == id) {
            if (previous) {
                previous->next = current->next;
            } else {
                *head = current->next;
            }
            free(current);
            return WH_SUCCESS;
        }
        previous = current;
        current = current->next;
    }

    return WH_ERROR_NOT_FOUND;
}

WarehouseStatus UpdateWarehouse(Warehouse *head, int id, const char *name,
                                const char *city, const char *manager,
                                const char *phone, int capacity, int currentStock) {
    if (!head) {
        return WH_ERROR_NULL_POINTER;
    }

    Warehouse *target = SearchWarehouseByID(head, id);
    if (!target) {
        return WH_ERROR_NOT_FOUND;
    }

    WarehouseStatus valStatus = ValidateWarehouseData(name, city, capacity, currentStock);
    if (valStatus != WH_SUCCESS) {
        return valStatus;
    }

    strncpy(target->warehouseName, name, sizeof(target->warehouseName) - 1);
    target->warehouseName[sizeof(target->warehouseName) - 1] = '\0';

    strncpy(target->city, city, sizeof(target->city) - 1);
    target->city[sizeof(target->city) - 1] = '\0';

    if (manager) {
        strncpy(target->manager, manager, sizeof(target->manager) - 1);
        target->manager[sizeof(target->manager) - 1] = '\0';
    } else {
        target->manager[0] = '\0';
    }

    if (phone) {
        strncpy(target->phone, phone, sizeof(target->phone) - 1);
        target->phone[sizeof(target->phone) - 1] = '\0';
    } else {
        target->phone[0] = '\0';
    }

    target->capacity = capacity;
    target->currentStock = currentStock;

    return WH_SUCCESS;
}

Warehouse *SearchWarehouseByID(Warehouse *head, int id) {
    Warehouse *current = head;
    while (current) {
        if (current->warehouseId == id) {
            return current;
        }
        current = current->next;
    }
    return NULL;
}

Warehouse *SearchWarehouseByName(Warehouse *head, const char *name) {
    if (!name) {
        return NULL;
    }
    Warehouse *current = head;
    while (current) {
        if (strcmp(current->warehouseName, name) == 0) {
            return current;
        }
        current = current->next;
    }
    return NULL;
}

int GetWarehouseCount(Warehouse *head) {
    int count = 0;
    Warehouse *current = head;
    while (current) {
        count++;
        current = current->next;
    }
    return count;
}

bool IsWarehouseEmpty(Warehouse *head) {
    return head == NULL;
}

void DisplayWarehouses(Warehouse *head) {
    Warehouse *current = head;
    while (current) {
        printf("ID: %d | Name: %s | City: %s | Manager: %s | Phone: %s | Capacity: %d | Stock: %d\n",
               current->warehouseId, current->warehouseName, current->city,
               current->manager, current->phone, current->capacity, current->currentStock);
        current = current->next;
    }
}

void DestroyWarehouseList(Warehouse **head) {
    if (!head) {
        return;
    }
    Warehouse *current = *head;
    while (current) {
        Warehouse *next = current->next;
        free(current);
        current = next;
    }
    *head = NULL;
}

typedef struct {
    int warehouseId;
    char warehouseName[50];
    char city[40];
    char manager[40];
    char phone[20];
    int capacity;
    int currentStock;
} WarehouseSerialized;

WarehouseStatus SaveWarehouseData(Warehouse *head) {
#ifdef _WIN32
    _mkdir("data");
#else
    mkdir("data", 0777);
#endif

    FILE *file = fopen("data/warehouse.dat", "wb");
    if (!file) {
        Toast_Show("Error: Failed to open database for writing!", TOAST_ERROR);
        return WH_ERROR_FILE_OPEN;
    }

    Warehouse *current = head;
    while (current) {
        WarehouseSerialized serialized;
        memset(&serialized, 0, sizeof(WarehouseSerialized));
        serialized.warehouseId = current->warehouseId;
        strncpy(serialized.warehouseName, current->warehouseName, sizeof(serialized.warehouseName) - 1);
        strncpy(serialized.city, current->city, sizeof(serialized.city) - 1);
        strncpy(serialized.manager, current->manager, sizeof(serialized.manager) - 1);
        strncpy(serialized.phone, current->phone, sizeof(serialized.phone) - 1);
        serialized.capacity = current->capacity;
        serialized.currentStock = current->currentStock;

        size_t written = fwrite(&serialized, sizeof(WarehouseSerialized), 1, file);
        if (written != 1) {
            fclose(file);
            Toast_Show("Error: Write failure to warehouse database!", TOAST_ERROR);
            return WH_ERROR_FILE_WRITE;
        }
        current = current->next;
    }

    fclose(file);
    return WH_SUCCESS;
}

WarehouseStatus LoadWarehouseData(Warehouse **head) {
    if (!head) {
        return WH_ERROR_NULL_POINTER;
    }

    FILE *file = fopen("data/warehouse.dat", "rb");
    if (!file) {
        if (errno == ENOENT) {
            *head = NULL;
            return WH_ERROR_NOT_FOUND;
        } else {
            Toast_Show("Error: Failed to open database file!", TOAST_ERROR);
            return WH_ERROR_FILE_OPEN;
        }
    }

    DestroyWarehouseList(head);

    WarehouseSerialized serialized;
    Warehouse *last = NULL;

    while (true) {
        size_t readCount = fread(&serialized, sizeof(WarehouseSerialized), 1, file);
        if (readCount == 0) {
            if (feof(file)) {
                break;
            }
            fclose(file);
            DestroyWarehouseList(head);
            Toast_Show("Error: Read failure from warehouse database!", TOAST_ERROR);
            return WH_ERROR_FILE_READ;
        }

        // Validate fields to check corruption
        if (serialized.warehouseId < 0) {
            fclose(file);
            DestroyWarehouseList(head);
            Toast_Show("Error: Corrupted database (invalid ID)!", TOAST_ERROR);
            return WH_ERROR_FILE_READ;
        }
        if (serialized.warehouseName[0] == '\0') {
            fclose(file);
            DestroyWarehouseList(head);
            Toast_Show("Error: Corrupted database (empty name)!", TOAST_ERROR);
            return WH_ERROR_FILE_READ;
        }
        if (serialized.city[0] == '\0') {
            fclose(file);
            DestroyWarehouseList(head);
            Toast_Show("Error: Corrupted database (empty city)!", TOAST_ERROR);
            return WH_ERROR_FILE_READ;
        }
        if (serialized.capacity <= 0) {
            fclose(file);
            DestroyWarehouseList(head);
            Toast_Show("Error: Corrupted database (invalid capacity)!", TOAST_ERROR);
            return WH_ERROR_FILE_READ;
        }
        if (serialized.currentStock < 0 || serialized.currentStock > serialized.capacity) {
            fclose(file);
            DestroyWarehouseList(head);
            Toast_Show("Error: Corrupted database (invalid stock)!", TOAST_ERROR);
            return WH_ERROR_FILE_READ;
        }

        // Ensure no duplicate IDs
        if (SearchWarehouseByID(*head, serialized.warehouseId) != NULL) {
            fclose(file);
            DestroyWarehouseList(head);
            Toast_Show("Error: Corrupted database (duplicate ID)!", TOAST_ERROR);
            return WH_ERROR_DUPLICATE_ID;
        }

        Warehouse *newWh = NULL;
        WarehouseStatus status = CreateWarehouse(
            serialized.warehouseId,
            serialized.warehouseName,
            serialized.city,
            serialized.manager,
            serialized.phone,
            serialized.capacity,
            serialized.currentStock,
            &newWh
        );

        if (status != WH_SUCCESS) {
            fclose(file);
            DestroyWarehouseList(head);
            Toast_Show("Error: Failed to instantiate warehouse from database!", TOAST_ERROR);
            return status;
        }

        if (*head == NULL) {
            *head = newWh;
        } else {
            last->next = newWh;
        }
        last = newWh;
    }

    fclose(file);
    return WH_SUCCESS;
}

WarehouseStatus BuildWarehouseSearchArray(Warehouse *head, Warehouse ***outArray, int *outCount) {
    if (!outArray || !outCount) {
        return WH_ERROR_NULL_POINTER;
    }
    
    int count = GetWarehouseCount(head);
    if (count <= 0) {
        *outArray = NULL;
        *outCount = 0;
        return WH_SUCCESS;
    }

    Warehouse **arr = (Warehouse **)malloc(sizeof(Warehouse *) * count);
    if (!arr) {
        return WH_ERROR_MEMORY_ALLOCATION;
    }

    Warehouse *current = head;
    for (int i = 0; i < count; ++i) {
        arr[i] = current;
        current = current->next;
    }

    *outArray = arr;
    *outCount = count;
    return WH_SUCCESS;
}

static int CompareWarehousePointers(const void *a, const void *b) {
    Warehouse *whA = *(Warehouse **)a;
    Warehouse *whB = *(Warehouse **)b;
    return (whA->warehouseId - whB->warehouseId);
}

void SortWarehouseSearchArray(Warehouse **array, int count) {
    if (array && count > 0) {
        qsort(array, count, sizeof(Warehouse *), CompareWarehousePointers);
    }
}

Warehouse *BinarySearchWarehouseByID(Warehouse **array, int count, int targetId) {
    if (!array || count <= 0) {
        return NULL;
    }
    
    int low = 0;
    int high = count - 1;
    while (low <= high) {
        int mid = low + (high - low) / 2;
        if (array[mid]->warehouseId == targetId) {
            return array[mid];
        } else if (array[mid]->warehouseId < targetId) {
            low = mid + 1;
        } else {
            high = mid - 1;
        }
    }
    
    return NULL;
}