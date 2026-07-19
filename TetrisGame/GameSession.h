#pragma once
#include "Board.h"
#include "Block.h"
#include "CommonDefs.h"
#include <vector>
#include <cstdint>

class CGameSession
{
public:
    void Start(uint32_t dwSeed);
    void ProcessTick(EGameInput eInput);

    // 상태 조회 (렌더링용)
    CBoard& GetBoard() { return m_board; }
    CBlock& GetCurBlock() { return m_curBlock; }
    CBlock& GetNextBlock() { return m_nextBlock; }
    int  GetHoldType() { return m_nHoldType; }
    bool IsHoldUsed() { return m_bHoldUsed; }
    int  GetScore() { return m_nScore; }
    int  GetLevel() { return m_nLevel; }
    int  GetLines() { return m_nLines; }
    bool IsGameOver() { return m_bGameOver; }
    int  GetTick() { return m_nTick; }

private:
    // 랜덤 숫자용 시드
    uint32_t m_dwSeed = 0;
    int  NextRand();
    int  NextBlockType();
    void SetBlockQueue();

    // 게임 로직
    void SpawnBlock();
    void LockBlock();
    bool TryMove(int dx, int dy);
    bool TryRotate();
    void HardDrop();
    void HoldBlock();
    void TickDrop();

private:
    CBoard m_board;
    CBlock m_curBlock;
    CBlock m_nextBlock;
    std::vector<int> m_queue;

    bool m_bGameOver = false;
    int  m_nLevel = 1;
    int  m_nTick = 0;
    int  m_nLines = 0;
    int  m_nScore = 0;
    int  m_nHoldType = 0;
    bool m_bHoldUsed = false;
    int  m_nDropCounter = 0;
};
