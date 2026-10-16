/**
  \file   atlas_socket.c
  \author Aleksey Lekhtin
  \date   Apr 01, 2024
  \brief  Socket abstract operation
*/

#include <stdio.h>

#include "atlas.h"
#include "atlas_socket.h"
#include "dbg.h"
#include "socket.h"
#include "log.h"

//---------------------------------------------------------
bool atlas_open_udp_socket()
{
    bool ok;
    for (size_t i = 0; i < atlas()->hosts_size; i++) {
        host_t *h = &atlas()->hosts[i];

        if (h->host && h->host[0] != 0) {
            if (h->port1 > 0) {
                DBGF(1, "\ncreate UDP socket for %s:%d", h->host, h->port1);
                h->sock1.ip = h->host;
                h->sock1.port = h->port1;
                snprintf(h->sock1.name, sizeof(h->sock1.name), "atlas-%d-%d", i + 1, h->port1);
                ok = socket_open(&h->sock1, SOCK_DGRAM, IPPROTO_UDP);
                if (ok) {
                    LOG(INFO, "Atlas started for %s:%d", h->host, h->port1);
                }
                // return FAIL;
            }

            if (h->port2 > 0) {
                DBGF(1, "create UPD socket for %s:%d", h->host, h->port2);
                h->sock2.ip = h->host;
                h->sock2.port = h->port2;
                snprintf(h->sock2.name, sizeof(h->sock2.name), "atlas-%d-%d", i + 1, h->port2);
                ok = socket_open(&h->sock2, SOCK_DGRAM, IPPROTO_UDP);
                if (ok) {
                    LOG(INFO, "Atlas started for %s:%d", h->host, h->port2);
                }
                // return FAIL;
            }
        }
    }
    return OK;
}

/*---------------------------------------------------------*/
void atlas_socket_close()
{
    DBG(4);
    for (size_t i = 0; i < atlas()->hosts_size; i++) {
        host_t *h = &atlas()->hosts[i];
        socket_close_fd(&h->sock1.fd);
        socket_close_fd(&h->sock2.fd);
    }
}

/*---------------------------------------------------------*/
bool atlas_socket_send(const char *bytes, int nbytes)
{
    static bool sent_ok;
    sent_ok = YES;
    for (size_t i = 0; i < atlas()->hosts_size; i++) {
        host_t *h = &atlas()->hosts[i];
        if (!h->host || h->host[0] == '\0') {
            continue;
        }
        if (h->sock1.fd != INVALID_SOCKET) {
            socket_t *sock = &h->sock1;
            int ret = socket_send(sock, bytes, nbytes);
            if (ret == -1) {
                sent_ok = NO;
                LOG(CRITICAL, "%s (fd:%d, cli_fd:%d) - send failed - '%s'", sock->name, sock->fd, sock->fd,
                    socket_strerror(sock->err));
                DBGF(0, "%s (fd:%d, cli_fd:%d) - send failed - '%s'", sock->name, sock->fd, sock->fd,
                     socket_strerror(sock->err));
            }
        }

        if (h->sock2.fd != INVALID_SOCKET) {
            socket_t *sock = &h->sock2;
            int ret = socket_send(sock, bytes, nbytes);
            if (ret == -1) {
                sent_ok = NO;
                LOG(CRITICAL, "%s (fd:%d, cli_fd:%d) - send failed - '%s'", sock->name, sock->fd, sock->fd,
                    socket_strerror(sock->err));
                DBGF(0, "%s (fd:%d, cli_fd:%d) - send failed - '%s'", sock->name, sock->fd, sock->fd,
                     socket_strerror(sock->err));
            }
        }
    }
    return sent_ok;
}
