/**
  \file   siu_cmd_socket.c
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
bool siu_cmd_open_socket(void *obj)
{
    siu_t *siu = (siu_t *) obj;
    siu->cmd.sock.port = siu->cmd.port;
    snprintf(siu->cmd.sock.name, sizeof(siu->cmd.sock.name), "siu-cmd");
    DBGF(5, "[tid:%08lX] --- %s - port %d",
        pthread_self(), siu->cmd.sock.name, siu->cmd.sock.port);
    if (socket_open(&siu->cmd.sock, SOCK_STREAM, IPPROTO_IP)) {
        if (socket_bind(&siu->cmd.sock)) {
            if (socket_listen(&siu->cmd.sock)) {
                return OK;
            }
        }
    }
    siu_cmd_close_socket(siu);
    return FAIL;
}

//---------------------------------------------------------
void siu_cmd_close_socket(void *obj)
{
    siu_t *siu = (siu_t *) obj;
    if (siu->cmd.sock.fd == INVALID_SOCKET) {
        return;
    }
    DBGF(5, "[tid:%08lX] lock siu->cmd.mutex", pthread_self());
    pthread_mutex_lock(&siu->cmd.mutex);
    socket_close_fd(&siu->cmd.sock.fd);
    DBGF(5, "[tid:%08lX] unlock siu->cmd.mutex", pthread_self());
    pthread_mutex_unlock(&siu->cmd.mutex);
}

//---------------------------------------------------------
int siu_cmd_accept(void *obj)
{
    siu_t *siu = (siu_t *) obj;
    DBGF(3, "[tid:%08lX] --- %s waiting on port %d",
        pthread_self(), siu->cmd.sock.name, siu->cmd.sock.port);
    int fd = socket_accept(&siu->cmd.sock);
    if (fd == INVALID_SOCKET) {
        return INVALID_SOCKET;
    }
    if (debug_level > 0) {
        char buf[NI_MAXHOST * 3] = {0};
        if (strcasecmp(siu->cmd.sock.cli_ip, siu->cmd.sock.cli_host) == 0) {
            strncpy(buf, siu->cmd.sock.cli_host, sizeof(buf) - 1);
        } else {
            snprintf(buf, sizeof(buf), "%s '%s'", siu->cmd.sock.cli_ip, siu->cmd.sock.cli_host);
        }
        DBGF(1, "[tid:%08lX] --- request accepted (%d), cli_fd:%d (from '%s')",
            pthread_self(), siu->cmd.sock.port, fd, buf);
    }
    return fd;
}

//---------------------------------------------------------
int siu_cmd_socket_send(socket_t *sock, const char *bytes, int nbytes)
{
    return socket_send(sock, bytes, nbytes);
}
