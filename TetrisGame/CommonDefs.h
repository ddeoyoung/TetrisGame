#pragma once

// 게임 상태
enum EGameState {
    STATE_TITLE = 0,
    STATE_SINGLE,
    STATE_MULTI_LOBBY,
    STATE_MULTI_PLAY,
    STATE_REPLAY,
};

// 타이틀 메뉴
enum ETitleMenu {
    MENU_SINGLE = 0,
    MENU_MULTI,
    MENU_REPLAY,
    MENU_COUNT,
};

// 게임 입력 (틱 단위로 기록/전송)
enum EGameInput {
    INPUT_NONE = 0,
    INPUT_LEFT,
    INPUT_RIGHT,
    INPUT_DOWN,
    INPUT_ROTATE,
    INPUT_HARD_DROP,
    INPUT_HOLD,
};

// 멀티 로비 상태
enum ELobbyState {
    LOBBY_SELECT = 0,   // HOST / JOIN 선택
    LOBBY_HOSTING,      // 호스트 대기 중
    LOBBY_JOIN_IP,      // IP 입력 중
    LOBBY_CONNECTING,   // 연결 중
};
