#pragma once
#include <windows.h>

// GDI 더블 버퍼링
class CRenderer
{
public:
    CRenderer();
    ~CRenderer();

    void Initialize(HDC hdc, int w, int h);
    void Cleanup();

    HDC  BeginDraw();
    void EndDraw(HDC hdc);

private:
    HDC     m_hMemDC;
    HBITMAP m_hBmp;
    HBITMAP m_hOldBmp;
    int     m_w, m_h;
};
