#pragma once

#include <Windows.h>
#include <gdiplus.h>
#include <vector>
#include <string>
#include "MemoWindow.h"
#include "MonitorWindow.h"
#include "JsonHandler.h"

#define WIN32_LEAN_AND_MEAN
#define NOMINMAX

using namespace Gdiplus;

enum PetState { IDLE, WALK };
enum Direction { LEFT = -1, RIGHT = 1 };

class DesktopPet {
public:
    DesktopPet(HINSTANCE hInstance);
    ~DesktopPet();

    bool Initialize();
    void Run();

private:
    void LoadFrames();
    void UpdateSize();
    void MovePet();
    void RandomStateChange();
    void ShowContextMenu(POINT pt);
    void OpenMemo();
    void ShowSystemMonitor();
    void Cleanup();
    void UpdateLayeredWindow();
    bool IsPointOnPet(POINT pt);
    SIZE GetPetSize() const;
    void InitializeWindow();
    void HandleMouseEvents(UINT uMsg, LPARAM lParam);

    static LRESULT CALLBACK WindowProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam);

    HINSTANCE hInstance;
    HWND hwnd;

    // GDI+资源
    Gdiplus::Image* frames[2][6] = { {nullptr} }; // [state][frame]

    // 宠物状态
    PetState state;
    Direction direction;
    int frameIndex;
    int scaleFactor;
    int fps;
    int speed;
    bool isPaused;
    bool isDragging;
    POINT dragPoint;
    int positionY;
    int positionX;
    bool firstMove;

    // 子窗口
    MemoWindow* memoWindow;
    MonitorWindow* monitorWindow;

    // JSON处理器
    JsonHandler jsonHandler;

    // 定时器ID
    UINT_PTR animationTimerId;
    UINT_PTR stateTimerId;
    UINT_PTR hoverTimerId;

    // 屏幕尺寸
    int screenWidth;
    int screenHeight;
    ULONG_PTR gdiplusToken;  // GDI+ 令牌

    bool isDragStarting; // 标记拖动是否正在开始
    POINT dragStartPoint; // 拖动起始点（屏幕坐标）
    POINT lastDragPoint;
    DWORD dragStartTime; // 拖动开始时间
    bool wasPausedBeforeDrag; // 拖动前是否暂停
};