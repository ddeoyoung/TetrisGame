#include "Renderer.h"

CRenderer::CRenderer()
    : m_hMemDC(nullptr), m_hBmp(nullptr), m_hOldBmp(nullptr), m_w(0), m_h(0)
{
}

CRenderer::~CRenderer() { Cleanup(); }

void CRenderer::Initialize(HDC hdc, int w, int h)
{
    Cleanup();

    m_w = w;
    m_h = h;
    m_hMemDC = CreateCompatibleDC(hdc);
    m_hBmp = CreateCompatibleBitmap(hdc, w, h);
    m_hOldBmp = (HBITMAP)SelectObject(m_hMemDC, m_hBmp);
}

void CRenderer::Cleanup()
{
    if (m_hMemDC) {
        if (m_hOldBmp) SelectObject(m_hMemDC, m_hOldBmp);
        DeleteDC(m_hMemDC);
        m_hMemDC = nullptr;
        m_hOldBmp = nullptr;
    }


    if (m_hBmp) {
        DeleteObject(m_hBmp);
        m_hBmp = nullptr;
    }
}

HDC CRenderer::BeginDraw()
{
    RECT rc = { 0, 0, m_w, m_h };
    HBRUSH bg = CreateSolidBrush(RGB(40, 40, 40));

    FillRect(m_hMemDC, &rc, bg);
    DeleteObject(bg);

    return m_hMemDC;
}

// 더블 버퍼링
void CRenderer::EndDraw(HDC hdc)
{
    BitBlt(hdc, 0, 0, m_w, m_h, m_hMemDC, 0, 0, SRCCOPY);

}
