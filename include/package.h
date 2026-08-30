#ifndef PACKAGE_H
#define PACKAGE_H

#include <stdbool.h>
#include <stddef.h>

#define PACKAGE_ORIGIN_LEN      100
#define PACKAGE_DESTINATION_LEN 100
#define PACKAGE_CARGO_TYPE_LEN  50
#define PACKAGE_STATUS_LEN      32

typedef struct {
    int packageId;
    char origin[PACKAGE_ORIGIN_LEN];
    char destination[PACKAGE_DESTINATION_LEN];
    char cargoType[PACKAGE_CARGO_TYPE_LEN];
    float weight;
    int priority;
    char status[PACKAGE_STATUS_LEN];
} Package;

typedef struct PackageQueueNode {
    Package package;
    struct PackageQueueNode *next;
} PackageQueueNode;

typedef struct {
    PackageQueueNode *front;
    PackageQueueNode *rear;
    size_t size;
} PackageQueue;

/* Priority queue ordering: 0 is highest, then 1, then 2.  Enqueue inserts
 * in O(n), preserving FIFO order among packages with equal priority.  The
 * front node is therefore always the next package for O(1) peek/dequeue. */

bool InitializePackageQueue(PackageQueue *queue);
bool EnqueuePackage(PackageQueue *queue, const Package *package);
bool DequeuePackage(PackageQueue *queue, Package *outPackage);
bool PeekPackage(const PackageQueue *queue, Package *outPackage);
bool IsPackageQueueEmpty(const PackageQueue *queue);
size_t GetPackageQueueSize(const PackageQueue *queue);
void DestroyPackageQueue(PackageQueue *queue);

/* Read-only view for analytics and other consumers; ownership remains here. */
const PackageQueueNode *GetPackageQueueHead(void);

#endif
