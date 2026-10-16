/**
  \file   thr_callbacks.c
  \author Aleksey Lekhtin
  \date   Mar  5, 2026
  \brief  Типовые обработчики разных потоков (ads/siu/dtr/wits)
*/

#include <string.h>
#include <sys/stat.h>

#include <libgen.h>
#include <stdlib.h>

#include "ads.h"
#include "ads_socket.h"
#include "dbg.h"
#include "dtr.h"
#include "siu.h"
#include "siu_socket.h"
#include "thr_callbacks.h"
#include "utils.h"
#include "aps_wits.h"
#include "aps_wits_socket.h"
#include "thr_t.h"

//---------------------------------------------------------
static int thread_client_stop(thr_t *thr)
{
    pthread_mutex_t *mutex;
    client_t *client = (client_t *) thr->payload;
    socket_t *cli_sock = &client->sock;

    if (thr->type == THR_TYPE_ADS_CMD) {
        ads_t *ads = (ads_t *) client->data;
        mutex = &ads->cmd.mutex;
    } else if (thr->type == THR_TYPE_ADS_DATA) {
        ads_t *ads = (ads_t *) client->data;
        mutex = &ads->data.mutex;
    } else if (thr->type == THR_TYPE_SIU_CMD) {
        siu_t *siu = (siu_t *) client->data;
        mutex = &siu->cmd.mutex;
    } else if (thr->type == THR_TYPE_SIU_DATA) {
        siu_t *siu = (siu_t *) client->data;
        mutex = &siu->data.mutex;
    } else if (thr->type == THR_TYPE_DTR_CMD) {
        dtr_t *dtr = (dtr_t *) client->data;
        mutex = &dtr->cmd.mutex;
    } else if (thr->type == THR_TYPE_DTR_DATA) {
        dtr_t *dtr = (dtr_t *) client->data;
        mutex = &dtr->data.mutex;
    } else if (thr->type == THR_TYPE_APS_WITS) {
        aps_wits_t *wits = (aps_wits_t *) client->data;
        mutex = &wits->mutex;
    } else {
        DBGF(3, "--- %s - UNKNOWN TYPE '%s' - skip",
             thr->name, thr_type_to_string(thr->type));
        return -1;
    }

    DBGF(3, "--- %s close client connections %s fd:%d",
         thr->name, cli_sock->name, cli_sock->fd);
    DBGF(5, "lock %s::cmd.mutex", thr->name);
    pthread_mutex_lock(mutex);
    socket_close_fd(&cli_sock->fd);
    DBGF(5, "unlock %s::cmd.mutex", thr->name);
    pthread_mutex_unlock(mutex);
    return 0;
}

//---------------------------------------------------------
int thread_stop(thr_t *thr)
{
    socket_t *sock = NULL;
    if (thr->type == THR_TYPE_ADS_CMD) {
        ads_t *ads = (ads_t *) thr->payload;
        sock = &ads->cmd.sock;
    } else if (thr->type == THR_TYPE_ADS_DATA) {
        ads_t *ads = (ads_t *) thr->payload;
        sock = &ads->data.sock;
    } else if (thr->type == THR_TYPE_SIU_CMD) {
        siu_t *siu = (siu_t *) thr->payload;
        sock = &siu->cmd.sock;
    } else if (thr->type == THR_TYPE_SIU_DATA) {
        siu_t *siu = (siu_t *) thr->payload;
        sock = &siu->data.sock;
    } else if (thr->type == THR_TYPE_DTR_CMD) {
        dtr_t *dtr = (dtr_t *) thr->payload;
        sock = &dtr->cmd.sock;
    } else if (thr->type == THR_TYPE_DTR_DATA) {
        dtr_t *dtr = ((dtr_t *) thr->payload);
        sock = &dtr->data.sock;
    } else if (thr->type == THR_TYPE_APS_WITS) {
        aps_wits_t *wits = (aps_wits_t *) thr->payload;
        sock = &wits->sock;
    } else {
        DBGF(3, "--- %s - UNKNOWN TYPE '%s' - skip",
             thr->name, thr_type_to_string(thr->type));
        return -1;
    }
    DBGF(3, "[tid:%08lX] %s going to close connection fd:%d", pthread_self(), thr->name, sock->fd);
    socket_close_fd(&sock->fd);
    nanosleep(&(struct timespec){.tv_nsec = TS_50MS}, NULL);
    return 0;
}

//---------------------------------------------------------
int thread_start(thr_t *thr)
{
    DBGF(3, "[tid:%08lX] --- %s (%s) going to be started...",
         pthread_self(), thr->name, thr_type_to_string(thr->type));

    int group_id;
    char *cli_name;
    socket_t *sock = NULL;
    uint32_t counter[8] = {0};
    uint32_t *cnt = NULL;

    bool (*open_socket)(void *obj) = NULL;
    int (*accept_socket)(void *obj) = NULL;
    int (*thread_client)(thr_t *thr) = NULL;

    if (thr->type == THR_TYPE_ADS_CMD) {
        cli_name = "ads-cmd-cli";
        group_id = 1;
        cnt = &counter[0];
        open_socket = ads_cmd_open_socket;
        accept_socket = ads_cmd_accept;
        thread_client = ads_cmd_client_thread;
        ads_t *ads = (ads_t *) thr->payload;
        sock = &ads->cmd.sock;
    } else if (thr->type == THR_TYPE_ADS_DATA) {
        cli_name = "ads-data-cli";
        group_id = 10;
        cnt = &counter[1];
        open_socket = ads_data_open_socket;
        accept_socket = ads_data_accept;
        thread_client = ads_data_client_thread;
        ads_t *ads = (ads_t *) thr->payload;
        sock = &ads->data.sock;
    } else if (thr->type == THR_TYPE_SIU_CMD) {
        cli_name = "siu-cmd-cli";
        group_id = 2;
        cnt = &counter[2];
        open_socket = siu_cmd_open_socket;
        accept_socket = siu_cmd_accept;
        thread_client = siu_cmd_client_thread;
        siu_t *siu = (siu_t *) thr->payload;
        sock = &siu->cmd.sock;
    } else if (thr->type == THR_TYPE_SIU_DATA) {
        cli_name = "siu-data-cli";
        group_id = 20;
        cnt = &counter[3];
        open_socket = siu_data_open_socket;
        accept_socket = siu_data_accept;
        thread_client = siu_data_client_thread;
        siu_t *siu = (siu_t *) thr->payload;
        sock = &siu->data.sock;
    } else if (thr->type == THR_TYPE_DTR_CMD) {
        cli_name = "dtr-cmd-cli";
        group_id = 3;
        cnt = &counter[4];
        open_socket = dtr_cmd_open_socket;
        accept_socket = dtr_cmd_accept;
        thread_client = dtr_cmd_client_thread;
        dtr_t *dtr = (dtr_t *) thr->payload;
        sock = &dtr->cmd.sock;
    } else if (thr->type == THR_TYPE_DTR_DATA) {
        group_id = 30;
        cli_name = "dtr-data-cli";
        cnt = &counter[5];
        open_socket = dtr_data_open_socket;
        accept_socket = dtr_data_accept;
        thread_client = dtr_data_client_thread;
        dtr_t *dtr = ((dtr_t *) thr->payload);
        sock = &dtr->data.sock;
    } else if (thr->type == THR_TYPE_APS_WITS) {
        cli_name = "aps-wits-cli";
        group_id = 4;
        cnt = &counter[6];
        open_socket = aps_wits_open_socket;
        accept_socket = aps_wits_accept;
        thread_client = aps_wits_client_thread;
        aps_wits_t *wits = (aps_wits_t *) thr->payload;
        sock = &wits->sock;
    } else {
        DBGF(3, "[tid:%08lX] --- %s - UNKNOWN TYPE '%s' - skip", pthread_self(),
             thr->name, thr_type_to_string(thr->type));
        return -1;
    }

    do {
        if (!open_socket(thr->payload)) {
            if (thr->shuttingdown) {
                break;
            }
            nanosleep(&(struct timespec){.tv_sec = 2}, NULL);
            continue;
        }

        do {
            int cli_fd = accept_socket(thr->payload);
            if (cli_fd == INVALID_SOCKET || cli_fd == 0) {
                nanosleep(&(struct timespec){.tv_sec = 0, .tv_nsec = TS_100MS}, NULL);
                break;
            }

            // specail case
            THR_STOP_GROUP(group_id);

            client_t *client = malloc(sizeof(client_t)); // TODO memory leak ?
            client->data = thr->payload;
            client->sock = *sock;
            client->sock.fd = cli_fd;
            (*cnt)++;
            snprintf(client->sock.name, sizeof(client->sock.name),
                     "%s-%02d-%02u-%d", cli_name, group_id, *cnt, client->sock.fd);
            THR_PROP_CLONE(prop, thr, THR_DETACH, thread_client, thread_client_stop, client, true, client->sock.name);
            THR_PROP_START(thr_cli, &prop);
            if (!thr_cli) {
                break;
            }
        } while (!thr->shuttingdown);
    } while (!thr->shuttingdown);
    DBGF(3, "[tid:%08lX] --- finished %s %s:%d",
         pthread_self(), thr->name, basename(__FILE__), __LINE__);
    return 0;
}
