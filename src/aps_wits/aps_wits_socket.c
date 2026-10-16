/**
  \file   aps_wits_socket.c
  \author Aleksey Lekhtin
  \date   Apr 01, 2024
  \brief  Socket abstract operation
*/

#include <stdio.h>

#include "aps_wits.h"
#include "aps_wits_socket.h"
#include "dbg.h"
#include "socket.h"
#include "log.h"

//-----------------------------------------------------------
bool aps_wits_open_socket(void *obj)
{
    aps_wits_t *wits = (aps_wits_t *) obj;
    wits->sock.port = wits->port;
    snprintf(wits->sock.name, sizeof(wits->sock.name), "wits");
    DBGF(5, "[tid:%08lX] --- %s - port %d",
         pthread_self(), wits->sock.name, wits->sock.port);
    if (socket_open(&wits->sock, SOCK_STREAM, IPPROTO_IP)) {
        if (socket_bind(&wits->sock)) {
            if (socket_listen(&wits->sock)) {
                return OK;
            }
        }
    }
    LOG(ERROR, "can't open port %d (%s)",
        wits->sock.port, socket_strerror(wits->sock.err));
    DBGF(0, "ERROR: can't open port %d (%s)",
         wits->sock.port, socket_strerror(wits->sock.err));
    socket_close_fd(&wits->sock.fd);
    return FAIL;
}

//---------------------------------------------------------
void aps_wits_close_socket(void *obj)
{
    aps_wits_t *wits = (aps_wits_t *) obj;
    if (wits->sock.fd != INVALID_SOCKET) {
        return;
    }
    DBG(5, "lock wits->mutex");
    pthread_mutex_lock(&wits->mutex);
    socket_close_fd(&wits->sock.fd);
    DBG(5, "unlock wits->mutex");
    pthread_mutex_unlock(&wits->mutex);
}

//---------------------------------------------------------
int aps_wits_accept(void *obj)
{
    aps_wits_t *wits = (aps_wits_t *) obj;
    DBGF(3, "--- waiting on port %d", wits->sock.port);
    DBGF(3, "[tid:%08lX] --- %s waiting on port %d",
         pthread_self(), wits->sock.name, wits->sock.port);
    int fd = socket_accept(&wits->sock);
    if (fd != INVALID_SOCKET) {
        if (debug_level > 0) {
            char buf[NI_MAXHOST * 3] = {0};
            if (strcasecmp(wits->sock.cli_ip, wits->sock.cli_host) == 0) {
                strncpy(buf, wits->sock.cli_host, sizeof(buf) - 1);
            } else {
                snprintf(buf, sizeof(buf), "%s '%s'", wits->sock.cli_ip, wits->sock.cli_host);
            }
            DBGF(1, "-- request accepted (%d), cli_fd:%d (from '%s')", wits->sock.port, fd, buf);
        }
    }
    return fd;
}
