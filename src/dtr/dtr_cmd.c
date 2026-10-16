/**
  \file   dtr_cmd.c
  \author Aleksey Lekhtin
  \date   Feb 26, 2025
  \brief  The Depth Tracker command interface
*/

#include <fcntl.h>
#include <libgen.h>
#include <stdio.h>
#include <string.h>

#include "cfg.h"
#include "dbg.h"
#include "dtr.h"
#include "dtr_socket.h"
#include "siu.h"

//---------------------------------------------------------
int dtr_cmd_client_thread(thr_t *thr)
{
    client_t *client = (client_t *)thr->payload;
    dtr_t *dtr = (dtr_t *) client->data;
    dtr->cmd.cli_thr = thr;
    socket_t *cli_sock = &client->sock;
    DBGF(3, "--- %s started on socket %d, port %d",
         thr->name, cli_sock->fd, dtr->cmd.port);

    char command[1024 * 16] = {0};
    const int length = sizeof(command) - 1;
    do {
        DBGNL(8);
        DBGF(8, "--- %s (socket:%d) %s - going to receive...", cli_sock->name, cli_sock->fd, thr->name);
        int nread = socket_recv(cli_sock, command, length);
        DBGF(9, "socket_recv() return %d", nread);
        if (nread <= 0) {
            // error or EOF(client disconnected)
            break;
        }
        command[nread] = '\0';
        DBGF(8, "got %d bytes", nread);
        pthread_mutex_lock(&thr->mutex);
        siu_cmd_response(command, &siu()->vars, dtr_cmd_socket_send, cli_sock);
        pthread_mutex_unlock(&thr->mutex);

    } while (!thr->shuttingdown);
    DBGF(3, "%s - shuttingdown...", thr->name);

    dtr->cmd.cli_thr = NULL;
    DBGF(3, "--- %s finished %s:%d", thr->name, basename(__FILE__), __LINE__);
    return 0;
}
