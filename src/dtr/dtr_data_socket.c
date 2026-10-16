/**
  \file   dtr_data_socket.c
  \author Aleksey Lekhtin
  \date   Mar 10, 2025
  \brief  Socket abstract operation
*/

#include <errno.h>
#include <stdio.h>
#include <strings.h>
#include <string.h>

#include "dbg.h"
#include "dtr.h"
#include "dtr_socket.h"
#include "log.h"

//---------------------------------------------------------
const socket_t *dtr_data_socket()
{
    return &dtr()->data.sock;
}

//---------------------------------------------------------
bool dtr_data_open_socket(void *obj)
{
    dtr_t *dtr = (dtr_t *) obj;
    dtr->data.sock.port = dtr->data.port;
    snprintf(dtr->data.sock.name, sizeof(dtr->data.sock.name), "dtr-data");
    DBGF(5, "[tid:%08lX] --- %s - port %d",
         pthread_self(), dtr->data.sock.name, dtr->data.sock.port);
    if (socket_open(&dtr->data.sock, SOCK_STREAM, IPPROTO_IP)) {
        if (socket_bind(&dtr->data.sock)) {
            if (socket_listen(&dtr->data.sock)) {
                return OK;
            }
        }
    }
    DBGF(0, "[tid:%08lX] ERROR: can't open port %d (%s)",
         pthread_self(), dtr->data.sock.port, socket_strerror(dtr->data.sock.err));
    socket_close_fd(&dtr->data.sock.fd);
    return FAIL;
}

//---------------------------------------------------------
void dtr_data_close_socket(void *obj)
{
    dtr_t *dtr = (dtr_t *) obj;
    socket_close_fd(&dtr->data.sock.fd);
}

//---------------------------------------------------------
int dtr_data_accept(void *obj)
{
    dtr_t *dtr = (dtr_t *) obj;
    DBGF(3, "[tid:%08lX] --- %s waiting on port %d",
         pthread_self(), dtr->data.sock.name, dtr->data.sock.port);
    int fd = socket_accept(&dtr->data.sock);
    if (fd != INVALID_SOCKET) {
        if (debug_level > 0) {
            char buf[NI_MAXHOST * 3] = {0};
            if (strcasecmp(dtr->data.sock.cli_ip, dtr->data.sock.cli_host) == 0) {
                strncpy(buf, dtr->data.sock.cli_host, sizeof(buf) - 1);
            } else {
                snprintf(buf, sizeof(buf), "%s '%s'", dtr->data.sock.cli_ip, dtr->data.sock.cli_host);
            }
            DBGF(1, "[tid:%08lX] --- request accepted (%d), cli_fd:%d (from '%s')",
                 pthread_self(), dtr->data.sock.port, fd, buf);
        }
    }
    return fd;
}

//---------------------------------------------------------
bool dtr_data_socket_send(const char *bytes, int nbytes)
{
    socket_t *sock = &dtr()->data.sock;
    int ret = socket_send(sock, bytes, nbytes);
    if (ret == -1) {
        int err = sock->err;
        if (err != EPIPE && err != ECONNRESET) { ; // TODO
        }
        LOG(ERROR, "%s/%d fd:%d - send failed - '%s'", sock->name, sock->port, sock->fd, socket_strerror(sock->err));
        DBGF(0, "%s/%d fd:%d - send failed - '%s'", sock->name, sock->port, sock->fd, socket_strerror(sock->err));
        return FAIL;
    }
    DBGF(3, "%s/%d fd:%d - sent %d bytes to %s:%d", sock->name, sock->port, sock->fd,
         ret, inet_ntoa(sock->sockaddrin.sin_addr), sock->port);
    return OK;
}

//---------------------------------------------------------
int dtr_data_socket_recv(char *buf, int nbuf)
{
    socket_t *sock = &dtr()->data.sock;
    DBGF(3, "%s/%d (cli_fd:%d) - waiting obj... ", sock->name, sock->port, sock->fd);
    int nread = socket_recv(sock, buf, nbuf);
    return nread;
}
