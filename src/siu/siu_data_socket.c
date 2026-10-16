/**
  \file   siu_data_socket.c
  \author Aleksey Lekhtin
  \date   Mar 10, 2025
  \brief  Socket abstract operation
*/

#include <stdio.h>
#include <strings.h>
#include <string.h>

#include "dbg.h"
#include "siu.h"
#include "siu_socket.h"

//---------------------------------------------------------
bool siu_data_open_socket(void *obj)
{
    siu_t *siu = (siu_t *) obj;
    siu->data.sock.port = siu->data.port;
    snprintf(siu->data.sock.name, sizeof(siu->data.sock.name), "siu-data");
    DBGF(5, "[tid:%08lX] --- %s - port %d",
        pthread_self(), siu->data.sock.name, siu->data.sock.port);
    if (socket_open(&siu->data.sock, SOCK_STREAM, IPPROTO_IP)) {
        if (socket_bind(&siu->data.sock)) {
            if (socket_listen(&siu->data.sock)) {
                return OK;
            }
        }
    }
    siu_data_close_socket(obj);
    return FAIL;
}

//---------------------------------------------------------
void siu_data_close_socket(void *obj)
{
    siu_t *siu = (siu_t *) obj;
    if (siu->data.sock.fd == INVALID_SOCKET) {
        return;
    }
    DBG(5, "lock siu->data.mutex");
    pthread_mutex_lock(&siu->data.mutex);
    socket_close_fd(&siu->data.sock.fd);
    DBG(5, "unlock siu->data.mutex");
    pthread_mutex_unlock(&siu->data.mutex);
}

//---------------------------------------------------------
int siu_data_accept(void *obj)
{
    siu_t *siu = (siu_t *) obj;
    DBGF(3, "[tid:%08lX] --- %s waiting on port %d",
        pthread_self(), siu->data.sock.name, siu->data.sock.port);
    int fd = socket_accept(&siu->data.sock);
    if (fd == INVALID_SOCKET) {
        return INVALID_SOCKET;
    }
    if (debug_level > 0) {
        char buf[NI_MAXHOST * 3] = {0};
        if (strcasecmp(siu->data.sock.cli_ip, siu->data.sock.cli_host) == 0) {
            strncpy(buf, siu->data.sock.cli_host, sizeof(buf) - 1);
        } else {
            snprintf(buf, sizeof(buf), "%s '%s'", siu->data.sock.cli_ip, siu->data.sock.cli_host);
        }
        DBGF(1, "[tid:%08lX] --- request accepted (%d), cli_fd:%d (from '%s')",
            pthread_self(), siu->data.sock.port, fd, buf);
    }
    return fd;
}
