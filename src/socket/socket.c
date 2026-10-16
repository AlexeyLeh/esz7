/**
  \file   socket.c
  \author Aleksey Lekhtin
  \date   Feb 28, 2005
  \brief  Socket operations
*/

#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <errno.h>

#include "dbg.h"
#include "socket.h"
#include "log.h"

//-----------------------------------------------------------
static bool set_socket_opts(socket_t *sock)
{
#ifdef __linux__
    const int enable = 1;
#endif
#ifdef _WIN32
    DWORD enable = 1;
#endif
    socklen_t optlen = sizeof(enable);
    int ret = setsockopt(sock->fd, SOL_SOCKET, SO_REUSEADDR, (char *) &enable, optlen);
    if (ret < 0) {
        set_socket_errno(sock);
        LOG(ERROR, "%d - setsockopt(SO_REUSEADDR) error (%s)", sock->fd, socket_strerror(sock->err));
        DBGF(0, "ERROR: %d - setsockopt(SO_REUSEADDR) error (%s)", sock->fd, socket_strerror(sock->err));
        return FAIL;
    }
    DBGF(3, "%d - setsockopt(SO_REUSEADDR) to %d", sock->fd, enable);

    //-------------------------------------------
#ifdef __linux__
    const int keepalive = 0;
#endif
#ifdef _WIN32
    DWORD keepalive = 0;
#endif
    optlen = sizeof(keepalive);
    ret = setsockopt(sock->fd, SOL_SOCKET, SO_KEEPALIVE, (char *) &keepalive, optlen);
    if (ret < 0) {
        set_socket_errno(sock);
        DBGF(0, "ERROR: %d - setsockopt(SO_KEEPALIVE) error (%s)", sock->fd, socket_strerror(sock->err));
        return FAIL;
    }
    DBGF(3, "%d - setsockopt(SO_KEEPALIVE) to %d", sock->fd, keepalive);

    //-------------------------------------------
#ifdef __linux__
    const int dontroute = 1;
#endif
#ifdef _WIN32
    DWORD dontroute = 1;
#endif
    optlen = sizeof(enable);
    ret = setsockopt(sock->fd, SOL_SOCKET, SO_DONTROUTE, (char *) &dontroute, optlen);
    if (ret < 0) {
        set_socket_errno(sock);
        LOG(ERROR, "%d - setsockopt(SO_DONTROUTE) error (%s)", sock->fd, socket_strerror(sock->err));
        DBGF(0, "ERROR: %d - setsockopt(SO_DONTROUTE) error (%s)", sock->fd, socket_strerror(sock->err));
        return FAIL;
    }
    DBGF(3, "%d - setsockopt(SO_DONTROUTE) to %d", sock->fd, dontroute);
    return OK;
}

//-----------------------------------------------------------
/**
 * Open either TCP or UDP socket
 * @param sock - the socket info
 * @param type - the socket type
 * - SOCK_STREAM
 * - SOCK_DGRAM
 * @param proto - IP protocols (IPPROTO_IP or IPPROTO_UDP)
 * @return OK - FAIL
 */
bool socket_open(socket_t *sock, int type, int proto)
{
    DBGF(1, "[tid:%08lX] --- %s - '%s:%d', type: %s (%d), protocol: %s (%d)", pthread_self(),
         sock->name, sock->ip,
         sock->port, socket_type(type), type, socket_proto(proto), proto);
    socket_close_fd(&sock->fd);

    sock->fd = socket(AF_INET, type, proto);
    if (sock->fd == INVALID_SOCKET || sock->fd == 0) {
        set_socket_errno(sock);
        LOG(ERROR, "%s - %s", sock->name, socket_strerror(sock->err));
        DBGF(0, "\nERROR: %s - %s\n", sock->name, socket_strerror(sock->err));
        return FAIL;
    }
    sock->type = type;
    DBGF(1, "[tid:%08lX] --- socket(AF_INET(%d), %d, %d) => %d", pthread_self(), AF_INET, type, proto, sock->fd);

    if (!set_socket_opts(sock)) {
        return FAIL;
    }

    sock->sockaddrin.sin_family = AF_INET;
    sock->sockaddrin.sin_port = htons(sock->port);
    sock->sockaddrin.sin_addr.s_addr = INADDR_ANY;
    DBGF(4, "[tid:%08lX] --- socket(%d, %d, %d) => %d", pthread_self(), AF_INET, type, proto, sock->fd);
    DBGF(4, "[tid:%08lX] --- sockaddrin:        0x%08X", pthread_self(), (uintptr_t) &sock->sockaddrin);
    DBGF(4, "[tid:%08lX] ---  .sin_addr:        0x%08X", pthread_self(), (uintptr_t) &sock->sockaddrin.sin_addr);
    DBGF(4, "[tid:%08lX] ---  .sin_addr.s_addr: 0x%08X", pthread_self(), sock->sockaddrin.sin_addr.s_addr);

    if (type == SOCK_DGRAM) {
        if (!sock->ip || sock->ip[0] == 0 || strcasecmp(sock->ip, "0.0.0.0") == 0) {
            DBGF(0, "\nERROR: invalid ip address '%s'\n", sock->ip);
            socket_close_fd(&sock->fd);
            return FAIL;
        }

        // Привязываем UDP-сокет к порту
        // Для UDP-сервера bind обязателен, чтобы ОС не генерировала случайный исходящий порт.
        DBGF(3, "[tid:%08lX] --- bind(%d, 0x%08X, %u)", pthread_self(), sock->fd, (uintptr_t) &sock->sockaddrin,
             sizeof(sock->sockaddrin));
        if (bind(sock->fd, (struct sockaddr *) &sock->sockaddrin, sizeof(sock->sockaddrin)) < 0) {
            set_socket_errno(sock);
            LOG(ERROR, "[tid:%08lX] %s - bind failed: %s (port: %d)", pthread_self(), sock->name,
                 socket_strerror(sock->err), sock->port);
            DBGF(0, "[tid:%08lX] %s - bind failed: %s (port: %d)", pthread_self(), sock->name,
                 socket_strerror(sock->err), sock->port);
            socket_close_fd(&sock->fd);
            return FAIL;
        }

        int rc = inet_pton(AF_INET, sock->ip, &sock->sockaddrin.sin_addr);
        if (rc != 1) {
            set_socket_errno(sock);
            LOG(ERROR, "invalid IPv4 network address: %s (%s)", sock->ip, socket_strerror(sock->err));
            DBGF(0, "\nERROR: invalid IPv4 network address: %s (%s)\n", sock->ip, socket_strerror(sock->err));
            socket_close_fd(&sock->fd);
            return FAIL;
        }
    }

    const char *s = inet_ntoa(sock->sockaddrin.sin_addr);
    DBGF(1, "[tid:%08lX] --- fd:%d, ip: %s (%s), bound to port:%d",
         pthread_self(), sock->fd, (sock->ip ? sock->ip : "INADDR_ANY"), s, sock->port);
    return OK;
}

//-----------------------------------------------------------
void socket_close_fd(SOCKET *fd)
{
    pthread_t tid = pthread_self();
    if (fd && *fd != INVALID_SOCKET) {
        if (*fd == 1 || *fd == 2) {
            return; // :)
        }
        DBGF(8, "[tid:%08lX] fd:%d", tid, *fd);
        int ret = shutdown(*fd, SHUT_RDWR);
        if (ret == -1) {
            int e = errno;
            if (e != ENOTCONN) {
                LOG(ERROR, "socket shutdown(%d) error (%d) %s", *fd, e, strerror(e));
                DBGF(0, "ERROR: socket shutdown(%d) error (%d) %s", *fd, e, strerror(e));
            }
        }
        ret = closesocket(*fd);
        if (ret == -1) {
            int e = errno;
            LOG(ERROR, "socket close(%d) error (%d) %s", *fd, e, strerror(e));
            DBGF(0, "ERROR: socket close(%d) error (%d) %s", *fd, e, strerror(e));
        }
        *fd = INVALID_SOCKET;
        DBGF(9, "fd:%d", *fd);
        return;
    }
    DBGF(8, "fd:%d", *fd);
}

//-----------------------------------------------------------
bool socket_bind(socket_t *sock)
{
    const char *s = inet_ntoa(sock->sockaddrin.sin_addr);
    DBGF(8, "%s - bind(fd:%d, %s:%d)", sock->name, sock->fd, (s ? s : "?.?.?.?"), sock->port);
    int ret = bind(sock->fd, (SOCKADDR *) &sock->sockaddrin, sizeof(SOCKADDR));
    if (ret == SOCKET_ERROR) {
        set_socket_errno(sock);
        LOG(ERROR, "%s: %s:%d - %s", sock->name, (s ? s : "?.?.?.?"), sock->port, socket_strerror(sock->err));
        DBGF(0, "ERROR: %s: %s:%d - %s", sock->name, (s ? s : "?.?.?.?"), sock->port, socket_strerror(sock->err));
        return FAIL;
    }

    return OK;
}

//-----------------------------------------------------------
bool socket_listen(socket_t *sock)
{
    DBGF(8, "%s - on port %d", sock->name, sock->port);
    int ret = listen(sock->fd, 20);
    if (ret == SOCKET_ERROR) {
        set_socket_errno(sock);
        LOG(ERROR, "%s %s", sock->name, socket_strerror(sock->err));
        DBGF(0, "ERROR: %s %s\n", sock->name, socket_strerror(sock->err));
        return FAIL;
    }
    return OK;
}

//-----------------------------------------------------------
int socket_accept(socket_t *sock)
{
    pthread_t tid = pthread_self();
    DBGF(4, "[tid:%08lX] %s waiting on %d (fd:%d)", tid, sock->name, sock->port, sock->fd);
    socklen_t fromlen = sizeof(SOCKADDR);

    int fd = accept(sock->fd, (SOCKADDR *) &sock->sockaddrin, &fromlen);

    if (fd == INVALID_SOCKET || fd == 0) {
        set_socket_errno(sock);
        DBGF(2, "[tid:%08lX] %s:%d fd:%d - got event: %s",
             tid, sock->name, sock->port, fd, socket_strerror(sock->err));
        return INVALID_SOCKET;
    }
    const char *s = inet_ntoa(sock->sockaddrin.sin_addr);
    strncpy(sock->cli_ip, (s ? s : "-.-.-.-"), sizeof(sock->cli_ip) - 1);

    sock->cli_host[0] = 0;
    int ret = getnameinfo((SOCKADDR *) &sock->sockaddrin, sizeof(SOCKADDR_IN), sock->cli_host, NI_MAXHOST, NULL, 0, 0);
    if (debug_level > 0) {
        if (ret == 0) {
            DBGF(4, "\n[tid:%08lX] %s accepted on socket:%d from %s %s",
                 tid, sock->name, sock->fd, sock->cli_ip, sock->cli_host);
        } else {
            DBGF(1, "\n[tid:%08lX] %s - getnameinfo issue - %s\n",
                 tid, sock->name, gai_strerror(ret));
        }
    }
    return fd;
}

//-----------------------------------------------------------
int socket_recv(socket_t *sock, char *buf, int buflen)
{
    DBGF(8, "--- %s (socket:%d)\n", sock->name, sock->fd);
    int nread = (int) recv(sock->fd, buf, buflen, 0);
    DBGF(5, "%s (socket:%d) - return %d", sock->name, sock->fd, nread);
    if (nread == -1) {
        set_socket_errno(sock);
        DBGF(1, "%s (socket:%d) - ERROR: %s", sock->name, sock->fd, socket_strerror(sock->err));
        return -1;
    }
    if (nread == 0) {
        set_socket_errno(sock);
        if (sock->err != 0) {
            if (sock->err != ENOENT) {
                DBGF(2, "%s (socket:%d) - ERROR: no obj - %s", sock->name, sock->fd, socket_strerror(sock->err));
            }
        }
        return 0;
    }
    return nread;
}

//-----------------------------------------------------------
int socket_send(socket_t *sock, const char *data, size_t datalen)
{
    if (!data || datalen == 0) {
        LOG(ERROR, "%s (socket:%d) - WARNING: no obj sent", sock->name, sock->fd);
        DBGF(0, "%s (socket:%d) - WARNING: no obj sent", sock->name, sock->fd);
        sock->err = EINVAL;
        return -1;
    }
    if (sock->fd == INVALID_SOCKET || sock->fd == 0) {
        sock->err = EINVAL;
        LOG(ERROR, "%s (socket:%d) - socket is invalid", sock->name, sock->fd);
        DBGF(0, "%s (socket:%d) - socket is invalid", sock->name, sock->fd);
        return -1;
    }

    int sent_bytes = 0;
    int ret = 0;
    size_t len = datalen;
    const char *p = data;
    do {
        p += ret;
        len -= ret;
        if (sock->type == SOCK_DGRAM) {
            // UDP
            DBGF(5, "sendto(%d, '%s', len: %u) [UDP]", sock->fd, sock->ip, len);
            ret = sendto(sock->fd, p, len, MSG_NOSIGNAL, (SOCKADDR *) &sock->sockaddrin,
                         (socklen_t) sizeof(sock->sockaddrin));
        } else if (sock->type == SOCK_STREAM) {
            // TCP
            DBGF(5, "send(%d, '%s', len: %u) [TCP]", sock->fd, sock->cli_ip, len);
            ret = send(sock->fd, p, len, MSG_NOSIGNAL);
        } else {
            sock->err = EINVAL;
            DBGF(5, "500: INTERNAL ERROR: unsupported socket type %d (%s)", sock->type, socket_type(sock->type));
            return -1;
        }
        if (ret == SOCKET_ERROR) {
            set_socket_errno(sock);
            return -1;
        }
        if ((size_t) ret != len) {
            DBGF(5, "%s (socket:%d) - sent %d bytes", sock->name, sock->fd, ret);
        }
        sent_bytes += ret;
    } while ((size_t) ret != len);

    DBGF(4, "%s (socket:%d) - sent %d bytes to %s:%d", sock->name, sock->fd, ret, inet_ntoa(sock->sockaddrin.sin_addr),
         sock->port);
    return sent_bytes;
}

//-----------------------------------------------------------
const char *socket_type(int type)
{
    if (type == SOCK_STREAM) {
        return "SOCK_STREAM";
    }
    if (type == SOCK_DGRAM) {
        return "SOCK_DGRAM";
    }
    return "UNKN";
}

//-----------------------------------------------------------
const char *socket_proto(int proto)
{
    if (proto == IPPROTO_IP) {
        return "IPPROTO_IP";
    }
#ifdef _WIN32
    if (proto == IPPROTO_IPV4) return "IPPROTO_IPV4";
#endif
    if (proto == IPPROTO_IPV6) {
        return "IPPROTO_IPV6";
    }
    if (proto == IPPROTO_UDP) {
        return "IPPROTO_UDP";
    }
    return "UNKN";
}
