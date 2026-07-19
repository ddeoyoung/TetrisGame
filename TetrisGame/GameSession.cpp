#include "GameSession.h"

// 랜덤 숫자 만들기. 시드로 계산해서 나오기 때문에
// 같은 시드로 시작하면 항상 같은 순서로 나온다.
int CGameSession::NextRand()
{
    m_dwSeed = m_dwSeed * 214013 + 2531011;
    return (m_dwSeed >> 16) & 0x7fff;
}

void CGameSession::Start(uint32_t dwSeed)
{
    m_dwSeed = dwSeed;

    m_bGameOver = false;
    m_nLevel = 1;
    m_nLines = 0;
    m_nScore = 0;
    m_nHoldType = 0;
    m_bHoldUsed = false;
    m_nTick = 0;
    m_nDropCounter = 0;
    m_queue.clear();

    m_board.Clear();
    m_curBlock.Init(NextBlockType());
    m_nextBlock.Init(NextBlockType());
}

void CGameSession::ProcessTick(EGameInput eInput)
{
    if (m_bGameOver) return;

    // 플레이어 입력 처리
    switch (eInput) {
    case INPUT_LEFT:
        TryMove(-1, 0);
        break;
    case INPUT_RIGHT:
        TryMove(1, 0);
        break;
    case INPUT_DOWN:
        if (TryMove(0, 1)) m_nScore += 1;
        break;
    case INPUT_ROTATE:
        TryRotate();
        break;
    case INPUT_HARD_DROP:
        HardDrop();
        break;
    case INPUT_HOLD:
        HoldBlock();
        break;
    case INPUT_NONE:
        break;
    }

    if (eInput != INPUT_HARD_DROP && !m_bGameOver) {
        TickDrop();
    }

    m_nTick++;
}

void CGameSession::TickDrop()
{
    // 레벨별로 떨어지는 속도 간격 (틱 단위)
    // 기본값 50ms
    // 레벨1 20틱(1초), 레벨2 18틱(0.9초) ...
    int nDropInterval = 20 - (m_nLevel - 1) * 2;
    if (nDropInterval < 2) nDropInterval = 2;

    m_nDropCounter++;
    if (m_nDropCounter >= nDropInterval) {
        m_nDropCounter = 0;
        if (!TryMove(0, 1))
            LockBlock();
    }
}

// 1~7번이 한 세트로 랜덤 중복 방지를 위함
// 골고루 나올 수 있도록
void CGameSession::SetBlockQueue()
{
    m_queue.clear();
    for (int i = 1; i <= BLOCK_COUNT; i++)
        m_queue.push_back(i);

    for (int i = (int)m_queue.size() - 1; i > 0; i--) {
        int j = NextRand() % (i + 1);
        int nTmp = m_queue[i];
        m_queue[i] = m_queue[j];
        m_queue[j] = nTmp;
    }
}

int CGameSession::NextBlockType()
{
    if (m_queue.empty())
        SetBlockQueue();

    int nType = m_queue.back();
    m_queue.pop_back();
    return nType;
}

void CGameSession::SpawnBlock()
{
    m_curBlock = m_nextBlock;
    m_curBlock.Init(m_curBlock.GetType());
    m_nextBlock.Init(NextBlockType());

    if (!m_board.IsPlacable(m_curBlock)) {
        m_bGameOver = true;
    }
}

void CGameSession::LockBlock()
{
    m_board.PlaceBlock(m_curBlock);

    int nLines = m_board.ClearFullLines();
    if (nLines > 0) {
        // 한 번에 지운 줄 수에 따라 점수 다르게
        int nGain = 0;
        if (nLines == 1) nGain = 100;
        else if (nLines == 2) nGain = 300;
        else if (nLines == 3) nGain = 500;
        else if (nLines == 4) nGain = 800;

        m_nScore += nGain * m_nLevel;
        m_nLines += nLines;

        // 10줄마다 레벨 1 증가
        m_nLevel = m_nLines / 10 + 1;
    }

    m_bHoldUsed = false;
    SpawnBlock();
}

bool CGameSession::TryMove(int dx, int dy)
{
    m_curBlock.Move(dx, dy);
    if (m_board.IsPlacable(m_curBlock)) {
        return true;
    }
        

    m_curBlock.Move(-dx, -dy);
    return false;
}

bool CGameSession::TryRotate()
{
    m_curBlock.RotateClockWise();
    if (m_board.IsPlacable(m_curBlock)) {
        return true;
    }
        

    m_curBlock.RotateReverseClockWise();
    return false;
}

void CGameSession::HardDrop()
{
    int nDropped = 0;
    while (TryMove(0, 1))
        nDropped++;

    m_nScore += nDropped * 2;

    LockBlock();
}

void CGameSession::HoldBlock()
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
