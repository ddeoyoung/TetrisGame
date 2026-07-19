#pragma once
#include <cstdint>
#include <vector>
#include "CommonDefs.h"

enum EReplayMode {
    REPLAY_SINGLE = 0,
    REPLAY_MULTI = 1,
};

struct TReplayHeader {
    EReplayMode eMode;
    int nPlayerCount;
    uint32_t dwSeed;
    int nTotalTicks;
};

// 1틱 입력 데이터
struct TTickInput {
    EGameInput eInput1; // 플레이어 1
    EGameInput eInput2; // 플레이어 2
};

// 재생 상태
enum EPlaybackState {
    PLAYBACK_STOPPED,
    PLAYBACK_PLAYING,
    PLAYBACK_PAUSED,
};

enum EPlaybackSpeed {
    SPEED_HALF = 0,
    SPEED_1X = 1,
    SPEED_2X = 2,
    SPEED_4X = 4,
};

class CReplayRecorder
{
public:
    void Start(uint32_t dwSeed, EReplayMode eMode);
    void RecordTick(EGameInput eInput1, EGameInput eInput2 = INPUT_NONE);
    bool SaveToFile(const wchar_t* szPath);
    bool IsRecording() { return m_bRecording; }
    void Stop() { m_bRecording = false; }

private:
    EReplayMode m_eMode = REPLAY_SINGLE;
    bool m_bRecording = false;
    uint32_t m_dwSeed = 0;
    std::vector<TTickInput> m_vecInputs;
};

class CReplayPlayer
{
public:
    bool LoadFromFile(const wchar_t* szPath);

    void Play();
    void Pause();
    void TogglePlayPause();
    void SetSpeed(EPlaybackSpeed eSpeed);

    int  GetTicksToAdvance();
    TTickInput GetInput(int nTick);
    void AdvanceTick() { m_nCurrentTick++; }

    EPlaybackState GetState() { return m_eState; }
    EPlaybackSpeed GetSpeed() { return m_eSpeed; }
    EReplayMode GetMode() { return m_header.eMode; }
    uint32_t GetSeed(){ return m_header.dwSeed; }

    int  GetTotalTicks() { return (int)m_header.nTotalTicks; }
    int  GetCurrentTick(){ return m_nCurrentTick; }
    bool IsFinished(){ 
        return m_nCurrentTick >= (int)m_header.nTotalTicks; }

    void Reset();

private:
    TReplayHeader m_header = {};
    std::vector<TTickInput> m_vecInputs;

    EPlaybackState m_eState = PLAYBACK_STOPPED;
    EPlaybackSpeed m_eSpeed = SPEED_1X;

    int m_nCurrentTick = 0;
    int m_nSubTickCounter = 0;
};
