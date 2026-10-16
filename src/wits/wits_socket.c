//
// Created by LAV on 25.08.2026.
//

#include "dbg.h"
#include "log.h"
#include "socket.h"
#include "wits.h"

//-----------------------------------------------------------
bool wits_open_socket(socket_t *sock)
{
    DBGF(5, "[tid:%08lX] --- %s - port %d",
         pthread_self(), sock->name, sock->port);
    if (socket_open(sock, SOCK_STREAM, IPPROTO_IP)) {
        if (socket_bind(sock)) {
            if (socket_listen(sock)) {
                return OK;
            }
        }
    }
    LOG(ERROR, "can't open port %d (%s)",
        sock->port, socket_strerror(sock->err));
    DBGF(0, "ERROR: can't open port %d (%s)",
         sock->port, socket_strerror(sock->err));
    socket_close_fd(&sock->fd);
    return FAIL;
}

//---------------------------------------------------------
int wits_accept(socket_t *sock)
{
    DBGF(3, "--- waiting on port %d", sock->port);
    DBGF(3, "[tid:%08lX] --- %s waiting on port %d",
         pthread_self(), sock->name, sock->port);
    const int fd = socket_accept(sock);
    if (fd != INVALID_SOCKET) {
        if (debug_level > 0) {
            char buf[NI_MAXHOST * 3] = {0};
            if (strcasecmp(sock->cli_ip, sock->cli_host) == 0) {
                strncpy(buf, sock->cli_host, sizeof(buf) - 1);
            } else {
                snprintf(buf, sizeof(buf), "%s '%s'", sock->cli_ip, sock->cli_host);
            }
            DBGF(1, "-- request accepted (%d), cli_fd:%d (from '%s')", sock->port, fd, buf);
        }
    }
    return fd;
}
