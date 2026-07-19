#include "App.h"
#include <ctime>
#include <cstdio>

constexpr int BOARD_X = 20;
constexpr int BOARD_Y = 20;
constexpr int TICK_INTERVAL = 50; // 50ms = 초당 20틱
constexpr int NET_PORT = 7777;

// 싱글 플레이 창 크기
constexpr int SINGLE_W = 20 + (BOARD_W * CELL_SIZE) + 20 + 130 + 10;
constexpr int SINGLE_H = 20 + (BOARD_H * CELL_SIZE) + 20;

// 멀티 플레이 창 크기
constexpr int MULTI_W = 20 + (BOARD_W * CELL_SIZE) + 20 + 100 + 20 + (BOARD_W * CELL_SIZE) + 20; // | [유저1]   [유저2] |
constexpr int MULTI_H = 20 + (BOARD_H * CELL_SIZE) + 20;

// 리플레이 하단 UI높이
constexpr int REPLAY_UI_H = 60;

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
    m_eState = STATE_TITLE;
    m_nMenuSelect = 0;

    RECT rc;
    GetClientRect(m_hWnd, &rc);

    HDC hdc = GetDC(m_hWnd);
    m_renderer.Initialize(hdc, rc.right, rc.bottom);
    ReleaseDC(m_hWnd, hdc);
}

void CApp::Redraw()
{
    InvalidateRect(m_hWnd, nullptr, FALSE);
}

void CApp::ResizeWindow(int nClientW, int nClientH)
{
    DWORD style = (DWORD)GetWindowLong(m_hWnd, GWL_STYLE);
    RECT rc = { 0, 0, nClientW, nClientH };
    AdjustWindowRect(&rc, style, FALSE);

    int winW = rc.right - rc.left;
    int winH = rc.bottom - rc.top;

    SetWindowPos(m_hWnd, nullptr, 0, 0, winW, winH, SWP_NOMOVE | SWP_NOZORDER);

    HDC hdc = GetDC(m_hWnd);
    m_renderer.Initialize(hdc, nClientW, nClientH);
    ReleaseDC(m_hWnd, hdc);
}

void CApp::ReturnToTitle()
{
    KillTimer(m_hWnd, 1);
    m_net.Cleanup();
    m_eState = STATE_TITLE;
    m_nMenuSelect = 0;
    m_ePendingInput = INPUT_NONE;
    m_eMyInput = INPUT_NONE;
    m_bMultiFinished = false;
    ResizeWindow(SINGLE_W, SINGLE_H);
    Redraw();
}


void CApp::OnDraw(HDC hdc)
{
    HDC mem = m_renderer.BeginDraw();

    switch (m_eState) {
    case STATE_TITLE:
        DrawTitle(mem);
        break;
    case STATE_SINGLE:
        DrawSinglePlay(mem);
        break;
    case STATE_MULTI_LOBBY:
        DrawLobby(mem);
        break;
    case STATE_MULTI_PLAY:
        DrawMultiPlay(mem);
        break;
    case STATE_REPLAY:
        DrawReplay(mem);
        break;
    }

    m_renderer.EndDraw(hdc);
}

void CApp::Update()
{
    switch (m_eState) {
    case STATE_SINGLE:
        UpdateSingle();
        break;
    case STATE_MULTI_LOBBY:
        UpdateLobby();
        break;
    case STATE_MULTI_PLAY:
        UpdateMulti();
        break;
    case STATE_REPLAY:
        UpdateReplay();
        break;
    }
}

void CApp::OnKeyDown(WPARAM key)
{
    switch (m_eState) {
    case STATE_TITLE:
        OnKeyTitle(key);
        break;
    case STATE_SINGLE:
        OnKeySingle(key);
        break;
    case STATE_MULTI_LOBBY:
        OnKeyLobby(key);
        break;
    case STATE_MULTI_PLAY:
        OnKeyMulti(key);
        break;
    case STATE_REPLAY:
        OnKeyReplay(key);
        break;
    }
}

void CApp::OnChar(wchar_t ch)
{
    if (m_eState == STATE_MULTI_LOBBY) {
        OnCharLobby(ch);
    }
}

///////////////////////////////////////////////////////////////////////////////////
//  타이틀 화면

void CApp::DrawTitle(HDC hdc)
{
    RECT rcClient;
    GetClientRect(m_hWnd, &rcClient);
    int nW = rcClient.right;
    int nH = rcClient.bottom;

    SetBkMode(hdc, TRANSPARENT);

    // TETRIS
    HFONT fontTitle = CreateFont(60, 0, 0, 0, FW_BOLD,
        FALSE, FALSE, FALSE, DEFAULT_CHARSET, 0, 0, 0, 0, L"Small Fonts");
    HFONT oldFont = (HFONT)SelectObject(hdc, fontTitle);

    LPCWSTR sTitle = L"TETRIS";
    COLORREF titleColors[] = {
        RGB(145, 220, 230),  // T 하늘
        RGB(245, 230, 155),  // E 노랑
        RGB(190, 160, 220),  // T 보라
        RGB(235, 155, 155),  // R 핑크
        RGB(150, 170, 225),  // I 파랑
        RGB(160, 215, 165),  // S 초록
    };

    SIZE totalSize;
    GetTextExtentPoint32(hdc, sTitle, 6, &totalSize);
    int tx = (nW - totalSize.cx) / 2;
    int ty = nH / 4;

    for (int i = 0; i < 6; i++) {
        SetTextColor(hdc, titleColors[i]);
        SIZE cs;
        GetTextExtentPoint32(hdc, &sTitle[i], 1, &cs);
        TextOut(hdc, tx, ty, &sTitle[i], 1);
        tx += cs.cx;
    }

    SelectObject(hdc, oldFont);
    DeleteObject(fontTitle);

    // 메뉴 (싱글, 멀티, 리플레이)
    HFONT fontMenu = CreateFont(24, 0, 0, 0, FW_BOLD,
        FALSE, FALSE, FALSE, DEFAULT_CHARSET, 0, 0, 0, 0, L"Small Fonts");
    oldFont = (HFONT)SelectObject(hdc, fontMenu);

    LPCWSTR menuTexts[] = { L"SINGLE PLAY", L"MULTI PLAY", L"REPLAY" };

    int menuY = nH / 2;
    int menuGap = 40;

    for (int i = 0; i < MENU_COUNT; i++) {
        if (i == m_nMenuSelect) {
            SetTextColor(hdc, RGB(255, 255, 255));
            SIZE sz;
            GetTextExtentPoint32(hdc, menuTexts[i], (int)wcslen(menuTexts[i]), &sz);
            int mx = (nW - sz.cx) / 2;
            int my = menuY + i * menuGap;
            TextOut(hdc, mx - 25, my, L">", 1);
            TextOut(hdc, mx, my, menuTexts[i], (int)wcslen(menuTexts[i]));
        }
        else {
            SetTextColor(hdc, RGB(100, 100, 100));
            SIZE sz;
            GetTextExtentPoint32(hdc, menuTexts[i], (int)wcslen(menuTexts[i]), &sz);
            int mx = (nW - sz.cx) / 2;
            int my = menuY + i * menuGap;
            TextOut(hdc, mx, my, menuTexts[i], (int)wcslen(menuTexts[i]));
        }
    }

    SelectObject(hdc, oldFont);
    DeleteObject(fontMenu);

    // 하단 조작법 안내 UI
    HFONT fontHelp = CreateFont(14, 0, 0, 0, FW_NORMAL,
        FALSE, FALSE, FALSE, DEFAULT_CHARSET, 0, 0, 0, 0, L"Small Fonts");
    oldFont = (HFONT)SelectObject(hdc, fontHelp);
    SetTextColor(hdc, RGB(80, 80, 80));

    RECT rcHelp = { 0, nH - 50, nW, nH - 30 };
    DrawText(hdc, L"Up/Down: Select    Enter: Start", -1, &rcHelp,
        DT_CENTER | DT_SINGLELINE);

    SelectObject(hdc, oldFont);
    DeleteObject(fontHelp);
}

void CApp::OnKeyTitle(WPARAM key)
{
    switch (key) {
    case VK_UP:
        m_nMenuSelect = (m_nMenuSelect + MENU_COUNT - 1) % MENU_COUNT;
        break;
    case VK_DOWN:
        m_nMenuSelect = (m_nMenuSelect + 1) % MENU_COUNT;
        break;
    case VK_RETURN:
        switch (m_nMenuSelect) {
        case MENU_SINGLE:
            StartSinglePlay();
            break;
        case MENU_MULTI:
            StartMultiLobby();
            break;
        case MENU_REPLAY:
            StartReplay();
            break;
        }
        break;
    }
    Redraw();
}

/////////////////////////////////////////////////////////////////////////////////
//  싱글 플레이

void CApp::StartSinglePlay()
{
    m_eState = STATE_SINGLE;
    m_ePendingInput = INPUT_NONE;

    m_dwCurrentSeed = (uint32_t)time(nullptr);
    m_session.Start(m_dwCurrentSeed);

    // 리플레이 녹화 시작
    m_recorder.Start(m_dwCurrentSeed, REPLAY_SINGLE);

    SetTimer(m_hWnd, 1, TICK_INTERVAL, nullptr);
}

void CApp::UpdateSingle()
{
    EGameInput eInput = m_ePendingInput;
    m_ePendingInput = INPUT_NONE;

    // 입력 기록
    m_recorder.RecordTick(eInput);

    // 게임 진행
    m_session.ProcessTick(eInput);

    // 게임오버 시 리플레이 저장
    if (m_session.IsGameOver() && m_recorder.IsRecording()) {
        m_recorder.Stop();
        SaveReplay();
    }

    Redraw();
}

void CApp::OnKeySingle(WPARAM key)
{
    if (m_session.IsGameOver()) {
        if (key == 'R') {
            StartSinglePlay();
        }
        if (key == VK_ESCAPE) {
            ReturnToTitle();
        }
        return;
    }

    switch (key) {
    case VK_LEFT:
        m_ePendingInput = INPUT_LEFT;
        break;
    case VK_RIGHT:
        m_ePendingInput = INPUT_RIGHT;
        break;
    case VK_DOWN:
        m_ePendingInput = INPUT_DOWN;
        break;
    case VK_UP:
        m_ePendingInput = INPUT_ROTATE;
        break;
    case VK_SPACE:
        m_ePendingInput = INPUT_HARD_DROP;
        break;
    case 'H':
        m_ePendingInput = INPUT_HOLD;
        break;
    case VK_ESCAPE:
        ReturnToTitle();
        return;
    default: return;
    }

    Redraw();
}

void CApp::DrawSinglePlay(HDC hdc)
{
    DrawSession(hdc, m_session, BOARD_X, BOARD_Y);
    int sx = BOARD_X + BOARD_W * CELL_SIZE + 20;
    DrawSidePanel(hdc, m_session, sx, BOARD_Y);

    if (m_session.IsGameOver())
        DrawGameOver(hdc, BOARD_X, BOARD_Y);
}



/////////////////////////////////////////////////////////////////////////////////
//  멀티플레이 대기실

void CApp::StartMultiLobby()
{
    m_eState = STATE_MULTI_LOBBY;
    m_eLobbyState = LOBBY_SELECT;
    m_nLobbySelect = 0;
    m_nInputLen = 0;
    memset(m_szInputIP, 0, sizeof(m_szInputIP));

    m_net.InitWinsock();
    Redraw();
}

void CApp::UpdateLobby()
{
    if (m_eLobbyState == LOBBY_HOSTING) {
        // 상대 접속 확인
        if (m_net.AcceptClient()) {
            // 호스트가 시드 생성 후 전송
            uint32_t dwSeed = (uint32_t)time(nullptr);

            TPacketGameStart pkt = {};
            pkt.header.eType = PKT_GAME_START;
            pkt.header.nSize = sizeof(pkt);
            pkt.dwSeed = dwSeed;
            m_net.SendPacket(&pkt, sizeof(pkt));

            StartMultiPlay(dwSeed);
        }


        Redraw();
    }

    else if (m_eLobbyState == LOBBY_CONNECTING) {
        // 클라: 시드 수신 대기
        if (m_net.HasData()) {
            TPacketGameStart pkt = {};
            if (m_net.RecvPacket(&pkt, sizeof(pkt))) {
                StartMultiPlay(pkt.dwSeed);
            }
        }

        Redraw();
    }
}

void CApp::OnKeyLobby(WPARAM key)
{
    if (key == VK_ESCAPE) {
        if (m_eLobbyState == LOBBY_SELECT) {
            m_net.Cleanup();
            ReturnToTitle();
        }
        else {
            // 호스팅/연결 취소 시 -> 로비 선택으로
            m_net.Cleanup();
            m_net.InitWinsock();
            m_eLobbyState = LOBBY_SELECT;
            KillTimer(m_hWnd, 1);
        }

        Redraw();

        return;
    }

    switch (m_eLobbyState) {
    case LOBBY_SELECT:

        if (key == VK_UP || key == VK_DOWN)
            m_nLobbySelect = 1 - m_nLobbySelect;  // 0 <-> 1

        if (key == VK_RETURN) {
            if (m_nLobbySelect == 0) {
                // HOST
                if (m_net.HostGame(NET_PORT)) {
                    m_eLobbyState = LOBBY_HOSTING;
                    SetTimer(m_hWnd, 1, 100, nullptr);
                }
            }

            else {
                // JOIN - IP 입력 모드
                m_eLobbyState = LOBBY_JOIN_IP;
                m_nInputLen = 0;
                memset(m_szInputIP, 0, sizeof(m_szInputIP));
            }
        }
        break;

    case LOBBY_JOIN_IP:
        if (key == VK_RETURN && m_nInputLen > 0) {
            // 연결 시도
            if (m_net.JoinGame(m_szInputIP, NET_PORT)) {
                m_eLobbyState = LOBBY_CONNECTING;
                SetTimer(m_hWnd, 1, 100, nullptr);
            }
        }
        if (key == VK_BACK && m_nInputLen > 0) {
            m_nInputLen--;
            m_szInputIP[m_nInputLen] = '\0';
        }
        break;
    }

    Redraw();
}

void CApp::OnCharLobby(wchar_t ch)
{
    if (m_eLobbyState != LOBBY_JOIN_IP) return;

    // 숫자랑 '.'만 허용
    if ((ch >= '0' && ch <= '9') || ch == '.') {
        if (m_nInputLen < 15) {
            m_szInputIP[m_nInputLen++] = (char)ch;
            m_szInputIP[m_nInputLen] = '\0';
        }
    }

    Redraw();
}

void CApp::DrawLobby(HDC hdc)
{
    RECT rcClient;
    GetClientRect(m_hWnd, &rcClient);
    int nW = rcClient.right;
    int nH = rcClient.bottom;

    SetBkMode(hdc, TRANSPARENT);

    HFONT fontTitle = CreateFont(30, 0, 0, 0, FW_BOLD,
        FALSE, FALSE, FALSE, DEFAULT_CHARSET, 0, 0, 0, 0, L"Small Fonts");
    HFONT fontMenu = CreateFont(20, 0, 0, 0, FW_BOLD,
        FALSE, FALSE, FALSE, DEFAULT_CHARSET, 0, 0, 0, 0, L"Small Fonts");
    HFONT fontSmall = CreateFont(14, 0, 0, 0, FW_NORMAL,
        FALSE, FALSE, FALSE, DEFAULT_CHARSET, 0, 0, 0, 0, L"Small Fonts");

    HFONT oldFont = (HFONT)SelectObject(hdc, fontTitle);

    // 멀티플레이 타이틀
    SetTextColor(hdc, RGB(145, 220, 230));
    RECT rcTitle = { 0, nH / 5, nW, nH / 5 + 40 };
    DrawText(hdc, L"MULTI PLAY", -1, &rcTitle, DT_CENTER | DT_SINGLELINE);

    SelectObject(hdc, fontMenu);

    switch (m_eLobbyState) {
    case LOBBY_SELECT: {
        LPCWSTR items[] = { L"HOST (Create Room)", L"JOIN (Enter IP)" };
        int my = nH / 2 - 20;
        for (int i = 0; i < 2; i++) {
            if (i == m_nLobbySelect) {
                SetTextColor(hdc, RGB(255, 255, 255));
                SIZE sz;
                GetTextExtentPoint32(hdc, items[i], (int)wcslen(items[i]), &sz);
                int mx = (nW - sz.cx) / 2;
                TextOut(hdc, mx - 20, my + i * 35, L">", 1);
                TextOut(hdc, mx, my + i * 35, items[i], (int)wcslen(items[i]));
            }
            else {
                SetTextColor(hdc, RGB(100, 100, 100));
                SIZE sz;
                GetTextExtentPoint32(hdc, items[i], (int)wcslen(items[i]), &sz);
                int mx = (nW - sz.cx) / 2;
                TextOut(hdc, mx, my + i * 35, items[i], (int)wcslen(items[i]));
            }
        }
        break;
    }

    case LOBBY_HOSTING: {
        SetTextColor(hdc, RGB(200, 200, 200));
        RECT rc1 = { 0, nH / 2 - 20, nW, nH / 2 + 10 };
        DrawText(hdc, L"Waiting for opponent...", -1, &rc1, DT_CENTER | DT_SINGLELINE);

        wchar_t buf[64];
        wsprintf(buf, L"Port: %d", NET_PORT);
        SetTextColor(hdc, RGB(150, 150, 150));
        RECT rc2 = { 0, nH / 2 + 20, nW, nH / 2 + 50 };
        DrawText(hdc, buf, -1, &rc2, DT_CENTER | DT_SINGLELINE);
        break;
    }

    case LOBBY_JOIN_IP: {
        SetTextColor(hdc, RGB(200, 200, 200));
        RECT rc1 = { 0, nH / 2 - 40, nW, nH / 2 - 10 };
        DrawText(hdc, L"Enter Host IP:", -1, &rc1, DT_CENTER | DT_SINGLELINE);

        // IP 입력 필드
        wchar_t wIP[64] = {};
        for (int i = 0; i < m_nInputLen; i++)
            wIP[i] = (wchar_t)m_szInputIP[i];
        wIP[m_nInputLen] = L'_';  // 커서
        wIP[m_nInputLen + 1] = 0;

        SetTextColor(hdc, RGB(255, 255, 255));
        RECT rc2 = { 0, nH / 2, nW, nH / 2 + 30 };
        DrawText(hdc, wIP, -1, &rc2, DT_CENTER | DT_SINGLELINE);
        break;
    }

    case LOBBY_CONNECTING: {
        SetTextColor(hdc, RGB(200, 200, 200));
        RECT rc1 = { 0, nH / 2, nW, nH / 2 + 30 };
        DrawText(hdc, L"Connecting...", -1, &rc1, DT_CENTER | DT_SINGLELINE);
        break;
    }
    }

    // 하단 안내
    SelectObject(hdc, fontSmall);
    SetTextColor(hdc, RGB(80, 80, 80));
    RECT rcHelp = { 0, nH - 40, nW, nH - 20 };
    DrawText(hdc, L"ESC: Back", -1, &rcHelp, DT_CENTER | DT_SINGLELINE);

    SelectObject(hdc, oldFont);
    DeleteObject(fontTitle);
    DeleteObject(fontMenu);
    DeleteObject(fontSmall);
}


//////////////////////////////////////////////////////////////
//  멀티 플레이

void CApp::StartMultiPlay(uint32_t dwSeed)
{
    KillTimer(m_hWnd, 1);

    m_eState = STATE_MULTI_PLAY;

    // 창 크기 확장
    ResizeWindow(MULTI_W, MULTI_H);

    // 양쪽 동일 시드로 시작
    m_sessionMe.Start(dwSeed);
    m_sessionEnemy.Start(dwSeed);

    m_dwCurrentSeed = dwSeed;
    m_nCurrentTick = 0;
    m_eMyInput = INPUT_NONE;
    m_eEnemyInput = INPUT_NONE;
    m_bEnemyInputReceived = false;
    m_bMyGameOver = false;
    m_bEnemyGameOver = false;
    m_bMultiFinished = false;

    // 리플레이 녹화 시작
    m_recorder.Start(dwSeed, REPLAY_MULTI);

    SetTimer(m_hWnd, 1, TICK_INTERVAL, nullptr);
}

void CApp::UpdateMulti()
{
    if (m_bMultiFinished) return;

    // 연결 끊김 체크
    if (m_net.GetState() == NET_DISCONNECTED) {
        m_bMultiFinished = true;
        KillTimer(m_hWnd, 1);
        if (m_recorder.IsRecording()) m_recorder.Stop();
        Redraw();
        return;
    }

    // 1. 내 입력 전송
    TPacketTickInput pkt = {};
    pkt.header.eType = PKT_TICK_INPUT;
    pkt.header.nSize = sizeof(pkt);
    pkt.nTick = m_nCurrentTick;
    pkt.eInput = m_eMyInput;

    m_net.SendPacket(&pkt, sizeof(pkt));

    // 2. 상대 입력 수신
    if (!m_bEnemyInputReceived && m_net.HasData()) {
        TPacketTickInput recvPkt = {};
        if (m_net.RecvPacket(&recvPkt, sizeof(recvPkt))) {
            if (recvPkt.header.eType == PKT_TICK_INPUT) {
                m_eEnemyInput = recvPkt.eInput;
                m_bEnemyInputReceived = true;
            }
            else if (recvPkt.header.eType == PKT_GAME_OVER) {
                m_bEnemyGameOver = true;
                m_bEnemyInputReceived = true;
                m_eEnemyInput = INPUT_NONE;
            }
        }
    }

    // 3. 양쪽 입력 도착 -> 1틱 진행
    if (m_bEnemyInputReceived) {
        // 입력 기록
        if (m_net.IsHost()) {
            m_recorder.RecordTick(m_eMyInput, m_eEnemyInput);
        }
        else {
            m_recorder.RecordTick(m_eEnemyInput, m_eMyInput);
        }

        m_sessionMe.ProcessTick(m_eMyInput);
        m_sessionEnemy.ProcessTick(m_eEnemyInput);

        // 내 게임오버 체크
        if (m_sessionMe.IsGameOver() && !m_bMyGameOver) {
            m_bMyGameOver = true;
            TPacketGameOver goPkt = {};
            goPkt.header.eType = PKT_GAME_OVER;
            goPkt.header.nSize = sizeof(goPkt);
            goPkt.nTick = m_nCurrentTick;
            m_net.SendPacket(&goPkt, sizeof(goPkt));
        }

        // 승패 판정
        if (m_bMyGameOver || m_bEnemyGameOver) {
            m_bMultiFinished = true;
            KillTimer(m_hWnd, 1);
            if (m_recorder.IsRecording()) {
                m_recorder.Stop();
                SaveReplay();
            }
        }

        m_eMyInput = INPUT_NONE;
        m_eEnemyInput = INPUT_NONE;
        m_bEnemyInputReceived = false;
        m_nCurrentTick++;
    }

    Redraw();
}

void CApp::OnKeyMulti(WPARAM key)
{
    if (m_bMultiFinished) {
        if (key == VK_ESCAPE) {
            ReturnToTitle();
        }
        return;
    }

    switch (key) {
    case VK_LEFT:
        m_eMyInput = INPUT_LEFT;
        break;
    case VK_RIGHT:
        m_eMyInput = INPUT_RIGHT;
        break;
    case VK_DOWN:
        m_eMyInput = INPUT_DOWN;
        break;
    case VK_UP:
        m_eMyInput = INPUT_ROTATE;
        break;
    case VK_SPACE:
        m_eMyInput = INPUT_HARD_DROP;
        break;
    case 'H':
        m_eMyInput = INPUT_HOLD;
        break;
    case VK_ESCAPE:
        if (m_recorder.IsRecording()) m_recorder.Stop();
        ReturnToTitle();
        return;
    }
}

void CApp::DrawMultiPlay(HDC hdc)
{
    int nGap = 100;
    int nMyX = 20;
    int nMyY = 20;
    int nEnemyX = 20 + BOARD_W * CELL_SIZE + 20 + nGap + 20;
    int nEnemyY = 20;

    // 내 보드
    DrawSession(hdc, m_sessionMe, nMyX, nMyY);
    if (m_sessionMe.IsGameOver())
        DrawGameOver(hdc, nMyX, nMyY);

    // 상대 보드
    DrawSession(hdc, m_sessionEnemy, nEnemyX, nEnemyY);
    if (m_sessionEnemy.IsGameOver())
        DrawGameOver(hdc, nEnemyX, nEnemyY);

    // 중앙 정보 패널
    int cx = 20 + BOARD_W * CELL_SIZE + 20;
    int cy = 20;

    SetBkMode(hdc, TRANSPARENT);
    HFONT fontLabel = CreateFont(14, 0, 0, 0, FW_BOLD,
        FALSE, FALSE, FALSE, DEFAULT_CHARSET, 0, 0, 0, 0, L"Small Fonts");
    HFONT fontVal = CreateFont(20, 0, 0, 0, FW_BOLD,
        FALSE, FALSE, FALSE, DEFAULT_CHARSET, 0, 0, 0, 0, L"Small Fonts");
    HFONT oldFont = (HFONT)SelectObject(hdc, fontLabel);

    // "YOU" / "ENEMY" 라벨
    SetTextColor(hdc, RGB(145, 220, 230));
    TextOut(hdc, nMyX + BOARD_W * CELL_SIZE / 2 - 15, nMyY - 18, L"YOU", 3);
    SetTextColor(hdc, RGB(235, 155, 155));
    TextOut(hdc, nEnemyX + BOARD_W * CELL_SIZE / 2 - 25, nEnemyY - 18, L"ENEMY", 5);

    // 중앙 스코어
    wchar_t buf[32];

    SetTextColor(hdc, RGB(180, 180, 180));
    TextOut(hdc, cx + 10, cy + 20, L"SCORE", 5);
    SelectObject(hdc, fontVal);
    SetTextColor(hdc, RGB(255, 255, 255));
    wsprintf(buf, L"%d", m_sessionMe.GetScore());
    TextOut(hdc, cx + 10, cy + 40, buf, (int)wcslen(buf));

    SelectObject(hdc, fontLabel);
    SetTextColor(hdc, RGB(180, 180, 180));
    TextOut(hdc, cx + 10, cy + 80, L"LEVEL", 5);
    SelectObject(hdc, fontVal);
    SetTextColor(hdc, RGB(255, 255, 255));
    wsprintf(buf, L"%d", m_sessionMe.GetLevel());
    TextOut(hdc, cx + 10, cy + 100, buf, (int)wcslen(buf));

    SelectObject(hdc, fontLabel);
    SetTextColor(hdc, RGB(180, 180, 180));
    TextOut(hdc, cx + 10, cy + 140, L"LINES", 5);
    SelectObject(hdc, fontVal);
    SetTextColor(hdc, RGB(255, 255, 255));
    wsprintf(buf, L"%d", m_sessionMe.GetLines());
    TextOut(hdc, cx + 10, cy + 160, buf, (int)wcslen(buf));

    // 멀티 결과
    if (m_bMultiFinished) {
        DrawMultiResult(hdc);
    }

    SelectObject(hdc, oldFont);
    DeleteObject(fontLabel);
    DeleteObject(fontVal);
}

void CApp::DrawMultiResult(HDC hdc)
{
    RECT rcClient;
    GetClientRect(m_hWnd, &rcClient);
    int nW = rcClient.right;
    int nH = rcClient.bottom;

    // 반투명 오버레이 효과 (검은 사각형)
    HBRUSH brOverlay = CreateSolidBrush(RGB(0, 0, 0));
    RECT rcOverlay = { nW / 2 - 120, nH / 2 - 50, nW / 2 + 120, nH / 2 + 50 };
    FillRect(hdc, &rcOverlay, brOverlay);
    DeleteObject(brOverlay);

    HFONT fontResult = CreateFont(36, 0, 0, 0, FW_BOLD,
        FALSE, FALSE, FALSE, DEFAULT_CHARSET, 0, 0, 0, 0, L"Small Fonts");
    HFONT oldFont = (HFONT)SelectObject(hdc, fontResult);

    SetBkMode(hdc, TRANSPARENT);

    LPCWSTR szResult;
    COLORREF clrResult;

    if (m_net.GetState() == NET_DISCONNECTED) {
        szResult = L"DISCONNECTED";
        clrResult = RGB(200, 200, 200);
    }
    else if (m_bMyGameOver && m_bEnemyGameOver) {
        szResult = L"DRAW";
        clrResult = RGB(245, 230, 155);
    }
    else if (m_bMyGameOver) {
        szResult = L"YOU LOSE";
        clrResult = RGB(235, 155, 155);
    }
    else {
        szResult = L"YOU WIN!";
        clrResult = RGB(145, 220, 230);
    }

    SetTextColor(hdc, clrResult);
    RECT rcText = { nW / 2 - 120, nH / 2 - 40, nW / 2 + 120, nH / 2 };
    DrawText(hdc, szResult, -1, &rcText, DT_CENTER | DT_SINGLELINE | DT_VCENTER);

    SelectObject(hdc, oldFont);
    DeleteObject(fontResult);

    HFONT fontSmall = CreateFont(14, 0, 0, 0, FW_NORMAL,
        FALSE, FALSE, FALSE, DEFAULT_CHARSET, 0, 0, 0, 0, L"Small Fonts");
    oldFont = (HFONT)SelectObject(hdc, fontSmall);
    SetTextColor(hdc, RGB(150, 150, 150));
    RECT rcSub = { nW / 2 - 120, nH / 2 + 10, nW / 2 + 120, nH / 2 + 40 };
    DrawText(hdc, L"Press ESC to exit", -1, &rcSub, DT_CENTER | DT_SINGLELINE);

    SelectObject(hdc, oldFont);
    DeleteObject(fontSmall);
}

////////////////////////////////////////////////////////////////////
//  리플레이

void CApp::StartReplay()
{
    wchar_t szPath[MAX_PATH] = {};

    OPENFILENAME ofn = {};
    ofn.lStructSize = sizeof(ofn);
    ofn.hwndOwner = m_hWnd;
    ofn.lpstrFilter = L"Replay Files (*.rep)\0*.rep\0All Files\0*.*\0";
    ofn.lpstrFile = szPath;
    ofn.nMaxFile = MAX_PATH;
    ofn.Flags = OFN_FILEMUSTEXIST;

    if (!GetOpenFileName(&ofn))
        return;

    if (!m_replayPlayer.LoadFromFile(szPath)) {
        MessageBox(m_hWnd, L"Invalid replay file.", L"Error", MB_OK);
        return;
    }

    m_eState = STATE_REPLAY;

    // 리플레이 모드에 따라 창 크기 결정
    if (m_replayPlayer.GetMode() == REPLAY_MULTI) {
        ResizeWindow(MULTI_W, MULTI_H + REPLAY_UI_H);
    }
    else {
        ResizeWindow(SINGLE_W, SINGLE_H + REPLAY_UI_H);
    }

    m_session.Start(m_replayPlayer.GetSeed());

    if (m_replayPlayer.GetMode() == REPLAY_MULTI) {
        m_sessionEnemy.Start(m_replayPlayer.GetSeed());
    }

    m_replayPlayer.Play();
    SetTimer(m_hWnd, 1, TICK_INTERVAL, nullptr);
}

void CApp::UpdateReplay()
{
    int nTicksToAdvance = m_replayPlayer.GetTicksToAdvance();

    for (int i = 0; i < nTicksToAdvance; i++) {
        if (m_replayPlayer.IsFinished()) break;

        int nTick = m_replayPlayer.GetCurrentTick();
        TTickInput input = m_replayPlayer.GetInput(nTick);

        m_session.ProcessTick(input.eInput1);

        if (m_replayPlayer.GetMode() == REPLAY_MULTI) {
            m_sessionEnemy.ProcessTick(input.eInput2);
        }

        m_replayPlayer.AdvanceTick();
    }

    // 재생 완료
    if (m_replayPlayer.IsFinished()) {
        m_replayPlayer.Pause();
    }

    Redraw();
}

void CApp::OnKeyReplay(WPARAM key)
{
    switch (key) {
    case VK_SPACE:
        m_replayPlayer.TogglePlayPause();
        break;
    case '1':
        m_replayPlayer.SetSpeed(SPEED_HALF);
        break;
    case '2':
        m_replayPlayer.SetSpeed(SPEED_1X);
        break;
    case '3':
        m_replayPlayer.SetSpeed(SPEED_2X);
        break;
    case '4':
        m_replayPlayer.SetSpeed(SPEED_4X);
        break;
    case VK_ESCAPE:
        m_replayPlayer.Reset();
        KillTimer(m_hWnd, 1);
        ReturnToTitle();
        return;
    }
    Redraw();
}

void CApp::DrawReplay(HDC hdc)
{
    if (m_replayPlayer.GetMode() == REPLAY_MULTI) {
        // 멀티 리플레이: 보드 2개
        int nGap = 100;
        int nMyX = 20;
        int nMyY = 20;
        int nEnemyX = 20 + BOARD_W * CELL_SIZE + 20 + nGap + 20;
        int nEnemyY = 20;

        DrawSession(hdc, m_session, nMyX, nMyY);
        DrawSession(hdc, m_sessionEnemy, nEnemyX, nEnemyY);

        // 라벨
        SetBkMode(hdc, TRANSPARENT);
        HFONT fontLabel = CreateFont(14, 0, 0, 0, FW_BOLD,
            FALSE, FALSE, FALSE, DEFAULT_CHARSET, 0, 0, 0, 0, L"Small Fonts");
        HFONT oldFont = (HFONT)SelectObject(hdc, fontLabel);

        SetTextColor(hdc, RGB(145, 220, 230));
        TextOut(hdc, nMyX + BOARD_W * CELL_SIZE / 2 - 15, nMyY - 18, L"P1", 2);
        SetTextColor(hdc, RGB(235, 155, 155));
        TextOut(hdc, nEnemyX + BOARD_W * CELL_SIZE / 2 - 15, nEnemyY - 18, L"P2", 2);
        SelectObject(hdc, oldFont);
        DeleteObject(fontLabel);
    }
    else {
        // 싱글 리플레이
        DrawSession(hdc, m_session, BOARD_X, BOARD_Y);
        int sx = BOARD_X + BOARD_W * CELL_SIZE + 20;
        DrawSidePanel(hdc, m_session, sx, BOARD_Y);
    }

    DrawReplayUI(hdc);
}

void CApp::DrawReplayUI(HDC hdc)
{
    int nBarY = 20 + BOARD_H * CELL_SIZE + 10;

    SetBkMode(hdc, TRANSPARENT);

    HFONT font = CreateFont(14, 0, 0, 0, FW_NORMAL,
        FALSE, FALSE, FALSE, DEFAULT_CHARSET, 0, 0, 0, 0, L"Small Fonts");
    HFONT oldFont = (HFONT)SelectObject(hdc, font);

    // 상태 텍스트
    SetTextColor(hdc, RGB(180, 180, 180));
    wchar_t buf[128];

    const wchar_t* szState = L"STOPPED";
    if (m_replayPlayer.GetState() == PLAYBACK_PLAYING) szState = L"PLAYING";
    if (m_replayPlayer.GetState() == PLAYBACK_PAUSED)  szState = L"PAUSED";

    const wchar_t* szSpeed = L"1x";
    switch (m_replayPlayer.GetSpeed()) {
    case SPEED_HALF: szSpeed = L"0.5x"; break;
    case SPEED_1X:   szSpeed = L"1x";   break;
    case SPEED_2X:   szSpeed = L"2x";   break;
    case SPEED_4X:   szSpeed = L"4x";   break;
    }

    wsprintf(buf, L"%s  Speed: %s  Tick: %d / %d",
        szState, szSpeed,
        m_replayPlayer.GetCurrentTick(),
        m_replayPlayer.GetTotalTicks());
    TextOut(hdc, 20, nBarY, buf, (int)wcslen(buf));

    // 상태 바 표시
    int nBarX = 20;
    int nBarW = BOARD_W * CELL_SIZE;
    int nBarH = 8;
    int nProgY = nBarY + 20;

    RECT rcBg = { nBarX, nProgY, nBarX + nBarW, nProgY + nBarH };
    HBRUSH brBg = CreateSolidBrush(RGB(60, 60, 60));
    FillRect(hdc, &rcBg, brBg);
    DeleteObject(brBg);

    float fProgress = 0.0f;
    if (m_replayPlayer.GetTotalTicks() > 0) {
        fProgress = (float)m_replayPlayer.GetCurrentTick()
                  / (float)m_replayPlayer.GetTotalTicks();
    }
    int nFillW = (int)(nBarW * fProgress);

    RECT rcFill = { nBarX, nProgY, nBarX + nFillW, nProgY + nBarH };
    HBRUSH brFill = CreateSolidBrush(RGB(145, 220, 230)); // 하늘색
    FillRect(hdc, &rcFill, brFill);
    DeleteObject(brFill);

    // 조작 안내
    SetTextColor(hdc, RGB(80, 80, 80));
    TextOut(hdc, 20, nProgY + 15,
        L"Space: Play/Pause   1~4: Speed   ESC: Exit", 43);

    SelectObject(hdc, oldFont);
    DeleteObject(font);
}

////////////////////////////////////////////
//  리플레이 저장
void CApp::SaveReplay()
{
    int nResult = MessageBox(m_hWnd,
        L"Save replay?", L"Replay", MB_YESNO | MB_ICONQUESTION);

    if (nResult == IDYES) {
        wchar_t szPath[MAX_PATH] = L"replay.rep";

        OPENFILENAME ofn = {};
        ofn.lStructSize = sizeof(ofn);
        ofn.hwndOwner = m_hWnd;
        ofn.lpstrFilter = L"Replay Files (*.rep)\0*.rep\0All Files\0*.*\0";
        ofn.lpstrFile = szPath;
        ofn.nMaxFile = MAX_PATH;
        ofn.lpstrDefExt = L"rep";
        ofn.Flags = OFN_OVERWRITEPROMPT;

        if (GetSaveFileName(&ofn)) {
            m_recorder.SaveToFile(szPath);
        }
    }
}

////////////////////////////////////////////////////
// 공용 함수
void CApp::DrawSession(HDC hdc, CGameSession& session, int nX, int nY)
{
    session.GetBoard().Draw(hdc, nX, nY);

    if (!session.IsGameOver()) {
        session.GetBoard().DrawGhost(hdc, session.GetCurBlock(), nX, nY);
        session.GetBoard().DrawBlock(hdc, session.GetCurBlock(), nX, nY);
    }
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

void CApp::DrawSidePanel(HDC hdc, CGameSession& session, int nX, int nY)
{
    int sx = nX;
    int sy = nY;

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
    SetTextColor(hdc, session.IsHoldUsed() ? RGB(100, 100, 100) : RGB(180, 180, 180));
    TextOut(hdc, sx, sy, L"HOLD", 4);

    RECT holdBg = { sx, sy + 25, sx + 100, sy + 105 };
    HBRUSH bgBr = CreateSolidBrush(RGB(25, 25, 25));
    FillRect(hdc, &holdBg, bgBr);
    DeleteObject(bgBr);

    if (session.GetHoldType() > 0)
        DrawBlockPreview(hdc, session.GetHoldType(), sx + 10, sy + 35, 20);

    // Next
    SetTextColor(hdc, RGB(180, 180, 180));
    TextOut(hdc, sx, sy + 120, L"NEXT", 4);

    RECT nextBg = { sx, sy + 145, sx + 100, sy + 225 };
    bgBr = CreateSolidBrush(RGB(25, 25, 25));
    FillRect(hdc, &nextBg, bgBr);
    DeleteObject(bgBr);

    DrawBlockPreview(hdc, session.GetNextBlock().GetType(), sx + 10, sy + 155, 20);

    // Score / Level / Lines
    //SetTextColor(hdc, RGB(180, 180, 180)); 
    SetTextColor(hdc, RGB(245, 230, 155));
    TextOut(hdc, sx, sy + 250, L"SCORE", 5);

    SelectObject(hdc, fontVal);
    SetTextColor(hdc, RGB(255, 255, 255));
    wchar_t buf[32];
    wsprintf(buf, L"%d", session.GetScore());
    TextOut(hdc, sx, sy + 275, buf, (int)wcslen(buf));

    SelectObject(hdc, fontTitle);
    SetTextColor(hdc, RGB(180, 180, 180));
    TextOut(hdc, sx, sy + 320, L"LEVEL", 5);

    SelectObject(hdc, fontVal);
    SetTextColor(hdc, RGB(255, 255, 255));
    wsprintf(buf, L"%d", session.GetLevel());
    TextOut(hdc, sx, sy + 345, buf, (int)wcslen(buf));

    SelectObject(hdc, fontTitle);
    SetTextColor(hdc, RGB(180, 180, 180));
    TextOut(hdc, sx, sy + 390, L"LINES", 5);

    SelectObject(hdc, fontVal);
    SetTextColor(hdc, RGB(255, 255, 255));
    wsprintf(buf, L"%d", session.GetLines());
    TextOut(hdc, sx, sy + 415, buf, (int)wcslen(buf));


    // 조작 키
    SelectObject(hdc, fontSmall);
    SetTextColor(hdc, RGB(100, 100, 100));

    int ky = sy + 450;

    TextOut(hdc, sx, ky,       L"<- ->  Move", 11); // 글자수 만큼
    TextOut(hdc, sx, ky + 18,  L"Up     Rotate", 13);
    TextOut(hdc, sx, ky + 36,  L"Down   Soft Drop", 16);
    TextOut(hdc, sx, ky + 54,  L"Space  Hard Drop", 16);
    TextOut(hdc, sx, ky + 72,  L"H      Hold", 11);
    TextOut(hdc, sx, ky + 90,  L"ESC    Exit", 12);

    SelectObject(hdc, oldFont);
    DeleteObject(fontTitle);
    DeleteObject(fontVal);
    DeleteObject(fontSmall);

}

void CApp::DrawGameOver(HDC hdc, int nBoardX, int nBoardY)
{
    int bx = nBoardX;
    int by = nBoardY;
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

    RECT rcSub1 = { bx, by + bh / 2 + 15, bx + bw, by + bh / 2 + 45 };
    DrawText(hdc, L"Press R to ReStart", -1, &rcSub1, DT_CENTER | DT_VCENTER | DT_SINGLELINE);
    RECT rcSub2 = { bx, by + bh / 2 + 15, bx + bw, by + bh / 2 + 90 };
    DrawText(hdc, L"Press ESC to Exit", -1, &rcSub2, DT_CENTER | DT_VCENTER | DT_SINGLELINE);

    SelectObject(hdc, oldFont);
    DeleteObject(font2);
}
