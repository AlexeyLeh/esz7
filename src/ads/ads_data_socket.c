/**
  \file   ads_data_socket.c
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

//---------------------------------------------------------
const socket_t *ads_data_socket()
{
    return &ads()->data.sock;
}

//---------------------------------------------------------
bool ads_data_open_socket(void *obj)
{
    ads_t *ads = (ads_t *)obj;
    ads->data.sock.port = ads->data.port;
    snprintf(ads->data.sock.name, sizeof(ads->data.sock.name), "ads-data");
    DBGF(3, "[tid:%08lX] --- %s waiting on port %d",
        pthread_self(), ads->data.sock.name, ads->data.sock.port);
    if (socket_open(&ads->data.sock, SOCK_STREAM, IPPROTO_IP)) {
        if (socket_bind(&ads->data.sock)) {
            if (socket_listen(&ads->data.sock)) {
                return OK;
            }
        }
    }
    ads_data_close_socket(ads);
    return FAIL;
}

//---------------------------------------------------------
void ads_data_close_socket(void *obj)
{
    ads_t *ads = (ads_t *)obj;
    if (ads->data.sock.fd == INVALID_SOCKET) {
        return;
    }
    DBG(5, "lock ads->data.mutex");
    pthread_mutex_lock(&ads->data.mutex);
    socket_close_fd(&ads->data.sock.fd);
    DBG(5, "unlock ads->data.mutex");
    pthread_mutex_unlock(&ads->data.mutex);
}

//---------------------------------------------------------
int ads_data_accept(void *obj)
{
    ads_t *ads = (ads_t *)obj;
    DBGF(3, "[tid:%08lX] --- %s waiting on port %d",
        pthread_self(), ads->data.sock.name, ads->data.sock.port);
    int fd = socket_accept(&ads->data.sock);
    if (fd == INVALID_SOCKET) {
        return fd;
    }
    if (debug_level > 0) {
        char buf[NI_MAXHOST * 3] = {0};
        if (strcasecmp(ads->data.sock.cli_ip, ads->data.sock.cli_host) == 0) {
            strncpy(buf, ads->data.sock.cli_host, sizeof(buf) - 1);
        } else {
            snprintf(buf, sizeof(buf), "%s '%s'", ads->data.sock.cli_ip, ads->data.sock.cli_host);
        }
        DBGF(1, "--- request accepted (%d), cli_fd:%d (from '%s')", ads->data.sock.port, fd, buf);
    }
    return fd;
}
