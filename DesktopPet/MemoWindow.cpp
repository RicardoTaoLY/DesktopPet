#include "MemoWindow.h"
#include <windowsx.h>
#include <commctrl.h>
#include <string>
#include <iomanip>
#include <sstream>
#include <ctime>

#pragma comment(lib, "comctl32.lib")

MemoWindow::MemoWindow(HINSTANCE hInstance, JsonHandler& jsonHandler)  
    : hInstance(hInstance), jsonHandler(jsonHandler), hwnd(NULL), dragging(false), dragPoint({0, 0}), hEdit(NULL), hList(NULL) {  
}

MemoWindow::~MemoWindow() {
    Close();
}

void MemoWindow::Close() {
    if (hwnd) {
        DestroyWindow(hwnd);
        hwnd = nullptr;  // 关键：将句柄设为nullptr
    }
}

bool MemoWindow::Create() {
    INITCOMMONCONTROLSEX icc;
    icc.dwSize = sizeof(icc);
    icc.dwICC = ICC_LISTVIEW_CLASSES;
    InitCommonControlsEx(&icc);

    WNDCLASS wc = {};
    wc.lpfnWndProc = [](HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam) -> LRESULT {
        if (uMsg == WM_NCCREATE) {
            LPCREATESTRUCT create = reinterpret_cast<LPCREATESTRUCT>(lParam);
            SetWindowLongPtr(hwnd, GWLP_USERDATA, reinterpret_cast<LONG_PTR>(create->lpCreateParams));
            return TRUE;
        }

        MemoWindow* pThis = reinterpret_cast<MemoWindow*>(GetWindowLongPtr(hwnd, GWLP_USERDATA));
        if (pThis) {
            return pThis->HandleMessage(hwnd, uMsg, wParam, lParam);
        }

        return DefWindowProc(hwnd, uMsg, wParam, lParam);
        };
    wc.hInstance = hInstance;
    wc.lpszClassName = L"MemoWindowClass";
    wc.hbrBackground = CreateSolidBrush(RGB(34, 34, 34));
    RegisterClass(&wc);

    hwnd = CreateWindowEx(
        WS_EX_TOPMOST,
        L"MemoWindowClass",
        L"像素备忘录",
        WS_POPUP | WS_CAPTION | WS_SYSMENU | WS_MINIMIZEBOX | WS_SIZEBOX,
        CW_USEDEFAULT, CW_USEDEFAULT, 400, 500,
        NULL, NULL, hInstance, this
    );

    if (!hwnd) return false;

    // 创建UI控件
    hEdit = CreateWindowEx(
        0, L"EDIT", NULL,
        WS_CHILD | WS_VISIBLE | WS_BORDER | ES_MULTILINE | ES_AUTOVSCROLL | ES_WANTRETURN,
        10, 50, 380, 100,
        hwnd, NULL, hInstance, NULL
    );

    HWND hAddButton = CreateWindow(
        L"BUTTON", L"添加备忘录",
        WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON,
        10, 160, 380, 30,
        hwnd, (HMENU)1, hInstance, NULL
    );

    hList = CreateWindowEx(
        WS_EX_CLIENTEDGE, WC_LISTVIEW, NULL,
        WS_CHILD | WS_VISIBLE | LVS_REPORT | LVS_SINGLESEL | LVS_NOCOLUMNHEADER | LVS_OWNERDRAWFIXED | WS_VSCROLL,
        10, 200, 380, 280,
        hwnd, NULL, hInstance, NULL
    );

    ListView_SetExtendedListViewStyle(hList, LVS_EX_FULLROWSELECT);

    LVCOLUMN col;
    col.mask = LVCF_WIDTH;
    col.cx = 365;
    ListView_InsertColumn(hList, 0, &col);

    jsonHandler.LoadMemos();
    UpdateMemoList();

    return true;
}

void MemoWindow::Show() {
    if (hwnd) {
        ShowWindow(hwnd, SW_SHOW);
        UpdateWindow(hwnd);
    }
}

void MemoWindow::Hide() {
    if (hwnd) ShowWindow(hwnd, SW_HIDE);
}

LRESULT MemoWindow::HandleMessage(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam) {
    switch (uMsg) {
    case WM_COMMAND: {
        int id = LOWORD(wParam);
        if (id == 1) { // Add button
            AddMemo();
        }
        break;
    }
    case WM_NOTIFY: {
        LPNMHDR nmhdr = (LPNMHDR)lParam;
        if (nmhdr->idFrom == 0 && nmhdr->code == NM_DBLCLK) {
            int index = ListView_GetSelectionMark(hList);
            if (index != -1) {
                DeleteMemo(index);
            }
        }
        break;
    }
    case WM_CTLCOLOREDIT: {
        HDC hdc = (HDC)wParam;
        SetBkColor(hdc, RGB(34, 34, 34));
        SetTextColor(hdc, RGB(255, 255, 255));
        return (LRESULT)GetStockObject(DC_BRUSH);
    }
    case WM_CLOSE:  // 处理关闭消息
        Hide();     // 隐藏窗口而不是销毁
        return 0;   // 阻止默认关闭行为
    case WM_DESTROY:
        jsonHandler.SaveMemos();
        break;
    case WM_SIZE: {
        if (hEdit) {
            SetWindowPos(hEdit, NULL, 10, 50, LOWORD(lParam) - 20, 100, SWP_NOZORDER);
            SetWindowPos(GetDlgItem(hwnd, 1), NULL, 10, 160, LOWORD(lParam) - 20, 30, SWP_NOZORDER);
            SetWindowPos(hList, NULL, 10, 200, LOWORD(lParam) - 20, HIWORD(lParam) - 210, SWP_NOZORDER);

            LVCOLUMN col;
            col.mask = LVCF_WIDTH;
            col.cx = LOWORD(lParam) - 25;
            ListView_SetColumn(hList, 0, &col);
        }
        break;
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
    case WM_MEASUREITEM: {
        PMEASUREITEMSTRUCT pmis = (PMEASUREITEMSTRUCT)lParam;
        pmis->itemHeight = 60; // 每个备忘录项的高度
        return TRUE;
    }
    case WM_DRAWITEM: {
        PDRAWITEMSTRUCT pdis = (PDRAWITEMSTRUCT)lParam;
        if (pdis->itemAction == ODA_DRAWENTIRE) {
            auto memos = jsonHandler.GetMemos();
            if (pdis->itemID < memos.size()) {
                const MemoData& memo = memos[pdis->itemID];

                HBRUSH hBrush = CreateSolidBrush(RGB(51, 51, 51));
                FillRect(pdis->hDC, &pdis->rcItem, hBrush);
                DeleteObject(hBrush);

                // 绘制时间戳
                std::string timestamp = FormatTimestamp(memo.timestamp);
                SetTextColor(pdis->hDC, RGB(136, 136, 136));
                SetBkMode(pdis->hDC, TRANSPARENT);
                TextOutA(pdis->hDC, pdis->rcItem.left + 5, pdis->rcItem.top + 5, timestamp.c_str(), timestamp.length());

                // 绘制内容
                SetTextColor(pdis->hDC, RGB(255, 255, 255));
                RECT rcText = pdis->rcItem;
                rcText.top += 20;
                DrawTextA(pdis->hDC, memo.content.c_str(), -1, &rcText, DT_LEFT | DT_WORDBREAK);

                // 绘制边框
                HBRUSH hBorder = CreateSolidBrush(RGB(102, 102, 102));
                FrameRect(pdis->hDC, &pdis->rcItem, hBorder);
                DeleteObject(hBorder);
            }
        }
        return TRUE;
    }
    default:
        return DefWindowProc(hwnd, uMsg, wParam, lParam);
    }
    return 0;
}

void MemoWindow::UpdateMemoList() {
    ListView_DeleteAllItems(hList);

    auto memos = jsonHandler.GetMemos();
    for (int i = 0; i < memos.size(); i++) {
        LVITEM item = {};
        item.mask = LVIF_TEXT | LVIF_PARAM;
        item.iItem = i;
        item.lParam = i;
        item.pszText = LPSTR_TEXTCALLBACK;
        ListView_InsertItem(hList, &item);
    }
}

void MemoWindow::AddMemo() {
    int len = GetWindowTextLengthA(hEdit);
    if (len > 0) {
        char* buffer = new char[len + 1];
        GetWindowTextA(hEdit, buffer, len + 1);

        jsonHandler.AddMemo(buffer);
        SetWindowTextA(hEdit, "");
        UpdateMemoList();

        delete[] buffer;
    }
}

void MemoWindow::DeleteMemo(int index) {
    jsonHandler.DeleteMemo(index);
    UpdateMemoList();
}

std::string MemoWindow::FormatTimestamp(time_t timestamp) {
    tm tm;
    localtime_s(&tm, &timestamp);

    char buffer[20];
    strftime(buffer, sizeof(buffer), "%Y-%m-%d %H:%M:%S", &tm);
    return buffer;
}