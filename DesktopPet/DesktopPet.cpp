#include "DesktopPet.h"
#include <gdiplus.h>
#include <commctrl.h>
#include <string>
#include <shlobj.h>
#include <shlwapi.h>
#include <random>
#include "Resource.h"
#include <windowsx.h>

#pragma comment(lib, "gdiplus.lib")
#pragma comment(lib, "shlwapi.lib")

using namespace Gdiplus;

DesktopPet::DesktopPet(HINSTANCE hInstance)
    : hInstance(hInstance), jsonHandler("memos.json"), scaleFactor(4), fps(15), speed(4),
    positionY(0), positionX(0), firstMove(true), memoWindow(nullptr), monitorWindow(nullptr),
    animationTimerId(0), stateTimerId(0), hoverTimerId(0), gdiplusToken(0), isDragStarting(false), 
    wasPausedBeforeDrag(false) , lastDragPoint({ 0, 0 })
{

    // 初始化状态
    state = IDLE;
    direction = LEFT; // 初始方向向左
    frameIndex = 0;
    isPaused = false;
    isDragging = false;

    // 初始化随机种子
    std::srand(static_cast<unsigned>(std::time(nullptr)));

    // 初始化GDI+
    GdiplusStartupInput gdiplusStartupInput;
    GdiplusStartup(&gdiplusToken, &gdiplusStartupInput, NULL);

    // 获取屏幕尺寸
    screenWidth = GetSystemMetrics(SM_CXSCREEN);
    screenHeight = GetSystemMetrics(SM_CYSCREEN);
}

DesktopPet::~DesktopPet() {
    Cleanup();

    // 释放帧资源
    for (int state = 0; state < 2; state++) {
        for (int frame = 0; frame < 6; frame++) {
            if (frames[state][frame]) {
                delete frames[state][frame];
            }
        }
    }

    if (memoWindow) delete memoWindow;
    if (monitorWindow) delete monitorWindow;

    GdiplusShutdown(gdiplusToken);
}

bool DesktopPet::Initialize() {
    // 注册窗口类
    WNDCLASS wc = {};
    wc.lpfnWndProc = WindowProc;
    wc.hInstance = hInstance;
    wc.lpszClassName = L"DesktopPetClass";
    wc.hCursor = LoadCursor(NULL, IDC_ARROW);
    wc.hbrBackground = (HBRUSH)GetStockObject(NULL_BRUSH);

    if (!RegisterClass(&wc)) {
        return false;
    }

    InitializeWindow();

    // 加载资源
    LoadFrames();

    // 启动定时器
    animationTimerId = SetTimer(hwnd, 1, 1000 / fps, nullptr);  // 动画定时器
    stateTimerId = SetTimer(hwnd, 2, 3000, nullptr);           // 状态切换定时器 (3秒)

    // 创建子窗口
    memoWindow = new MemoWindow(hInstance, jsonHandler);
    memoWindow->Create();

    monitorWindow = new MonitorWindow(hInstance);
    monitorWindow->Create();

    // 首次更新窗口
    UpdateLayeredWindow();

    return true;
}

void DesktopPet::InitializeWindow() {
    // 创建全屏透明窗口（关键修改：使用WS_EX_NOACTIVATE）
    hwnd = CreateWindowEx(
        WS_EX_LAYERED | WS_EX_TOPMOST | WS_EX_NOACTIVATE, // 移除WS_EX_TRANSPARENT
        L"DesktopPetClass",
        L"桌面宠物",
        WS_POPUP,
        0, 0,
        screenWidth, screenHeight,
        nullptr,
        nullptr,
        hInstance,
        this
    );

    if (!hwnd) {
        return;
    }

    // 设置分层窗口属性
    BLENDFUNCTION blend = {
        AC_SRC_OVER,    // BlendOp
        0,              // BlendFlags
        0,              // 完全透明
        AC_SRC_ALPHA    // 使用Alpha通道
    };

    POINT ptSrc = { 0, 0 };
    SIZE sizeWnd = { screenWidth, screenHeight };
    POINT ptDst = { 0, 0 };

    ::UpdateLayeredWindow(
        hwnd,
        NULL,
        &ptDst,
        &sizeWnd,
        NULL,
        &ptSrc,
        0,
        &blend,
        ULW_ALPHA
    );

    // 显示窗口
    ShowWindow(hwnd, SW_SHOW);
    UpdateWindow(hwnd);

    // 初始位置（右下角）
    if (frames[0][0]) {
        positionX = screenWidth - frames[0][0]->GetWidth() * scaleFactor - 50;
        positionY = screenHeight - frames[0][0]->GetHeight() * scaleFactor - 50;
    }
    else {
        positionX = screenWidth - 32 * scaleFactor - 50;
        positionY = screenHeight - 32 * scaleFactor - 50;
    }
}

void DesktopPet::Run() {
    MSG msg;
    while (GetMessage(&msg, nullptr, 0, 0)) {
        TranslateMessage(&msg);
        DispatchMessage(&msg);
    }
}

void DesktopPet::LoadFrames() {
    const wchar_t* statePaths[2] = { L"frames\\idle_", L"frames\\walk_" };
    int stateFrames[2] = { 5, 6 };

    for (int s = 0; s < 2; s++) {
        for (int f = 0; f < stateFrames[s]; f++) {
            std::wstring path = statePaths[s] + std::to_wstring(f) + L".png";
            frames[s][f] = new Image(path.c_str());
        }
    }
}

void DesktopPet::UpdateSize() {
    // 不需要移动窗口，只需重绘
    InvalidateRect(hwnd, NULL, TRUE);
}

void DesktopPet::MovePet() {
    if (firstMove) {
        firstMove = false;
    }

    // 计算宠物尺寸
    int petWidth = 0;
    int petHeight = 0;
    if (frames[0][0]) {
        petWidth = frames[0][0]->GetWidth() * scaleFactor;
        petHeight = frames[0][0]->GetHeight() * scaleFactor;
    }
    else {
        petWidth = 32 * scaleFactor;
        petHeight = 32 * scaleFactor;
    }

    // 移动位置
    int distance = speed * static_cast<int>(direction) * (scaleFactor / 4);
    positionX += distance;

    // 边界碰撞检测
    if (positionX <= 0) {
        positionX = 0;
        direction = RIGHT; // 碰到左边界，向右走
        frameIndex = 0;
    }
    else if (positionX + petWidth >= screenWidth) {
        positionX = screenWidth - petWidth; // 确保不会超出右边界
        direction = LEFT; // 碰到右边界，向左走
        frameIndex = 0;
    }

    // 触发重绘
    InvalidateRect(hwnd, NULL, TRUE);
}

void DesktopPet::RandomStateChange() {
    if (!isPaused && !isDragging) {
        if (rand() % 100 < 80) { // 80%概率切换状态
            state = (state == IDLE) ? WALK : IDLE;
            frameIndex = 0; // 重置帧索引
        }
    }
}

void DesktopPet::ShowContextMenu(POINT pt) {
    HMENU hMenu = LoadMenu(hInstance, MAKEINTRESOURCE(IDR_MAIN_MENU));
    if (!hMenu) return;

    HMENU hSubMenu = GetSubMenu(hMenu, 0);
    if (!hSubMenu) {
        DestroyMenu(hMenu);
        return;
    }

    // 激活窗口以确保菜单显示
    SetForegroundWindow(hwnd);

    // 根据当前状态更新菜单项
    UINT stateItem = (state == IDLE) ? IDM_STATE_WALK : IDM_STATE_IDLE;
    std::wstring stateText = (state == IDLE) ? L"切换到行走状态" : L"切换到待机状态";
    ModifyMenu(hSubMenu, stateItem, MF_BYCOMMAND | MF_STRING, stateItem, stateText.c_str());

    // 显示菜单
    TrackPopupMenuEx(
        hSubMenu,
        TPM_LEFTALIGN | TPM_TOPALIGN | TPM_RIGHTBUTTON,
        pt.x, pt.y,
        hwnd,
        NULL
    );

    DestroyMenu(hMenu);
}

void DesktopPet::OpenMemo() {
    if (memoWindow) {
        memoWindow->Show();
        memoWindow->UpdateMemoList();
    }
}

void DesktopPet::ShowSystemMonitor() {
    if (monitorWindow) {
        // 计算宠物尺寸
        int petWidth = 0;
        int petHeight = 0;
        if (frames[0][0]) {
            petWidth = frames[0][0]->GetWidth() * scaleFactor;
            petHeight = frames[0][0]->GetHeight() * scaleFactor;
        }
        else {
            petWidth = 32 * scaleFactor;
            petHeight = 32 * scaleFactor;
        }

        // 获取监控窗口尺寸
        RECT monitorRect;
        GetWindowRect(monitorWindow->hwnd, &monitorRect);
        int monitorWidth = monitorRect.right - monitorRect.left;
        int monitorHeight = monitorRect.bottom - monitorRect.top;

        // 计算监控窗口位置
        int x = positionX;
        int y;

        // 检查宠物是否在屏幕上半部分
        if (positionY < screenHeight / 2) {
            // 宠物在上半屏，监控窗口显示在下方
            y = positionY + petHeight + 10; // 在宠物下方10像素

            // 确保不会超出屏幕底部
            if (y + monitorHeight > screenHeight) {
                y = screenHeight - monitorHeight - 10;
            }
        }
        else {
            // 宠物在下半屏，监控窗口显示在上方
            y = positionY - monitorHeight - 10; // 在宠物上方10像素

            // 确保不会超出屏幕顶部
            if (y < 0) {
                y = 10;
            }
        }

        // 确保不会超出屏幕右侧
        if (x + monitorWidth > screenWidth) {
            x = screenWidth - monitorWidth - 10;
        }

        // 确保不会超出屏幕左侧
        if (x < 0) {
            x = 10;
        }

        // 设置监控窗口位置
        SetWindowPos(monitorWindow->hwnd, NULL, x, y, 0, 0, SWP_NOSIZE | SWP_NOZORDER);

        // 显示监控窗口并启动定时器
        monitorWindow->Show();
        SetTimer(monitorWindow->hwnd, 1, 1000, NULL); // 每秒更新
    }
}

void DesktopPet::Cleanup() {
    // 停止所有定时器
    if (animationTimerId) KillTimer(hwnd, animationTimerId);
    if (stateTimerId) KillTimer(hwnd, stateTimerId);
    if (hoverTimerId) KillTimer(hwnd, hoverTimerId);

    // 关闭子窗口
    if (memoWindow) memoWindow->Hide();
    if (monitorWindow) monitorWindow->Hide();
}

void DesktopPet::UpdateLayeredWindow() {
    if (!hwnd) return;

    // 创建内存DC（全屏尺寸）
    HDC hdcScreen = GetDC(NULL);
    HDC hdcMem = CreateCompatibleDC(hdcScreen);
    HBITMAP hBitmap = CreateCompatibleBitmap(hdcScreen, screenWidth, screenHeight);
    HBITMAP hOldBitmap = (HBITMAP)SelectObject(hdcMem, hBitmap);

    // 使用GDI+绘制
    Graphics graphics(hdcMem);
    graphics.SetInterpolationMode(InterpolationModeNearestNeighbor);
    graphics.SetPixelOffsetMode(PixelOffsetModeHalf);

    // 绘制全透明背景
    graphics.Clear(Color(0, 0, 0, 0)); // 完全透明

    // 获取当前帧
    Gdiplus::Image* frame = frames[state][frameIndex];
    if (frame) {
        // 计算宠物尺寸
        int petWidth = frame->GetWidth() * scaleFactor;
        int petHeight = frame->GetHeight() * scaleFactor;

        // 根据方向翻转图像
        if (direction == LEFT) {
            // 创建翻转后的图像
            Bitmap flipped(petWidth, petHeight);
            Graphics flipGraphics(&flipped);
            flipGraphics.SetInterpolationMode(InterpolationModeNearestNeighbor);
            flipGraphics.SetPixelOffsetMode(PixelOffsetModeHalf);
            flipGraphics.SetSmoothingMode(SmoothingModeNone);
            flipGraphics.ScaleTransform(-1.0f, 1.0f);
            flipGraphics.TranslateTransform(-(float)petWidth, 0);
            flipGraphics.DrawImage(frame, 0, 0, petWidth, petHeight);

            // 绘制翻转后的宠物
            graphics.DrawImage(&flipped, positionX, positionY);
        }
        else {
            // 直接绘制宠物
            graphics.DrawImage(frame, positionX, positionY, petWidth, petHeight);
        }
    }

    // 设置分层窗口属性
    POINT ptSrc = { 0, 0 };
    SIZE sizeWnd = { screenWidth, screenHeight };
    BLENDFUNCTION blend = {
        AC_SRC_OVER,    // BlendOp
        0,              // BlendFlags
        255,            // 完全不透明
        AC_SRC_ALPHA    // 使用Alpha通道
    };

    // 更新分层窗口
    POINT ptDst = { 0, 0 };
    ::UpdateLayeredWindow(
        hwnd,
        hdcScreen,
        &ptDst,
        &sizeWnd,
        hdcMem,
        &ptSrc,
        0,
        &blend,
        ULW_ALPHA
    );

    // 清理资源
    SelectObject(hdcMem, hOldBitmap);
    DeleteObject(hBitmap);
    DeleteDC(hdcMem);
    ReleaseDC(NULL, hdcScreen);
}

bool DesktopPet::IsPointOnPet(POINT pt) {
    if (!frames[state][frameIndex]) return false;

    // 计算宠物尺寸
    int petWidth = frames[state][frameIndex]->GetWidth() * scaleFactor;
    int petHeight = frames[state][frameIndex]->GetHeight() * scaleFactor;

    // 计算宠物矩形区域
    RECT petRect = {
        positionX,
        positionY,
        positionX + petWidth,
        positionY + petHeight
    };

    // 检查点是否在宠物区域内
    return PtInRect(&petRect, pt);
}

SIZE DesktopPet::GetPetSize() const {
    SIZE size = { 0, 0 };

    if (frames[state][frameIndex]) {
        size.cx = frames[state][frameIndex]->GetWidth() * scaleFactor;
        size.cy = frames[state][frameIndex]->GetHeight() * scaleFactor;
    }
    else {
        size.cx = 32 * scaleFactor;
        size.cy = 32 * scaleFactor;
    }

    return size;
}

void DesktopPet::HandleMouseEvents(UINT uMsg, LPARAM lParam) {
    POINT pt = { GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam) };

    // 只处理宠物区域的事件
    if (!IsPointOnPet(pt)) {
        if (hoverTimerId) {
            KillTimer(hwnd, hoverTimerId);
            hoverTimerId = 0;
        }
        return;
    }

    switch (uMsg) {
    case WM_LBUTTONDOWN: {
        // 重置拖动状态
        isDragStarting = true;
        isDragging = false;

        // 记录拖动起始点
        dragPoint = pt;
        lastDragPoint = pt;

        // 记录开始时间
        dragStartTime = GetTickCount64();

        // 捕获鼠标
        SetCapture(hwnd);
        break;
    }

    case WM_LBUTTONUP: {
        // 释放鼠标捕获
        ReleaseCapture();

        if (isDragging) {
            // 结束拖动
            isDragging = false;

            // 恢复之前的暂停状态
            isPaused = wasPausedBeforeDrag;
        }
        else if (isDragStarting) {
            // 检查是否为单击
            DWORD elapsed = GetTickCount64() - dragStartTime;

            if (elapsed < 500) {
                // 左键单击：暂停/恢复动画
                isPaused = !isPaused;
            }
        }

        // 重置拖动状态
        isDragStarting = false;
        break;
    }

    case WM_LBUTTONDBLCLK:
        // 左键双击：打开备忘录
        OpenMemo();
        break;

    case WM_RBUTTONDOWN: {
        // 右键点击：显示上下文菜单
        POINT screenPt = pt;
        ClientToScreen(hwnd, &screenPt);
        ShowContextMenu(screenPt);
        break;
    }

    case WM_MOUSEMOVE: {
        if (isDragStarting) {
            // 检查是否应该开始拖动
            int dx = abs(pt.x - dragPoint.x);
            int dy = abs(pt.y - dragPoint.y);

            if (dx > 5 || dy > 5) {
                // 开始正式拖动
                isDragging = true;
                isDragStarting = false;

                // 保存拖动前的暂停状态
                wasPausedBeforeDrag = isPaused;

                // 暂停动画
                isPaused = true;

                // 更新最后拖动点
                lastDragPoint = pt;
            }
        }

        if (isDragging) {
            // 计算移动距离（相对于上一次位置）
            int dx = pt.x - lastDragPoint.x;
            int dy = pt.y - lastDragPoint.y;

            // 更新宠物位置
            positionX += dx;
            positionY += dy;

            // 获取宠物尺寸
            SIZE petSize = GetPetSize();

            // 边界检查
            positionX = max(0, min(positionX, screenWidth - petSize.cx));
            positionY = max(0, min(positionY, screenHeight - petSize.cy));

            // 更新最后拖动点
            lastDragPoint = pt;

            // 触发重绘
            InvalidateRect(hwnd, NULL, TRUE);
        }
        else if (!isDragging && !isDragStarting) {
            // 启动悬停定时器
            if (!hoverTimerId) {
                hoverTimerId = SetTimer(hwnd, 3, 2000, NULL);
            }
        }
        break;
    }
    }
}

LRESULT CALLBACK DesktopPet::WindowProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam) {
    DesktopPet* pThis = nullptr;

    if (uMsg == WM_NCCREATE) {
        CREATESTRUCT* pCreate = (CREATESTRUCT*)lParam;
        pThis = (DesktopPet*)pCreate->lpCreateParams;
        SetWindowLongPtr(hwnd, GWLP_USERDATA, (LONG_PTR)pThis);
    }
    else {
        pThis = (DesktopPet*)GetWindowLongPtr(hwnd, GWLP_USERDATA);
    }

    if (pThis) {
        switch (uMsg) {
            // 关键增强：拦截所有鼠标消息
        case WM_MOUSEACTIVATE:
            // 防止窗口激活（避免焦点问题）
            return MA_NOACTIVATE;

        case WM_SETCURSOR: {
            if (LOWORD(lParam) == HTCLIENT) {
                // 在宠物区域设置光标
                if (pThis->isDragging || pThis->isDragStarting) {
                    // 拖动时显示移动光标
                    SetCursor(LoadCursor(NULL, IDC_SIZEALL));
                }
                else {
                    // 正常状态显示箭头光标
                    SetCursor(LoadCursor(NULL, IDC_ARROW));
                }
                return TRUE;
            }
            break;
        }

        case WM_NCHITTEST: {
            // 获取鼠标位置（屏幕坐标）
            POINT pt = { GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam) };

            // 检查点是否在宠物上
            if (pThis->IsPointOnPet(pt)) {
                return HTCLIENT; // 点在宠物上，返回客户区
            }
            return HTTRANSPARENT; // 点不在宠物上，穿透消息
        }

        case WM_TIMER: {
            if (wParam == 1) { // 动画定时器
                if (!pThis->isPaused) {
                    pThis->frameIndex++;
                    int frameCount = (pThis->state == IDLE) ? 5 : 6;
                    pThis->frameIndex %= frameCount;

                    if (pThis->state == WALK) {
                        pThis->MovePet();
                    }
                    // 触发重绘
                    InvalidateRect(hwnd, NULL, TRUE);
                }
            }
            else if (wParam == 2) { // 状态切换定时器
                pThis->RandomStateChange();
                SetTimer(hwnd, 2, (2000 + rand() % 2500), NULL); // 1.5-4秒
            }
            else if (wParam == 3) { // 悬停定时器
                POINT pt;
                GetCursorPos(&pt);

                if (pThis->IsPointOnPet(pt)) {
                    pThis->ShowSystemMonitor();
                }
                else {
                    if (pThis->monitorWindow) {
                        pThis->monitorWindow->Hide();
                        KillTimer(pThis->monitorWindow->hwnd, 1);
                    }
                }
                // 停止悬停定时器
                KillTimer(hwnd, pThis->hoverTimerId);
                pThis->hoverTimerId = 0;
            }
            break;
        }
        case WM_PAINT: {
            PAINTSTRUCT ps;
            BeginPaint(hwnd, &ps);
            pThis->UpdateLayeredWindow();
            EndPaint(hwnd, &ps);
            return 0;
        }
        case WM_LBUTTONDOWN:
        case WM_LBUTTONUP:
        case WM_LBUTTONDBLCLK:
        case WM_RBUTTONDOWN:
        case WM_MOUSEMOVE:
            pThis->HandleMouseEvents(uMsg, lParam);
            return 0;

        case WM_COMMAND: {
            int cmd = LOWORD(wParam);
            switch (cmd) {
            case IDM_MEMO:
                pThis->OpenMemo();
                break;
            case IDM_STATE_IDLE:
                pThis->state = IDLE;
                pThis->frameIndex = 0;
                InvalidateRect(hwnd, NULL, TRUE);
                break;
            case IDM_STATE_WALK:
                pThis->state = WALK;
                pThis->frameIndex = 0;
                InvalidateRect(hwnd, NULL, TRUE);
                break;
            case IDM_SIZE_INC:
                if (pThis->scaleFactor < 8) {
                    pThis->scaleFactor++;
                    pThis->UpdateSize();
                }
                break;
            case IDM_SIZE_DEC:
                if (pThis->scaleFactor > 1) {
                    pThis->scaleFactor--;
                    pThis->UpdateSize();
                }
                break;
            case IDM_SPEED_INC:
                if (pThis->fps < 30) {
                    pThis->fps += 5;
                    KillTimer(hwnd, pThis->animationTimerId);
                    pThis->animationTimerId = SetTimer(hwnd, 1, 1000 / pThis->fps, NULL);
                }
                break;
            case IDM_SPEED_DEC:
                if (pThis->fps > 6) {
                    pThis->fps -= 5;
                    KillTimer(hwnd, pThis->animationTimerId);
                    pThis->animationTimerId = SetTimer(hwnd, 1, 1000 / pThis->fps, NULL);
                }
                break;
            case IDM_EXIT:
                PostQuitMessage(0);
                break;
            }
            break;
        }
        case WM_DESTROY:
            pThis->Cleanup();
            PostQuitMessage(0);
            break;
        default:
            // 处理 WM_CONTEXTMENU 消息
            if (uMsg == WM_CONTEXTMENU) {
                // 完全拦截右键消息
                POINT pt = { GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam) };
                if (pThis->IsPointOnPet(pt)) {
                    pThis->ShowContextMenu(pt);
                    return 0; // 已处理，阻止默认行为
                }
            }
            return DefWindowProc(hwnd, uMsg, wParam, lParam);
        }
        return 0;
    }
    return DefWindowProc(hwnd, uMsg, wParam, lParam);
}