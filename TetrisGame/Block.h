#pragma once

enum BlockType {
    BLOCK_I = 1,
    BLOCK_O,
    BLOCK_T,
    BLOCK_S,
    BLOCK_Z,
    BLOCK_J,
    BLOCK_L,

    BLOCK_COUNT = 7
};

struct CellPos {
    int row, col;
};

class CBlock
{
public:
    CBlock();
    void Init(int type);

    int  GetType()     { return m_type; }
    int  GetX()        { return m_x; }
    int  GetY()        { return m_y; }
    int  GetRotation() { return m_rot; }

    void SetPos(int x, int y) { m_x = x; m_y = y; }
    void Move(int dx, int dy) { m_x += dx; m_y += dy; }

    void RotateClockWise();
    void RotateReverseClockWise();

    void GetCells(CellPos out[4]);       // 보드 좌표
    void GetLocalCells(CellPos out[4]);  // 로컬 좌표 (프리뷰용)

private:
    int m_type;
    int m_rot;
    int m_x, m_y;
};
