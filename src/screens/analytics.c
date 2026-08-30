#include "screens.h"
#include "common.h"
#include "theme_manager.h"
#include "components.h"
#include "layout.h"
#include "warehouse.h"
#include "package.h"
#include "route.h"
#include "window_manager.h"
#include <errno.h>
#include <stdarg.h>
#include <stdio.h>
#include <string.h>
#include <time.h>

#ifdef _WIN32
#include <direct.h>
#else
#include <sys/stat.h>
#endif

#define MAX_ANALYTICS_BARS 7
#define MAX_ANALYTICS_STATUSES 8
#define MAX_ANALYTICS_PRIORITY_BUCKETS 5

typedef struct {
    char name[PACKAGE_STATUS_LEN];
    int count;
} AnalyticsStatusCount;

typedef struct {
    int warehouseCount;
    int totalCapacity;
    int totalStock;
    double overallUtilization;
    double averageUtilization;
    int nearCapacityCount;
    int underutilizedCount;

    size_t queuedPackageCount;
    float queuedWeight;
    int priorityCounts[MAX_ANALYTICS_PRIORITY_BUCKETS];
    int otherPriorityCount;
    AnalyticsStatusCount statusCounts[MAX_ANALYTICS_STATUSES];
    int statusCount;

    size_t routeNodeCount;
    size_t routeEdgeCount;
    char routeStatus[32];
    int chartCount;
    float chartValues[MAX_ANALYTICS_BARS];
    char chartLabels[MAX_ANALYTICS_BARS][16];
} AnalyticsMetrics;

static AnalyticsMetrics s_Metrics;
static bool s_IsRefreshing = false;
static float s_RefreshTimer = 0.0f;

#define ANALYTICS_REPORT_PATH "reports/analytics_report.pdf"
#define PDF_PAGE_WIDTH 612
#define PDF_PAGE_HEIGHT 792
#define PDF_PAGE_BUFFER_SIZE 16384

typedef struct {
    char content[PDF_PAGE_BUFFER_SIZE];
    size_t length;
} PdfPage;

static bool PdfAppend(PdfPage *page, const char *format, ...) {
    if (!page || page->length >= sizeof(page->content)) {
        return false;
    }

    va_list args;
    va_start(args, format);
    int written = vsnprintf(page->content + page->length,
                            sizeof(page->content) - page->length,
                            format, args);
    va_end(args);

    if (written < 0 || (size_t)written >= sizeof(page->content) - page->length) {
        return false;
    }
    page->length += (size_t)written;
    return true;
}

static bool PdfAddText(PdfPage *page, int x, int y, int fontSize, const char *text) {
    if (!PdfAppend(page, "BT /F1 %d Tf 1 0 0 1 %d %d Tm (", fontSize, x, y)) {
        return false;
    }

    for (const unsigned char *ch = (const unsigned char *)text; *ch; ++ch) {
        if (*ch == '(' || *ch == ')' || *ch == '\\') {
            if (!PdfAppend(page, "\\%c", *ch)) {
                return false;
            }
        } else if (*ch == '\r' || *ch == '\n') {
            if (!PdfAppend(page, " ")) {
                return false;
            }
        } else if (*ch >= 32) {
            if (!PdfAppend(page, "%c", *ch)) {
                return false;
            }
        }
    }
    return PdfAppend(page, ") Tj ET\n");
}

static bool PdfAddRule(PdfPage *page, int y) {
    return PdfAppend(page, "0.75 w 50 %d m 562 %d l S\n", y, y);
}

static bool PdfAddMetric(PdfPage *page, int *y, const char *label, const char *value) {
    if (!PdfAddText(page, 60, *y, 11, label) ||
        !PdfAddText(page, 350, *y, 11, value)) {
        return false;
    }
    *y -= 22;
    return true;
}

static bool EnsureReportsDirectory(void) {
#ifdef _WIN32
    if (_mkdir("reports") != 0 && errno != EEXIST) {
        return false;
    }
#else
    if (mkdir("reports", 0777) != 0 && errno != EEXIST) {
        return false;
    }
#endif
    return true;
}

static bool WritePdfObject(FILE *file, long *offset, int objectNumber,
                           const char *body) {
    *offset = ftell(file);
    return fprintf(file, "%d 0 obj\n%s\nendobj\n", objectNumber, body) >= 0;
}

static bool WritePdfStream(FILE *file, long *offset, int objectNumber,
                           const PdfPage *page) {
    *offset = ftell(file);
    if (fprintf(file, "%d 0 obj\n<< /Length %zu >>\nstream\n",
                objectNumber, page->length) < 0) {
        return false;
    }
    if (page->length > 0 && fwrite(page->content, 1, page->length, file) != page->length) {
        return false;
    }
    return fprintf(file, "\nendstream\nendobj\n") >= 0;
}

static bool WriteAnalyticsPdf(void) {
    if (!EnsureReportsDirectory()) {
        return false;
    }

    PdfPage pages[2] = { 0 };
    char line[128];
    char generatedAt[64] = "Unavailable";
    time_t now = time(NULL);
    struct tm localTime;
    if (now != (time_t)-1) {
#ifdef _WIN32
        if (localtime_s(&localTime, &now) == 0) {
#else
        if (localtime_r(&now, &localTime) != NULL) {
#endif
            strftime(generatedAt, sizeof(generatedAt), "%Y-%m-%d %H:%M:%S", &localTime);
        }
    }

    if (!PdfAddText(&pages[0], 50, 742, 20, "SMART NAVIGATION & LOGISTICS") ||
        !PdfAddText(&pages[0], 50, 716, 20, "MANAGEMENT SYSTEM") ||
        !PdfAddText(&pages[0], 50, 682, 14, "ANALYTICS REPORT") ||
        !PdfAddText(&pages[0], 50, 654, 10, "Report generated: ") ||
        !PdfAddText(&pages[0], 155, 654, 10, generatedAt) ||
        !PdfAddRule(&pages[0], 638) ||
        !PdfAddText(&pages[0], 50, 612, 14, "1. WAREHOUSE ANALYTICS")) {
        return false;
    }

    int y = 580;
    if (s_Metrics.warehouseCount == 0) {
        if (!PdfAddText(&pages[0], 60, y, 11, "No warehouse data available")) {
            return false;
        }
        y -= 30;
    } else {
        snprintf(line, sizeof(line), "%d", s_Metrics.warehouseCount);
        if (!PdfAddMetric(&pages[0], &y, "Total Warehouses", line)) return false;
        snprintf(line, sizeof(line), "%d m\263", s_Metrics.totalCapacity);
        if (!PdfAddMetric(&pages[0], &y, "Total Capacity", line)) return false;
        snprintf(line, sizeof(line), "%d m\263", s_Metrics.totalStock);
        if (!PdfAddMetric(&pages[0], &y, "Current Stock", line)) return false;
        snprintf(line, sizeof(line), "%.1f%%", s_Metrics.overallUtilization);
        if (!PdfAddMetric(&pages[0], &y, "Overall Utilization", line)) return false;
        snprintf(line, sizeof(line), "%.1f%%", s_Metrics.averageUtilization);
        if (!PdfAddMetric(&pages[0], &y, "Average Warehouse Utilization", line)) return false;
        snprintf(line, sizeof(line), "%d", s_Metrics.nearCapacityCount);
        if (!PdfAddMetric(&pages[0], &y, "Near-Capacity Warehouses", line)) return false;
        snprintf(line, sizeof(line), "%d", s_Metrics.underutilizedCount);
        if (!PdfAddMetric(&pages[0], &y, "Underutilized Warehouses", line)) return false;
    }

    if (!PdfAddRule(&pages[0], y - 4) ||
        !PdfAddText(&pages[0], 50, y - 30, 14, "SYSTEM OVERVIEW")) {
        return false;
    }
    y -= 60;
    snprintf(line, sizeof(line), "%d warehouses / %d m\263 capacity",
             s_Metrics.warehouseCount, s_Metrics.totalCapacity);
    if (!PdfAddMetric(&pages[0], &y, "Warehouse Network", line)) return false;
    snprintf(line, sizeof(line), "%zu packages / %.1f kg queued",
             s_Metrics.queuedPackageCount, s_Metrics.queuedWeight);
    if (!PdfAddMetric(&pages[0], &y, "Package Queue", line)) return false;
    snprintf(line, sizeof(line), "%zu hubs / %zu connections",
             s_Metrics.routeNodeCount, s_Metrics.routeEdgeCount);
    if (!PdfAddMetric(&pages[0], &y, "Route Network", line)) return false;
    snprintf(line, sizeof(line), "%.1f%%", s_Metrics.overallUtilization);
    if (!PdfAddMetric(&pages[0], &y, "Warehouse Utilization", line)) return false;

    if (!PdfAddText(&pages[1], 50, 742, 16, "SMART NAVIGATION & LOGISTICS - ANALYTICS") ||
        !PdfAddText(&pages[1], 50, 704, 14, "2. PACKAGE ANALYTICS")) {
        return false;
    }
    y = 672;
    if (s_Metrics.queuedPackageCount == 0) {
        if (!PdfAddText(&pages[1], 60, y, 11, "No packages currently queued")) return false;
        y -= 30;
    } else {
        snprintf(line, sizeof(line), "%zu", s_Metrics.queuedPackageCount);
        if (!PdfAddMetric(&pages[1], &y, "Packages Currently in Queue", line)) return false;
        snprintf(line, sizeof(line), "%.1f kg", s_Metrics.queuedWeight);
        if (!PdfAddMetric(&pages[1], &y, "Total Queued Weight", line)) return false;
    }
    if (!PdfAddText(&pages[1], 60, y, 11, "Priority Distribution")) return false;
    y -= 22;
    for (int i = 0; i < MAX_ANALYTICS_PRIORITY_BUCKETS; ++i) {
        snprintf(line, sizeof(line), "P%d: %d", i, s_Metrics.priorityCounts[i]);
        if (!PdfAddText(&pages[1], 80, y, 10, line)) return false;
        y -= 18;
    }
    if (s_Metrics.otherPriorityCount > 0) {
        snprintf(line, sizeof(line), "Other: %d", s_Metrics.otherPriorityCount);
        if (!PdfAddText(&pages[1], 80, y, 10, line)) return false;
        y -= 18;
    }
    if (!PdfAddText(&pages[1], 60, y, 11, "Status Distribution")) return false;
    y -= 22;
    if (s_Metrics.statusCount == 0) {
        if (!PdfAddText(&pages[1], 80, y, 10, "none")) return false;
        y -= 18;
    } else {
        for (int i = 0; i < s_Metrics.statusCount; ++i) {
            snprintf(line, sizeof(line), "%s: %d", s_Metrics.statusCounts[i].name,
                     s_Metrics.statusCounts[i].count);
            if (!PdfAddText(&pages[1], 80, y, 10, line)) return false;
            y -= 18;
        }
    }

    if (!PdfAddRule(&pages[1], y - 4) ||
        !PdfAddText(&pages[1], 50, y - 30, 14, "3. ROUTE ANALYTICS")) {
        return false;
    }
    y -= 60;
    if (s_Metrics.routeNodeCount == 0) {
        if (!PdfAddText(&pages[1], 60, y, 11, "No route network available")) return false;
        y -= 30;
    } else {
        snprintf(line, sizeof(line), "%zu", s_Metrics.routeNodeCount);
        if (!PdfAddMetric(&pages[1], &y, "Total Hubs", line)) return false;
        snprintf(line, sizeof(line), "%zu", s_Metrics.routeEdgeCount);
        if (!PdfAddMetric(&pages[1], &y, "Total Directed Connections", line)) return false;
        if (!PdfAddMetric(&pages[1], &y, "Graph Connectivity Status", s_Metrics.routeStatus)) return false;
    }
    if (!PdfAddText(&pages[1], 60, y, 11, "Algorithm Availability")) return false;
    y -= 22;
    if (!PdfAddText(&pages[1], 80, y, 10, "BFS - Available")) return false;
    y -= 18;
    if (!PdfAddText(&pages[1], 80, y, 10, "DFS - Available")) return false;
    y -= 18;
    if (!PdfAddText(&pages[1], 80, y, 10, "Dijkstra - Available")) return false;

    FILE *file = fopen(ANALYTICS_REPORT_PATH, "wb");
    if (!file) {
        return false;
    }

    long offsets[8] = { 0 };
    bool success = fprintf(file, "%%PDF-1.4\n%%\xE2\xE3\xCF\xD3\n") >= 0;
    const char *catalog = "<< /Type /Catalog /Pages 2 0 R >>";
    const char *pagesObject = "<< /Type /Pages /Kids [3 0 R 5 0 R] /Count 2 >>";
    const char *page1 = "<< /Type /Page /Parent 2 0 R /MediaBox [0 0 612 792] /Resources << /Font << /F1 7 0 R >> >> /Contents 4 0 R >>";
    const char *page2 = "<< /Type /Page /Parent 2 0 R /MediaBox [0 0 612 792] /Resources << /Font << /F1 7 0 R >> >> /Contents 6 0 R >>";
    const char *font = "<< /Type /Font /Subtype /Type1 /BaseFont /Helvetica /Encoding /WinAnsiEncoding >>";
    if (success) success = WritePdfObject(file, &offsets[1], 1, catalog);
    if (success) success = WritePdfObject(file, &offsets[2], 2, pagesObject);
    if (success) success = WritePdfObject(file, &offsets[3], 3, page1);
    if (success) success = WritePdfStream(file, &offsets[4], 4, &pages[0]);
    if (success) success = WritePdfObject(file, &offsets[5], 5, page2);
    if (success) success = WritePdfStream(file, &offsets[6], 6, &pages[1]);
    if (success) success = WritePdfObject(file, &offsets[7], 7, font);

    long xrefOffset = ftell(file);
    if (success && fprintf(file, "xref\n0 8\n0000000000 65535 f \n") < 0) success = false;
    for (int i = 1; success && i < 8; ++i) {
        if (fprintf(file, "%010ld 00000 n \n", offsets[i]) < 0) success = false;
    }
    if (success && fprintf(file, "trailer\n<< /Size 8 /Root 1 0 R >>\nstartxref\n%ld\n%%%%EOF\n", xrefOffset) < 0) {
        success = false;
    }
    if (ferror(file) != 0 || fclose(file) != 0) {
        success = false;
    }
    return success;
}

static void ResetAnalyticsMetrics(void) {
    memset(&s_Metrics, 0, sizeof(s_Metrics));
    strcpy(s_Metrics.routeStatus, "EMPTY");
}

static void CalculateWarehouseAnalytics(void) {
    Warehouse *head = GetWarehouseListHead();
    const Warehouse *warehouse = head;
    double utilizationSum = 0.0;
    int utilizationCount = 0;

    s_Metrics.warehouseCount = GetWarehouseCount(head);

    while (warehouse) {
        s_Metrics.totalCapacity += warehouse->capacity;
        s_Metrics.totalStock += warehouse->currentStock;

        if (warehouse->capacity > 0) {
            double utilization = ((double)warehouse->currentStock / (double)warehouse->capacity) * 100.0;
            utilizationSum += utilization;
            utilizationCount++;

            if (utilization >= 90.0) {
                s_Metrics.nearCapacityCount++;
            }
            if (utilization < 50.0) {
                s_Metrics.underutilizedCount++;
            }

            if (s_Metrics.chartCount < MAX_ANALYTICS_BARS) {
                snprintf(s_Metrics.chartLabels[s_Metrics.chartCount],
                         sizeof(s_Metrics.chartLabels[s_Metrics.chartCount]),
                         "WH-%d", warehouse->warehouseId);
                s_Metrics.chartValues[s_Metrics.chartCount] = (float)utilization;
                s_Metrics.chartCount++;
            }
        }
        warehouse = warehouse->next;
    }

    if (s_Metrics.totalCapacity > 0) {
        s_Metrics.overallUtilization =
            ((double)s_Metrics.totalStock / (double)s_Metrics.totalCapacity) * 100.0;
    }
    if (utilizationCount > 0) {
        s_Metrics.averageUtilization = utilizationSum / (double)utilizationCount;
    }
}

static void CalculatePackageAnalytics(void) {
    const PackageQueueNode *node = GetPackageQueueHead();
    while (node) {
        const Package *package = &node->package;
        s_Metrics.queuedPackageCount++;
        s_Metrics.queuedWeight += package->weight;

        if (package->priority >= 0 && package->priority < MAX_ANALYTICS_PRIORITY_BUCKETS) {
            s_Metrics.priorityCounts[package->priority]++;
        } else {
            s_Metrics.otherPriorityCount++;
        }

        int statusIndex = -1;
        for (int i = 0; i < s_Metrics.statusCount; ++i) {
            if (strcmp(s_Metrics.statusCounts[i].name, package->status) == 0) {
                statusIndex = i;
                break;
            }
        }
        if (statusIndex < 0 && s_Metrics.statusCount < MAX_ANALYTICS_STATUSES) {
            statusIndex = s_Metrics.statusCount++;
            strncpy(s_Metrics.statusCounts[statusIndex].name,
                    package->status, PACKAGE_STATUS_LEN - 1);
            s_Metrics.statusCounts[statusIndex].name[PACKAGE_STATUS_LEN - 1] = '\0';
        }
        if (statusIndex >= 0) {
            s_Metrics.statusCounts[statusIndex].count++;
        }

        node = node->next;
    }
}

static void CalculateRouteAnalytics(void) {
    const RouteGraph *graph = GetRouteGraph();
    if (!graph) {
        return;
    }

    s_Metrics.routeNodeCount = GetRouteNodeCount(graph);
    s_Metrics.routeEdgeCount = GetRouteEdgeCount(graph);
    if (s_Metrics.routeNodeCount == 0) {
        return;
    }

    const RouteNode *start = graph->nodesHead;
    int reachableCount = 0;
    int traversal[MAX_TRAVERSAL_NODES];
    int traversalCount = 0;
    for (const RouteNode *node = graph->nodesHead; node; node = node->next) {
        if (BFSRouteSearch(graph, start->nodeId, node->nodeId,
                           traversal, &traversalCount)) {
            reachableCount++;
        }
    }

    if (reachableCount == (int)s_Metrics.routeNodeCount) {
        strcpy(s_Metrics.routeStatus, "CONNECTED");
    } else {
        strcpy(s_Metrics.routeStatus, "PARTIALLY DISCONNECTED");
    }
}

static void RecalculateAnalytics(void) {
    ResetAnalyticsMetrics();
    CalculateWarehouseAnalytics();
    CalculatePackageAnalytics();
    CalculateRouteAnalytics();
}

static void onRefreshAnalytics(void) {
    s_IsRefreshing = true;
    s_RefreshTimer = 0.35f;
    Toast_Show("Refreshing live analytics from active modules...", TOAST_INFO);
}

static void onExportReport(void) {
    if (WriteAnalyticsPdf()) {
        Toast_Show("Analytics PDF exported successfully.", TOAST_SUCCESS);
    } else {
        Toast_Show("Failed to export Analytics PDF.", TOAST_ERROR);
    }
}

static void FormatPrioritySummary(char *output, size_t outputSize) {
    size_t used = 0;
    output[0] = '\0';
    for (int i = 0; i < MAX_ANALYTICS_PRIORITY_BUCKETS && used < outputSize; ++i) {
        int written = snprintf(output + used, outputSize - used, "%sP%d: %d",
                               i == 0 ? "" : "  ", i, s_Metrics.priorityCounts[i]);
        if (written < 0 || (size_t)written >= outputSize - used) {
            return;
        }
        used += (size_t)written;
    }
    if (s_Metrics.otherPriorityCount > 0 && used < outputSize) {
        snprintf(output + used, outputSize - used, "  Other: %d", s_Metrics.otherPriorityCount);
    }
}

static void FormatStatusSummary(char *output, size_t outputSize) {
    size_t used = 0;
    output[0] = '\0';
    if (s_Metrics.statusCount == 0) {
        snprintf(output, outputSize, "none");
        return;
    }
    for (int i = 0; i < s_Metrics.statusCount && used < outputSize; ++i) {
        int written = snprintf(output + used, outputSize - used, "%s%s: %d",
                               i == 0 ? "" : "  ", s_Metrics.statusCounts[i].name,
                               s_Metrics.statusCounts[i].count);
        if (written < 0 || (size_t)written >= outputSize - used) {
            return;
        }
        used += (size_t)written;
    }
}

void InitAnalyticsScreen(void) {
    s_IsRefreshing = false;
    s_RefreshTimer = 0.0f;
    RecalculateAnalytics();
}

void UpdateAnalyticsScreen(void) {
    if (!s_IsRefreshing) {
        return;
    }

    s_RefreshTimer -= GetFrameTime();
    if (s_RefreshTimer <= 0.0f) {
        s_IsRefreshing = false;
        RecalculateAnalytics();
        Toast_Show("Live analytics refreshed successfully.", TOAST_SUCCESS);
    }
}

void DrawAnalyticsScreen(void) {
    int contentX = GetContentX();
    int contentWidth = GetContentWidth();
    int cardHeight = CARD_HEIGHT;

    char warehouseValue[64];
    char warehouseSubtext[96];
    if (s_Metrics.warehouseCount == 0) {
        snprintf(warehouseValue, sizeof(warehouseValue), "No data");
        snprintf(warehouseSubtext, sizeof(warehouseSubtext), "No warehouse data available");
    } else {
        snprintf(warehouseValue, sizeof(warehouseValue), "%d Hubs", s_Metrics.warehouseCount);
        snprintf(warehouseSubtext, sizeof(warehouseSubtext), "%d near capacity | %d underutilized",
                 s_Metrics.nearCapacityCount, s_Metrics.underutilizedCount);
    }

    char capacityValue[64];
    char capacitySubtext[64];
    if (s_Metrics.warehouseCount == 0) {
        snprintf(capacityValue, sizeof(capacityValue), "No data");
        snprintf(capacitySubtext, sizeof(capacitySubtext), "No warehouse data available");
    } else {
        snprintf(capacityValue, sizeof(capacityValue), "%d m³", s_Metrics.totalCapacity);
        snprintf(capacitySubtext, sizeof(capacitySubtext), "Total storage capacity");
    }

    char stockValue[64];
    char stockSubtext[64];
    if (s_Metrics.warehouseCount == 0) {
        snprintf(stockValue, sizeof(stockValue), "No data");
        snprintf(stockSubtext, sizeof(stockSubtext), "No warehouse data available");
    } else {
        snprintf(stockValue, sizeof(stockValue), "%d m³", s_Metrics.totalStock);
        snprintf(stockSubtext, sizeof(stockSubtext), "Current stored inventory");
    }

    char utilizationValue[64];
    char utilizationSubtext[96];
    if (s_Metrics.warehouseCount == 0) {
        snprintf(utilizationValue, sizeof(utilizationValue), "No data");
        snprintf(utilizationSubtext, sizeof(utilizationSubtext), "No warehouse data available");
    } else {
        snprintf(utilizationValue, sizeof(utilizationValue), "%.1f%%", s_Metrics.overallUtilization);
        snprintf(utilizationSubtext, sizeof(utilizationSubtext), "Average: %.1f%%", s_Metrics.averageUtilization);
    }

    char packageValue[64];
    char packageSubtext[96];
    if (s_Metrics.queuedPackageCount == 0) {
        snprintf(packageValue, sizeof(packageValue), "Empty");
        snprintf(packageSubtext, sizeof(packageSubtext), "No packages currently queued");
    } else {
        snprintf(packageValue, sizeof(packageValue), "%zu Packages", s_Metrics.queuedPackageCount);
        snprintf(packageSubtext, sizeof(packageSubtext), "%.1f kg queued | %d statuses",
                 s_Metrics.queuedWeight, s_Metrics.statusCount);
    }

    char queuedWeightValue[64];
    char queuedWeightSubtext[64];
    if (s_Metrics.queuedPackageCount == 0) {
        snprintf(queuedWeightValue, sizeof(queuedWeightValue), "No data");
        snprintf(queuedWeightSubtext, sizeof(queuedWeightSubtext), "No packages currently queued");
    } else {
        snprintf(queuedWeightValue, sizeof(queuedWeightValue), "%.1f kg", s_Metrics.queuedWeight);
        snprintf(queuedWeightSubtext, sizeof(queuedWeightSubtext), "Total queued package weight");
    }

    char routeValue[64];
    char routeSubtext[96];
    if (s_Metrics.routeNodeCount == 0) {
        snprintf(routeValue, sizeof(routeValue), "No data");
        snprintf(routeSubtext, sizeof(routeSubtext), "No route network available");
    } else {
        snprintf(routeValue, sizeof(routeValue), "%zu Hubs", s_Metrics.routeNodeCount);
        snprintf(routeSubtext, sizeof(routeSubtext), "%s", s_Metrics.routeStatus);
    }

    char routeConnectionsValue[64];
    char routeConnectionsSubtext[64];
    if (s_Metrics.routeNodeCount == 0) {
        snprintf(routeConnectionsValue, sizeof(routeConnectionsValue), "No data");
        snprintf(routeConnectionsSubtext, sizeof(routeConnectionsSubtext), "No route network available");
    } else {
        snprintf(routeConnectionsValue, sizeof(routeConnectionsValue), "%zu", s_Metrics.routeEdgeCount);
        snprintf(routeConnectionsSubtext, sizeof(routeConnectionsSubtext), "Directed connections");
    }

    DrawUICard(GetCardRect(0, 0, 4, cardHeight),
               "WAREHOUSE ANALYTICS", warehouseValue, warehouseSubtext, g_Theme.accentBlue);
    DrawUICard(GetCardRect(1, 0, 4, cardHeight),
               "TOTAL CAPACITY", capacityValue, capacitySubtext, g_Theme.accentTeal);
    DrawUICard(GetCardRect(2, 0, 4, cardHeight),
               "CURRENT STOCK", stockValue, stockSubtext, g_Theme.warning);
    DrawUICard(GetCardRect(3, 0, 4, cardHeight),
               "WAREHOUSE UTILIZATION", utilizationValue, utilizationSubtext, g_Theme.success);

    DrawUICard(GetCardRect(0, 1, 4, cardHeight),
               "PACKAGES IN QUEUE", packageValue, packageSubtext, g_Theme.warning);
    DrawUICard(GetCardRect(1, 1, 4, cardHeight),
               "QUEUED WEIGHT", queuedWeightValue, queuedWeightSubtext, g_Theme.accentBlue);
    DrawUICard(GetCardRect(2, 1, 4, cardHeight),
               "ROUTE HUBS", routeValue, routeSubtext, g_Theme.accentTeal);
    DrawUICard(GetCardRect(3, 1, 4, cardHeight),
               "ROUTE CONNECTIONS", routeConnectionsValue, routeConnectionsSubtext, g_Theme.success);

    int cardsBottom = GetCardY(1, cardHeight) + cardHeight;
    int chartY = GetToolbarY(cardsBottom);
    int chartH = GetScreenHeight() - chartY - STATUS_BAR_HEIGHT;
    Rectangle chartPanel = { (float)contentX, (float)chartY, (float)contentWidth, (float)chartH };
    DrawRectangleRounded(chartPanel, 0.04f, 4, g_Theme.bgSecondary);
    DrawRectangleRoundedLines(chartPanel, 0.04f, 4, g_Theme.border);

    DrawSectionTitle("LIVE SNAPSHOT — Warehouse Utilization",
                     (float)(contentX + CARD_INTERNAL_PADDING),
                     (float)(chartY + CARD_INTERNAL_PADDING));

    int btnW = 140;
    Rectangle exportRect = { (float)(contentX + contentWidth - btnW - CARD_INTERNAL_PADDING),
                             (float)(chartY + CARD_INTERNAL_PADDING - 4),
                             (float)btnW, (float)BUTTON_HEIGHT };
    Rectangle refreshRect = { (float)(exportRect.x - btnW - BUTTON_SPACING),
                              (float)(chartY + CARD_INTERNAL_PADDING - 4),
                              (float)btnW, (float)BUTTON_HEIGHT };
    DrawUIButton(refreshRect, "Refresh Analytics", false, onRefreshAnalytics);
    DrawUIButton(exportRect, "Export PDF", false, onExportReport);

    char prioritySummary[192];
    char statusSummary[256];
    FormatPrioritySummary(prioritySummary, sizeof(prioritySummary));
    FormatStatusSummary(statusSummary, sizeof(statusSummary));
    char packageSummary[512];
    snprintf(packageSummary, sizeof(packageSummary), "Queued package priorities: %s",
             prioritySummary);
    DrawTextRegular(packageSummary, (float)(contentX + CARD_INTERNAL_PADDING),
                    (float)(chartY + CARD_INTERNAL_PADDING + 32),
                    FONT_SMALL_LABEL, g_Theme.textSecondary);
    char statusLine[320];
    snprintf(statusLine, sizeof(statusLine), "Queued package statuses: %s", statusSummary);
    DrawTextRegular(statusLine, (float)(contentX + CARD_INTERNAL_PADDING),
                    (float)(chartY + CARD_INTERNAL_PADDING + 52),
                    FONT_SMALL_LABEL, g_Theme.textSecondary);
    char routeLine[192];
    snprintf(routeLine, sizeof(routeLine),
             "Route graph: %s",
             s_Metrics.routeStatus);
    DrawTextRegular(routeLine, (float)(contentX + CARD_INTERNAL_PADDING),
                    (float)(chartY + CARD_INTERNAL_PADDING + 72),
                    FONT_SMALL_LABEL, g_Theme.textSecondary);
    DrawTextRegular("GRAPH ALGORITHMS  ✓ BFS — Available   ✓ DFS — Available   ✓ Dijkstra — Available",
                    (float)(contentX + CARD_INTERNAL_PADDING),
                    (float)(chartY + CARD_INTERNAL_PADDING + 92),
                    FONT_SMALL_LABEL, g_Theme.textSecondary);

    int axisX = contentX + 60;
    int axisY = chartY + chartH - 50;
    int axisW = contentWidth - 100;
    int axisH = chartH - 160;
    if (axisH < 80) {
        axisH = 80;
    }

    int gridLines = 4;
    for (int i = 0; i <= gridLines; ++i) {
        int gy = axisY - (i * (axisH / gridLines));
        DrawLine(axisX, gy, axisX + axisW, gy, g_Theme.border);
        char label[16];
        snprintf(label, sizeof(label), "%d%%", i * 25);
        DrawTextRegular(label, (float)(axisX - 35), (float)(gy - 6),
                        FONT_MICRO_LABEL, g_Theme.textSecondary);
    }

    if (s_IsRefreshing) {
        DrawRectangle(axisX, axisY - axisH, axisW, axisH,
                      ColorAlpha(g_Theme.bgPrimary, 0.6f));
        const char *loadingText = "Recalculating live analytics...";
        Vector2 loadTextSize = MeasureWithFont(g_FontBold, loadingText, FONT_SMALL_LABEL);
        DrawTextBold(loadingText, axisX + (axisW - loadTextSize.x) / 2.0f,
                     axisY - axisH / 2.0f, FONT_SMALL_LABEL, g_Theme.textPrimary);
    } else if (s_Metrics.chartCount == 0) {
        const char *emptyText = "No warehouse utilization data available.";
        Vector2 emptySize = MeasureWithFont(g_FontRegular, emptyText, FONT_SMALL_LABEL);
        DrawTextRegular(emptyText, axisX + (axisW - emptySize.x) / 2.0f,
                        axisY - axisH / 2.0f, FONT_SMALL_LABEL, g_Theme.textSecondary);
    } else {
        int spacing = axisW / s_Metrics.chartCount;
        int barWidth = spacing > 60 ? 40 : (spacing - 12);
        if (barWidth < 12) {
            barWidth = 12;
        }
        Vector2 mouse = Window_GetInputMousePosition();
        bool modalActive = Modal_IsActive();

        for (int i = 0; i < s_Metrics.chartCount; ++i) {
            int bx = axisX + (i * spacing) + (spacing - barWidth) / 2;
            float ratio = s_Metrics.chartValues[i] / 100.0f;
            if (ratio < 0.0f) ratio = 0.0f;
            if (ratio > 1.0f) ratio = 1.0f;
            int barH = (int)(axisH * ratio);
            int by = axisY - barH;
            Rectangle barRect = { (float)bx, (float)by, (float)barWidth, (float)barH };
            bool hovered = !modalActive && CheckCollisionPointRec(mouse, barRect);
            DrawRectangleRounded(barRect, 0.15f, 4,
                                 hovered ? g_Theme.accentTeal : g_Theme.accentBlue);
            DrawTextRegular(s_Metrics.chartLabels[i], (float)bx,
                            (float)(axisY + 12), FONT_MICRO_LABEL,
                            g_Theme.textSecondary);
            if (hovered) {
                char value[24];
                snprintf(value, sizeof(value), "%.1f%%", s_Metrics.chartValues[i]);
                DrawTextBold(value, (float)bx, (float)(by - 22),
                             FONT_MICRO_LABEL, g_Theme.textPrimary);
            }
        }
    }

    char thresholdLine[192];
    snprintf(thresholdLine, sizeof(thresholdLine),
             "Near capacity: %d (>= 90%%)    Underutilized: %d (< 50%%)",
             s_Metrics.nearCapacityCount, s_Metrics.underutilizedCount);
    DrawTextRegular(thresholdLine, (float)(contentX + CARD_INTERNAL_PADDING),
                    (float)(axisY + 34), FONT_MICRO_LABEL, g_Theme.textSecondary);
}
