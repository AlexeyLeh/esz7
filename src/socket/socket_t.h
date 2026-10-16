/**
  \file   socket_t.h
  \author Aleksey Lekhtin
  \date   Feb 26, 2025
  \brief  trivial socket type definition
*/

#ifndef SOCKET_T_H_
#define SOCKET_T_H_

#ifdef WIN32
#include <winsock2.h>
#include <ws2tcpip.h>
#endif

#ifdef __linux__
#include <arpa/inet.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <netdb.h>

#define INVALID_SOCKET (-1)
#define SOCKET_ERROR (-1)
typedef int SOCKET;
typedef struct sockaddr SOCKADDR;
typedef struct sockaddr_in SOCKADDR_IN;
#define closesocket(fd) close(fd)
#endif // __linux__

#ifndef SHUT_RDWR
#define SHUT_RDWR SD_BOTH
#endif

enum {
    SOCKET_NAME_LEN = 64
};

typedef struct socket_t
{
    char        name[SOCKET_NAME_LEN];
    const char  *ip;
    uint16_t    port;
    int         err;
    int         type;           // socket type
    SOCKET      fd;
    char        cli_ip[64];
    char        cli_host[NI_MAXHOST];
    SOCKADDR_IN sockaddrin;
} socket_t;

#endif // SOCKET_T_H_
