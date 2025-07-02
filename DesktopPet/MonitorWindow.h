#pragma once

#include <Windows.h>
#include <pdh.h>
#include <pdhmsg.h>

class MonitorWindow {
public:
    MonitorWindow(HINSTANCE hInstance);
    ~MonitorWindow();

    bool Create();
    void Show();
    void Hide();
    bool IsVisible();
    void UpdateStats();
    LRESULT HandleMessage(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam);

    void DrawProgressBar(HDC hdc, RECT rc, int value);

    HINSTANCE hInstance;
    HWND hwnd;
    POINT dragPoint;
    bool dragging;

    // PDH查询相关
    PDH_HQUERY cpuQuery;
    PDH_HCOUNTER cpuTotal;

    // 监控数据
    double cpuUsage;
    int memPercent;
};