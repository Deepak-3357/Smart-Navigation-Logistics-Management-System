#ifndef ROUTE_H
#define ROUTE_H

#include <stdbool.h>
#include <stddef.h>

#define MAX_ROUTE_NODE_NAME 64
#define MAX_TRAVERSAL_NODES 100

typedef struct RouteEdge {
    int destNodeId;
    float weight;
    struct RouteEdge *next;
} RouteEdge;

typedef struct RouteNode {
    int nodeId;
    char name[MAX_ROUTE_NODE_NAME];
    RouteEdge *edgesHead;
    struct RouteNode *next;
} RouteNode;

typedef struct {
    RouteNode *nodesHead;
    size_t nodeCount;
    size_t edgeCount;
} RouteGraph;

// Graph Operations
bool InitializeRouteGraph(RouteGraph *graph);
bool AddRouteNode(RouteGraph *graph, int nodeId, const char *name);
bool AddRouteEdge(RouteGraph *graph, int srcNodeId, int destNodeId, float weight);
bool RemoveRouteEdge(RouteGraph *graph, int srcNodeId, int destNodeId);
RouteNode *FindRouteNode(const RouteGraph *graph, int nodeId);
size_t GetRouteNodeCount(const RouteGraph *graph);
size_t GetRouteEdgeCount(const RouteGraph *graph);
void DestroyRouteGraph(RouteGraph *graph);

/* Read-only view for analytics; the Routes screen retains graph ownership. */
const RouteGraph *GetRouteGraph(void);

// Traversal and Pathfinding Algorithms
bool BFSRouteSearch(const RouteGraph *graph, int srcNodeId, int destNodeId, int *outTraversalOrder, int *outTraversalCount);
bool DFSRouteSearch(const RouteGraph *graph, int srcNodeId, int *outTraversalOrder, int *outTraversalCount);
bool FindShortestRoute(const RouteGraph *graph, int srcNodeId, int destNodeId, float *outTotalDistance, int *outPath, int *outPathLength);

#endif // ROUTE_H
