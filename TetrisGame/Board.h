#pragma once
#include <windows.h>
#include "Block.h"

constexpr int BOARD_W = 10;
constexpr int BOARD_H = 20;
constexpr int CELL_SIZE = 28;


class CBoard
{
public:
    CBoard();

    void Clear();
    int  GetCell(int row, int col);
    void SetCell(int row, int col, int val);
    bool IsInside(int row, int col);
    bool IsEmpty(int row, int col); 
     
    int  ClearFullLines();

    void Draw(HDC hdc, int nOffX, int nOffY);

    void DrawBlock(HDC hdc, CBlock& block, int nOffX, int nOffY);
    void DrawGhost(HDC hdc, CBlock& block, int nOffX, int nOffY);

    int  CalcGhostY(CBlock& block);

    bool IsPlacable(CBlock& block);
    void PlaceBlock(CBlock& block);



private:
    int m_cells[BOARD_H][BOARD_W];
};
