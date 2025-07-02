#pragma once

#include <Windows.h>
#include <vector>
#include <string>
#include "JsonHandler.h"

class MemoWindow {
public:
    MemoWindow(HINSTANCE hInstance, JsonHandler& jsonHandler);

    ~MemoWindow();

    bool Create();
    void Show();
    void Hide();
    void Close();  // 添加关闭方法
    void UpdateMemoList();
    LRESULT HandleMessage(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam);

private:
    void AddMemo();
    void DeleteMemo(int index);
    std::string FormatTimestamp(time_t timestamp);

    HINSTANCE hInstance;
    HWND hwnd;
    HWND hEdit;
    HWND hList;
    JsonHandler& jsonHandler;
    POINT dragPoint;
    bool dragging;
};