#include "MultiPlay.h"

CMultiPlay::CMultiPlay() {}

CMultiPlay::~CMultiPlay() { Cleanup(); }

bool CMultiPlay::InitWinsock()
{
    if (m_bWinsockInit) return true;

    WSADATA wsa;
    if (WSAStartup(MAKEWORD(2, 2), &wsa) != 0)
        return false;

    m_bWinsockInit = true;

    return true;
}

void CMultiPlay::Cleanup()
{
    if (m_sockGame != INVALID_SOCKET) {
        closesocket(m_sockGame);
        m_sockGame = INVALID_SOCKET;
    }

    if (m_sockListen != INVALID_SOCKET) {
        closesocket(m_sockListen);
        m_sockListen = INVALID_SOCKET;
    }

    if (m_bWinsockInit) {
        WSACleanup();
        m_bWinsockInit = false;
    }
    m_eState = NET_IDLE;
}

bool CMultiPlay::HostGame(int nPort)
{
    m_bHost = true;

    // AF_INET : IPv4
    // IPv45 주소를 쓰는 TCP 소켓을 만들기
    m_sockListen = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);

    if (m_sockListen == INVALID_SOCKET) {
        return false;
    }

    // SO_REUSEADDR: 포트 재사용 허용 옵션 (바인드 실패하지 않게, 빠른 재시작)
    int opt = 1; // 켜기
    setsockopt(m_sockListen, SOL_SOCKET, SO_REUSEADDR, (char*)&opt, sizeof(opt));

    // INADDR_ANY : 모든 경로에서 받기
    // 
    sockaddr_in addr = {};
    addr.sin_family = AF_INET;
    addr.sin_port = htons((u_short)nPort);
    addr.sin_addr.s_addr = INADDR_ANY;


    if (bind(m_sockListen, (sockaddr*)&addr, sizeof(addr)) == SOCKET_ERROR) {
        closesocket(m_sockListen);
        m_sockListen = INVALID_SOCKET;
        return false;
    }

    if (listen(m_sockListen, 1) == SOCKET_ERROR) {
        closesocket(m_sockListen);
        m_sockListen = INVALID_SOCKET;
        return false;
    }

    // ioctlsocket(소켓, 명령, 값)
    // FIONBIO : 논블로킹 모드
    u_long mode = 1;
    ioctlsocket(m_sockListen, FIONBIO, &mode);

    m_eState = NET_LISTENING;
    return true;
}

bool CMultiPlay::AcceptClient()
{
    if (m_eState != NET_LISTENING) return false;

    m_sockGame = accept(m_sockListen, nullptr, nullptr);
    if (m_sockGame == INVALID_SOCKET)
        return false;

    // 대기 소켓 정리
    closesocket(m_sockListen);
    m_sockListen = INVALID_SOCKET;

    // 게임 소켓 논블로킹
    u_long mode = 1;
    ioctlsocket(m_sockGame, FIONBIO, &mode);

    // Nagle 비활성화 (지연 최소화)
    int opt = 1;
    setsockopt(m_sockGame, IPPROTO_TCP, TCP_NODELAY, (char*)&opt, sizeof(opt));

    m_eState = NET_CONNECTED;

    return true;
}

bool CMultiPlay::JoinGame(const char* szIP, int nPort)
{
    m_bHost = false;

    m_sockGame = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (m_sockGame == INVALID_SOCKET)
        return false;

    sockaddr_in addr = {};
    addr.sin_family = AF_INET;
    addr.sin_port = htons((u_short)nPort);
    inet_pton(AF_INET, szIP, &addr.sin_addr);

    if (connect(m_sockGame, (sockaddr*)&addr, sizeof(addr)) == SOCKET_ERROR) {
        closesocket(m_sockGame);
        m_sockGame = INVALID_SOCKET;

        return false;
    }

    // 논블로킹 모드 FIONBIO
    u_long mode = 1;
    ioctlsocket(m_sockGame, FIONBIO, &mode);

    int opt = 1;
    setsockopt(m_sockGame, IPPROTO_TCP, TCP_NODELAY, (const char*)&opt, sizeof(opt));

    m_eState = NET_CONNECTED;
    return true;
}

bool CMultiPlay::SendPacket(const void* pData, int nSize)
{
    if (m_sockGame == INVALID_SOCKET) return false;

    int nSent = 0;
    while (nSent < nSize) {
        int n = send(m_sockGame, (const char*)pData + nSent, nSize - nSent, 0);

        if (n == SOCKET_ERROR) {
            int err = WSAGetLastError();

            // 블로킹이었다면 막혔을 상황 / 재시도
            if (err == WSAEWOULDBLOCK) continue;
                
            m_eState = NET_DISCONNECTED;
            return false;
        }
        nSent += n;
    }

    return true;
}

bool CMultiPlay::RecvPacket(void* pBuf, int nSize)
{
    if (m_sockGame == INVALID_SOCKET) return false;

    int nRecv = 0;
    while (nRecv < nSize) {
        int n = recv(m_sockGame, (char*)pBuf + nRecv, nSize - nRecv, 0);
        if (n == SOCKET_ERROR) {
            int err = WSAGetLastError();

            if (err == WSAEWOULDBLOCK)
                return false;

            m_eState = NET_DISCONNECTED;
            return false;
        }
        if (n == 0) {
            m_eState = NET_DISCONNECTED;
            return false;
        }

        nRecv += n;
    }

    return true;
}

bool CMultiPlay::HasData()
{
    if (m_sockGame == INVALID_SOCKET) return false;

    fd_set readSet;
    FD_ZERO(&readSet);
    FD_SET(m_sockGame, &readSet);

    timeval tv = { 0, 0 };
    return select(0, &readSet, nullptr, nullptr, &tv) > 0;
}
