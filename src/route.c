#include "route.h"
#include <math.h>
#include <stdlib.h>
#include <string.h>
#include <float.h>

bool InitializeRouteGraph(RouteGraph *graph) {
    if (!graph) {
        return false;
    }
    graph->nodesHead = NULL;
    graph->nodeCount = 0;
    graph->edgeCount = 0;
    return true;
}

RouteNode *FindRouteNode(const RouteGraph *graph, int nodeId) {
    if (!graph) {
        return NULL;
    }
    RouteNode *curr = graph->nodesHead;
    while (curr) {
        if (curr->nodeId == nodeId) {
            return curr;
        }
        curr = curr->next;
    }
    return NULL;
}

bool AddRouteNode(RouteGraph *graph, int nodeId, const char *name) {
    if (!graph || nodeId <= 0 || !name || name[0] == '\0') {
        return false;
    }
    // Check duplicate ID
    if (FindRouteNode(graph, nodeId) != NULL) {
        return false;
    }

    RouteNode *newNode = (RouteNode *)malloc(sizeof(RouteNode));
    if (!newNode) {
        return false;
    }

    newNode->nodeId = nodeId;
    strncpy(newNode->name, name, MAX_ROUTE_NODE_NAME - 1);
    newNode->name[MAX_ROUTE_NODE_NAME - 1] = '\0';
    newNode->edgesHead = NULL;
    newNode->next = NULL;

    // Insert at front of graph nodes list
    newNode->next = graph->nodesHead;
    graph->nodesHead = newNode;
    graph->nodeCount++;

    return true;
}

bool AddRouteEdge(RouteGraph *graph, int srcNodeId, int destNodeId, float weight) {
    if (!graph || srcNodeId == destNodeId || !isfinite(weight) || weight < 0.0f) {
        return false;
    }

    RouteNode *src = FindRouteNode(graph, srcNodeId);
    RouteNode *dest = FindRouteNode(graph, destNodeId);
    if (!src || !dest) {
        return false;
    }

    // Check duplicate edge
    RouteEdge *curr = src->edgesHead;
    while (curr) {
        if (curr->destNodeId == destNodeId) {
            return false; // Duplicate edge not allowed
        }
        curr = curr->next;
    }

    // Allocate new edge
    RouteEdge *newEdge = (RouteEdge *)malloc(sizeof(RouteEdge));
    if (!newEdge) {
        return false;
    }

    newEdge->destNodeId = destNodeId;
    newEdge->weight = weight;
    newEdge->next = src->edgesHead;
    src->edgesHead = newEdge;
    graph->edgeCount++;

    return true;
}

bool RemoveRouteEdge(RouteGraph *graph, int srcNodeId, int destNodeId) {
    if (!graph) {
        return false;
    }

    RouteNode *src = FindRouteNode(graph, srcNodeId);
    if (!src) {
        return false;
    }

    RouteEdge *curr = src->edgesHead;
    RouteEdge *prev = NULL;
    while (curr) {
        if (curr->destNodeId == destNodeId) {
            if (prev) {
                prev->next = curr->next;
            } else {
                src->edgesHead = curr->next;
            }
            free(curr);
            graph->edgeCount--;
            return true;
        }
        prev = curr;
        curr = curr->next;
    }

    return false;
}

size_t GetRouteNodeCount(const RouteGraph *graph) {
    return graph ? graph->nodeCount : 0;
}

size_t GetRouteEdgeCount(const RouteGraph *graph) {
    return graph ? graph->edgeCount : 0;
}

void DestroyRouteGraph(RouteGraph *graph) {
    if (!graph) {
        return;
    }

    RouteNode *currNode = graph->nodesHead;
    while (currNode) {
        RouteNode *nextNode = currNode->next;
        
        // Free all edges of this node
        RouteEdge *currEdge = currNode->edgesHead;
        while (currEdge) {
            RouteEdge *nextEdge = currEdge->next;
            free(currEdge);
            currEdge = nextEdge;
        }
        
        free(currNode);
        currNode = nextNode;
    }

    graph->nodesHead = NULL;
    graph->nodeCount = 0;
    graph->edgeCount = 0;
}

// BFS uses a fixed traversal buffer defined by MAX_TRAVERSAL_NODES.
// With linked-list node lookup and linear visited checks, the actual
// worst-case complexity is O(V^2 + E*V).
typedef struct BFSQueue {
    int data[MAX_TRAVERSAL_NODES];
    int front;
    int rear;
} BFSQueue;

static void InitBFSQueue(BFSQueue *q) {
    q->front = 0;
    q->rear = 0;
}

static bool IsBFSQueueEmpty(const BFSQueue *q) {
    return q->front == q->rear;
}

static bool PushBFSQueue(BFSQueue *q, int val) {
    if (q->rear >= MAX_TRAVERSAL_NODES) {
        return false;
    }
    q->data[q->rear++] = val;
    return true;
}

static int PopBFSQueue(BFSQueue *q) {
    if (IsBFSQueueEmpty(q)) {
        return -1;
    }
    return q->data[q->front++];
}

static bool Contains(const int *arr, int size, int val) {
    for (int i = 0; i < size; ++i) {
        if (arr[i] == val) {
            return true;
        }
    }
    return false;
}

bool BFSRouteSearch(const RouteGraph *graph, int srcNodeId, int destNodeId, int *outTraversalOrder, int *outTraversalCount) {
    if (!graph || !outTraversalOrder || !outTraversalCount) {
        return false;
    }

    *outTraversalCount = 0;
    if (graph->nodeCount == 0) {
        return false;
    }

    RouteNode *srcNode = FindRouteNode(graph, srcNodeId);
    if (!srcNode) {
        return false;
    }

    BFSQueue queue;
    InitBFSQueue(&queue);

    int visited[MAX_TRAVERSAL_NODES];
    int visitedCount = 0;

    PushBFSQueue(&queue, srcNodeId);
    visited[visitedCount++] = srcNodeId;

    bool reached = false;

    while (!IsBFSQueueEmpty(&queue)) {
        int currId = PopBFSQueue(&queue);
        outTraversalOrder[(*outTraversalCount)++] = currId;

        if (currId == destNodeId) {
            reached = true;
        }

        RouteNode *node = FindRouteNode(graph, currId);
        if (node) {
            RouteEdge *edge = node->edgesHead;
            while (edge) {
                if (!Contains(visited, visitedCount, edge->destNodeId)) {
                    if (visitedCount < MAX_TRAVERSAL_NODES) {
                        visited[visitedCount++] = edge->destNodeId;
                        PushBFSQueue(&queue, edge->destNodeId);
                    }
                }
                edge = edge->next;
            }
        }
    }

    return reached;
}

// DFS has the same linked-list lookup and linear visited-check costs:
// O(V^2 + E*V) worst case, with O(V) traversal stack/visited storage.
static void DFSRecursive(const RouteGraph *graph, int nodeId, int *visited, int *visitedCount, int *outTraversalOrder, int *outTraversalCount) {
    if (*visitedCount >= MAX_TRAVERSAL_NODES) {
        return;
    }

    visited[(*visitedCount)++] = nodeId;
    outTraversalOrder[(*outTraversalCount)++] = nodeId;

    RouteNode *node = FindRouteNode(graph, nodeId);
    if (node) {
        RouteEdge *edge = node->edgesHead;
        while (edge) {
            if (!Contains(visited, *visitedCount, edge->destNodeId)) {
                DFSRecursive(graph, edge->destNodeId, visited, visitedCount, outTraversalOrder, outTraversalCount);
            }
            edge = edge->next;
        }
    }
}

bool DFSRouteSearch(const RouteGraph *graph, int srcNodeId, int *outTraversalOrder, int *outTraversalCount) {
    if (!graph || !outTraversalOrder || !outTraversalCount) {
        return false;
    }

    *outTraversalCount = 0;
    if (graph->nodeCount == 0) {
        return false;
    }

    if (!FindRouteNode(graph, srcNodeId)) {
        return false;
    }

    int visited[MAX_TRAVERSAL_NODES];
    int visitedCount = 0;

    DFSRecursive(graph, srcNodeId, visited, &visitedCount, outTraversalOrder, outTraversalCount);
    return true;
}

// Helper structure for Dijkstra. The simple array-based minimum search is
// O(V^2); linear graph/node lookups make the overall worst case O(V^2 + E*V).
typedef struct {
    int nodeId;
    float dist;
    int prevId;
    bool visited;
} DijkstraNode;

bool FindShortestRoute(const RouteGraph *graph, int srcNodeId, int destNodeId, float *outTotalDistance, int *outPath, int *outPathLength) {
    if (!graph || !outTotalDistance || !outPath || !outPathLength || graph->nodeCount == 0) {
        return false;
    }

    *outPathLength = 0;
    *outTotalDistance = FLT_MAX;

    RouteNode *srcNode = FindRouteNode(graph, srcNodeId);
    RouteNode *destNode = FindRouteNode(graph, destNodeId);
    if (!srcNode || !destNode) {
        return false;
    }

    // Allocate node table for Dijkstra
    int n = (int)graph->nodeCount;
    DijkstraNode *nodes = (DijkstraNode *)malloc(sizeof(DijkstraNode) * n);
    if (!nodes) {
        return false;
    }

    // Map graph nodes to node table
    RouteNode *curr = graph->nodesHead;
    for (int i = 0; i < n; ++i) {
        nodes[i].nodeId = curr->nodeId;
        nodes[i].dist = (curr->nodeId == srcNodeId) ? 0.0f : FLT_MAX;
        nodes[i].prevId = -1;
        nodes[i].visited = false;
        curr = curr->next;
    }

    for (int step = 0; step < n; ++step) {
        // Find unvisited node with minimum distance
        int minIdx = -1;
        float minDist = FLT_MAX;
        for (int i = 0; i < n; ++i) {
            if (!nodes[i].visited && nodes[i].dist < minDist) {
                minDist = nodes[i].dist;
                minIdx = i;
            }
        }

        if (minIdx == -1 || minDist == FLT_MAX) {
            break; // Target or remaining nodes unreachable
        }

        nodes[minIdx].visited = true;
        int currId = nodes[minIdx].nodeId;

        if (currId == destNodeId) {
            break; // Found shortest path to destination
        }

        RouteNode *rNode = FindRouteNode(graph, currId);
        if (rNode) {
            RouteEdge *edge = rNode->edgesHead;
            while (edge) {
                // Find neighbor index in DijkstraNode table
                int neighborIdx = -1;
                for (int i = 0; i < n; ++i) {
                    if (nodes[i].nodeId == edge->destNodeId) {
                        neighborIdx = i;
                        break;
                    }
                }

                if (neighborIdx != -1 && !nodes[neighborIdx].visited) {
                    float newDist = minDist + edge->weight;
                    if (newDist < nodes[neighborIdx].dist) {
                        nodes[neighborIdx].dist = newDist;
                        nodes[neighborIdx].prevId = currId;
                    }
                }
                edge = edge->next;
            }
        }
    }

    // Find destination index in the node table
    int destIdx = -1;
    for (int i = 0; i < n; ++i) {
        if (nodes[i].nodeId == destNodeId) {
            destIdx = i;
            break;
        }
    }

    if (destIdx == -1 || nodes[destIdx].dist == FLT_MAX) {
        free(nodes);
        return false; // No path found
    }

    *outTotalDistance = nodes[destIdx].dist;

    // Reconstruct path
    int tempPath[MAX_TRAVERSAL_NODES];
    int tempCount = 0;
    int currId = destNodeId;

    while (currId != -1) {
        tempPath[tempCount++] = currId;
        // Find predecessor
        int predId = -1;
        for (int i = 0; i < n; ++i) {
            if (nodes[i].nodeId == currId) {
                predId = nodes[i].prevId;
                break;
            }
        }
        currId = predId;
        if (tempCount >= MAX_TRAVERSAL_NODES) {
            break;
        }
    }

    // Reverse path to get source -> destination order
    for (int i = 0; i < tempCount; ++i) {
        outPath[i] = tempPath[tempCount - 1 - i];
    }
    *outPathLength = tempCount;

    free(nodes);
    return true;
}
