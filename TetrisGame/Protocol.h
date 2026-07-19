#pragma once
#include <cstdint>
#include "CommonDefs.h"

#pragma pack(push, 1)

enum EPacketType : uint8_t {
    PKT_NONE = 1,
    PKT_GAME_START,
    PKT_TICK_INPUT,
    PKT_GAME_OVER,
};

struct TPacketHeader {
    EPacketType eType;
    uint16_t nSize;
};

// 게임 시작 (호스트 -> 클라, 시드 공유)
struct TPacketGameStart {
    TPacketHeader header;
    uint32_t dwSeed;
};

// 틱 입력 (호스트 <-> 클라)
struct TPacketTickInput {
    TPacketHeader header;
    uint32_t nTick;
    EGameInput eInput;
};

// 게임오버
struct TPacketGameOver {
    TPacketHeader header;
    uint32_t nTick;
};

#pragma pack(pop)
