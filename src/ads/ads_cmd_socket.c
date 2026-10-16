/**
  \file   ads_cmd_socket.c
  \author Aleksey Lekhtin
  \date   Mar 10, 2025
  \brief  Socket abstract operation
*/

#include <stdio.h>
#include <strings.h>
#include <string.h>

#include "ads.h"
#include "ads_socket.h"
#include "dbg.h"
#include "log.h"

//---------------------------------------------------------
bool ads_cmd_open_socket(void *obj)
{
    ads_t *ads = (ads_t *) obj;
    ads->cmd.sock.port = ads->cmd.port;
    snprintf(ads->cmd.sock.name, sizeof(ads->cmd.sock.name), "ads-cmd");
    DBGF(3, "[tid:%08lX] --- %s waiting on port %d",
         pthread_self(), ads->cmd.sock.name, ads->cmd.sock.port);
    if (socket_open(&ads->cmd.sock, SOCK_STREAM, IPPROTO_IP)) {
        if (socket_bind(&ads->cmd.sock)) {
            if (socket_listen(&ads->cmd.sock)) {
                return OK;
            }
        }
    }
    LOG(ERROR, "can't open port %d (%s)", ads->cmd.sock.port, socket_strerror(ads->cmd.sock.err));
    DBGF(0, "ERROR: can't open port %d (%s)", ads->cmd.sock.port, socket_strerror(ads->cmd.sock.err));
    socket_close_fd(&ads->cmd.sock.fd);
    return FAIL;
}

//---------------------------------------------------------
void ads_cmd_close_socket(void *obj)
{
    ads_t *ads = (ads_t *) obj;
    if (ads->cmd.sock.fd == INVALID_SOCKET) {
        return;
    }
    DBG(5, "lock ads->cmd.mutex");
    pthread_mutex_lock(&ads->cmd.mutex);
    socket_close_fd(&ads->cmd.sock.fd);
    DBG(5, "unlock ads->cmd.mutex");
    pthread_mutex_unlock(&ads->cmd.mutex);
}

//---------------------------------------------------------
void ads_set_cmd_cli_socket(int fd)
{
    DBGF(8, "cli_fd:%d", fd);
    ads()->cmd.sock.fd = fd;
}

//---------------------------------------------------------
int ads_cmd_accept(void *obj)
{
    ads_t *ads = (ads_t *) obj;
    DBGF(3, "[tid:%08lX] --- %s waiting on port %d",
         pthread_self(), ads->cmd.sock.name, ads->cmd.sock.port);
    int fd = socket_accept(&ads->cmd.sock);
    if (fd != INVALID_SOCKET) {
        if (debug_level > 0) {
            char buf[NI_MAXHOST * 3] = {0};
            if (strcasecmp(ads->cmd.sock.cli_ip, ads->cmd.sock.cli_host) == 0) {
                strncpy(buf, ads->cmd.sock.cli_host, sizeof(buf) - 1);
            } else {
                snprintf(buf, sizeof(buf), "%s '%s'", ads->cmd.sock.cli_ip, ads->cmd.sock.cli_host);
            }
            DBGF(1, "--- request accepted (%d), cli_fd:%d (from '%s')", ads->cmd.sock.port, fd, buf);
        }
    }
    return fd;
}

//---------------------------------------------------------
int ads_cmd_socket_send(socket_t *sock, const char *bytes, int nbytes)
{
    return socket_send(sock, bytes, nbytes);
}

//---------------------------------------------------------
int ads_cmd_socket_recv(char *buf, int nbuf)
{
    socket_t *sock = &ads()->cmd.sock;
    int nread = socket_recv(sock, buf, nbuf);
    return nread;
}
