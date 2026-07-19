#include "Replay.h"
#include <cstdio>


void CReplayRecorder::Start(uint32_t dwSeed, EReplayMode eMode)
{
    m_bRecording = true;
    m_dwSeed = dwSeed;
    m_eMode = eMode;
    m_vecInputs.clear();
}

void CReplayRecorder::RecordTick(EGameInput eInput1, EGameInput eInput2)
{
    if (!m_bRecording) return;

    TTickInput tick;
    tick.eInput1 = eInput1;
    tick.eInput2 = eInput2;
    m_vecInputs.push_back(tick); // 기록
}

bool CReplayRecorder::SaveToFile(const wchar_t* szPath)
{
    FILE* fp = nullptr;

    if ( _wfopen_s(&fp, szPath, L"wb") != 0 || !fp ) {
        return false;
    }

    TReplayHeader header = {};
    header.eMode = m_eMode;
    header.nPlayerCount = (m_eMode == REPLAY_MULTI) ? 2 : 1;
    header.dwSeed = m_dwSeed;
    header.nTotalTicks = (uint32_t)m_vecInputs.size();

    fwrite(&header, sizeof(header), 1, fp);

    if (!m_vecInputs.empty()) {
        fwrite(m_vecInputs.data(), sizeof(TTickInput), m_vecInputs.size(), fp);
    }

    fclose(fp);
    m_bRecording = false;

    return true;
}

void CReplayPlayer::Play() { m_eState = PLAYBACK_PLAYING; }
void CReplayPlayer::Pause() { m_eState = PLAYBACK_PAUSED; }

bool CReplayPlayer::LoadFromFile(const wchar_t* szPath)
{
    FILE* fp = nullptr;
    if (_wfopen_s(&fp, szPath, L"rb") != 0 || !fp) return false;

    fread(&m_header, sizeof(m_header), 1, fp);

    m_vecInputs.resize(m_header.nTotalTicks);

    if (m_header.nTotalTicks > 0) {
        fread(m_vecInputs.data(), sizeof(TTickInput), m_header.nTotalTicks, fp);
    }

    fclose(fp);

    m_nCurrentTick = 0;
    m_eState = PLAYBACK_STOPPED;
    m_eSpeed = SPEED_1X;
    m_nSubTickCounter = 0;

    return true;
}


void CReplayPlayer::TogglePlayPause()
{
    if (m_eState == PLAYBACK_PLAYING)
        m_eState = PLAYBACK_PAUSED;
    else if (m_eState == PLAYBACK_PAUSED || m_eState == PLAYBACK_STOPPED)
        m_eState = PLAYBACK_PLAYING;
}

void CReplayPlayer::SetSpeed(EPlaybackSpeed eSpeed)
{
    m_eSpeed = eSpeed;
    m_nSubTickCounter = 0;
}

int CReplayPlayer::GetTicksToAdvance()
{
    if (m_eState != PLAYBACK_PLAYING) return 0;
    if (IsFinished()) return 0;

    if (m_eSpeed == SPEED_HALF) {

        m_nSubTickCounter++;

        if (m_nSubTickCounter >= 2) {
            m_nSubTickCounter = 0;
            return 1;
        }

        return 0;
    }

    return (int)m_eSpeed;
}

TTickInput CReplayPlayer::GetInput(int nTick)
{
    if (nTick < 0 || nTick >= (int)m_vecInputs.size())
        return { INPUT_NONE, INPUT_NONE };

    return m_vecInputs[nTick];
}

void CReplayPlayer::Reset()
{
    m_header = {};
    m_vecInputs.clear();
    m_eState = PLAYBACK_STOPPED;
    m_eSpeed = SPEED_1X;
    m_nCurrentTick = 0;
    m_nSubTickCounter = 0;
}
