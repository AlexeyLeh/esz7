/**
  \file   dtr_cmd_socket.c
  \author Aleksey Lekhtin
  \date   Mar 10, 2025
  \brief  Socket abstract operation
*/

#include <stdio.h>
#include <strings.h>
#include <string.h>

#include "dbg.h"
#include "dtr.h"
#include "log.h"

//---------------------------------------------------------
const socket_t *dtr_cmd_socket()
{
    return &dtr()->cmd.sock;
}

//---------------------------------------------------------
bool dtr_cmd_open_socket(void *obj)
{
    dtr_t *dtr = (dtr_t *) obj;
    dtr->cmd.sock.port = dtr->cmd.port;
    snprintf(dtr->cmd.sock.name, sizeof(dtr->cmd.sock.name), "dtr-cmd");
    DBGF(5, "[tid:%08lX] --- %s - port %d",
         pthread_self(), dtr->cmd.sock.name, dtr->cmd.sock.port);
    if (socket_open(&dtr->cmd.sock, SOCK_STREAM, IPPROTO_IP)) {
        if (socket_bind(&dtr->cmd.sock)) {
            if (socket_listen(&dtr->cmd.sock)) {
                return OK;
            }
        }
    }
    LOG(ERROR, "can't open port %d (%s)", dtr->cmd.sock.port, socket_strerror(dtr->cmd.sock.err));
    DBGF(0, "ERROR: can't open port %d (%s)", dtr->cmd.sock.port, socket_strerror(dtr->cmd.sock.err));
    socket_close_fd(&dtr->cmd.sock.fd);
    return FAIL;
}

//---------------------------------------------------------
void dtr_cmd_close_socket(void *obj)
{
    DBG(8);
    dtr_t *dtr = (dtr_t *) obj;
    socket_close_fd(&dtr->cmd.sock.fd);
}

//---------------------------------------------------------
void dtr_cmd_cli_socket_close()
{
    DBGF(8, "socket_close_fd(cli_fd:%d)", dtr()->cmd.sock.fd);
    socket_close_fd(&dtr()->cmd.sock.fd);
}

//---------------------------------------------------------
int dtr_cmd_accept(void *obj)
{
    dtr_t *dtr = (dtr_t *) obj;
    DBGF(3, "[tid:%08lX] --- %s waiting on port %d",
         pthread_self(), dtr->cmd.sock.name, dtr->cmd.sock.port);
    int fd = socket_accept(&dtr->cmd.sock);
    if (fd == INVALID_SOCKET) {
        return -1;
    }
    if (debug_level > 0) {
        char buf[NI_MAXHOST * 3] = {0};
        if (strcasecmp(dtr->cmd.sock.cli_ip, dtr->cmd.sock.cli_host) == 0) {
            strncpy(buf, dtr->cmd.sock.cli_host, sizeof(buf) - 1);
        } else {
            snprintf(buf, sizeof(buf), "%s '%s'", dtr->cmd.sock.cli_ip, dtr->cmd.sock.cli_host);
        }
        DBGF(2, "[tid:%08lX] -- request accepted (%d), cli_fd:%d (from '%s')",
             pthread_self(), dtr->cmd.sock.port, fd, buf);
    }
    return fd;
}

//---------------------------------------------------------
int dtr_cmd_socket_send(socket_t *sock, const char *bytes, int nbytes)
{
    return socket_send(sock, bytes, nbytes);
}

//---------------------------------------------------------
int dtr_cmd_socket_recv(char *buf, int nbuf)
{
    socket_t *sock = &dtr()->cmd.sock;
    int nread = socket_recv(sock, buf, nbuf);
    return nread;
}
