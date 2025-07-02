#include "MonitorWindow.h"
#include <sstream>
#include <iomanip>
#include <windowsx.h>
#include <psapi.h>
#include <algorithm> // 添加min函数支持

#pragma comment(lib, "pdh.lib")

MonitorWindow::MonitorWindow(HINSTANCE hInstance)
    : hInstance(hInstance), hwnd(NULL), dragging(false), cpuUsage(0), 
    memPercent(0), dragPoint{ 0, 0 }
{

    // 初始化PDH查询
    PdhOpenQuery(NULL, NULL, &cpuQuery);
    PdhAddEnglishCounter(cpuQuery, L"\\Processor(_Total)\\% Processor Time", NULL, &cpuTotal);
    PdhCollectQueryData(cpuQuery);
}

MonitorWindow::~MonitorWindow() {
    if (hwnd) DestroyWindow(hwnd);
    PdhCloseQuery(cpuQuery);
}

bool MonitorWindow::Create() {
    WNDCLASS wc = {};
    wc.lpfnWndProc = [](HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam) -> LRESULT {
        if (uMsg == WM_NCCREATE) {
            LPCREATESTRUCT create = reinterpret_cast<LPCREATESTRUCT>(lParam);
            SetWindowLongPtr(hwnd, GWLP_USERDATA, reinterpret_cast<LONG_PTR>(create->lpCreateParams));
            return TRUE;
        }

        MonitorWindow* pThis = reinterpret_cast<MonitorWindow*>(GetWindowLongPtr(hwnd, GWLP_USERDATA));
        if (pThis) {
            return pThis->HandleMessage(hwnd, uMsg, wParam, lParam);
        }

        return DefWindowProc(hwnd, uMsg, wParam, lParam);
        };
    wc.hInstance = hInstance;
    wc.lpszClassName = L"MonitorWindowClass";
    wc.hbrBackground = CreateSolidBrush(RGB(34, 34, 34));
    RegisterClass(&wc);

    // 创建窗口
    hwnd = CreateWindowEx(
        WS_EX_TOPMOST,
        L"MonitorWindowClass",
        L"系统监控",
        WS_POPUP | WS_BORDER,
        CW_USEDEFAULT, CW_USEDEFAULT, 250, 180,
        NULL, NULL, hInstance, this
    );

    return hwnd != NULL;
}

void MonitorWindow::Show() {
    if (hwnd) {
        ShowWindow(hwnd, SW_SHOW);
        UpdateWindow(hwnd);
    }
}

void MonitorWindow::Hide() {
    if (hwnd) ShowWindow(hwnd, SW_HIDE);
}

bool MonitorWindow::IsVisible() {
    return hwnd && IsWindowVisible(hwnd);
}

void MonitorWindow::UpdateStats() {
    // 更新CPU使用率
    PDH_FMT_COUNTERVALUE cpuValue;
    PdhCollectQueryData(cpuQuery);
    PdhGetFormattedCounterValue(cpuTotal, PDH_FMT_DOUBLE, NULL, &cpuValue);
    cpuUsage = cpuValue.doubleValue;

    // 更新内存使用率
    MEMORYSTATUSEX memStatus;
    memStatus.dwLength = sizeof(memStatus);
    GlobalMemoryStatusEx(&memStatus);

    DWORDLONG usedMem = memStatus.ullTotalPhys - memStatus.ullAvailPhys;
    memPercent = (int)((usedMem * 100) / memStatus.ullTotalPhys);

    // 重绘窗口
    if (hwnd) {
        InvalidateRect(hwnd, NULL, TRUE);
        UpdateWindow(hwnd);
    }
}

LRESULT MonitorWindow::HandleMessage(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam) {
    switch (uMsg) {
    case WM_PAINT: {
        PAINTSTRUCT ps;
        HDC hdc = BeginPaint(hwnd, &ps);

        RECT rc;
        GetClientRect(hwnd, &rc);

        // 绘制背景
        HBRUSH hBgBrush = CreateSolidBrush(RGB(34, 34, 34));
        FillRect(hdc, &rc, hBgBrush);
        DeleteObject(hBgBrush);

        // 绘制标题
        HFONT hTitleFont = CreateFont(18, 0, 0, 0, FW_BOLD, FALSE, FALSE, FALSE,
            DEFAULT_CHARSET, OUT_OUTLINE_PRECIS, CLIP_DEFAULT_PRECIS,
            CLEARTYPE_QUALITY, DEFAULT_PITCH, L"微软雅黑");
        HFONT hOldFont = (HFONT)SelectObject(hdc, hTitleFont);

        SetTextColor(hdc, RGB(255, 255, 255));
        SetBkMode(hdc, TRANSPARENT);
        TextOutW(hdc, 80, 5, L"系统监控", 4);

        // 使用正常字体
        HFONT hNormalFont = CreateFont(14, 0, 0, 0, FW_NORMAL, FALSE, FALSE, FALSE,
            DEFAULT_CHARSET, OUT_OUTLINE_PRECIS, CLIP_DEFAULT_PRECIS,
            CLEARTYPE_QUALITY, DEFAULT_PITCH, L"微软雅黑");
        SelectObject(hdc, hNormalFont);

        // CPU监控
        // CPU标签
        TextOutW(hdc, 10, 40, L"CPU:", 4);

        // CPU值
        std::wstring cpuStr = std::to_wstring((int)cpuUsage) + L"%";
        TextOutW(hdc, 210 - cpuStr.length() * 8, 40, cpuStr.c_str(), cpuStr.length());

        // CPU进度条
        RECT cpuBar = { 10, 60, 230, 80 };
        DrawProgressBar(hdc, cpuBar, (int)cpuUsage);

        // 内存监控
        
        // Replace the problematic line with the following code to ensure the correct length is passed to TextOutW.
        TextOutW(hdc, 10, 90, L"内存:", wcslen(L"内存:"));
        // 内存值
        std::wstring memStr = std::to_wstring(memPercent) + L"%";
        TextOutW(hdc, 210 - memStr.length() * 8, 90, memStr.c_str(), memStr.length());

        // 内存进度条
        RECT memBar = { 10, 110, 230, 130 };
        DrawProgressBar(hdc, memBar, memPercent);

        // 绘制窗口边框
        HPEN hPen = CreatePen(PS_SOLID, 1, RGB(102, 102, 102));
        HPEN hOldPen = (HPEN)SelectObject(hdc, hPen);
        HBRUSH hOldBrush = (HBRUSH)SelectObject(hdc, GetStockObject(NULL_BRUSH));
        Rectangle(hdc, rc.left, rc.top, rc.right, rc.bottom);
        SelectObject(hdc, hOldBrush);
        SelectObject(hdc, hOldPen);
        DeleteObject(hPen);

        // 恢复原始字体并删除创建的字体
        SelectObject(hdc, hOldFont);
        DeleteObject(hTitleFont);
        DeleteObject(hNormalFont);

        EndPaint(hwnd, &ps);
        return 0;
    }
    case WM_MOUSEMOVE: {
        if (dragging) {
            POINT pt = { GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam) };
            ClientToScreen(hwnd, &pt);

            RECT rc;
            GetWindowRect(hwnd, &rc);
            int x = rc.left + pt.x - dragPoint.x;
            int y = rc.top + pt.y - dragPoint.y;

            SetWindowPos(hwnd, NULL, x, y, 0, 0, SWP_NOSIZE | SWP_NOZORDER);
        }
        break;
    }
    case WM_LBUTTONDOWN: {
        SetCapture(hwnd);
        dragPoint.x = GET_X_LPARAM(lParam);
        dragPoint.y = GET_Y_LPARAM(lParam);
        dragging = true;
        break;
    }
    case WM_LBUTTONUP: {
        ReleaseCapture();
        dragging = false;
        break;
    }
    case WM_TIMER:
        if (wParam == 1) {
            UpdateStats();
        }
        break;
    case WM_ERASEBKGND:
        return 1; // 防止闪烁
    default:
        return DefWindowProc(hwnd, uMsg, wParam, lParam);
    }
    return 0;
}

void MonitorWindow::DrawProgressBar(HDC hdc, RECT rc, int value) {
    // 绘制背景
    HBRUSH hBgBrush = CreateSolidBrush(RGB(34, 34, 34));
    FillRect(hdc, &rc, hBgBrush);
    DeleteObject(hBgBrush);

    // 绘制边框
    HPEN hPen = CreatePen(PS_SOLID, 1, RGB(102, 102, 102));
    HPEN hOldPen = (HPEN)SelectObject(hdc, hPen);
    HBRUSH hOldBrush = (HBRUSH)SelectObject(hdc, GetStockObject(NULL_BRUSH));
    Rectangle(hdc, rc.left, rc.top, rc.right, rc.bottom);

    // 绘制进度条
    if (value > 0) {
        int progressWidth = (rc.right - rc.left - 4) * value / 100;

        COLORREF barColor;
        if (value < 60) {
            barColor = RGB(50, 205, 50); // 绿色
        }
        else if (value < 80) {
            barColor = RGB(255, 165, 0); // 橙色
        }
        else {
            barColor = RGB(220, 20, 60); // 红色
        }

        // 创建实心画刷
        HBRUSH hBarBrush = CreateSolidBrush(barColor);
        RECT progressRect = {
            rc.left + 2,
            rc.top + 2,
            rc.left + 2 + progressWidth,
            rc.bottom - 2
        };
        FillRect(hdc, &progressRect, hBarBrush);

        // 绘制像素化效果（仅在进度条区域内）
        for (int y = progressRect.top; y < progressRect.bottom; y += 4) {
            for (int x = progressRect.left; x < progressRect.right; x += 4) {
                SetPixel(hdc, x, y, RGB(
                    min(255, GetRValue(barColor) + 20),
                    min(255, GetGValue(barColor) + 20),
                    min(255, GetBValue(barColor) + 20)
                ));
            }
        }

        DeleteObject(hBarBrush);
    }

    SelectObject(hdc, hOldBrush);
    SelectObject(hdc, hOldPen);
    DeleteObject(hPen);
}