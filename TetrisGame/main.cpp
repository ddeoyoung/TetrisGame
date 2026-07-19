#include "App.h"

CApp g_app;

LRESULT CALLBACK WndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
    switch (msg) {
    case WM_CREATE:
        g_app.Initialize(hWnd);
        return 0;

    case WM_PAINT: {
        PAINTSTRUCT ps;
        HDC hdc = BeginPaint(hWnd, &ps);
        g_app.OnDraw(hdc);
        EndPaint(hWnd, &ps);
        return 0;
    }

    case WM_TIMER:
        g_app.Update();
        return 0;

    case WM_KEYDOWN:
        g_app.OnKeyDown(wParam);
        return 0;


    case WM_CHAR:
        g_app.OnChar((wchar_t)wParam);
        return 0;

    case WM_ERASEBKGND:
        return 1;

    case WM_DESTROY:
        KillTimer(hWnd, 1);
        PostQuitMessage(0);
        return 0;
    }

    return DefWindowProc(hWnd, msg, wParam, lParam);
}

int WINAPI wWinMain(HINSTANCE hInst, HINSTANCE, LPWSTR, int nCmdShow)
{
    WNDCLASS wc = {};
    wc.lpfnWndProc = WndProc;
    wc.hInstance = hInst;
    wc.lpszClassName = L"TetrisGame";

    wc.hCursor = LoadCursor(nullptr, IDC_ARROW);
    RegisterClass(&wc);

    int clientW = 20 + BOARD_W * CELL_SIZE + 20 + 130 + 10;
    int clientH = 20 + BOARD_H * CELL_SIZE + 20;

    DWORD style = WS_OVERLAPPED | WS_CAPTION | WS_SYSMENU | WS_MINIMIZEBOX;

    RECT rc = { 0, 0, clientW, clientH };
    AdjustWindowRect(&rc, style, FALSE);

    int winW = rc.right - rc.left;
    int winH = rc.bottom - rc.top;

    HWND hWnd = CreateWindowEx(0, L"TetrisGame", L"Tetris", style,
        CW_USEDEFAULT, CW_USEDEFAULT, winW, winH,
        nullptr, nullptr, hInst, nullptr);

    if (!hWnd) return 1;

    ShowWindow(hWnd, nCmdShow);
    UpdateWindow(hWnd);

    MSG msg;
    while (GetMessage(&msg, nullptr, 0, 0)) {
        TranslateMessage(&msg);
        DispatchMessage(&msg);
    }

    return (int)msg.wParam;
}
