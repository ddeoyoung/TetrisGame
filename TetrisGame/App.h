#pragma once
#pragma comment(lib, "comdlg32.lib")

#include "MultiPlay.h"
#include <windows.h>
#include <commdlg.h>
#include <vector>
#include <cstdlib>
#include <cstdint>
#include "Renderer.h"
#include "Board.h"
#include "Block.h"
#include "CommonDefs.h"
#include "GameSession.h"
#include "Protocol.h"
#include "Replay.h"

class CApp
{
public:
    void Initialize(HWND hWnd);
    void OnDraw(HDC hdc);
    void Update();
    void OnKeyDown(WPARAM key);
    void OnChar(wchar_t ch);

private:
    void Redraw();
    void ResizeWindow(int nClientW, int nClientH);

    // 타이틀 화면
    void DrawTitle(HDC hdc);
    void OnKeyTitle(WPARAM key);
    void ReturnToTitle();

    // 싱글 플레이
    void StartSinglePlay();
    void UpdateSingle();
    void OnKeySingle(WPARAM key);
    void DrawSinglePlay(HDC hdc);



    // 멀티 대기실
    void StartMultiLobby();
    void UpdateLobby();
    void OnKeyLobby(WPARAM key);
    void OnCharLobby(wchar_t ch);
    void DrawLobby(HDC hdc);

    // 멀티 플레이
    void StartMultiPlay(uint32_t dwSeed);
    void UpdateMulti();
    void OnKeyMulti(WPARAM key);
    void DrawMultiPlay(HDC hdc);

    // 리플레이
    void StartReplay();
    void UpdateReplay();
    void OnKeyReplay(WPARAM key);
    void DrawReplay(HDC hdc);
    void DrawReplayUI(HDC hdc);


    // 공용
    void DrawSession(HDC hdc, CGameSession& session, int nX, int nY);
    void DrawSidePanel(HDC hdc, CGameSession& session, int nX, int nY);
    void DrawBlockPreview(HDC hdc, int type, int x, int y, int nCellSize);
    void DrawGameOver(HDC hdc, int nBoardX, int nBoardY);
    void DrawMultiResult(HDC hdc);


    void SaveReplay();

private:
    HWND m_hWnd = nullptr;
    CRenderer m_renderer;

    EGameState m_eState = STATE_TITLE;
    int m_nMenuSelect = 0;

    // 싱글 플레이
    CGameSession m_session;
    EGameInput m_ePendingInput = INPUT_NONE;
    uint32_t m_dwCurrentSeed = 0;


    // 멀티 플레이
    CGameSession m_sessionMe;
    CGameSession m_sessionEnemy;
    CMultiPlay m_net;

    ELobbyState m_eLobbyState = LOBBY_SELECT;
    int m_nLobbySelect = 0; // 호스트 0, 클라이언트 1
    char m_szInputIP[64] = {};
    int  m_nInputLen = 0;

    EGameInput m_eMyInput = INPUT_NONE;
    EGameInput m_eEnemyInput = INPUT_NONE;
    bool m_bEnemyInputReceived = false;
    int  m_nCurrentTick = 0;
    bool m_bMyGameOver = false;
    bool m_bEnemyGameOver = false;
    bool m_bMultiFinished = false;


    // 리플레이
    CReplayRecorder m_recorder;
    CReplayPlayer m_replayPlayer;


    // 멀티플레이용 세션 (리플레이 재생에도 사용)
    // m_sessionEnemy 재사용
};
