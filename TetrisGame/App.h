#pragma once
#include <windows.h>
#include <vector>
#include <cstdlib>
#include "Renderer.h"
#include "Board.h"
#include "Block.h"
#include "CommonDefs.h"

class CApp
{
public:
    void Initialize(HWND hWnd);
    void OnDraw(HDC hdc);
    void Update();
    void OnKeyDown(WPARAM key);

private:
    void Redraw();
    void SpawnBlock();
    void LockBlock();
    bool TryMove(int dx, int dy);
    bool TryRotate();
    void HardDrop();
    void HoldBlock();
    void UpdateSpeed();

    void DrawGameOver(HDC hdc);
    void DrawSidePanel(HDC hdc);
    void DrawBlockPreview(HDC hdc, int type, int x, int y, int nCellSize);

    int  NextBlockType();
    void SetBlockQueue();

    void DrawTitle(HDC hdc);
    void OnKeyTitle();

    void DrawSinglePlay(HDC hdc, HDC mem);

    void OnPlay();

private:
    HWND m_hWnd = nullptr;
    CRenderer m_renderer;
    CBoard m_board;
    CBlock m_curBlock;
    CBlock m_nextBlock;

    // 랜덤 중복 방지를 위한 블럭 한세트
    std::vector<int> m_queue;

    bool m_bGameOver = false;
    int m_nLevel = 1;
    int m_nLines = 0;
    int m_nScore = 0;

    int m_nHoldType = 0;
    bool m_bHoldUsed = false;

    // 게임 상태
    eGameState m_eState = STATE_TITLE;
};
