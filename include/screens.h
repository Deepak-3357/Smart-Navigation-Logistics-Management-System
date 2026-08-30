#ifndef SCREENS_H
#define SCREENS_H

void InitDashboardScreen(void);
void UpdateDashboardScreen(void);
void DrawDashboardScreen(void);

void InitWarehouseScreen(void);
void UpdateWarehouseScreen(void);
void DrawWarehouseScreen(void);

void InitPackagesScreen(void);
void UpdatePackagesScreen(void);
void DrawPackagesScreen(void);

void InitRoutesScreen(void);
void UpdateRoutesScreen(void);
void DrawRoutesScreen(void);
void RunRouteOptimizerWorkflow(void);

void InitAnalyticsScreen(void);
void UpdateAnalyticsScreen(void);
void DrawAnalyticsScreen(void);

/* Cross-screen quick workflows use the existing module-owned data structures. */
void ScanWarehouseCargoWorkflow(void);
void DispatchNextFleetWorkflow(void);

void InitSettingsScreen(void);
void UpdateSettingsScreen(void);
void DrawSettingsScreen(void);

#endif // SCREENS_H
