#include "package.h"

#include <math.h>
#include <stdlib.h>
#include <string.h>

static bool IsValidPackage(const Package *package) {
    if (!package || package->packageId <= 0 ||
        package->origin[0] == '\0' || package->destination[0] == '\0' ||
        package->cargoType[0] == '\0' ||
        package->status[0] == '\0' ||
        !isfinite(package->weight) || package->weight < 0.0f ||
        package->priority < 0 || package->priority > 2) {
        return false;
    }

    return memchr(package->origin, '\0', PACKAGE_ORIGIN_LEN) != NULL &&
           memchr(package->destination, '\0', PACKAGE_DESTINATION_LEN) != NULL &&
           memchr(package->cargoType, '\0', PACKAGE_CARGO_TYPE_LEN) != NULL &&
           memchr(package->status, '\0', PACKAGE_STATUS_LEN) != NULL;
}

bool InitializePackageQueue(PackageQueue *queue) {
    if (!queue) {
        return false;
    }

    queue->front = NULL;
    queue->rear = NULL;
    queue->size = 0;
    return true;
}

bool EnqueuePackage(PackageQueue *queue, const Package *package) {
    if (!queue || !IsValidPackage(package)) {
        return false;
    }

    PackageQueueNode *node = malloc(sizeof(*node));
    if (!node) {
        return false;
    }

    node->package = *package;
    node->next = NULL;

    if (!queue->front || package->priority < queue->front->package.priority) {
        node->next = queue->front;
        queue->front = node;
        if (!queue->rear) {
            queue->rear = node;
        }
    } else {
        /* Insert after all packages with an equal or higher queue priority.
         * This keeps equal-priority packages in their original FIFO order. */
        PackageQueueNode *current = queue->front;
        while (current->next &&
               current->next->package.priority <= package->priority) {
            current = current->next;
        }
        node->next = current->next;
        current->next = node;
        if (!node->next) {
            queue->rear = node;
        }
    }
    queue->size++;
    return true;
}

bool DequeuePackage(PackageQueue *queue, Package *outPackage) {
    if (!queue || !queue->front) {
        return false;
    }

    PackageQueueNode *node = queue->front;
    if (outPackage) {
        *outPackage = node->package;
    }

    queue->front = node->next;
    if (!queue->front) {
        queue->rear = NULL;
    }
    queue->size--;
    free(node);
    return true;
}

bool PeekPackage(const PackageQueue *queue, Package *outPackage) {
    if (!queue || !queue->front || !outPackage) {
        return false;
    }

    *outPackage = queue->front->package;
    return true;
}

bool IsPackageQueueEmpty(const PackageQueue *queue) {
    return !queue || queue->size == 0;
}

size_t GetPackageQueueSize(const PackageQueue *queue) {
    return queue ? queue->size : 0;
}

void DestroyPackageQueue(PackageQueue *queue) {
    if (!queue) {
        return;
    }

    PackageQueueNode *node = queue->front;
    while (node) {
        PackageQueueNode *next = node->next;
        free(node);
        node = next;
    }

    queue->front = NULL;
    queue->rear = NULL;
    queue->size = 0;
}
