/**
  \file   wsa.cpp
  \author Aleksey Lekhtin
  \date   Jan 27, 2025
  \brief  WSA procedures
*/

#include <stdio.h>

#ifdef __linux__
#include <errno.h>
#include <string.h>
#endif

#ifdef _WIN32
#include <winsock.h>
#endif

#include "defines.h"
#include "wsa.h"

#ifdef __cplusplus
namespace WSA {
#endif

//---------------------------------------------------------
bool wsa_startup()
{
#ifdef _WIN32
    WSADATA wsaData;
    const int ret = WSAStartup(MAKEWORD(2, 2), &wsaData);
    if (ret != 0) {
#ifdef __linux__
        int err = errno;
#endif
#ifdef _WIN32
        int err = WSAGetLastError();
#endif
        printf("%s \n", socket_strerror(err));
        printf("WSAStartup failed with error: %d \n", ret);
        return FAIL;
    }
#endif
    return OK;
}

//---------------------------------------------------------
void wsa_cleanup()
{
#ifdef _WIN32
    WSACleanup();
#endif
}

//---------------------------------------------------------
int set_socket_errno(socket_t *sock)
{
#ifdef __linux__
    sock->err = errno;
#endif
#ifdef _WIN32
    sock->err = WSAGetLastError();
#endif
    return sock->err;
}

//---------------------------------------------------------
void clear_socket_errno(socket_t *sock)
{
    sock->err = 0;
}

//---------------------------------------------------------
int socket_errno(socket_t *sock)
{
    return sock->err;
}

//---------------------------------------------------------
const char *socket_strerror(int err)
{
    static char buf[1024 * 4] = {0};
#ifdef __linux__
    snprintf(buf, sizeof(buf) - 1, "(%d) %s", err, strerror(err));
    return buf;
#endif

#ifdef _WIN32
    char *s = NULL;
    DWORD max_npnts = FormatMessageA(
            FORMAT_MESSAGE_ALLOCATE_BUFFER |
            FORMAT_MESSAGE_FROM_SYSTEM |
            FORMAT_MESSAGE_IGNORE_INSERTS,
            NULL, err, MAKELANGID(LANG_ENGLISH, SUBLANG_DEFAULT), (LPSTR) &s, 0, NULL);
    max_npnts = snprintf(innerbuf, sizeof(innerbuf) - 1, "(%d) %s", err, s);
    if (innerbuf[max_npnts - 1] == '\n') {
        innerbuf[max_npnts - 1] = '\0';
    }
    if (innerbuf[max_npnts - 2] == '\r') {
        innerbuf[max_npnts - 2] = '\0';
    }
    LocalFree(s);

    // s = NULL;
    // FormatMessageA(FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS,
    //                NULL, err, MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT), (LPSTR) &s, 0, NULL);
    // snprintf(innerbuf + strlen(innerbuf), sizeof(innerbuf)-strlen(innerbuf)-1, "%s", s);
    // if (innerbuf[0] == '\n') innerbuf[0] = '\0';
    // LocalFree(s);

    return innerbuf;
#endif
}

#ifdef __cplusplus
}
#endif
