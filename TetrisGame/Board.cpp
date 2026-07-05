#include "Board.h"

// 블록 색상 (0=빈칸, 1~7=블록)
static COLORREF s_colors[] = {
    RGB(40, 40, 40),
    RGB(145, 220, 230),  // I
    RGB(245, 230, 155),  // O
    RGB(190, 160, 220),  // T
    RGB(160, 215, 165),  // S
    RGB(235, 155, 155),  // Z
    RGB(150, 170, 225),  // J
    RGB(240, 195, 150),  // L
};


CBoard::CBoard()
{
    Clear();
}

void CBoard::Clear()
{
    for (int r = 0; r < BOARD_H; r++)
        for (int c = 0; c < BOARD_W; c++)
            m_cells[r][c] = 0;
}

int CBoard::GetCell(int row, int col)
{
    if (!IsInside(row, col)) return -1;
    return m_cells[row][col];
}

void CBoard::SetCell(int row, int col, int val)
{
    if (IsInside(row, col))
        m_cells[row][col] = val;
}

bool CBoard::IsInside(int row, int col)
{
    return row >= 0 && row < BOARD_H && col >= 0 && col < BOARD_W;
}


bool CBoard::IsEmpty(int row, int col)
{
    if (!IsInside(row, col)) return false;
    return m_cells[row][col] == 0;
}

int CBoard::ClearFullLines()
{
    int nCleared = 0;

    for (int r = BOARD_H - 1; r >= 0; r--) {
        bool bFull = true;
        for (int c = 0; c < BOARD_W; c++) {
            if (m_cells[r][c] == 0) { bFull = false; break; }
        }

        if (bFull) {
            nCleared++;
            for (int rr = r; rr > 0; rr--) {
                for (int c = 0; c < BOARD_W; c++)
                    m_cells[rr][c] = m_cells[rr - 1][c];
            }
            for (int c = 0; c < BOARD_W; c++)
                m_cells[0][c] = 0;

            r++;  // 같은 줄 다시 검사
        }
    }
    return nCleared;
}


static void DrawCellBorder(HDC hdc, int x, int y)
{
    // 밝은쪽
    HPEN penLight = CreatePen(PS_SOLID, 1, RGB(255, 255, 255));
    HPEN old = (HPEN)SelectObject(hdc, penLight);

    MoveToEx(hdc, x, y + CELL_SIZE - 1, nullptr);
    LineTo(hdc, x, y);
    LineTo(hdc, x + CELL_SIZE - 1, y);

    SelectObject(hdc, old);
    DeleteObject(penLight);

    // 어두운쪽
    HPEN penDark = CreatePen(PS_SOLID, 1, RGB(30, 30, 30));
    old = (HPEN)SelectObject(hdc, penDark);

    MoveToEx(hdc, x + CELL_SIZE - 1, y, nullptr);
    LineTo(hdc, x + CELL_SIZE - 1, y + CELL_SIZE - 1);
    LineTo(hdc, x, y + CELL_SIZE - 1);

    SelectObject(hdc, old);
    DeleteObject(penDark);
}


void CBoard::Draw(HDC hdc, int nOffX, int nOffY)
{
    for (int r = 0; r < BOARD_H; r++) {
        for (int c = 0; c < BOARD_W; c++) {
            int x = nOffX + c * CELL_SIZE;
            int y = nOffY + r * CELL_SIZE;

            RECT rc = { x, y, x + CELL_SIZE, y + CELL_SIZE };

            HBRUSH br = CreateSolidBrush(s_colors[m_cells[r][c]]);
            FillRect(hdc, &rc, br);
            DeleteObject(br);

            if (m_cells[r][c] > 0) {
                DrawCellBorder(hdc, x, y);
            }
            else {
                HPEN pen = CreatePen(PS_SOLID, 1, RGB(60, 60, 60));
                HPEN old = (HPEN)SelectObject(hdc, pen);
                SelectObject(hdc, GetStockObject(NULL_BRUSH));
                Rectangle(hdc, x, y, x + CELL_SIZE, y + CELL_SIZE);
                SelectObject(hdc, old);
                DeleteObject(pen);
            }
        }
    }

    // 보드 테두리
    HPEN pen = CreatePen(PS_SOLID, 2, RGB(140, 140, 140));
    HPEN old = (HPEN)SelectObject(hdc, pen);
    SelectObject(hdc, GetStockObject(NULL_BRUSH));
    Rectangle(hdc, nOffX - 1, nOffY - 1,
        nOffX + BOARD_W * CELL_SIZE + 1, nOffY + BOARD_H * CELL_SIZE + 1);
    SelectObject(hdc, old);
    DeleteObject(pen);
}


bool CBoard::IsPlacable(CBlock& block)
{
    CellPos cells[4];
    block.GetCells(cells);

    for (int i = 0; i < 4; i++) {
        int r = cells[i].row;
        int c = cells[i].col;

        if (c < 0 || c >= BOARD_W || r >= BOARD_H)
            return false;
        if (r < 0) continue; 
        if (m_cells[r][c] != 0)
            return false;
    } 
    return true;
}

void CBoard::PlaceBlock(CBlock& block)
{
    CellPos cells[4];
    block.GetCells(cells);

    for (int i = 0; i < 4; i++) {
        int r = cells[i].row;
        int c = cells[i].col;
        if (IsInside(r, c))
            m_cells[r][c] = block.GetType();
    }
}

void CBoard::DrawBlock(HDC hdc, CBlock& block, int nOffX, int nOffY)
{
    CellPos cells[4];
    block.GetCells(cells);

    COLORREF clr = s_colors[block.GetType()];

    for (int i = 0; i < 4; i++) {
        int r = cells[i].row;
        int c = cells[i].col;

        if (r < 0) continue;

        int x = nOffX + c * CELL_SIZE;
        int y = nOffY + r * CELL_SIZE;

        RECT rc = { x, y, x + CELL_SIZE, y + CELL_SIZE };
        HBRUSH br = CreateSolidBrush(clr);

        FillRect(hdc, &rc, br);
        DeleteObject(br);

        DrawCellBorder(hdc, x, y);
    }
}

int CBoard::CalcGhostY(CBlock& block)
{
    CBlock ghost = block;

    while (true) {
        ghost.Move(0, 1);

        if (!IsPlacable(ghost)) {
            ghost.Move(0, -1);
            break;
        }
    }

    return ghost.GetY();
}

// 미리보기
void CBoard::DrawGhost(HDC hdc, CBlock& block, int nOffX, int nOffY)
{
    
    int nGhostY = CalcGhostY(block);
    if (nGhostY == block.GetY()) return;

    CBlock ghost = block;
    ghost.SetPos(block.GetX(), nGhostY);

    CellPos cells[4];
    ghost.GetCells(cells);

    for (int i = 0; i < 4; i++) {
        int r = cells[i].row;
        int c = cells[i].col;

        if (r < 0) continue;

        int x = nOffX + c * CELL_SIZE;
        int y = nOffY + r * CELL_SIZE;

        HPEN pen = CreatePen(PS_SOLID, 1, RGB(120, 120, 120));
        HPEN old = (HPEN)SelectObject(hdc, pen);

        SelectObject(hdc, GetStockObject(NULL_BRUSH));
        Rectangle(hdc, x + 1, y + 1, x + CELL_SIZE - 1, y + CELL_SIZE - 1);
        SelectObject(hdc, old);
        DeleteObject(pen);
    }
}
