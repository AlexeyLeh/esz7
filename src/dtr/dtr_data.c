/**
  \file   dtr_data.c
  \author Aleksey Lekhtin
  \date   Feb 26, 2025
  \brief  The Depth Tracker interface
*/

#include <libgen.h>
#include <malloc.h>
#include <string.h>
#include <sys/stat.h>

#include "cfg.h"
#include "dbg.h"
#include "dtr.h"
#include "dtr_socket.h"
#include "core.h"
#include "utils.h"
#include "log.h"


//-----------------------------------------------------------
void dtr_data_send()
{
    if (dtr()->data.cli_thr) {
        DBGF(3, "dtr data client thr:  %p", dtr()->data.cli_thr);
        dtr_data_create_packet();
        thr_wakeup(dtr()->data.cli_thr);
    } else {
        DBGF(8, "dtr data client thr: %p", dtr()->data.cli_thr);
    }
}

//-----------------------------------------------------------
/**
 * Цикличное ожидание данных от ESZ7 ADC и отправка клиенту
 * @param thr - данные по потоку
 * @param dtr - данные по соединению с SureShot
 * @return - OK
 * @return - FAIL
 */
static bool dtr_data_send_to_client(dtr_t *dtr)
{
    thr_t *thr = dtr->data.cli_thr;

    pthread_mutex_lock(&thr->mutex);
    int rc = 0;
    while (thr->wakeup != YES && rc == 0) {
        DBGF(8, "%s pthread_cond_wait() - waiting for event", thr->name);
        rc = pthread_cond_wait(&thr->cond, &thr->mutex);
        DBGF(4, "%s pthread_cond_wait() - enabled to work ( rc:%d %s)",
             thr->name, rc, (rc != 0 ? strerror(rc) : ""));
    }
    thr->wakeup = NO;
    if (thr->shuttingdown) {
        DBG(3, "=== got a shutting down...");
        pthread_mutex_unlock(&thr->mutex);
        return FAIL;
    }

    // send points
    if (!dtr->packet.bytes || dtr->packet.nbytes == 0) {
        LOG(ERROR, "505: INTERNAL ERROR: Nothing to send");
        DBG(0, "\n505: INTERNAL ERROR: Nothing to send\n");
        pthread_mutex_unlock(&thr->mutex);
        return FAIL;
    }

    DBGF(8, "%s", dtr->packet.bytes);
    bool ok = dtr_data_socket_send(dtr->packet.bytes, dtr->packet.nbytes);
    if (!ok) {
        nanosleep(&(struct timespec){.tv_sec = 0, .tv_nsec = TS_100MS}, NULL);
        pthread_mutex_unlock(&thr->mutex);
        return FAIL;
    }

    pthread_mutex_unlock(&thr->mutex);
    return OK;
}

//-----------------------------------------------------------
//__attribute__((unused))
int dtr_data_client_thread(thr_t *thr)
{
    client_t *client = (client_t *) thr->payload;
    dtr_t *dtr = client->data;
    dtr->data.cli_thr = thr;
    socket_t *cli_sock = &client->sock;
    DBGF(3, "--- %s started on socket %d, port %d",
         thr->name, cli_sock->fd, dtr->data.port);

    if (dtr->data.online == NO) {
        bool ok;
        dtr->data.online = YES;
        core__wakeup();
        do {
            ok = dtr_data_send_to_client(dtr);
        } while (ok && !thr->shuttingdown);
    }
    DBGF(4, "%s - shuttingdown...", thr->name);

    dtr->data.online = NO;
    core__wakeup();

    dtr->data.cli_thr = NULL;
    DBGF(3, "--- %s finished %s:%d", thr->name, basename(__FILE__), __LINE__);
    return 0;
}
