#include "App.h"
#include <ctime>

constexpr int BOARD_X = 20;
constexpr int BOARD_Y = 20;

static int GetDropInterval(int nLevel)
{
    int ms = 1000 - (nLevel - 1) * 80;
    if (ms < 100) ms = 100;
    return ms;
}

static COLORREF s_previewColors[] = {
    RGB(40, 40, 40),
    RGB(145, 220, 230),  RGB(245, 230, 155),
    RGB(190, 160, 220),  RGB(160, 215, 165),
    RGB(235, 155, 155),  RGB(150, 170, 225),
    RGB(240, 195, 150),
};


void CApp::Initialize(HWND h)
{
    m_hWnd = h;
    m_bGameOver = false;
    m_nLevel = 1;
    m_nLines = 0;
    m_nScore = 0;
    m_nHoldType = 0;
    m_bHoldUsed = false;

    srand(time(nullptr));

    m_queue.clear();

    RECT rc;
    GetClientRect(m_hWnd, &rc);

    HDC hdc = GetDC(m_hWnd);

    m_renderer.Initialize(hdc, rc.right, rc.bottom);
    ReleaseDC(m_hWnd, hdc);

    m_board.Clear();

    m_curBlock.Init(NextBlockType());
    m_nextBlock.Init(NextBlockType());

    SetTimer(m_hWnd, 1, GetDropInterval(m_nLevel), nullptr);

}

void CApp::OnDraw(HDC hdc)
{
    HDC mem = m_renderer.BeginDraw();

    m_board.Draw(mem, BOARD_X, BOARD_Y);

    if (!m_bGameOver) {
        m_board.DrawGhost(mem, m_curBlock, BOARD_X, BOARD_Y);
        m_board.DrawBlock(mem, m_curBlock, BOARD_X, BOARD_Y);
    }

    DrawSidePanel(mem);

    if (m_bGameOver)
        DrawGameOver(mem);

    m_renderer.EndDraw(hdc);

}

void CApp::Update()
{
    if (m_bGameOver) return;

    if (!TryMove(0, 1))
        LockBlock();

    Redraw();
}

void CApp::OnKeyDown(WPARAM key)
{
    // 게임오버
    if (m_bGameOver) {
        // 재시작
        if (key == 'R') {

            m_board.Clear();
            m_bGameOver = false;
            m_nLevel = 1;
            m_nLines = 0;
            m_nScore = 0;
            m_nHoldType = 0;
            m_bHoldUsed = false;
            m_queue.clear();

            m_curBlock.Init(NextBlockType());
            m_nextBlock.Init(NextBlockType());

            SetTimer(m_hWnd, 1, GetDropInterval(m_nLevel), nullptr);
            Redraw();
        }

        return;
    }

    switch (key) {
    case VK_LEFT:
        TryMove(-1, 0);
        break;
    case VK_RIGHT:
        TryMove(1, 0);
        break;
    case VK_DOWN:
        if (TryMove(0, 1))
            m_nScore += 1;
        break;
    case VK_UP:
        TryRotate();
        break;
    case VK_SPACE:
        HardDrop(); 
        break;
    case 'H':
        HoldBlock();
        break;
    default: 
        return;
    }

    Redraw();
}


void CApp::SpawnBlock()
{
    m_curBlock = m_nextBlock;
    m_curBlock.Init(m_curBlock.GetType());
    m_nextBlock.Init(NextBlockType());

    if (!m_board.IsPlacable(m_curBlock)) {
        m_bGameOver = true;
        KillTimer(m_hWnd, 1);
    }
}

void CApp::LockBlock()
{
    m_board.PlaceBlock(m_curBlock);

    int nLines = m_board.ClearFullLines();
    if (nLines > 0) {
        static const int lineScore[] = { 0, 100, 300, 500, 800 };
        m_nScore += lineScore[nLines] * m_nLevel;
        m_nLines += nLines;

        int nNewLevel = m_nLines / 10 + 1;
        if (nNewLevel != m_nLevel) {
            m_nLevel = nNewLevel;
            UpdateSpeed();
        }
    }

    m_bHoldUsed = false;
    SpawnBlock();
}


bool CApp::TryMove(int dx, int dy)
{
    m_curBlock.Move(dx, dy);
    if (m_board.IsPlacable(m_curBlock))
        return true;

    m_curBlock.Move(-dx, -dy);
    return false;
}

bool CApp::TryRotate()
{
    // 일단 시계방향 회전
    // 그 자리에 놓을 수 있으면 true
    m_curBlock.RotateClockWise();
    if (m_board.IsPlacable(m_curBlock))
        return true;

    // 회전 불가능하면 되돌리기 (반시계) false 
    m_curBlock.RotateReverseClockWise();
    return false;
}

void CApp::HardDrop()
{
    int nDropped = 0;
    while (TryMove(0, 1))
        nDropped++;

    m_nScore += nDropped * 2;
    LockBlock();
}

void CApp::HoldBlock()
{
    if (m_bHoldUsed) return;

    m_bHoldUsed = true;
    int nCurType = m_curBlock.GetType();

    if (m_nHoldType == 0) {
        m_nHoldType = nCurType;
        SpawnBlock();
    }
    else {
        int nTmp = m_nHoldType;
        m_nHoldType = nCurType;
        m_curBlock.Init(nTmp);
    }
}

// 1~7번이 한 세트로 랜덤 중복 방지를 위함
// 골고루 나올 수 있도록
void CApp::SetBlockQueue()
{
    m_queue.clear();
    for (int i = 1; i <= BLOCK_COUNT; i++)
        m_queue.push_back(i);

    for (int i = (int)m_queue.size() - 1; i > 0; i--) {
        int j = rand() % (i + 1);
        int nTmp = m_queue[i];
        m_queue[i] = m_queue[j];
        m_queue[j] = nTmp;
    }
}

int CApp::NextBlockType()
{
    if (m_queue.empty())
        SetBlockQueue();

    int nType = m_queue.back();
    m_queue.pop_back();
    return nType;
}

void CApp::UpdateSpeed()
{
    KillTimer(m_hWnd, 1);
    SetTimer(m_hWnd, 1, GetDropInterval(m_nLevel), nullptr);
}

void CApp::DrawBlockPreview(HDC hdc, int type, int x, int y, int nCellSize)
{
    CBlock tmp;
    tmp.Init(type);

    CellPos cells[4];
    tmp.GetLocalCells(cells);

    COLORREF clr = s_previewColors[type];

    for (int i = 0; i < 4; i++) {
        int cx = x + cells[i].col * nCellSize;
        int cy = y + cells[i].row * nCellSize;

        RECT rc = { cx, cy, cx + nCellSize, cy + nCellSize };
        HBRUSH br = CreateSolidBrush(clr);
        FillRect(hdc, &rc, br);
        DeleteObject(br);

        HPEN pen = CreatePen(PS_SOLID, 1, RGB(20, 20, 20));
        HPEN old = (HPEN)SelectObject(hdc, pen);
        SelectObject(hdc, GetStockObject(NULL_BRUSH));
        Rectangle(hdc, cx, cy, cx + nCellSize, cy + nCellSize);
        SelectObject(hdc, old);
        DeleteObject(pen);
    }
}


void CApp::DrawSidePanel(HDC hdc)
{
    int sx = BOARD_X + BOARD_W * CELL_SIZE + 20;
    int sy = BOARD_Y;

    SetBkMode(hdc, TRANSPARENT);

    LPCWSTR font1 = L"Small Fonts"; // 도트 느낌 폰트
    LPCWSTR font2 = L"Verdana"; // 작은 크기에서도 잘보임
    LPCWSTR font3 = L"Arial";

    HFONT fontTitle = CreateFont(20, 0, 0, 0, FW_BOLD,
        FALSE, FALSE, FALSE, DEFAULT_CHARSET, 0, 0, 0, 0, font1);
    HFONT fontVal = CreateFont(28, 0, 0, 0, FW_BOLD,
        FALSE, FALSE, FALSE, DEFAULT_CHARSET, 0, 0, 0, 0, font1);
    HFONT fontSmall = CreateFont(14, 0, 0, 0, FW_NORMAL,
        FALSE, FALSE, FALSE, DEFAULT_CHARSET, 0, 0, 0, 0, font1);

    HFONT oldFont = (HFONT)SelectObject(hdc, fontTitle);


    // Hold
    SetTextColor(hdc, m_bHoldUsed ? RGB(100, 100, 100) : RGB(180, 180, 180));
    TextOut(hdc, sx, sy, L"HOLD", 4);

    RECT holdBg = { sx, sy + 25, sx + 100, sy + 105 };
    HBRUSH bgBr = CreateSolidBrush(RGB(25, 25, 25));
    FillRect(hdc, &holdBg, bgBr);
    DeleteObject(bgBr);

    if (m_nHoldType > 0)
        DrawBlockPreview(hdc, m_nHoldType, sx + 10, sy + 35, 20);


    // Next
    SetTextColor(hdc, RGB(180, 180, 180));
    TextOut(hdc, sx, sy + 120, L"NEXT", 4);

    RECT nextBg = { sx, sy + 145, sx + 100, sy + 225 };
    bgBr = CreateSolidBrush(RGB(25, 25, 25));
    FillRect(hdc, &nextBg, bgBr);
    DeleteObject(bgBr);

    DrawBlockPreview(hdc, m_nextBlock.GetType(), sx + 10, sy + 155, 20);


    // Score / Level / Lines
    //SetTextColor(hdc, RGB(180, 180, 180)); 
    SetTextColor(hdc, RGB(245, 230, 155));
    TextOut(hdc, sx, sy + 250, L"SCORE", 5);

    SelectObject(hdc, fontVal);
    SetTextColor(hdc, RGB(255, 255, 255));
    wchar_t buf[32];
    wsprintf(buf, L"%d", m_nScore);
    TextOut(hdc, sx, sy + 275, buf, (int)wcslen(buf));

    SelectObject(hdc, fontTitle);
    SetTextColor(hdc, RGB(180, 180, 180));
    TextOut(hdc, sx, sy + 320, L"LEVEL", 5);

    SelectObject(hdc, fontVal);
    SetTextColor(hdc, RGB(255, 255, 255));
    wsprintf(buf, L"%d", m_nLevel);
    TextOut(hdc, sx, sy + 345, buf, (int)wcslen(buf));

    SelectObject(hdc, fontTitle);
    SetTextColor(hdc, RGB(180, 180, 180));
    TextOut(hdc, sx, sy + 390, L"LINES", 5);

    SelectObject(hdc, fontVal);
    SetTextColor(hdc, RGB(255, 255, 255));
    wsprintf(buf, L"%d", m_nLines);
    TextOut(hdc, sx, sy + 415, buf, (int)wcslen(buf));


    // 조작 키
    SelectObject(hdc, fontSmall);
    SetTextColor(hdc, RGB(100, 100, 100));

    int ky = sy + 470;

    TextOut(hdc, sx, ky,       L"<- ->  Move", 11); // 글자수 만큼
    TextOut(hdc, sx, ky + 18,  L"Up     Rotate", 13);
    TextOut(hdc, sx, ky + 36,  L"Down   Soft Drop", 16);
    TextOut(hdc, sx, ky + 54,  L"Space  Hard Drop", 16);
    TextOut(hdc, sx, ky + 72,  L"H      Hold", 11);
    TextOut(hdc, sx, ky + 90,  L"R      Restart", 14);

    SelectObject(hdc, oldFont);
    DeleteObject(fontTitle);
    DeleteObject(fontVal);
    DeleteObject(fontSmall);

}

void CApp::DrawGameOver(HDC hdc)
{
    int bx = BOARD_X;
    int by = BOARD_Y;
    int bw = BOARD_W * CELL_SIZE;
    int bh = BOARD_H * CELL_SIZE;

    // 보드 검정
    for (int r = 0; r < BOARD_H; r++) {
        for (int c = 0; c < BOARD_W; c++) {
            int x = bx + c * CELL_SIZE;
            int y = by + r * CELL_SIZE;
            RECT rc = { x, y, x + CELL_SIZE, y + CELL_SIZE };

            HBRUSH br = CreateSolidBrush(RGB(0, 0, 0));
            FillRect(hdc, &rc, br);
            DeleteObject(br);
        }
    }

    LPCWSTR fontName = L"Small Fonts";

    SetBkMode(hdc, TRANSPARENT);

    HFONT font = CreateFont(44, 0, 0, 0, FW_BOLD,
        FALSE, FALSE, FALSE, DEFAULT_CHARSET, 0, 0, 0, 0, fontName);
    HFONT oldFont = (HFONT)SelectObject(hdc, font);

    
    LPCWSTR sGameOver = L"GAME OVER";
    COLORREF letterColors[] = {
        RGB(235, 155, 155), // G 핑크
        RGB(240, 195, 150), // A 오렌지
        RGB(245, 230, 155), // M 노랑
        RGB(160, 215, 165), // E 초록
        RGB(200, 200, 200), // 
        RGB(145, 220, 230), // O 하늘
        RGB(150, 170, 225), // V 파랑
        RGB(190, 160, 220), // E 보라
        RGB(235, 155, 155), // R 핑크
    };

    // 전체 너비 계산
    SIZE totalSize;
    GetTextExtentPoint32(hdc, sGameOver, 9, &totalSize);
    int startX = bx + (bw - totalSize.cx) / 2;
    int textY = by + bh / 2 - 40;

    int cx = startX;
    for (int i = 0; i < 9; i++) {
        SetTextColor(hdc, letterColors[i]);
        SIZE charSize;

        GetTextExtentPoint32(hdc, &sGameOver[i], 1, &charSize);
        TextOut(hdc, cx, textY, &sGameOver[i], 1);

        cx += charSize.cx;
    }

    //SetTextColor(hdc, RGB(100, 100, 100));
    SetTextColor(hdc, RGB(200, 200, 200));

    HFONT font2 = CreateFont(20, 0, 0, 0, FW_NORMAL,
        FALSE, FALSE, FALSE, DEFAULT_CHARSET, 0, 0, 0, 0, fontName);
    SelectObject(hdc, font2);
    DeleteObject(font);

    RECT rcSub = { bx, by + bh / 2 + 15, bx + bw, by + bh / 2 + 45 };
    DrawText(hdc, L"Press R", -1, &rcSub, DT_CENTER | DT_VCENTER | DT_SINGLELINE);

    SelectObject(hdc, oldFont);
    DeleteObject(font2);
}

void CApp::Redraw()
{
    InvalidateRect(m_hWnd, nullptr, FALSE);
}
