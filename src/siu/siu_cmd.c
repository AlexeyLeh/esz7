/**
  \file   siu_cmd.c
  \author Aleksey Lekhtin
  \date   Feb 26, 2025
  \brief  The SIU command interface
*/

#include <libgen.h>
#include <stdio.h>
#include <string.h>

#include "cfg.h"
#include "dbg.h"
#include "siu.h"
#include "siu_socket.h"

//---------------------------------------------------------
int siu_cmd_client_thread(thr_t *thr)
{
    client_t *client = (client_t *) thr->payload;
    siu_t *siu = client->data;
    siu->cmd.cli_thr = thr;
    socket_t *cli_sock = &client->sock;
    DBGF(3, "[tid:%08lX] --- %s started on socket: %d",
        pthread_self(), thr->name, cli_sock->fd);

    char command[1024 * 16] = {0};
    int length = sizeof(command) - 1;
    do {
        int nread = socket_recv(cli_sock, command, length);
        if (nread <= 0) {
            return 1; // error or EOF(client disconnected) - close job
        }
        command[nread] = '\0';
        DBGF(4, "[tid:%08lX] got %d bytes", pthread_self(), nread);
        siu_cmd_response(command, &siu->vars, siu_cmd_socket_send, cli_sock);
    } while (!thr->shuttingdown);
    DBGF(3, "[tid:%08lX] %s - shuttingdown...", pthread_self(), thr->name);

    siu->cmd.cli_thr = NULL;
    DBGF(3, "[tid:%08lX] --- %s finished %s:%d",
        pthread_self(), thr->name, basename(__FILE__), __LINE__);
    return 0;
}
