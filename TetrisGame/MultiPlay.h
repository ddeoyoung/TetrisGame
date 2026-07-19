#pragma once

#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif

#include <winsock2.h>
#include <ws2tcpip.h>
#pragma comment(lib, "ws2_32.lib")

#include "CommonDefs.h"

// 연결 상태
enum ENetState {
    NET_IDLE,
    NET_LISTENING,
    NET_CONNECTING,
    NET_CONNECTED,
    NET_DISCONNECTED,
};

class CMultiPlay
{
public:
    CMultiPlay();
    ~CMultiPlay();

    bool InitWinsock();
    void Cleanup();

    // 호스트 (서버 역할)
    bool HostGame(int nPort);
    bool AcceptClient();

    // 클라이언트
    bool JoinGame(const char* szIP, int nPort);

    // 송수신
    bool SendPacket(const void* pData, int nSize);
    bool RecvPacket(void* pBuf, int nSize);
    bool HasData();

    ENetState GetState(){ return m_eState; }
    bool IsHost() { return m_bHost; }


private:
    SOCKET m_sockListen = INVALID_SOCKET;
    SOCKET m_sockGame = INVALID_SOCKET;
    ENetState m_eState = NET_IDLE;

    bool m_bHost = false;
    bool m_bWinsockInit = false;
};
