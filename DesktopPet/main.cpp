#include "DesktopPet.h"

int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCmdShow) {
    // ≥ı ºªØGDI+
    Gdiplus::GdiplusStartupInput gdiplusStartupInput;
    ULONG_PTR gdiplusToken;
    Gdiplus::GdiplusStartup(&gdiplusToken, &gdiplusStartupInput, NULL);

    DesktopPet pet(hInstance);
    if (pet.Initialize()) {
        pet.Run();
    }

    Gdiplus::GdiplusShutdown(gdiplusToken);
    return 0;
}